#ifndef Bone_h_
#define Bone_h_

#include <cglm/types.h>
#include <cgltf.h>

typedef struct Bone Bone;

typedef uint8_t BoneID;

struct Bone {
    mat4 off;

    char* name;
    Bone* children;

    BoneID numChildren;
    BoneID id;
};

void Bone_Init(Bone* bone, Bone* bones, const cgltf_node* joint, BoneID* numBones, mat4* inverseBindMatrices);
void Bone_Destroy(Bone* bone);

void Bone_DrawDebugGui(const Bone* bone, const char name[]);

#endif
