#include "NPip.h"

void NPip_Init(NPip* const pip, const NPipInitInfo info) {
    Pip_Init(&pip->base, info.baseInfo);

    GPUBuffer_Init(&pip->meshesDataBuffer, INIT_NUM_MESHES * info.meshDataSize, GL_DYNAMIC_STORAGE_BIT);

    pip->meshesAllocationSize = INIT_NUM_MESHES;
}
MeshID NPip_NewMesh(NPip* const pip, const GLsizeiptr meshDataSize) {
    RegionSize newSize;
    bool isLast;

    const MeshID id = Pip_NewMesh(&pip->base, &newSize, &isLast);

    Pip_UploadMeshFirstTime(&pip->base, (PipUploadMeshFirstTimeInfo){
	.meshId = id, .baseInstance = 0, .numInstances = 1, .isCommandLast = isLast
    });

    if (newSize) {
	GPUBuffer_Realloc(
	    &pip->meshesDataBuffer, pip->meshesAllocationSize * meshDataSize,
	    newSize * meshDataSize, GL_DYNAMIC_STORAGE_BIT
	);

	pip->meshesAllocationSize = newSize;
    }

    //GPUBuffer_SubData(pip->meshesDataBuffer, id * meshDataSize, meshDataSize, data);

    return id;
}
void NPip_Run(const NPip* const pip) {
    Pip_Run(&pip->base);

    GPUBuffer_BindBase(pip->meshesDataBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_INSTANCES_DATA);
    
    Pip_Draw(&pip->base);
}
