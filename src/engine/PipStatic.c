#include "PipStatic.h"

void PipStatic_Init(PipStatic* const pip, const PipInitInfo info) {
    Pip_Init(&pip->base, info);

    GPUBuffer_Init(
	&pip->instancesDataBuffer, 
	INIT_NUM_INSTANCES * info.instanceDataSize, GL_DYNAMIC_STORAGE_BIT
    );
}
MeshID PipStatic_NewInstance(
    PipStatic* const pip, const GLsizeiptr instanceDataSize, const NewInstanceInfo info
) {
    const InstanceID oldSize = pip->base.instancesAllocationSize;

    const MeshID ret = Pip_NewInstance(&pip->base, info);

    if (pip->base.instancesAllocationSize != oldSize) {
	GPUBuffer_Realloc(
	    &pip->instancesDataBuffer, oldSize * instanceDataSize, 
	    pip->base.instancesAllocationSize * instanceDataSize, GL_DYNAMIC_STORAGE_BIT
	);
    }

    return ret;
}
void PipStatic_Run(const PipStatic* const pip) {
    GPUBuffer_BindBase(
	pip->instancesDataBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_INSTANCES_DATA
    );

    Pip_Run(&pip->base);
}
