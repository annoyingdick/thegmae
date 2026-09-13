#ifndef Animation_h_
#define Animation_h_

#include "Bone.h"

typedef uint8_t AnimationID;
typedef size_t KeyframeID;

typedef struct {
    vec3* translationVectors;
    float* timestamps;

    KeyframeID numKeyframes;
} KeyframeSetTranslation;

typedef struct {
    versor* rotationVersors;
    float* timestamps;

    KeyframeID numKeyframes;
} KeyframeSetRotation;

typedef struct {
    vec3* scaleVectors;
    float* timestamps;

    KeyframeID numKeyframes;
} KeyframeSetScale;

typedef struct {
    KeyframeSetTranslation* setsTrans;
    KeyframeSetRotation* setsRot;
    KeyframeSetScale* setsScale;
    char* name;

    float duration;
} Animation;

void Animation_Init(Animation* animation, const cgltf_animation* cgltfAnimation, cgltf_size numJoints);
void Animation_Destroy(const Animation* animation, BoneID numBones);

void Animation_DrawDebugGui(const Animation* animation, const char name[]);

#endif
