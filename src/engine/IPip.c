#include "DrawElementsIndirectCommand.h"
#include "Uniform.h"
#include "IPip.h"

static ShaderProgram newInstanceProgram, deleteInstanceProgram, clearCommandsProgram;
static Uniform baseMeshIdUniform_newInstance, baseMeshIdUniform_deleteInstance;

void IPip_PreInit() {
    ShaderProgram_Init_Compute(&newInstanceProgram, (ShaderProgramInitInfo_Compute){"newInstance.comp"});
    ShaderProgram_Init_Compute(&deleteInstanceProgram, (ShaderProgramInitInfo_Compute){"deleteInstance.comp"});
    ShaderProgram_Init_Compute(&clearCommandsProgram, (ShaderProgramInitInfo_Compute){"clearCommands.comp"});

    Uniform_Init(&baseMeshIdUniform_newInstance, newInstanceProgram, "baseMeshId");
    Uniform_Init(&baseMeshIdUniform_deleteInstance, deleteInstanceProgram, "baseMeshId");
}

void IPip_Init(IPip* const pip, const IPipInitInfo info) {
    Pip_Init(&pip->base, info.baseInfo);

    ShaderProgram_Init_Compute(&pip->processInstancesProgram, info.processInstancesShaderInfo);

    for (int i = 0; i < 2; i++) {
	GL_CHECK(GPUBuffer_Init(&pip->usedInstancesIdsBuffer + i, INIT_NUM_INSTANCES * sizeof(GLuint), GL_NONE));
    }

    pip->numInstances = 0;
    pip->instancesAllocationSize = INIT_NUM_INSTANCES;

    mallocarr(pip->baseInstances, INIT_NUM_MESHES + 1);
}
MeshID IPip_NewMesh(IPip* const pip) {
    RegionSize newSize;
    bool isLast;

    const MeshID id = Pip_NewMesh(&pip->base, &newSize, &isLast);

    if (newSize) reallocarr(pip->baseInstances, newSize);
    if (isLast) {
	pip->baseInstances[id] = pip->numInstances;
	pip->baseInstances[id + 1] = pip->numInstances;
    }

    Pip_UploadMeshFirstTime(&pip->base, (PipUploadMeshFirstTimeInfo){
	.meshId = id,
	.baseInstance = pip->numInstances,
	.numInstances = 0,
	.isCommandLast = isLast
    });

    return id;
}
InstanceID IPip_NewInstance(IPip* const pip, const NewInstanceInfo info) {
    if (++pip->numInstances > pip->instancesAllocationSize) {
	const InstanceID oldSize = pip->instancesAllocationSize;

	//GLuint newDataBuffers[MAX_RING_BUFFERS];

	//gpu rewrites the contents of this buffer every frame so there is no need to copy the previous data
	GL_CHECK(GPUBuffer_Destroy(pip->usedInstancesIdsBuffer));
	GL_CHECK(GPUBuffer_Init(
	    &pip->usedInstancesIdsBuffer, 
	    (pip->instancesAllocationSize += pip->instancesAllocationSize / 2) * (GLsizeiptr)sizeof(GLuint), GL_NONE
	));

	GL_CHECK(GPUBuffer_Realloc(
	    &pip->instancesMeshIdsBuffer, oldSize * (GLsizeiptr)sizeof(GLuint), 
	    pip->instancesAllocationSize * (GLsizeiptr)sizeof(GLuint), GL_NONE
	));
    }

    if (info.isFirstForThisMesh) GL_CHECK(GPUBuffer_SubData(
	pip->base.commandsBuffer, info.meshId * (GLintptr)sizeof(DrawElementsIndirectCommand), sizeof(GLuint), &info.numIndices
    ));

    for (MeshID i = info.meshId + 1; i < pip->base.nextMeshId + 1; i++) ++pip->baseInstances[i];

    GL_CHECK(GPUBuffer_BindBase(pip->base.commandsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_COMMANDS));
    GL_CHECK(GPUBuffer_BindBase(pip->instancesMeshIdsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_INSTANCES_MESH_IDS));

    ShaderProgram_Use(newInstanceProgram);

    GL_CHECK(Uniform_Set_1UI(baseMeshIdUniform_newInstance, info.meshId + 1));

    GL_CHECK(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));
    GL_CHECK(glDispatchCompute(pip->base.nextMeshId - info.meshId, 1, 1));

    return pip->baseInstances[info.meshId];
}
void IPip_DeleteInstance(IPip* const pip, const DeleteInstanceInfo info) {
    --pip->numInstances;

    if (info.isLastForThisMesh) GL_CHECK(glClearNamedBufferSubData(
	pip->base.commandsBuffer.buf, GL_R32UI, info.meshId * (GLintptr)sizeof(DrawElementsIndirectCommand), 
	sizeof(GLuint), GL_RED, GL_UNSIGNED_INT, NULL
    ));

    for (MeshID i = info.meshId + 1; i < pip->base.nextMeshId + 1; i++) --pip->baseInstances[i];

    GPUBuffer_BindBase(pip->base.commandsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_COMMANDS);
    GPUBuffer_BindBase(pip->instancesMeshIdsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_INSTANCES_MESH_IDS);

    //glMemoryBarrier(GL_NONE);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    ShaderProgram_Use(deleteInstanceProgram);

    GL_CHECK(Uniform_Set_1UI(baseMeshIdUniform_deleteInstance, info.meshId + 1));

    GL_CHECK(glDispatchCompute(pip->base.nextMeshId - info.meshId, 1, 1));
}
void IPip_Run(const IPip* const pip) {
    Pip_Run(&pip->base);

    GPUBuffer_BindBase(pip->usedInstancesIdsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_USED_INSTANCES_IDS);
    GPUBuffer_BindBase(pip->instancesMeshIdsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_INSTANCES_MESH_IDS);

    ShaderProgram_Dispatch(clearCommandsProgram, pip->base.nextMeshId);
    ShaderProgram_Dispatch(pip->processInstancesProgram, pip->numInstances);

    Pip_Draw(&pip->base);
}
