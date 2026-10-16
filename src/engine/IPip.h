#ifndef IPip_h_
#define IPip_h_

#include "Pip.h"

#define INIT_NUM_INSTANCES 2

//Instanced pipeline struct derived from Pip. It can render multiple instances of one mesh and perform culling on them on gpu 
//side. Useless on its own, because it doesn't have any instance data buffers
typedef struct {
    Pip base;
    GPUBuffer usedInstancesIdsBuffer, instancesMeshIdsBuffer; //private

    ShaderProgram processInstancesProgram;
    InstanceID numInstances, instancesAllocationSize;

    InstanceID* baseInstances;
} IPip;

typedef struct {
    PipInitInfo baseInfo;

    ShaderProgramInitInfo_Compute processInstancesShaderInfo;

    size_t instanceDataSize;
} IPipInitInfo;

void IPip_PreInit();

void IPip_Init(IPip* pip, IPipInitInfo info);
MeshID IPip_NewMesh(IPip* pip);
InstanceID IPip_NewInstance(IPip* pip, NewInstanceInfo info);
void IPip_DeleteInstance(IPip* pip, DeleteInstanceInfo info);
void IPip_Run(const IPip* pip);

#endif
