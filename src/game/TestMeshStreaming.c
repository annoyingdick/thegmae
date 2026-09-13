#include <stdlib.h>
#include <cglm/vec3.h>
#include "engine/InstancesHandler.h"
#include "engine/PhysicsSimulationHandler.h"

static const char* const meshNames[] = {
    "arrow.gltf", "boss.gltf", "cube.gltf", "lod0.gltf", "lod1.gltf", "MosquitoInAmber.gltf"
};

#define SPAWN_RATE_DIV 10
#define DELETE_RATE_DIV 5
#define NUM_MESHES (sizeof(meshNames) / sizeof(*meshNames))

#define ENABLE_REMOVING

static Mesh meshes[NUM_MESHES];
static InstancePtrID ptrs[UINT8_MAX];
static PhysBodyID bodies[UINT8_MAX];

static InstancePtrID numObjects;

void TestMeshStreaming_Loop() {
    //const Time standard = 16666666;

    if (numObjects < UINT8_MAX && rand() < RAND_MAX / SPAWN_RATE_DIV) {
	const MeshID id = rand() % NUM_MESHES;

	if (!Mesh_IsValid(meshes + id)) Mesh_Init(meshes + id, GRAPHICS_PIPELINE_NORMAL, meshNames[id]);

	ptrs[numObjects] = IH_NewInstance(meshes + id);
	bodies[numObjects++] = PSH_CreateBoxBody(true, GLM_VEC3_ZERO, GLM_VEC3_ONE);
    }
#ifdef ENABLE_REMOVING
    else if (numObjects && rand() < RAND_MAX / DELETE_RATE_DIV) {
	IH_DeleteInstance(ptrs[--numObjects]);
	PSH_DeleteBody(bodies[numObjects]);

	for (MeshID i = 0; i < (MeshID)NUM_MESHES; i++) {
	    if (Mesh_IsValid(meshes + i) && !meshes[i].numInstances) {
		Mesh_Destroy(meshes + i);

		break;
	    }
	}
    }
#endif

    for (InstancePtrID i = 0; i < numObjects; i++) PSH_GetTransform(bodies[i], IH_GetUploadPtr(ptrs[i]));
}
