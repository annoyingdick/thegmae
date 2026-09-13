#ifndef AnimationTrack_h_
#define AnimationTrack_h_

#include <stdbool.h>
#include "Animation.h"

#define VARS_ANIMATIONTRACK \
X(float, time) \
X(float, weight) \
X(float, peakWeight) \
X(float, lastTransZ) \
X(float, averageSpeedZ)

typedef struct {
    const Animation* animation;

#define X(type, name) type name;
    VARS_ANIMATIONTRACK
#undef X
} AnimationTrack;

void AnimationTrack_Init(AnimationTrack* track, const Animation* animation);
float AnimationTrack_GetPathLengthToStop(const AnimationTrack* track);
bool AnimationTrack_IsFinished(const AnimationTrack* track);
void AnimationTrack_Go(AnimationTrack* track);
void AnimationTrack_GoTillEnd(AnimationTrack* track);
void AnimationTrack_FadeIn(AnimationTrack* track, bool add);

void AnimationTrack_DrawDebugGui(const AnimationTrack* track, const char name[]);

#endif
