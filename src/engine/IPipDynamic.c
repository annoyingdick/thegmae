#include "IPipDynamic.h"

#define INSTANCE_DATA_BUFFER_FLAGS GL_MAP_WRITE_BIT + GL_MAP_PERSISTENT_BIT

static void mapInstanceDataBuffers(IPipDynamic* const pip, const GLsizeiptr size) {
    for (RingBufferID i = 0; i < NUM_RING_BUFFERS; i++) {
	pip->instancesDataBufferPointers[i] = GPUBuffer_Map(
	    pip->instancesDataBuffers[i], size, 
	    INSTANCE_DATA_BUFFER_FLAGS + GL_MAP_FLUSH_EXPLICIT_BIT + GL_MAP_UNSYNCHRONIZED_BIT
	);
    }
}

void IPipDynamic_Init(IPipDynamic* const pip, const IPipInitInfo info) {
    const GLsizeiptr size = INIT_NUM_INSTANCES * info.instanceDataSize;

    IPip_Init(&pip->base, info);

    foreach (GPUBuffer* const buffer, pip->instancesDataBuffers, NUM_RING_BUFFERS)
	GPUBuffer_Init(buffer, size, INSTANCE_DATA_BUFFER_FLAGS);
    forend

    mapInstanceDataBuffers(pip, size);
}
MeshID IPipDynamic_NewInstance(IPipDynamic* const pip, const GLsizeiptr instanceDataSize, const NewInstanceInfo info) {
    const InstanceID oldSize = pip->base.instancesAllocationSize, ret = IPip_NewInstance(&pip->base, info);

    if (pip->base.instancesAllocationSize != oldSize) {
	foreach (GPUBuffer* const buffer, pip->instancesDataBuffers, NUM_RING_BUFFERS)
	    GPUBuffer_Realloc(
		buffer, oldSize * instanceDataSize, 
		pip->base.instancesAllocationSize * instanceDataSize, INSTANCE_DATA_BUFFER_FLAGS
	    );
	forend

	mapInstanceDataBuffers(pip, pip->base.instancesAllocationSize * instanceDataSize);

	//if we won't wait for mapping to finish, 
	//some artifacts will be seen for a couple of frames
	glFinish();
    }

    return ret;
}
void IPipDynamic_Run(const IPipDynamic* const pip, const RingBufferID instancesDataBufferId) {
    GPUBuffer_BindBase(
	pip->instancesDataBuffers[instancesDataBufferId], 
	GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_INSTANCES_DATA
    );

    IPip_Run(&pip->base);
}
