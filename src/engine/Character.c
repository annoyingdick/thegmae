#include <stdio.h>
#include <string.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>
#include <cglm/struct/mat4.h>
#include <cglm/struct/quat.h>
#include "WindowHandler.h"
#include "NavigationHandler.h"
#include "CharactersHandler.h"
#include "DebugGuiHandler.h"

//#define DO_NOT_KILL

#define INIT_WEAPON_ANIMPACK(x, sprint)	(const char* const []){ \
    [WEAPON_ANIMATION_IDLE - NUM_NOTHING_ANIMATIONS] = x"_idle", \
    [WEAPON_ANIMATION_WALK - NUM_NOTHING_ANIMATIONS] = x"_walk", \
    [WEAPON_ANIMATION_SPRINT - NUM_NOTHING_ANIMATIONS] = sprint"sprint", \
    [WEAPON_ANIMATION_IDLE_STEADY - NUM_NOTHING_ANIMATIONS] = x"_idlesteady", \
    [WEAPON_ANIMATION_SHOOT - NUM_NOTHING_ANIMATIONS] = x"_shoot", \
    [WEAPON_ANIMATION_EQUIP - NUM_NOTHING_ANIMATIONS] = x"_equip", \
    [WEAPON_ANIMATION_UNEQUIP - NUM_NOTHING_ANIMATIONS] = x"_unequip" \
}

#define FOREACH_WEAPON_SLOT(x) for (SlotID _i = 0; _i < SLOT_MAX_ENUM - 1; _i++) { \
AnimationTrack* const x = character->wtracks[_i] - NUM_NOTHING_ANIMATIONS;

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
    WEAPON_ANIMATION_SPRINT, WEAPON_ANIMATION_WALK, NOTHING_ANIMATION_SPRINT, NOTHING_ANIMATION_WALK 
};

static bool isStopped(const Character* const character) {
    return vec3sArray_IsEmpty(&character->path);
}
static bool isPathLongerThan(Character* const character, const float length) {
    float finalLength;

    vec3s lastWaypoint;

    finalLength = 0;
    lastWaypoint = character->position;

    aforeach (vec3s* const waypoint, &character->path)
	finalLength += glms_vec3_distance(lastWaypoint, *waypoint);

	if (finalLength > length) return true;

	lastWaypoint = *waypoint;
    forend

    return false;
}
static bool isSwapping(const Character* const character) {
    return character->currentSlot != character->wishSlot;
}
//unsafe to call whenever argument slot is 0
static AnimationTrack* getSlotWtracks(Character* const character, const SlotID slot) {
    return character->wtracks[slot - 1] - NUM_NOTHING_ANIMATIONS;
}
static bool shouldTryShoot(Character* const character) {
    const bool idle = Character_GetWeaponTracks(character, WEAPON_ANIMATION_IDLE)->weight > .8f;
    const bool idleSteady = Character_GetWeaponTracks(character, WEAPON_ANIMATION_IDLE_STEADY)->weight < .3f;

    return idle && idleSteady;
}
static bool shouldShoot(Character* const character) {
    const AnimationTrack* const shootTrack = Character_GetWeaponTracks(character, WEAPON_ANIMATION_SHOOT);

    const float factor = 5, minDot = .9f;

    vec2s toTarget;

    if (character->target) {
	toTarget = glms_vec2_normalize(glms_vec2_sub(VEC3XZ2(character->target->position), VEC3XZ2(character->position)));
    }
    else toTarget = character->direction;

    return shootTrack->time > shootTrack->animation->duration / factor && !shootTrack->weight
    && glms_vec2_dot(character->direction, toTarget) > minDot;
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
static versors animateRotation(const KeyframeSetRotation* const setRot, const float time) {
    const KeyframeID id = getCorrespondingKeyframeIndex(
	time, setRot->numKeyframes, setRot->timestamps
    );

    if (id) return glms_quat_slerp(
	setRot->rotationVersors[id - 1], setRot->rotationVersors[id], getAlpha(time, setRot->timestamps + id - 1)
    );

    return setRot->rotationVersors[id];
}
static vec3s animateTranslation(const KeyframeSetTranslation* const setTrans, const float time) {
    const KeyframeID id = getCorrespondingKeyframeIndex(
	time, setTrans->numKeyframes, setTrans->timestamps
    );

    if (id) return glms_vec3_lerp(
	setTrans->translationVectors[id - 1], setTrans->translationVectors[id], 
	getAlpha(time, setTrans->timestamps + id - 1)
    );

    return setTrans->translationVectors[id];
}
static void processTrack(
    const AnimationTrack* const track, const Bone* const bone, float* const totalWeight,
    const AnimationID animId, vec3s* const finalTrans, versors* const finalRot
) {
    vec3s trans;

    const float weight = fminf(track->weight, 1 - *totalWeight);

    const KeyframeSetTranslation* const setTrans = track->animation->setsTrans + bone->id;
    const KeyframeSetRotation* const setRot = track->animation->setsRot + bone->id;

    trans = glms_vec3_scale(animateTranslation(setTrans, track->time), weight);

    //root motion : discard any z coord changes
    if (animId != NOTHING_ANIMATION_DEATH_FROMFRONT && animId != NOTHING_ANIMATION_DEATH_FROMBACK && !bone->id) {
	trans.raw[2] = 0;
    }

    *finalTrans = glms_vec3_add(*finalTrans, trans);
    *finalRot = glms_quat_slerp(*finalRot, animateRotation(setRot, track->time), *totalWeight ? weight : 1);

    //glm_quat_mul(finalRot, rot, finalRot);

    *totalWeight += track->weight;
}
static void multiplyAnimation(const AnimationTrack* const track, const BoneID boneId, mat4s* const transform) {
    glm_vec3_add(
	transform->raw[3], animateTranslation(track->animation->setsTrans + boneId, track->time).raw, transform->raw[3]
    );

    *transform = glms_mat4_mul(
	*transform, glms_quat_mat4(animateRotation(track->animation->setsRot + boneId, track->time))
    );
}
static void handleBone(Character* const character, Bone* const bone, const mat4s parentTransform, bool isTop) {
    mat4s transform;
    vec3s finalTrans;
    versors finalRot;

    float totalWeight;

    finalTrans = glms_vec3_zero();
    finalRot = glms_quat_identity();
    totalWeight = 0;

    isTop = strcmp(bone->name, "mixamorig:Spine2") ? isTop : true;
    /*
    const AnimationTrack* const track = animId < NUM_NOTHING_ANIMATIONS
    ? character->tracks + animId : getSlotWtracks(
    character, animId == WEAPON_ANIMATION_EQUIP ? character->wishSlot : character->currentSlot) + animId;
    */

    //BENCHMARK_BEGIN
    for (AnimationID i = 0; i < ANIMATION_MAX_ENUM && totalWeight < 1; i++) {
	const AnimationID animId = (isTop ? topAnimationsOrder : bottomAnimationsOrder)[i];

	if (animId < NUM_NOTHING_ANIMATIONS) {
	    processTrack(character->tracks + animId, bone, &totalWeight, animId, &finalTrans, &finalRot);
	}
	else {
	    for (SlotID slot = 0; slot < SLOT_MAX_ENUM - 1 && totalWeight < 1; slot++) processTrack(
		character->wtracks[slot] + animId - NUM_NOTHING_ANIMATIONS, bone, &totalWeight, animId, &finalTrans, &finalRot
	    );
	}
    }
    //BENCHMARK_END

    transform = glms_quat_mat4(finalRot);

    glm_vec3_copy(finalTrans.raw, transform.raw[3]);

    if (character->currentSlot && !AnimationTrack_IsFinished(Character_GetWeaponTracks(character, WEAPON_ANIMATION_SHOOT))) {
	multiplyAnimation(character->tracks + WEAPON_ANIMATION_SHOOT, bone->id, &transform);
    }

    mat4s* const instDest = IH_GetUploadPtr(character->instance) + (bone->id * sizeof(mat4));

    transform = glms_mat4_mul(parentTransform, transform);

    if (!strcmp(bone->name, "mixamorig:RightHand")) {
	if (character->currentSlot) {
	    *(mat4s*)IH_GetUploadPtr(character->weaponInstance) = transform;

	    character->muzzlePosition = glms_mat4_mulv3(transform, *character->localMuzzlePosition, 1);

	    //glm_vec3_add(transform[3], VEC2XZ(character->direction, 1 / 3), character->muzzlePosition);
	}
	else glm_mat4_zero(IH_GetUploadPtr(character->weaponInstance));
    }

    *instDest = glms_mat4_mul(transform, bone->off);

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
	    set->translationVectors[id - 1].raw[2], set->translationVectors[id].raw[2], 
	    getAlpha(track->time, set->timestamps + id - 1)
	);

	float speed = track->lastTransZ - transZ;

	if (speed < 0) {
	    const float lastZ = set->translationVectors[set->numKeyframes - 1].raw[2];

	    speed += set->translationVectors[0].raw[2] - lastZ;
	}

	finalSpeed += fmaxf(speed * weight, 0);

	totalWeight += weight;

	track->lastTransZ = transZ;
    }

    return finalSpeed;
}
static float getWishDirection(Character* const character, const vec3s move, vec3s* const dest) {
    const float normalSpeed = 5;

    if (character->target && (isStopped(character) || character->state != CHARACTER_STATE_SPRINT)) {
	const float aimingSpeed = 10;

	//if (AnimationTrack_IsFinished(character->tracks + WEAPON_ANIMATION_SHOOT)) character->going2 = 0;

	*dest = glms_vec3_normalize(glms_vec3_sub(character->target->position, character->position));

	return aimingSpeed;
    }

    if (isStopped(character)) {
	*dest = VEC2XZ(character->direction, 1);

	return normalSpeed;
    }

    if (character->state != CHARACTER_STATE_AIM || character->path.numElements == 1 || glms_vec3_norm2(move) > 1) {
	*dest = move;
    }
    else *dest = glms_vec3_sub(character->path.elements[1], character->position);

    *dest = glms_vec3_normalize(*dest);

    return normalSpeed;
}
static vec2s slerp(const vec2s start, const vec2s end, const float t) {
    vec2s diff;

    const float dot = fminf(fmaxf(glms_vec2_dot(start, end), -1), 1), theta = acosf(dot) * t;

    diff = glms_vec2_scale_as(glms_vec2_sub(end, glms_vec2_scale(start, dot)), sinf(theta));

    return glms_vec2_normalize(glms_vec2_add(start, diff));
}
static void turnCharacter(Character* const character, const vec3s move) {
    vec3s wishDirection;

    const float rotationSpeed = getWishDirection(character, move, &wishDirection);

    character->direction = slerp(character->direction, VEC3XZ2(wishDirection), rotationSpeed * WH_GetDeltaTime());
}
static float processMove(Character* const character) {
    const vec3s move = glms_vec3_sub(character->path.elements[0], character->position);

    turnCharacter(character, move);

    if (!isStopped(character)) {
	const float speed = calculateSpeed(character);

	if (glms_vec3_norm2(move) <= speed * speed) {
	    //glm_vec3_copy(character->path.elements[0], character->position);

	    vec3sArray_RemoveElement(&character->path, 0);
	}

	character->position = glms_vec3_add(character->position, glms_vec3_scale_as(move, speed));

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
	[NOTHING_ANIMATION_SCARED] = "scared"
    };
    const char* const* const wnames[] = {
	INIT_WEAPON_ANIMPACK("pistol", "")
    };

    for (int i = 0; i < NUM_NOTHING_ANIMATIONS; i++) {
	if (!strcmp(animation->name, names[i])) AnimationTrack_Init(character->tracks + i, animation);
    }
    for (SlotID slot = 0; slot < SLOT_MAX_ENUM - 1; slot++) {
	const char* const* const pack = wnames[slot];

	for (int i = 0; i < NUM_WEAPON_ANIMATIONS; i++) {
	    if (!strcmp(animation->name, pack[i])) {
		AnimationTrack_Init(character->wtracks[slot] + i, animation);

		return;
	    }
	}
    }
}
static void stopIfStopped(Character* const character) {
    const AnimationTrack* const wtracks = getSlotWtracks(character, character->wishSlot);

    if ((!character->wishSlot || (!wtracks[WEAPON_ANIMATION_WALK].weight && !wtracks[WEAPON_ANIMATION_SPRINT].weight))
	&& !character->tracks[NOTHING_ANIMATION_WALK].weight && !character->tracks[NOTHING_ANIMATION_SPRINT].weight
    ) character->path.numElements = 0;
}
static void workerThrd(Character* const character) {
    //TODO: this can cause DAMAAGE when new characters are added to CH
    if (character->hasGun) character->target = CH_FindClosestVisibleAimableCharacter(character);
    else character->target = CH_FindClosestVisibleAimableArmedCharacter(character);

    character->aimTaskLock = false;
}
static void fadeAnimations(Character* const character) {
    AnimationTrack_FadeIn(
	character->tracks + NOTHING_ANIMATION_WALK,
	character->state == CHARACTER_STATE_NORMAL && (!character->wishSlot) &&
	isPathLongerThan(character, AnimationTrack_GetPathLengthToStop(character->tracks + NOTHING_ANIMATION_WALK))
    );
    AnimationTrack_FadeIn(
	character->tracks + NOTHING_ANIMATION_SPRINT, !character->wishSlot && character->state == CHARACTER_STATE_SPRINT && 
	isPathLongerThan(character, AnimationTrack_GetPathLengthToStop(character->tracks + NOTHING_ANIMATION_SPRINT))
    );

    FOREACH_WEAPON_SLOT(wtracks)
	const bool sprintCond = character->wishSlot && character->state == CHARACTER_STATE_SPRINT && 
	isPathLongerThan(character, AnimationTrack_GetPathLengthToStop(wtracks + WEAPON_ANIMATION_SPRINT));

	const bool isWish = character->wishSlot == _i + 1;
	const bool isTotallyLegitBusinessMan = character->currentSlot == _i + 1 && isWish;

	AnimationTrack_FadeIn(wtracks + WEAPON_ANIMATION_EQUIP, isSwapping(character) && !character->currentSlot);
	AnimationTrack_FadeIn(wtracks + WEAPON_ANIMATION_UNEQUIP, isSwapping(character) && character->currentSlot);
	AnimationTrack_FadeIn(
	    wtracks + WEAPON_ANIMATION_IDLE, 
	    (isTotallyLegitBusinessMan || wtracks[WEAPON_ANIMATION_EQUIP].weight == 1) && !sprintCond
	);
	AnimationTrack_FadeIn(
	    wtracks + WEAPON_ANIMATION_IDLE_STEADY, 
	    (isTotallyLegitBusinessMan || wtracks[WEAPON_ANIMATION_EQUIP].weight == 1) &&
	    !sprintCond && character->state != CHARACTER_STATE_AIM
	);
	AnimationTrack_FadeIn(
	    wtracks + WEAPON_ANIMATION_WALK, 
	    isWish && character->state != CHARACTER_STATE_SPRINT && 
	    isPathLongerThan(character, AnimationTrack_GetPathLengthToStop(wtracks + WEAPON_ANIMATION_WALK))
	);
	AnimationTrack_FadeIn(wtracks + WEAPON_ANIMATION_SPRINT, sprintCond);
    forend
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
static void animate(Character* const character) {
    mat4s characterMat = glms_mat4_identity();

    characterMat.raw[0][0] = -character->direction.raw[1];
    characterMat.raw[0][2] = character->direction.raw[0];

    characterMat.raw[2][0] = -character->direction.raw[0];
    characterMat.raw[2][2] = -character->direction.raw[1];

    glm_vec3_copy(character->position.raw, characterMat.raw[3]);

    handleBone(character, CH_GetMesh()->bones, characterMat, false);
}

void Character_Init(Character* const character, Mesh* const weaponMesh) {
    vec3sArray_Init(&character->path);

    CH_AddCharacter(character);

    character->instance = IH_NewInstance(CH_GetMesh());
    character->weaponInstance = IH_NewInstance(weaponMesh);

    character->direction.raw[1] = -1;

    character->tracks[NOTHING_ANIMATION_IDLE].weight = 1;

    for (SlotID i = 0; i < SLOT_MAX_ENUM - 1; i++) {
	character->wtracks[i][WEAPON_ANIMATION_SHOOT - NUM_NOTHING_ANIMATIONS].time = INFINITY;
    }

    character->areAnimationsLoaded = false;

    character->localMuzzlePosition = &weaponMesh->highestPoint;
}
bool Character_CanSeeDotCheck(Character* const character, Character* const them) {
    return glms_vec3_dot(VEC2XZ(character->direction, 1), glms_vec3_sub(them->position, character->position)) > 0;
}
bool Character_ShouldProcessShot(Character* character) {
    return CH_GetMesh()->animations && character->state != CHARACTER_STATE_DEAD
    && character->currentSlot && shouldTryShoot(character) && shouldShoot(character);
}
AnimationTrack* Character_GetDeathTrack(Character* const character) {
    const AnimationTrack* const front = character->tracks + NOTHING_ANIMATION_DEATH_FROMFRONT;

    return character->tracks + (front->weight ? NOTHING_ANIMATION_DEATH_FROMFRONT : NOTHING_ANIMATION_DEATH_FROMBACK);
}
AnimationTrack* Character_GetWeaponTracks(Character* const character, const AnimationID trackId) {
    return character->wtracks[character->currentSlot - 1] + trackId - NUM_NOTHING_ANIMATIONS;
}
void Character_BeginSprinting(Character* const character) {
    if (!isTired(character)) character->state = CHARACTER_STATE_SPRINT;
}
void Character_StopSprinting(Character* const character) {
    character->state = CHARACTER_STATE_NORMAL;
}
void Character_ChooseSlot(Character* const character, const SlotID slot) {
    if (character->hasGun) {
	AnimationTrack* const wtracks = getSlotWtracks(character, slot ? slot : character->currentSlot);

	if (!(character->wishSlot = slot) && character->state == CHARACTER_STATE_AIM) {
	    character->state = CHARACTER_STATE_NORMAL;
	}

	wtracks[slot ? WEAPON_ANIMATION_EQUIP : WEAPON_ANIMATION_UNEQUIP].time = 0;
    }
}
void Character_SwitchAim(Character* const character, const bool aim) {
    if (character->hasGun && (!aim || character->target || !isTired(character))) {
	if (aim && !character->wishSlot) Character_ChooseSlot(character, 1);
	if (character->state != CHARACTER_STATE_DEAD) character->state = aim ? CHARACTER_STATE_AIM : CHARACTER_STATE_NORMAL;
    }
}
void Character_GoTo(Character* const character, const vec3s goal, const TriangleID goalTri) {
    if (character->state != CHARACTER_STATE_DEAD) {
	NH_FindPath(character->position, goal, NH_GetClosestTriangle(character->position), goalTri, &character->path);
    }
}
void Character_TryInitAnimations(Character* const character) {
    if (!character->areAnimationsLoaded) {
	foreach (const Animation* const animation, CH_GetMesh()->animations, CH_GetMesh()->numAnimations) 
	    initAnimationPointer(character, animation);
	forend

	character->areAnimationsLoaded = true;
    }
}
void Character_HandleFatigue(Character* const character) {
    if (CH_GetMesh()->animations && character->state != CHARACTER_STATE_DEAD) {
	addFatigue(character);

	if (character->fatigue == 1 && !character->target) character->state = CHARACTER_STATE_NORMAL;
    }
}
void Character_Loop(Character* const character) {
    if (CH_GetMesh()->animations) {
	Character_TryInitAnimations(character);

	if (character->state == CHARACTER_STATE_DEAD) {
	    AnimationTrack* const track = Character_GetDeathTrack(character);

	    AnimationTrack_GoTillEnd(track);
	    AnimationTrack_FadeIn(track, true);
	}
	else {
	    AnimationTrack_Go(character->tracks + NOTHING_ANIMATION_IDLE);
	    AnimationTrack_Go(character->tracks + NOTHING_ANIMATION_WALK);
	    AnimationTrack_Go(character->tracks + NOTHING_ANIMATION_SPRINT);
	    AnimationTrack_Go(character->tracks + NOTHING_ANIMATION_SCARED);

	    FOREACH_WEAPON_SLOT(wtracks)
		AnimationTrack_Go(wtracks + WEAPON_ANIMATION_IDLE);
		AnimationTrack_Go(wtracks + WEAPON_ANIMATION_WALK);
		AnimationTrack_Go(wtracks + WEAPON_ANIMATION_SPRINT);
		AnimationTrack_Go(wtracks + WEAPON_ANIMATION_IDLE_STEADY);
		AnimationTrack_GoTillEnd(wtracks + WEAPON_ANIMATION_SHOOT);
	    forend

	    if (character->tracks[NOTHING_ANIMATION_SCARED].weight) {
		AnimationTrack* const scared = character->tracks + NOTHING_ANIMATION_SCARED;

		const bool isWalking = !isStopped(character) && character->state == CHARACTER_STATE_NORMAL;

		AnimationTrack_FadeIn(scared, !isWalking);

		if (isWalking) scared->weight = fmaxf(scared->weight, 1.f / 4);
	    }

	    if (isSwapping(character)) {
		AnimationTrack* const track = getSlotWtracks(
		    character, character->currentSlot ? character->currentSlot : character->wishSlot
		) + (character->currentSlot ? WEAPON_ANIMATION_UNEQUIP : WEAPON_ANIMATION_EQUIP);

		track->time += WH_GetDeltaTime();

		if (track->time > track->animation->duration) {
		    character->currentSlot = character->currentSlot ? 0 : character->wishSlot;
		}
	    }

	    fadeAnimations(character);
	    stopIfStopped(character);
	    processMove(character);

	    if (!character->aimTaskLock) {
		character->aimTaskLock = true;

		workerThrd(character);
		//TM_AddTask(&(Task){.function = (void*)workerThrd, .argument = character});
	    }

	    if (character->target) {
		if (character->state == CHARACTER_STATE_AIM) {
		    if (shouldTryShoot(character)) {
			AnimationTrack* const shootTrack = Character_GetWeaponTracks(character, WEAPON_ANIMATION_SHOOT);

			if (AnimationTrack_IsFinished(shootTrack)) shootTrack->time = shootTrack->weight = 0;
		    }
		}
		else if (character->hasGun) Character_SwitchAim(character, true);
		else if (character->target->hasGun) {
		    if (!character->tracks[NOTHING_ANIMATION_SCARED].weight) {
			character->tracks[NOTHING_ANIMATION_SCARED].weight = FLOAT_BINONE;
		    }

		    Character_GoTo(character, character->target->position, ESCAPE_FROM_DANGER_MODE);
		    Character_BeginSprinting(character);
		}
	    }

	    if (Character_ShouldProcessShot(character)) {
		R_NewMuzzleFlash((Ray){.position = character->muzzlePosition, .direction = VEC2XZ(character->direction, 1)});

		character->ammoMag--;

		if (character->target) {
		    //actually hit
#ifndef DO_NOT_KILL
		    AnimationTrack* const deathTrack = Character_GetDeathTrack(character->target)->weight ? 
		    Character_GetDeathTrack(character->target) :
		    character->target->tracks + (Character_CanSeeDotCheck(character->target, character)
		    ? NOTHING_ANIMATION_DEATH_FROMFRONT : NOTHING_ANIMATION_DEATH_FROMBACK);

		    //kill
		    if (deathTrack->weight) {
			deathTrack->time = fminf(deathTrack->time, deathTrack->animation->duration * 3 / 4);
		    }
		    else {
			character->target->state = CHARACTER_STATE_DEAD;

			deathTrack->weight = FLOAT_BINONE;

			character->target->target = NULL;

			character->target->path.numElements = 0;
		    }
#endif
		}
	    }
	}

	animate(character);
    }
}

DGH_BEGIN(Character, character, 18) {
    DGH_ARRAYT(character->path);

#define X(type, name) DGH_FIELD(character->name);
    VARS_CHARACTER
#undef X

    DGH_Vec3(character->position.raw, "position:");
    DGH_Vec2(character->direction.raw, "direction:");

    DGH_PTR(character->target);
    DGH_ARRAYN(character->tracks);
DGH_END }
