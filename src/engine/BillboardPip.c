#include <math.h>
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

    Uniform_Init(&pip->currentTimeUniform, pip->program, "currentTime");

    pip->texture = (float)TexturesHandler_BeginLoadingTask(texturePath, texturePath);

    mallocarr(pip->timestamps, INIT_NUM_VERTICES);

    //THE FACE TEXTURE MUST BE INITIALIZED RIGHT AFTER THE NORMAL ONE
    TexturesHandler_BeginLoadingTask(faceTexturePath, faceTexturePath);
}
BillboardID BillboardPip_NewBillboard(BillboardPip* const pip, vec3 position, vec3 direction) {
    const RegionSize oldSize = pip->verticesArena.size;

    const float data[] = {VEC3DUP(position), VEC3DUP(direction), pip->time, pip->texture};

    Region region;

    RegionSize newSize;

    if (Arena_RequestRegion(&pip->verticesArena, &region, &newSize, 1)) pip->numVertices++;
    if (newSize) {
	GL_CHECK(GPUBuffer_Realloc(
	    &pip->verticesBuffer, oldSize * VERTEX_SIZE, newSize * VERTEX_SIZE, GL_DYNAMIC_STORAGE_BIT
	));

	reallocarr(pip->timestamps, newSize);
    }

    pip->timestamps[region.position] = pip->time;

    GL_CHECK(GPUBuffer_SubData(pip->verticesBuffer, region.position * VERTEX_SIZE, sizeof(data), data));

    return region.position;
}
void BillboardPip_Run(BillboardPip* const pip) {
    const float timemult = 20;

    pip->time += WH_GetDeltaTime() * timemult;

    for (BillboardID i = 0; i < pip->numVertices; i++) {
	if (pip->timestamps[i] != NAN && pip->time - pip->timestamps[i] >= 1) {
	    pip->numVertices -= Arena_ReturnRegion(&pip->verticesArena, &(Region){.position = i, .size = 1});

	    //set position to NAN vector
	    GL_CHECK(GPUBuffer_SubData(pip->verticesBuffer, i * VERTEX_SIZE, sizeof(vec3), (vec3){NAN, NAN, NAN}));

	    pip->timestamps[i] = NAN;
	}
    }

    GPUBuffer_BindBase(pip->verticesBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_VERTICES);

    ShaderProgram_Use(pip->program);
    Uniform_Set_1F(pip->currentTimeUniform, pip->time);

    GL_CHECK(glDrawArrays(GL_POINTS, 0, pip->numVertices));
}
