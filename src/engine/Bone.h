#ifndef Bone_h_
#define Bone_h_

#include <cglm/types-struct.h>
#include <cgltf.h>

typedef struct Bone Bone;

typedef uint8_t BoneID;

struct Bone {
    mat4s off;

    char* name;
    Bone* children;

    BoneID numChildren;
    BoneID id;
};

void Bone_Init(Bone* bone, Bone* bones, const cgltf_node* joint, BoneID* numBones, const mat4s inverseBindMatrices[]);
void Bone_Destroy(Bone* bone);

void Bone_DrawDebugGui(const Bone* bone, const char name[]);

#endif
