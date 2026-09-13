#include <math.h>
#include "def.h"
#include "Bone.h"
#include "DebugGuiHandler.h"
#include "Animation.h"

void Animation_Init(Animation* const animation, const cgltf_animation* const cgltfAnimation, const cgltf_size numJoints) {
    callocarr(animation->setsTrans, numJoints);
    callocarr(animation->setsRot, numJoints);
    callocarr(animation->setsScale, numJoints);

    animation->name = strdup(cgltfAnimation->name);

    animation->duration = 0;

    foreach (const cgltf_animation_channel* channel, cgltfAnimation->channels, cgltfAnimation->channels_count)
	const KeyframeID numKeyframes = channel->sampler->input->count;

	const BoneID boneId = *(BoneID*)channel->target_node->extras.data;

	switch (channel->target_path) {
	case cgltf_animation_path_type_translation:
	    animation->setsTrans[boneId].numKeyframes = numKeyframes;

	    mallocarr(animation->setsTrans[boneId].timestamps, numKeyframes);
	    mallocarr(animation->setsTrans[boneId].translationVectors, numKeyframes);

	    cgltf_accessor_unpack_floats(channel->sampler->input, animation->setsTrans[boneId].timestamps, SIZE_MAX);
	    cgltf_accessor_unpack_floats(
		channel->sampler->output, animation->setsTrans[boneId].translationVectors[0], SIZE_MAX
	    );

	    animation->duration = fmaxf(animation->duration, animation->setsTrans[boneId].timestamps[numKeyframes - 1]);

	    break;
	case cgltf_animation_path_type_rotation:
	    animation->setsRot[boneId].numKeyframes = numKeyframes;

	    mallocarr(animation->setsRot[boneId].timestamps, numKeyframes);
	    mallocarr(animation->setsRot[boneId].rotationVersors, numKeyframes);

	    cgltf_accessor_unpack_floats(channel->sampler->input, animation->setsRot[boneId].timestamps, SIZE_MAX);
	    cgltf_accessor_unpack_floats(channel->sampler->output, animation->setsRot[boneId].rotationVersors[0], SIZE_MAX);

	    animation->duration = fmaxf(animation->duration, animation->setsRot[boneId].timestamps[numKeyframes - 1]);

	    break;
	case cgltf_animation_path_type_scale:
	    animation->setsScale[boneId].numKeyframes = numKeyframes;

	    mallocarr(animation->setsScale[boneId].timestamps, numKeyframes);
	    mallocarr(animation->setsScale[boneId].scaleVectors, numKeyframes);

	    cgltf_accessor_unpack_floats(channel->sampler->input, animation->setsScale[boneId].timestamps, SIZE_MAX);
	    cgltf_accessor_unpack_floats(channel->sampler->output, animation->setsScale[boneId].scaleVectors[0], SIZE_MAX);

	    break;
	case cgltf_animation_path_type_weights:
	    break;
	default:
	    throwFatal("Invalid animation!", "Invalid animation track");
	}
    forend
}
void Animation_Destroy(const Animation* animation, const BoneID numBones) {
    foreach (const KeyframeSetTranslation* const set, animation->setsTrans, numBones)
	free(set->timestamps);
	free(set->translationVectors);
    forend
    foreach (const KeyframeSetRotation* const set, animation->setsRot, numBones)
	free(set->timestamps);
	free(set->rotationVersors);
    forend
    foreach (const KeyframeSetScale* const set, animation->setsScale, numBones)
	free(set->timestamps);
	free(set->scaleVectors);
    forend

    free(animation->setsTrans);
    free(animation->setsRot);
    free(animation->setsScale);
}

DGH_BEGIN(Animation, animation, 2) {
    DGH_FIELD(animation->name);
    DGH_FIELD(animation->duration);
DGH_END }
