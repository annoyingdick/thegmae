#include <stddef.h>
#include "DrawElementsIndirectCommand.h"
#include "Render.h"
#include "Pip.h"

#define INIT_NUM_VERTICES 2

void Pip_Init(Pip* const pip, const PipInitInfo info) {
    const GLsizeiptr sizes[] = {
	INIT_NUM_VERTICES * info.vertexSize,
	(INIT_NUM_MESHES + 1) * sizeof(DrawElementsIndirectCommand)
    };
    const GLbitfield flags[] = {
	GL_DYNAMIC_STORAGE_BIT,
	GL_DYNAMIC_STORAGE_BIT
    };

    Arena_Init(&pip->verticesArena, INIT_NUM_VERTICES);
    Arena_Init(&pip->commandsArena, INIT_NUM_MESHES);

    ShaderProgram_Init_VF(&pip->mainProgram, info.mainShaderInfo);

    //hardcoded gpubuffers init
    GL_CHECK(GPUBuffer_MultiInit(&pip->verticesBuffer, 2, sizes, flags));

    pip->nextMeshId = 0;
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
MeshID Pip_NewMesh(Pip* const pip, RegionSize* const outNewSize, bool* const outIsLast) {
    const RegionSize oldSize = pip->commandsArena.size + 1;

    Region region;

    *outIsLast = Arena_RequestRegion(&pip->commandsArena, &region, outNewSize, 1);

    //there MUST always be one more command
    if (*outNewSize) GL_CHECK(GPUBuffer_Realloc(
	&pip->commandsBuffer, oldSize * (GLsizeiptr)sizeof(DrawElementsIndirectCommand), 
	++*outNewSize * (GLsizeiptr)sizeof(DrawElementsIndirectCommand), GL_DYNAMIC_STORAGE_BIT
    ));

    //printfd("LOAD: id=%u : isLast=%u\n", location.offset, isLast);

    if (*outIsLast) ++pip->nextMeshId;

    return region.position;
}

void Pip_UploadMesh(const Pip* const pip, const UploadMeshInfo info, const InstanceID numInstances) {
    //DRAWCOMMAND'S INSTANCECOUNT IS ALWAYS ZERO IN THE INSTANCED PIPELINES BEFORE DRAWING 
    //AND IS SAFE TO CHANGE BEFORE THE CLEARCOMMANDS COMPUTE SHADER RUN

    const GLuint data[] = {
	info.numIndices, numInstances, info.firstIndex, info.firstVertex
    };

    //const GLsizeiptr off = info.numIndices ? 0 : offsetof(DrawElementsIndirectCommand, firstIndex);

    GL_CHECK(GPUBuffer_SubData(
	pip->commandsBuffer, info.meshId * (GLintptr)sizeof(DrawElementsIndirectCommand), (GLsizeiptr)sizeof(data), data
    ));
}
void Pip_UploadMeshFirstTime(Pip* const pip, const PipUploadMeshFirstTimeInfo info) {
    const GLuint data[] = {
	//if this mesh has zero instances, "count" field must be zero too
	//firstIndex: 0, firstVertex: 0, baseInstance: pip->numInstances, count: 0, instanceCount: 0
	0, 0, info.baseInstance, 0, info.numInstances, 0, 0, info.baseInstance
    };

    const GLintptr commandSize = sizeof(DrawElementsIndirectCommand);
    const GLintptr offset = offsetof(DrawElementsIndirectCommand, firstIndex);

    //printfd("Uploading command data to gpu\n");

    GL_CHECK(GPUBuffer_SubData(
	pip->commandsBuffer,
	(info.meshId * commandSize) + offset, 
	//am i witnessing some life sentence crimes here right now?
	(GLsizeiptr)(2 + (info.isCommandLast ? (commandSize / sizeof(GLuint)) + 1 : 0)) * (GLsizeiptr)sizeof(GLuint),
	data
    ));
}
//'run' sounds cooler than either 'draw' or 'loop'
void Pip_Run(const Pip* const pip) {
    /*
#ifdef DEBUG
    if (!pip->baseInstances) {
	throwFatal("Graphics pipeline error has occurred!", "Tried to run a pipeline that is not initialized");
    }
#endif
*/
    GPUBuffer_Bind(pip->commandsBuffer, GL_DRAW_INDIRECT_BUFFER);
    GPUBuffer_BindBase(pip->verticesBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_VERTICES);
    GPUBuffer_BindBase(pip->commandsBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_COMMANDS);

    //glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}
void Pip_Draw(const Pip* const pip) {
    ShaderProgram_Use(pip->mainProgram);
    GL_CHECK(glMemoryBarrier(GL_COMMAND_BARRIER_BIT)); //this is what shitty documentation does to mf
    GL_CHECK(glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, pip->nextMeshId, 0));
}
