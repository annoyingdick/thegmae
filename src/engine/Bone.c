#include <stdlib.h>
#include <string.h>
#include "Bone.h"

void Bone_Init(
    Bone* const bone, Bone* const bones, const cgltf_node* const joint, 
    BoneID* const numBones, const mat4s inverseBindMatrices[const]
) {
    bone->name = strdup(joint->name);

    bone->numChildren = joint->children_count;
    //bone->children = bone->numChildren ? mallocd(bone->numChildren * sizeof(*bone->children)) : NULL;

    bone->id = *(BoneID*)joint->extras.data;

    bone->children = bones + *numBones;
    bone->off = inverseBindMatrices[bone->id];

    *numBones += bone->numChildren;

    for (size_t i = 0; i < bone->numChildren; i++) {
	Bone_Init(bone->children + i, bones, joint->children[i], numBones, inverseBindMatrices);
    }
}
void Bone_Destroy(Bone* const bone) {
    free(bone->name);

    //invalidate!
    //bone->name = NULL;

    for (size_t i = 0; i < bone->numChildren; i++) Bone_Destroy(bone->children + i);
}
