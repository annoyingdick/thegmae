#include <stdio.h>
#include <string.h>
#include <cglm/vec2.h>
#include <cglm/vec3.h>
#include <cglm/mat4.h>
#include <cglm/quat.h>
#include "WindowHandler.h"
#include "NavigationHandler.h"
#include "CharactersHandler.h"
#include "TaskManager.h"
#include "DebugGuiHandler.h"

#define DO_NOT_KILL

#define FLOAT_BINONE nextafterf(0, INFINITY)

static AnimationID topAnimationsOrder[] = {
    NOTHING_ANIMATION_DEATH_FROMFRONT,
    NOTHING_ANIMATION_DEATH_FROMBACK,
    WEAPON_ANIMATION_EQUIP,
    WEAPON_ANIMATION_UNEQUIP,
    WEAPON_ANIMATION_IDLE_STEADY,
    WEAPON_ANIMATION_SPRINT,
    WEAPON_ANIMATION_WALK,
    WEAPON_ANIMATION_IDLE,
    NOTHING_ANIMATION_SCARED,
    NOTHING_ANIMATION_SPRINT,
    NOTHING_ANIMATION_WALK,
    NOTHING_ANIMATION_IDLE,
    WEAPON_ANIMATION_SHOOT
};
static AnimationID bottomAnimationsOrder[] = {
    NOTHING_ANIMATION_DEATH_FROMFRONT,
    NOTHING_ANIMATION_DEATH_FROMBACK,
    WEAPON_ANIMATION_SPRINT,
    WEAPON_ANIMATION_WALK,
    NOTHING_ANIMATION_SPRINT,
    NOTHING_ANIMATION_WALK,
    WEAPON_ANIMATION_IDLE,
    NOTHING_ANIMATION_SCARED,
    NOTHING_ANIMATION_IDLE,
    WEAPON_ANIMATION_EQUIP,
    WEAPON_ANIMATION_UNEQUIP,
    WEAPON_ANIMATION_IDLE_STEADY,
    WEAPON_ANIMATION_SHOOT
};
const AnimationID moveAnimations[] = {
    WEAPON_ANIMATION_WALK, NOTHING_ANIMATION_SPRINT, NOTHING_ANIMATION_WALK 
};

//returns an index of a keyframe whose timestamp is bigger than the time argument
static KeyframeID getCorrespondingKeyframeIndex(
    const float time, const KeyframeID numKeyframes, const float timestamps[const]
) {
    bool passed;

    passed = true;

    for (KeyframeID i = 0; i < numKeyframes; i++) {
	if (passed && time <= timestamps[i]) return i;

	passed = time >= timestamps[i];
    }

    return numKeyframes - 1;
}
static float getAlpha(const float time, const float timestamps[const]) {
    return (time - timestamps[0]) / (timestamps[1] - timestamps[0]);
}
static void animateRotation(
    const KeyframeSetRotation* const setRot, const float time, versor dest
) {
    const KeyframeID id = getCorrespondingKeyframeIndex(
	time, setRot->numKeyframes, setRot->timestamps
    );

    if (id) {
	glm_quat_slerp(
	    setRot->rotationVersors[id - 1], setRot->rotationVersors[id],
	    getAlpha(time, setRot->timestamps + id - 1), dest
	);
    }
    else glm_quat_copy(setRot->rotationVersors[id], dest);
}
static void animateTranslation(
    const KeyframeSetTranslation* const setTrans, const float time, vec3 dest
) {
    const KeyframeID id = getCorrespondingKeyframeIndex(
	time, setTrans->numKeyframes, setTrans->timestamps
    );

    if (id) glm_vec3_lerp(
	setTrans->translationVectors[id - 1], setTrans->translationVectors[id], 
	getAlpha(time, setTrans->timestamps + id - 1), dest
    );
    else glm_vec3_copy(setTrans->translationVectors[id], dest);
}
static void multiplyAnimation(const AnimationTrack* const track, const BoneID boneId, mat4 transform) {
    mat4 mat;
    versor mulRot;
    vec3 mulTrans;

    animateRotation(track->animation->setsRot + boneId, track->time, mulRot);
    animateTranslation(track->animation->setsTrans + boneId, track->time, mulTrans);

    glm_vec3_add(transform[3], mulTrans, transform[3]);

    glm_quat_mat4(mulRot, mat);
    glm_mat4_mul(transform, mat, transform);
}
static void handleBone(const Character* const character, Bone* const bone, mat4 parentTransform, bool isTop) {
    mat4 transform;
    vec3 finalTrans = GLM_VEC3_ZERO_INIT;
    versor finalRot = GLM_QUAT_IDENTITY_INIT;

    float totalWeight;

    totalWeight = 0;

    isTop = strcmp(bone->name, "mixamorig:Spine2") ? isTop : true;

    //BENCHMARK_BEGIN
    for (AnimationID i = 0; i < ANIMATION_MAX_ENUM && totalWeight < 1; i++) {
	vec3 trans;
	versor rot;

	const AnimationID animId = (isTop ? topAnimationsOrder : bottomAnimationsOrder)[i];

	const AnimationTrack* const track = character->tracks + animId;

	const float weight = fminf(track->weight, 1 - totalWeight);

	const KeyframeSetTranslation* const setTrans = track->animation->setsTrans + bone->id;
	const KeyframeSetRotation* const setRot = track->animation->setsRot + bone->id;

	animateTranslation(setTrans, track->time, trans);
	animateRotation(setRot, track->time, rot);

	//root motion : discard any z coord changes
	if (animId != NOTHING_ANIMATION_DEATH_FROMFRONT && animId != NOTHING_ANIMATION_DEATH_FROMBACK && !bone->id) {
	    trans[2] = 0;
	}

	glm_vec3_scale(trans, weight, trans);
	glm_vec3_add(finalTrans, trans, finalTrans);

	glm_quat_slerp(finalRot, rot, totalWeight ? weight : 1, finalRot);
	//glm_quat_mul(finalRot, rot, finalRot);

	totalWeight += track->weight;
    }
    //BENCHMARK_END

    glm_quat_mat4(finalRot, transform);
    glm_vec3_copy(finalTrans, transform[3]);

    if (!AnimationTrack_IsFinished(character->tracks + WEAPON_ANIMATION_SHOOT)) {
	multiplyAnimation(character->tracks + WEAPON_ANIMATION_SHOOT, bone->id, transform);
    }

    vec4* const instDest = IH_GetUploadPtr(character->instance) + (bone->id * sizeof(mat4));

    glm_mat4_mul(parentTransform, transform, transform);

    if (!strcmp(bone->name, "mixamorig:RightHand")) {
	//if (character->currentSlot) glm_mat4_copy(transform, IH_GetUploadPtr(character->weaponInstance));
	glm_mat4_zero(IH_GetUploadPtr(character->weaponInstance));
    }

    glm_mat4_mul(transform, bone->off, instDest);

    foreach (Bone* const child, bone->children, bone->numChildren)
	handleBone(character, child, transform, isTop);
    forend
}
static float calculateSpeed(Character* const character) {
    float finalSpeed, totalWeight;

    finalSpeed = totalWeight = 0;

    for (size_t i = 0; i < ARRAYSIZE(moveAnimations) && totalWeight < 1; i++) {
	AnimationTrack* const track = character->tracks + moveAnimations[i];

	const KeyframeSetTranslation* const set = track->animation->setsTrans + 0;

	const KeyframeID id = getCorrespondingKeyframeIndex(track->time, set->numKeyframes, set->timestamps);

	const float weight = fminf(track->weight, 1 - totalWeight);

	const float transZ = glm_lerp(
	    set->translationVectors[id - 1][2], set->translationVectors[id][2], 
	    getAlpha(track->time, set->timestamps + id - 1)
	);

	float speed = track->lastTransZ - transZ;

	if (speed < 0) {
	    const float lastZ = set->translationVectors[set->numKeyframes - 1][2];

	    speed += set->translationVectors[0][2] - lastZ;
	}

	finalSpeed += fmaxf(speed * weight, 0);

	totalWeight += weight;

	track->lastTransZ = transZ;
    }

    return finalSpeed;
}
static void slerp(vec2 start, vec2 end, const float t, vec2 dest) {
    const float dot = fminf(fmaxf(glm_vec2_dot(start, end), -1), 1);

    vec2 scaledStart, diff;

    glm_vec2_scale(start, dot, diff);
    glm_vec2_sub(end, diff, diff);
    glm_vec2_normalize(diff);

    const float theta = acosf(dot) * t;

    glm_vec2_scale(start, cosf(theta), scaledStart);
    glm_vec2_scale(diff, sinf(theta), diff);
    glm_vec2_add(start, diff, dest);
    glm_vec2_normalize(dest);
}
static void turnCharacter(Character* const character) {
    const float rotationSpeed = 3;//getWishDirection(character, move, wishDirection);

    slerp(
	character->currentDirection, character->wishDirection, rotationSpeed * WH_GetDeltaTime(), character->currentDirection
    );
}
static float processMove(Character* const character) {
    turnCharacter(character);

    const float speed = calculateSpeed(character);

    vec3 move;

    glm_vec3_scale_as((vec3){character->currentDirection[0], 0, character->currentDirection[1]}, speed, move);
    glm_vec3_add(character->position, move, character->position);

    return speed;
}
static void initAnimationPointer(Character* const character, const Animation* const animation) {
    const char* const names[] = {
	[NOTHING_ANIMATION_IDLE] = "idle",
	[NOTHING_ANIMATION_WALK] = "walk",
	[NOTHING_ANIMATION_SPRINT] = "sprint",
	[NOTHING_ANIMATION_DEATH_FROMFRONT] = "death_fromfront",
	[NOTHING_ANIMATION_DEATH_FROMBACK] = "death_fromback",
	[NOTHING_ANIMATION_SCARED] = "scared",
	[WEAPON_ANIMATION_IDLE] = "pistol_idle",
	[WEAPON_ANIMATION_WALK] = "pistol_walk",
	[WEAPON_ANIMATION_SPRINT] = "pistol_sprint",
	[WEAPON_ANIMATION_IDLE_STEADY] = "pistol_idlesteady",
	[WEAPON_ANIMATION_SHOOT] = "pistol_shoot",
	[WEAPON_ANIMATION_EQUIP] = "pistol_equip",
	[WEAPON_ANIMATION_UNEQUIP] = "pistol_unequip"
    };

    for (int i = 0; i < ANIMATION_MAX_ENUM; i++) {
	if (!strcmp(animation->name, names[i])) {
	    AnimationTrack_Init(character->tracks + i, animation);

	    return;
	}
    }
}
static void moveAndAnimate(Character* const character) {
    mat4 characterMat = GLM_MAT4_IDENTITY_INIT;

    characterMat[0][0] = -character->currentDirection[1];
    characterMat[0][2] = character->currentDirection[0];

    characterMat[2][0] = -character->currentDirection[0];
    characterMat[2][2] = -character->currentDirection[1];

    processMove(character);

    glm_vec3_copy(character->position, characterMat[3]);

    handleBone(character, CH_GetMesh()->bones + 0, characterMat, false);
}

void Character_Init(Character* const character, Mesh* const weaponMesh) {
    CH_AddCharacter(character);

    character->instance = IH_NewInstance(CH_GetMesh());
    character->weaponInstance = IH_NewInstance(weaponMesh);

    character->currentDirection[1] = character->wishDirection[1] = -1;

    character->tracks[NOTHING_ANIMATION_IDLE].weight = 1;
    character->tracks[WEAPON_ANIMATION_SHOOT].time = INFINITY;

    character->areAnimationsLoaded = false;
}
void Character_Loop(Character* const character) {
    //load animations if they haven't been yet
    if (!CH_GetMesh()->animations) return;

    if (!character->areAnimationsLoaded) {
	foreach (const Animation* const animation, CH_GetMesh()->animations, CH_GetMesh()->numAnimations) 
	    initAnimationPointer(character, animation);
	forend

	character->areAnimationsLoaded = true;
    }

    AnimationTrack_Go(character->tracks + NOTHING_ANIMATION_IDLE);
    AnimationTrack_Go(character->tracks + NOTHING_ANIMATION_WALK);

    AnimationTrack_FadeIn(character->tracks + NOTHING_ANIMATION_WALK, character->go);

    moveAndAnimate(character);
}

DGH_BEGIN(Character, character, 16) {
#define X(type, name) DGH_FIELD(character->name);
    VARS_CHARACTER
#undef X

    DGH_Vec3(character->position, "position:");
    DGH_Vec2(character->currentDirection, "currentDirection:");
    DGH_Vec2(character->wishDirection, "wishDirection:");

    DGH_ARRAYN(character->tracks);
DGH_END }
