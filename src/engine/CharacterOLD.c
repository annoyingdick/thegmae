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

static bool isStopped(const Character* const character) {
    return vec3Array_IsEmpty(&character->path);
}
static bool isPathLongerThan(Character* const character, const float length) {
    float finalLength;

    float* lastWaypoint;

    finalLength = 0;
    lastWaypoint = character->position;

    aforeach (vec3* const waypoint, &character->path)
	finalLength += glm_vec3_distance(lastWaypoint, *waypoint);

	if (finalLength > length) return true;

	lastWaypoint = *waypoint;
    forend

    return false;
}
static bool isSwapping(const Character* const character) {
    return character->currentSlot != character->wishSlot;
}
static bool canShoot(const Character* const character) {
    const bool idle = character->tracks[WEAPON_ANIMATION_IDLE].weight > .8f;
    const bool idleSteady = character->tracks[WEAPON_ANIMATION_IDLE_STEADY].weight < .3f;

    return character->currentSlot && idle && idleSteady;
}
static bool isTired(const Character* const character) {
    const float threshold = .5f;

    return character->fatigue > threshold;
}
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
	if (character->currentSlot) glm_mat4_copy(transform, IH_GetUploadPtr(character->weaponInstance));
	else glm_mat4_zero(IH_GetUploadPtr(character->weaponInstance));
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
static float getWishDirection(Character* const character, vec3 move, vec3 dest) {
    const float normalSpeed = 5;

    if (character->aimOn && (isStopped(character) || character->state != CHARACTER_STATE_SPRINT)) {
	const float aimingSpeed = 16;

	//if (AnimationTrack_IsFinished(character->tracks + WEAPON_ANIMATION_SHOOT)) character->going2 = 0;

	glm_vec3_sub(character->aimOn->position, character->position, dest);
	glm_vec3_normalize(dest);

	return aimingSpeed;
    }

    if (isStopped(character)) {
	dest[0] = character->direction[0];
	dest[1] = 0;
	dest[2] = character->direction[1];
    }
    else if (character->state != CHARACTER_STATE_AIM || character->path.numElements == 1 || glm_vec3_norm2(move) > 1) {
	glm_vec3_copy(move, dest);
    }
    else glm_vec3_sub(character->path.elements[1], character->position, dest);

    glm_vec3_normalize(dest);

    return normalSpeed;
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
static void turnCharacter(Character* const character, vec3 move) {
    vec3 wishDirection;

    const float rotationSpeed = getWishDirection(character, move, wishDirection);

    slerp(
	character->direction, (vec2){wishDirection[0], wishDirection[2]}, 
	rotationSpeed * WH_GetDeltaTime(), character->direction
    );
}
static float processMove(Character* const character) {
    vec3 move;

    glm_vec3_sub(character->path.elements[0], character->position, move);

    turnCharacter(character, move);

    if (!isStopped(character)) {
	const float speed = calculateSpeed(character);

	if (glm_vec3_norm2(move) <= speed * speed) {
	    //glm_vec3_copy(character->path.elements[0], character->position);

	    vec3Array_RemoveElement(&character->path, 0);
	}
	glm_vec3_scale_as(move, speed, move);
	glm_vec3_add(character->position, move, character->position);

	return speed;
    }

    return 0;
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
static void stopIfStopped(Character* const character) {
    if (!character->tracks[WEAPON_ANIMATION_WALK].weight &&
	!character->tracks[NOTHING_ANIMATION_WALK].weight && !character->tracks[NOTHING_ANIMATION_SPRINT].weight
    ) character->path.numElements = 0;
}
static void workerThrd(Character* const character) {
    //TODO: this can cause DAMAAGE when new characters are added to CH
    if (character->hasGun) character->aimOn = CH_FindClosestVisibleAliveCharacter(character);
    else character->aimOn = CH_FindClosestVisibleAliveArmedCharacter(character);

    character->aimTaskLock = false;
}
static void aimOn(Character* const character) {
    AnimationTrack* const shootTrack = character->tracks + WEAPON_ANIMATION_SHOOT;

    if (!character->aimTaskLock) {
	character->aimTaskLock = true;

	workerThrd(character);
	//TM_AddTask(&(Task){.function = (void*)workerThrd, .argument = character});
    }

    if (character->aimOn) {
	if (character->state == CHARACTER_STATE_AIM) {
	    if (canShoot(character)) {
		const bool shouldShoot = shootTrack->time > shootTrack->animation->duration / 2 && !shootTrack->weight;

		if (AnimationTrack_IsFinished(shootTrack)) shootTrack->time = shootTrack->weight = 0;
		else if (shouldShoot && character->aimOn->state != CHARACTER_STATE_DEAD) {
		    shootTrack->weight = FLOAT_BINONE;
#ifndef DO_NOT_KILL
		    //kill
		    character->aimOn->state = CHARACTER_STATE_DEAD;

		    character->aimOn->tracks[Character_CanSeeDotCheck(character->aimOn, character)
		    ? NOTHING_ANIMATION_DEATH_FROMFRONT : NOTHING_ANIMATION_DEATH_FROMBACK].weight = FLOAT_BINONE;
#endif
		}
	    }
	}
	else if (character->hasGun) Character_SwitchAim(character, true);
	else if (character->aimOn->hasGun) {
	    if (!character->tracks[NOTHING_ANIMATION_SCARED].weight) {
		character->tracks[NOTHING_ANIMATION_SCARED].weight = FLOAT_BINONE;
	    }

	    Character_GoTo(character, character->aimOn->position, ESCAPE_FROM_DANGER_MODE);
	    Character_BeginSprinting(character);
	}
    }
}
static void fadeAnimations(Character* const character) {
    const bool sprintCond = character->state == CHARACTER_STATE_SPRINT && 
    isPathLongerThan(character, AnimationTrack_GetPathLengthToStop(character->tracks + NOTHING_ANIMATION_SPRINT));

    AnimationTrack_FadeIn(
	character->tracks + NOTHING_ANIMATION_WALK,
	character->state == CHARACTER_STATE_NORMAL && (!character->wishSlot) &&
	isPathLongerThan(character, AnimationTrack_GetPathLengthToStop(character->tracks + NOTHING_ANIMATION_WALK))
    );
    AnimationTrack_FadeIn(character->tracks + NOTHING_ANIMATION_SPRINT, sprintCond);
    AnimationTrack_FadeIn(character->tracks + WEAPON_ANIMATION_EQUIP, isSwapping(character) && !character->currentSlot);
    AnimationTrack_FadeIn(character->tracks + WEAPON_ANIMATION_UNEQUIP, isSwapping(character) && character->currentSlot);
    AnimationTrack_FadeIn(
	character->tracks + WEAPON_ANIMATION_IDLE, 
	((character->currentSlot && character->wishSlot) || character->tracks[WEAPON_ANIMATION_EQUIP].weight == 1) &&
	!sprintCond
    );
    AnimationTrack_FadeIn(
	character->tracks + WEAPON_ANIMATION_IDLE_STEADY, 
	((character->currentSlot && character->wishSlot) || character->tracks[WEAPON_ANIMATION_EQUIP].weight == 1) &&
	!sprintCond && character->state != CHARACTER_STATE_AIM
    );
    AnimationTrack_FadeIn(
	character->tracks + WEAPON_ANIMATION_WALK, 
	character->wishSlot && character->state != CHARACTER_STATE_SPRINT && 
	isPathLongerThan(character, AnimationTrack_GetPathLengthToStop(character->tracks + WEAPON_ANIMATION_WALK))
    );
}
static void moveAndAnimate(Character* const character) {
    mat4 characterMat = GLM_MAT4_IDENTITY_INIT;

    characterMat[0][0] = -character->direction[1];
    characterMat[0][2] = character->direction[0];

    characterMat[2][0] = -character->direction[0];
    characterMat[2][2] = -character->direction[1];

    processMove(character);

    glm_vec3_copy(character->position, characterMat[3]);

    handleBone(character, CH_GetMesh()->bones + 0, characterMat, false);
}
static void addFatigue(Character* const character) {
    const float factor = .3f;

    character->fatigue = fminf(
	fmaxf(
	    character->fatigue + (WH_GetDeltaTime() * factor
	    * (character->state == CHARACTER_STATE_NORMAL
	    || (character->state == CHARACTER_STATE_SPRINT && isStopped(character)) ? -1.f : 1)), 0
	), 1
    );
}

void Character_Init(Character* const character, Mesh* const weaponMesh) {
    vec3Array_Init(&character->path);

    CH_AddCharacter(character);

    character->instance = IH_NewInstance(CH_GetMesh());
    character->weaponInstance = IH_NewInstance(weaponMesh);

    character->direction[1] = -1;

    character->tracks[NOTHING_ANIMATION_IDLE].weight = 1;
    character->tracks[WEAPON_ANIMATION_SHOOT].time = INFINITY;

    character->areAnimationsLoaded = false;
}
bool Character_CanSeeDotCheck(Character* const character, Character* const them) {
    vec3 dir;

    glm_vec3_sub(them->position, character->position, dir);

    return glm_vec3_dot((vec3){character->direction[0], 0, character->direction[1]}, dir) > 0;
}
void Character_BeginSprinting(Character* const character) {
    if (!isTired(character)) character->state = CHARACTER_STATE_SPRINT;
}
void Character_StopSprinting(Character* const character) {
    character->state = CHARACTER_STATE_NORMAL;
}
void Character_ChooseSlot(Character* const character, const SlotID slot) {
    if (character->hasGun) {
	if (!(character->wishSlot = slot) && character->state == CHARACTER_STATE_AIM) {
	    character->state = CHARACTER_STATE_NORMAL;
	}

	character->tracks[slot ? WEAPON_ANIMATION_EQUIP : WEAPON_ANIMATION_UNEQUIP].time = 0;
    }
}
void Character_SwitchAim(Character* const character, const bool aim) {
    if (character->hasGun && (!aim || character->aimOn || !isTired(character))) {
	if (aim && !character->wishSlot) Character_ChooseSlot(character, 1);
	if (character->state != CHARACTER_STATE_DEAD) character->state = aim ? CHARACTER_STATE_AIM : CHARACTER_STATE_NORMAL;
    }
}
void Character_GoTo(Character* const character, vec3 goal, const TriangleID goalTri) {
    NH_FindPath(
	character->position, goal, NH_GetClosestTriangle(character->position), 
	goalTri, &character->path
    );
}
void Character_Loop(Character* const character) {
    /*

    const Animation* currentAnimation;

    const Animation* const* const group = getCurrentAnimationGroup(character);
    if (isStopped(character)) currentAnimation = group[
	!character->currentSlot || character->state == CHARACTER_STATE_AIM ? 
	ANIMATION_IDLE : WEAPON_ANIMATION_IDLE_STEADY
    ];
    else if (character->state == CHARACTER_STATE_SPRINT) currentAnimation = group[ANIMATION_SPRINT];
    else currentAnimation = group[ANIMATION_WALK];

    if (!currentAnimation) return;
    */

    if (!CH_GetMesh()->animations) return;

    if (!character->areAnimationsLoaded) {
	foreach (const Animation* const animation, CH_GetMesh()->animations, CH_GetMesh()->numAnimations) 
	    initAnimationPointer(character, animation);
	forend

	character->areAnimationsLoaded = true;
    }

    if (character->state == CHARACTER_STATE_DEAD) {
	const AnimationTrack* const front = character->tracks + NOTHING_ANIMATION_DEATH_FROMFRONT;

	AnimationTrack* const track = character->tracks + 
	(front->weight ? NOTHING_ANIMATION_DEATH_FROMFRONT : NOTHING_ANIMATION_DEATH_FROMBACK);

	AnimationTrack_GoTillEnd(track);
	AnimationTrack_FadeIn(track, true);

	character->path.numElements = 0;
	character->aimOn = NULL;
    }
    else {
	AnimationTrack_Go(character->tracks + NOTHING_ANIMATION_IDLE);
	AnimationTrack_Go(character->tracks + NOTHING_ANIMATION_WALK);
	AnimationTrack_Go(character->tracks + NOTHING_ANIMATION_SPRINT);
	AnimationTrack_Go(character->tracks + NOTHING_ANIMATION_SCARED);
	AnimationTrack_Go(character->tracks + WEAPON_ANIMATION_IDLE);
	AnimationTrack_Go(character->tracks + WEAPON_ANIMATION_IDLE_STEADY);
	AnimationTrack_Go(character->tracks + WEAPON_ANIMATION_WALK);
	AnimationTrack_GoTillEnd(character->tracks + WEAPON_ANIMATION_SHOOT);

	if (character->tracks[NOTHING_ANIMATION_SCARED].weight) {
	    AnimationTrack* const scared = character->tracks + NOTHING_ANIMATION_SCARED;

	    const bool isWalking = !isStopped(character) && character->state == CHARACTER_STATE_NORMAL;

	    AnimationTrack_FadeIn(scared, !isWalking);

	    if (isWalking) scared->weight = fmaxf(scared->weight, 1.f / 4);
	}

	if (isSwapping(character)) {
	    AnimationTrack* const track = character->tracks + 
	    (character->currentSlot ? WEAPON_ANIMATION_UNEQUIP : WEAPON_ANIMATION_EQUIP);

	    track->time += WH_GetDeltaTime();

	    if (track->time > track->animation->duration) {
		character->currentSlot = character->currentSlot ? 0 : character->wishSlot;
	    }
	}

	fadeAnimations(character);
	stopIfStopped(character);
	aimOn(character);
	addFatigue(character);

	if (character->fatigue == 1 && !character->aimOn) character->state = CHARACTER_STATE_NORMAL;
    }

    moveAndAnimate(character);
}

DGH_BEGIN(Character, character, 16) {
    DGH_ARRAYT(character->path);

#define X(type, name) DGH_FIELD(character->name);
    VARS_CHARACTER
#undef X

    DGH_Vec3(character->position, "position:");
    DGH_Vec2(character->direction, "direction:");

    DGH_PTR(character->aimOn);
    DGH_ARRAYN(character->tracks);
DGH_END }
