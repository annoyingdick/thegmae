#include <stddef.h>
#include "def.h"
#include "DrawElementsIndirectCommand.h"
#include "Uniform.h"
#include "Render.h"
#include "DebugGuiHandler.h"
#include "Pip.h"

#define INIT_NUM_VERTICES 2
#define INIT_NUM_MESHES 2

static GPUBuffer uniformsBuffer;

static ShaderProgram clearCommandsProgram, newInstanceProgram, deleteInstanceProgram;
static Uniform baseMeshIdUniform_newInstance, baseMeshIdUniform_deleteInstance;

static void uploadCommandData(const Pip* const pip, const RegionPosition meshId, const bool isCommandLast) {
    const GLuint data[] = {
	//if this mesh has zero instances, "count" field must be zero too
	//firstIndex: 0, firstVertex: 0, baseInstance: pip->numInstances, count: 0, instanceCount: 0
	0, 0, pip->numInstances, 0, 0, 0, 0, pip->numInstances
    };

    const GLintptr commandSize = sizeof(DrawElementsIndirectCommand);
    const GLintptr offset = offsetof(DrawElementsIndirectCommand, firstIndex);

    if (isCommandLast) {
	pip->baseInstances[meshId] = pip->numInstances;
	pip->baseInstances[meshId + 1] = pip->numInstances;
    }

    //printfd("Uploading command data to gpu\n");

    GL_CHECK(GPUBuffer_SubData(
	pip->commandsBuffer,
	(meshId * commandSize) + offset, 
	//am i witnessing some life sentence crime?
	(GLsizeiptr)(2 + (isCommandLast ? (commandSize / sizeof(GLuint)) + 1 : 0)) * (GLsizeiptr)sizeof(GLuint),
	data
    ));
}

void Pip_PreInit() {
    GL_CHECK(GPUBuffer_Init(&uniformsBuffer, sizeof(mat4), GL_DYNAMIC_STORAGE_BIT));
    GL_CHECK(GPUBuffer_BindBase(uniformsBuffer, GL_UNIFORM_BUFFER, 0));

    ShaderProgram_Init_Compute(&clearCommandsProgram, (ShaderProgramInitInfo_Compute){"clearCommands.comp"});
    ShaderProgram_Init_Compute(&newInstanceProgram, (ShaderProgramInitInfo_Compute){"newInstance.comp"});
    ShaderProgram_Init_Compute(&deleteInstanceProgram, (ShaderProgramInitInfo_Compute){"deleteInstance.comp"});

    Uniform_Init(&baseMeshIdUniform_newInstance, newInstanceProgram, "baseMeshId");
    Uniform_Init(&baseMeshIdUniform_deleteInstance, deleteInstanceProgram, "baseMeshId");
}
void Pip_PreRun(const mat4 pvMat) {
    GL_CHECK(GPUBuffer_SubData(uniformsBuffer, 0, sizeof(mat4), pvMat));
}

void Pip_Init(Pip* const pip, const PipInitInfo info) {
    const GLsizeiptr sizes[] = {
	INIT_NUM_VERTICES * info.vertexSize,
	(INIT_NUM_MESHES + 1) * sizeof(DrawElementsIndirectCommand),
	INIT_NUM_INSTANCES * sizeof(GLuint),
	INIT_NUM_INSTANCES * sizeof(GLuint)
    };
    const GLbitfield flags[] = {
	GL_DYNAMIC_STORAGE_BIT,
	GL_DYNAMIC_STORAGE_BIT,
	GL_NONE,
	GL_NONE
    };

    Arena_Init(&pip->verticesArena, INIT_NUM_VERTICES);
    Arena_Init(&pip->commandsArena, INIT_NUM_MESHES);

    ShaderProgram_Init_VF(&pip->mainProgram, info.mainShaderInfo);
    ShaderProgram_Init_Compute(&pip->processInstancesProgram, info.processInstancesShaderInfo);

    //hardcoded gpubuffers init
    GL_CHECK(GPUBuffer_MultiInit(&pip->verticesBuffer, 4, sizes, flags));

    pip->nextMeshId = pip->numInstances = 0;
    pip->instancesAllocationSize = INIT_NUM_INSTANCES;

    mallocarr(pip->baseInstances, INIT_NUM_MESHES + 1);
}
void Pip_UploadVertices(
    Pip* const pip, const GLsizeiptr vertexSize, const UploadVerticesInfo info
) {
    const RegionSize oldSize = pip->verticesArena.size;

    RegionSize newSize;

    Arena_RequestRegion(&pip->verticesArena, info.outRegion, &newSize, info.count);
    if (newSize) GL_CHECK(GPUBuffer_Realloc(
	&pip->verticesBuffer, oldSize * vertexSize, 
	newSize * vertexSize, GL_DYNAMIC_STORAGE_BIT
    ));

    GL_CHECK(GPUBuffer_SubData(
	pip->verticesBuffer, info.outRegion->position * vertexSize, 
	info.outRegion->size * vertexSize, info.data
    ));
}
MeshID Pip_NewMesh(Pip* const pip) {
    const RegionSize oldSize = pip->commandsArena.size + 1;

    Region region;
    RegionSize newSize;

    const bool isLast = Arena_RequestRegion(&pip->commandsArena, &region, &newSize, 1);

    //there MUST always be one more command
    if (newSize) {
	GL_CHECK(GPUBuffer_Realloc(
	    &pip->commandsBuffer, oldSize * (GLsizeiptr)sizeof(DrawElementsIndirectCommand), 
	    ++newSize * (GLsizeiptr)sizeof(DrawElementsIndirectCommand), GL_DYNAMIC_STORAGE_BIT
	));

	reallocarr(pip->baseInstances, newSize);
    }

    //printfd("LOAD: id=%u : isLast=%u\n", location.offset, isLast);

    uploadCommandData(pip, region.position, isLast);

    if (isLast) ++pip->nextMeshId;

    return region.position;
}
MeshID Pip_NewInstance(Pip* const pip, const NewInstanceInfo info) {
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
	pip->commandsBuffer, info.meshId * (GLintptr)sizeof(DrawElementsIndirectCommand), sizeof(GLuint), &info.numIndices
    ));

    for (MeshID i = info.meshId + 1; i < pip->nextMeshId + 1; i++) ++pip->baseInstances[i];

    GL_CHECK(GPUBuffer_BindBase(pip->commandsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_COMMANDS));
    GL_CHECK(GPUBuffer_BindBase(pip->instancesMeshIdsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_INSTANCES_MESH_IDS));

    ShaderProgram_Use(newInstanceProgram);

    Uniform_Set_1UI(baseMeshIdUniform_newInstance, info.meshId + 1);

    GL_CHECK(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));
    GL_CHECK(glDispatchCompute(pip->nextMeshId - info.meshId, 1, 1));

    return pip->baseInstances[info.meshId];
}
void Pip_UploadMesh(const Pip* const pip, const UploadMeshInfo info) {
    //DRAWCOMMAND'S INSTANCECOUNT IS ALWAYS ZERO BEFORE DRAWING 
    //AND IS SAFE TO CHANGE BEFORE THE CLEARCOMMANDS COMPUTE SHADER RUN

    const GLuint data[] = {
	info.numIndices, 0, info.firstIndex, info.firstVertex
    };

    //const GLsizeiptr off = info.numIndices ? 0 : offsetof(DrawElementsIndirectCommand, firstIndex);

    GL_CHECK(GPUBuffer_SubData(
	pip->commandsBuffer, info.meshId * (GLintptr)sizeof(DrawElementsIndirectCommand), (GLsizeiptr)sizeof(data), data
    ));
}
void Pip_DeleteMesh(Pip* const pip, const MeshID meshId, const Region* const verticesRegion) {
    if (verticesRegion) Arena_ReturnRegion(&pip->verticesArena, verticesRegion);

    pip->nextMeshId -= Arena_ReturnRegion(
	&pip->commandsArena, &(Region){.position = meshId, .size = 1}
    );
}
void Pip_DeleteInstance(Pip* const pip, const DeleteInstanceInfo info) {
    --pip->numInstances;

    if (info.isLastForThisMesh) GL_CHECK(glClearNamedBufferSubData(
	pip->commandsBuffer.buf, GL_R32UI, info.meshId * (GLintptr)sizeof(DrawElementsIndirectCommand), 
	sizeof(GLuint), GL_RED, GL_UNSIGNED_INT, NULL
    ));

    for (MeshID i = info.meshId + 1; i < pip->nextMeshId + 1; i++) --pip->baseInstances[i];

    GPUBuffer_BindBase(pip->commandsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_COMMANDS);
    GPUBuffer_BindBase(pip->instancesMeshIdsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_INSTANCES_MESH_IDS);

    ShaderProgram_Use(deleteInstanceProgram);

    Uniform_Set_1UI(baseMeshIdUniform_deleteInstance, info.meshId + 1);

    //glMemoryBarrier(GL_NONE);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    GL_CHECK(glDispatchCompute(pip->nextMeshId - info.meshId, 1, 1));
}
//'run' sounds cooler than either 'draw' or 'loop'
void Pip_Run(const Pip* const pip) {
    GPUBuffer_Bind(pip->commandsBuffer, GL_DRAW_INDIRECT_BUFFER);
    GPUBuffer_BindBase(pip->verticesBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_VERTICES);
    GPUBuffer_BindBase(pip->commandsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_COMMANDS);
    GPUBuffer_BindBase(pip->usedInstancesIdsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_USED_INSTANCES_IDS);
    GPUBuffer_BindBase(pip->instancesMeshIdsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_INSTANCES_MESH_IDS);

    ShaderProgram_Use(clearCommandsProgram);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    GL_CHECK(glDispatchCompute(pip->nextMeshId, 1, 1));

    ShaderProgram_Use(pip->processInstancesProgram);
    //glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
    GL_CHECK(glDispatchCompute(pip->numInstances, 1, 1));

    ShaderProgram_Use(pip->mainProgram);
    GL_CHECK(glMemoryBarrier(GL_COMMAND_BARRIER_BIT)); //this is what shitty documentation does to mf
    GL_CHECK(glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, pip->nextMeshId, 0));
}

DGH_BEGIN(Pip, pip, 20) {
    DGH_FIELD(&pip->verticesArena);
    DGH_FIELD(&pip->commandsArena);
    DGH_FIELD(pip->nextMeshId);
    DGH_FIELD(pip->numInstances);
    DGH_FIELD(pip->instancesAllocationSize);
    DGH_ARRAY(pip->baseInstances, pip->numInstances);
DGH_END }
