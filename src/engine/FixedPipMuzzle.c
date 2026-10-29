#include <math.h>
#include "vdef.h"
#include "TexturesHandler.h"
#include "Pip.h"
#include "FixedPipMuzzle.h"

#define INIT_NUM_VERTICES 2

#define VERTEX_SIZE ((2 * sizeof(vec3)) + (2 * sizeof(float)))

void FixedPipMuzzle_Init(
    FixedPipMuzzle* const pip, const char geometrySource[const], 
    const char texturePath[const], const char faceTexturePath[const]
) {
    FixedPip_Init(&pip->base, VERTEX_SIZE);

    //vec3 position, vec3 direction, float timestamp
    ShaderProgram_Init_VFG(&pip->base.program, (ShaderProgramInitInfo_VFG){
	.vfInfo = {.vertexShaderSourceFileName = "billboard.vert", .fragmentShaderSourceFileName = "normal.frag"},
	.geometryShaderSourceFileName = geometrySource
    });

    Uniform_Init(&pip->currentTimeUniform, pip->base.program, "currentTime");

    pip->texture = (float)TexturesHandler_BeginLoadingTask(texturePath, texturePath);

    mallocarr(pip->timestamps, INIT_NUM_VERTICES);

    //THE FACE TEXTURE MUST BE INITIALIZED RIGHT AFTER THE NORMAL ONE
    TexturesHandler_BeginLoadingTask(faceTexturePath, faceTexturePath);
}
void FixedPipMuzzle_NewMuzzle(FixedPipMuzzle* const pip, const vec3s position, const vec3s direction) {
    const float data[] = {VEC3DUP(position), VEC3DUP(direction), pip->time, pip->texture};

    RegionSize newSize;

    const MeshID id = FixedPip_NewMesh(&pip->base, VERTEX_SIZE, &newSize);

    if (newSize) reallocarr(pip->timestamps, newSize);

    pip->timestamps[id] = pip->time;

    GL_CHECK(GPUBuffer_SubData(pip->base.verticesBuffer, id * VERTEX_SIZE, sizeof(data), data));
}
void FixedPipMuzzle_Run(FixedPipMuzzle* const pip) {
    const float timemult = 20;

    pip->time += WH_GetDeltaTime() * timemult;

    for (MeshID i = 0; i < pip->base.numMeshes; i++) {
	if (pip->timestamps[i] != NAN && pip->time - pip->timestamps[i] >= 1) {
	    pip->base.numMeshes -= Arena_ReturnRegion(&pip->base.verticesArena, &(Region){.position = i, .size = 1});

	    //set position to NAN vector
	    GL_CHECK(GPUBuffer_SubData(pip->base.verticesBuffer, i * VERTEX_SIZE, sizeof(vec3), (vec3){NAN, NAN, NAN}));

	    pip->timestamps[i] = NAN;
	}
    }

    FixedPip_Run(&pip->base);

    Uniform_Set_1F(pip->currentTimeUniform, pip->time);

    GL_CHECK(glDrawArrays(GL_POINTS, 0, pip->base.numMeshes));
}
