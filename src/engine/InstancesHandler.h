#ifndef InstancesHandler_h_
#define InstancesHandler_h_

#include "Mesh.h"

typedef unsigned int InstancePtrID;

typedef struct {
    InstanceID id;
    Mesh* mesh;
} Instance;

void IH_Init();
InstancePtrID IH_NewInstance(Mesh* mesh);
void IH_DeleteInstance(InstancePtrID ptrId);
void IH_UploadStatic(InstancePtrID ptrId, size_t size, const void* data);
void* IH_GetUploadPtr(InstancePtrID ptrId);

void IH_DrawDebugGui();
void Instance_DrawDebugGui(const Instance* instance, const char name[]);

#endif
