#include <math.h>
#include "WindowHandler.h"
#include "DebugGuiHandler.h"
#include "AnimationTrack.h"

#define WEIGHT_CHANGE_TIME 0.5f

void AnimationTrack_Init(AnimationTrack* const track, const Animation* const animation) {
    track->animation = animation;

    const KeyframeSetTranslation* const set = animation->setsTrans + 0;

    const float z = set->translationVectors[0][2] - set->translationVectors[set->numKeyframes - 1][2];

    track->averageSpeedZ = z / set->timestamps[set->numKeyframes - 1];

    /*
    track->averageSpeedZ = INFINITY;

    for (KeyframeID i = 1; i < set->numKeyframes; i++) {
	const float z = set->translationVectors[i - 1][2] - set->translationVectors[i][2];

	track->averageSpeedZ = fminf(track->averageSpeedZ, z / (set->timestamps[i] - set->timestamps[i - 1]));
    }
    */
}
float AnimationTrack_GetPathLengthToStop(const AnimationTrack* const track) {
    const float t = track->peakWeight * WEIGHT_CHANGE_TIME;

    return track->averageSpeedZ * t / 2;
}
bool AnimationTrack_IsFinished(const AnimationTrack* const track) {
    return track->time >= track->animation->duration;
}
void AnimationTrack_Go(AnimationTrack* const track) {
    track->time = fmodf(track->time + WH_GetDeltaTime(), track->animation->duration);
}
void AnimationTrack_GoTillEnd(AnimationTrack* const track) {
    track->time = fminf(track->time + WH_GetDeltaTime(), track->animation->duration);
}
void AnimationTrack_FadeIn(AnimationTrack* const track, const bool add) {
    if (add) {
	track->weight = fminf(track->weight + (WH_GetDeltaTime() / WEIGHT_CHANGE_TIME), 1);
	track->peakWeight = track->weight;
    }
    else {
	track->weight = fmaxf(track->weight - (WH_GetDeltaTime() / WEIGHT_CHANGE_TIME), 0);

	if (!track->weight) track->peakWeight = 0;
    }
}

DGH_BEGIN(AnimationTrack, track, 6) {
    DGH_PTR(track->animation);

#define X(type, name) DGH_FIELD(track->name);
    VARS_ANIMATIONTRACK // +5
#undef X
DGH_END }
