#include <cglm/mat4.h>
#include "DebugGuiHandler.h"
#include "Bone.h"

void Bone_Init(
    Bone* const bone, Bone* const bones, const cgltf_node* const joint, 
    BoneID* const numBones, mat4* const inverseBindMatrices
) {
    bone->name = strdup(joint->name);

    bone->numChildren = joint->children_count;
    //bone->children = bone->numChildren ? mallocd(bone->numChildren * sizeof(*bone->children)) : NULL;

    bone->id = *(BoneID*)joint->extras.data;

    bone->children = bones + *numBones;

    *numBones += bone->numChildren;

    glm_mat4_copy(inverseBindMatrices[bone->id], bone->off);

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

DGH_BEGIN(Bone, bone, 10) {
    DGH_FIELD(bone->off);
    DGH_FIELD(bone->name);
    DGH_ARRAY(bone->children, bone->numChildren);
    DGH_FIELD(bone->numChildren);
    DGH_FIELD(bone->id);
DGH_END }
