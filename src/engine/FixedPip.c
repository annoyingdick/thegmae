#include <math.h>
#include "TexturesHandler.h"
#include "Pip.h"
#include "FixedPip.h"

#define INIT_NUM_VERTICES 2

#define VERTEX_SIZE (2 * sizeof(vec3) + 2 * sizeof(float))

void FixedPip_Init(FixedPip* const pip, const GLsizeiptr vertexSize) {
    Arena_Init(&pip->verticesArena, INIT_NUM_VERTICES);

    //vec3 position, vec3 direction, float timestamp
    GPUBuffer_Init(&pip->verticesBuffer, vertexSize * INIT_NUM_VERTICES, GL_DYNAMIC_STORAGE_BIT);
}
MeshID FixedPip_NewMesh(FixedPip* const pip, const GLsizeiptr vertexSize, RegionSize* const outNewSize) {
    const RegionSize oldSize = pip->verticesArena.size;

    Region region;

    if (Arena_RequestRegion(&pip->verticesArena, &region, outNewSize, 1)) pip->numMeshes++;
    if (*outNewSize) {
	GL_CHECK(GPUBuffer_Realloc(
	    &pip->verticesBuffer, oldSize * vertexSize, *outNewSize * vertexSize, GL_DYNAMIC_STORAGE_BIT
	));
    }

    return region.position;
}
void FixedPip_Run(const FixedPip* const pip) {
    GPUBuffer_BindBase(pip->verticesBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_VERTICES);

    ShaderProgram_Use(pip->program);
}
