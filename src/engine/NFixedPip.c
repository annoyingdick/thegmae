#include "Pip.h"
#include "NFixedPip.h"

void NFixedPip_Init(
    NFixedPip* const pip, const ShaderProgramInitInfo_VF shaderInfo, 
    const GLsizeiptr vertexSize, const GLsizeiptr meshDataSize
) {
    FixedPip_Init(&pip->base, vertexSize);

    ShaderProgram_Init_VF(&pip->base.program, shaderInfo);

    GPUBuffer_Init(&pip->meshesDataBuffer, INIT_NUM_MESHES * meshDataSize, GL_DYNAMIC_STORAGE_BIT);

    pip->meshesAllocationSize = INIT_NUM_MESHES;
}
MeshID NFixedPip_NewMesh(
    NFixedPip* const pip, const GLsizeiptr vertexSize, const GLsizeiptr meshDataSize, RegionSize* const outNewSize
) {
    const MeshID id = FixedPip_NewMesh(&pip->base, vertexSize, outNewSize);

    if (*outNewSize) {
	GPUBuffer_Realloc(
	    &pip->meshesDataBuffer, pip->meshesAllocationSize * meshDataSize, 
	    *outNewSize * meshDataSize, GL_DYNAMIC_STORAGE_BIT
	);

	pip->meshesAllocationSize = *outNewSize;
    }

    return id;

    //GPUBuffer_SubData(pip->meshesDataBuffer, id * meshDataSize, meshDataSize, data);
}
void NFixedPip_Run(const NFixedPip* const pip) {
    GPUBuffer_BindBase(pip->meshesDataBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_INSTANCES_DATA);

    FixedPip_Run(&pip->base);
}
