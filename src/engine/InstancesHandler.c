#include "def.h"
#include "DebugGuiHandler.h"
#include "InstancesHandler.h"

//#define IN_DEBUG
#define INIT_NUM_INSTANCE_POINTERS 2

static Arena arena;

static InstancePtrID nextPtrId;

static Instance* instances;

void IH_Init() {
    Arena_Init(&arena, INIT_NUM_INSTANCE_POINTERS);

    mallocarr(instances, INIT_NUM_INSTANCE_POINTERS);
}
InstancePtrID IH_NewInstance(Mesh* const mesh) {
    Region region;

    RegionSize newSize;

    if (Arena_RequestRegion(&arena, &region, &newSize, 1)) ++nextPtrId;
    if (newSize) reallocarr(instances, newSize);

    /*
    bool lol = arena.freeLocations.numElements && arena.freeLocations.elements[0].offset + arena.freeLocations.elements[0].size == arena.size && arena.freeLocations.elements[0].offset != nextPtrId;

    if (lol) {}
    */

    instances[region.position].id = Mesh_NewInstance(mesh);
    instances[region.position].mesh = mesh;

    for (InstancePtrID i = 0; i < nextPtrId; i++) {
	const Mesh* const iMesh = instances[i].mesh;

	if (!ISINVALID(instances[i].id) && iMesh->pipId == mesh->pipId && iMesh->id > mesh->id) ++instances[i].id;
    }

#ifdef IN_DEBUG
    printf("IH NEW INSTANCE: %u\n", region.position);
    for (InstancePtrID i = 0; i < nextPtrId; i++) {
	printf("%u ", usedMeshes[i]->pipId);
    }
    putchar('\n');
    for (InstancePtrID i = 0; i < nextPtrId; i++) {
	printf("%u ", ids[i]);
    }
    putchar('\n');
#endif

    return region.position;
}
void IH_DeleteInstance(const InstancePtrID ptrId) {
    Mesh* const mesh = instances[ptrId].mesh;

#ifdef DEBUG
    if (!Mesh_IsValid(mesh)) {
	throwFatal("Instances handler error occurred!", "Tried to delete instance that had an invalid mesh");
    }
#endif

    nextPtrId -= Arena_ReturnRegion(&arena, &(Region){.position = ptrId, .size = 1});

    for (InstancePtrID i = 0; i < nextPtrId; i++) {
	const Mesh* const iMesh = instances[i].mesh;

	if (!ISINVALID(instances[i].id) && iMesh->pipId == mesh->pipId && iMesh->id > mesh->id) --instances[i].id;
    }

    INVALIDATE(instances[ptrId].id);

#ifdef IN_DEBUG
    printf("IH DELETE INSTANCE: %u\n", ptrId);
    for (InstancePtrID i = 0; i < nextPtrId; i++) {
	printf("%u ", ids[i]);
    }
    printf("\n");
#endif

    Mesh_DeleteInstance(mesh);
}
void IH_UploadStatic(const InstancePtrID ptrId, const size_t size, const void* const data) {
    R_UploadStatic(instances[ptrId].id, size, data);
}
void* IH_GetUploadPtr(const InstancePtrID ptrId) {
    return R_GetUploadPtr(instances[ptrId].mesh->pipId, instances[ptrId].id);
}

void IH_DrawDebugGui() {
    DGH_FIELD(&arena);
    DGH_FIELD(nextPtrId);
    DGH_ARRAY(instances, nextPtrId);
}

void Instance_DrawDebugGui(const Instance* const instance, const char name[const]) {
    if (DGH_Begin(name, 2)) {
	DGH_FIELD(instance->id);
	DGH_PTR(instance->mesh);

	DGH_End();
    }
}
