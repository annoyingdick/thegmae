#include "TexturesHandler.h"
#include "Pip.h"
#include "BillboardPip.h"

#define INIT_NUM_VERTICES 2

#define VERTEX_SIZE (2 * sizeof(vec3) + 2 * sizeof(float))

void BillboardPip_Init(
    BillboardPip* const pip, const char geometrySource[const], const char texturePath[const], const char faceTexturePath[const]
) {
    Arena_Init(&pip->verticesArena, INIT_NUM_VERTICES);

    //vec3 position, vec3 direction, float timestamp
    GPUBuffer_Init(&pip->verticesBuffer, INIT_NUM_VERTICES * VERTEX_SIZE, GL_DYNAMIC_STORAGE_BIT);
    ShaderProgram_Init_VFG(&pip->program, (ShaderProgramInitInfo_VFG){
	.vfInfo = {.vertexShaderSourceFileName = "billboard.vert", .fragmentShaderSourceFileName = "normal.frag"},
	.geometryShaderSourceFileName = geometrySource
    });

    pip->texture = (float)TexturesHandler_BeginLoadingTask(texturePath, texturePath);

    //THE FACE TEXTURE MUST BE INITIALIZED RIGHT AFTER THE NORMAL ONE
    TexturesHandler_BeginLoadingTask(faceTexturePath, faceTexturePath);
}
BillboardID BillboardPip_NewBillboard(BillboardPip* const pip, vec3 position, vec3 direction) {
    const RegionSize oldSize = pip->verticesArena.size;

    const float data[] = {VEC3DUP(position), VEC3DUP(direction), 0, pip->texture};

    Region region;

    RegionSize newSize;

    if (Arena_RequestRegion(&pip->verticesArena, &region, &newSize, 1)) pip->numVertices++;
    if (newSize) GL_CHECK(GPUBuffer_Realloc(
	&pip->verticesBuffer, oldSize * VERTEX_SIZE, newSize * VERTEX_SIZE, GL_DYNAMIC_STORAGE_BIT
    ));

    GL_CHECK(GPUBuffer_SubData(pip->verticesBuffer, region.position * VERTEX_SIZE, sizeof(data), data));

    return region.position;
}
void BillboardPip_Run(const BillboardPip* const pip) {
    GPUBuffer_BindBase(pip->verticesBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_VERTICES);

    ShaderProgram_Use(pip->program);
    GL_CHECK(glDrawArrays(GL_POINTS, 0, pip->numVertices));
}
