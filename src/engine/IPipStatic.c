#include "IPipStatic.h"

void IPipStatic_Init(IPipStatic* const pip, const IPipInitInfo info) {
    IPip_Init(&pip->base, info);

    GPUBuffer_Init(
	&pip->instancesDataBuffer, 
	INIT_NUM_INSTANCES * info.instanceDataSize, GL_DYNAMIC_STORAGE_BIT
    );
}
MeshID IPipStatic_NewInstance(IPipStatic* const pip, const GLsizeiptr instanceDataSize, const NewInstanceInfo info) {
    const InstanceID oldSize = pip->base.instancesAllocationSize, ret = IPip_NewInstance(&pip->base, info);

    if (pip->base.instancesAllocationSize != oldSize) {
	GPUBuffer_Realloc(
	    &pip->instancesDataBuffer, oldSize * instanceDataSize, 
	    pip->base.instancesAllocationSize * instanceDataSize, GL_DYNAMIC_STORAGE_BIT
	);
    }

    return ret;
}
void IPipStatic_Run(const IPipStatic* const pip) {
    GPUBuffer_BindBase(
	pip->instancesDataBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_INSTANCES_DATA
    );

    IPip_Run(&pip->base);
}
