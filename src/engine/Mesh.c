#include <cglm/box.h>
#include "def.h" // IWYU pragma: keep
#include "TexturesHandler.h"
#include "DebugGuiHandler.h"

#define REDEF_PRINTFD
#include "ModelsHandler.h"

static void initMesh(Mesh* const mesh, const PipID pipId) {
    mesh->pipId = pipId;
    mesh->numInstances = mesh->numAnimations = 0;
    mesh->animations = NULL;
    mesh->usedTextures = NULL;

    //important, very important
    mesh->indicesRegion.size = 1;

    glm_aabb_invalidate(mesh->bounding);
}
static void makeCommand(Mesh* const mesh) {
    mesh->id = R_NewMesh(mesh->pipId);

    printfd("Id assigned: %u\n", mesh->id);
}
static void loadMesh(Mesh* const mesh) {
    //mesh->numAnimations = GLTF_LoadRig(gltf, &mesh->rootBone, &mesh->animations, &mesh->numBones);

    /*
    mesh->numUsedTextures = GLTF_LoadMesh(gltf, &(GLTFLoadMeshInfo){
	.pipId = mesh->pipId,

	.outVerticesRegion = &mesh->verticesRegion, .outIndicesRegion = &mesh->indicesRegion,
	.outUsedTextures = &mesh->usedTextures
    });
    */

    makeCommand(mesh);
}

void Mesh_Init(Mesh* const mesh, const PipID pipId, const char fileName[const]) {
    printfd("Loading a mesh: %s\n", fileName);

    initMesh(mesh, pipId);
    loadMesh(mesh);

    mesh->selfInTask = ModelsHandler_BeginLoadingTask(&(ModelLoadInfo){
	.meshId = mesh->id,
	.pipId = pipId,
	.fileName = strdup(fileName),
	.mesh = mesh
    });
}
void Mesh_InitWithData(Mesh* const mesh, const PipID pipId, const MeshInitWithDataInfo info) {
    initMesh(mesh, pipId);

    //mesh->numUsedTextures = info.numUsedTextures;

    //allocTextures(mesh);

    /*for (TextureID i = 0; i < info.numUsedTextures; i++) {
	mesh->usedTextures[i] = loadTexture(NULL, info.usedTextures[i]);
    }

    if (info.numUsedTextures) {
	const size_t numFloats = R_GetVertexSizeByPipelineId(pipId) / sizeof(float);

	for (size_t i = 0; i < info.verticesSize / numFloats; i++) {
	    info.vertices[(i * numFloats) + 3] += (float)mesh->usedTextures[0];
	}
    }
    */

    for (unsigned int i = 0; i < info.numIndices; i++) {
	printf("%u ", info.indices[i]);
    }

    R_UploadIndices(&mesh->indicesRegion, info.numIndices, info.indices);
    R_UploadVertices(
	mesh->pipId, 
	(UploadVerticesInfo){
	    .count = info.verticesSize, .outRegion = &mesh->verticesRegion, .data = info.vertices
	}
    );

    makeCommand(mesh);
}
bool Mesh_IsValid(const Mesh* const mesh) {
    return mesh->indicesRegion.size;
}
InstanceID Mesh_NewInstance(Mesh* const mesh) {
    const InstanceID baseId = R_NewInstance(mesh->pipId, (NewInstanceInfo){
	.meshId = mesh->id, .numIndices = mesh->indicesRegion.size, .isFirstForThisMesh = !mesh->numInstances
    });

    return baseId + mesh->numInstances++;
}
void Mesh_DeleteInstance(Mesh* const mesh) {
    R_DeleteInstance(mesh->pipId, (DeleteInstanceInfo){.meshId = mesh->id, .isLastForThisMesh = !--mesh->numInstances});
}
void Mesh_Destroy(Mesh* const mesh) {
    const bool isLoaded = mesh->indicesRegion.size != 1;

    R_DeleteMesh(mesh->pipId, mesh->id, isLoaded ? &mesh->indicesRegion : NULL, isLoaded ? &mesh->verticesRegion : NULL);

    if (mesh->bones) {
	Bone_Destroy(mesh->bones + 0);

	free(mesh->bones);

	mesh->bones = NULL;
    }
    if (mesh->usedTextures) {
	foreach (const TextureID* const tex, mesh->usedTextures, mesh->numUsedTextures) 
	    TexturesHandler_UnloadTexture(*tex); 
	forend

	free(mesh->usedTextures);
    }
    if (mesh->numAnimations) {
	for (AnimationID i = 0; i < mesh->numAnimations; i++) Animation_Destroy(mesh->animations + i, mesh->numBones);

	free(mesh->animations);
    }

    //mesh is unloaded, invalidate task so MH will halt the loading of the mesh
    if (!isLoaded) *mesh->selfInTask = NULL; 

    //invalidate
    mesh->indicesRegion.size = 0;
}

DGH_BEGIN(Mesh, mesh, 17) {
    DGH_FIELD(&mesh->verticesRegion);
    DGH_FIELD(&mesh->indicesRegion);

#define X(type, name) DGH_FIELD(mesh->name);
    VARS_MESH // +6
#undef X

    DGH_ARRAY(mesh->animations, mesh->numAnimations);
    DGH_ARRAY(mesh->usedTextures, mesh->numUsedTextures);
    DGH_ARRAY(mesh->bones, mesh->numBones);
DGH_END }
