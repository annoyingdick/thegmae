#include <math.h>
#include <cglm/vec2.h>
#include "TexturesHandler.h"
#include "Pip.h"
#include "NFixedPipFrame.h"

//positions (vec2) + texcoords (vec2), frames feature 16 vertices
#define MESH_SIZE 16 * sizeof(vec2) * 2
//timestamp (float)
#define MESH_DATA_SIZE sizeof(float)

#define FILLED_CORNER {pip->texture + 1, 0}
#define FREE_CORNER {pip->texture, 1}
#define RIGHT_CORNER {pip->texture + 1, 1}
#define LEFT_CORNER {pip->texture, 0}

//oh yeah, an unmaintainable brick straight out of ass that holds together the entire program
static const Index3D indices[] = {
    0, 1, 4, 1, 4, 5,
    1, 5, 6, 1, 2, 6,
    2, 6, 7, 2, 3, 7,
    4, 7, 8, 7, 8, 11,
    8, 9, 12, 9, 12, 13,
    9, 10, 13, 10, 13, 14,
    10, 11, 14, 11, 14, 15
};

void NFixedPipFrame_Init(NFixedPipFrame* const pip) {
    const char texturePath[] = "textures/frame.png";

    NFixedPip_Init(&pip->base, (ShaderProgramInitInfo_VF){
	.vertexShaderSourceFileName = "gui.vert", .fragmentShaderSourceFileName = "normal.frag"
    }, MESH_SIZE, MESH_DATA_SIZE);

    Uniform_Init(&pip->currentTimeUniform, pip->base.base.program, "currentTime");

    mallocarr(pip->timestamps, INIT_NUM_MESHES);
    mallocarr(pip->indices, INIT_NUM_MESHES);

    pip->texture = (float)TexturesHandler_BeginLoadingTask(texturePath, texturePath);
}
void NFixedPipFrame_NewFrame(NFixedPipFrame* const pip, vec2 size) {
    float windowX, windowY;

    WH_GetWindowSize(&windowX, &windowY);

    const float c = 100 / windowX;

    glm_vec2_div(size, (vec2){windowX, windowY}, size);

    //raw data for the gpu
    const vec2 data[] = {
	{0, 0}, FREE_CORNER,
	{c, 0}, LEFT_CORNER,
	{size[0] - c, 0}, RIGHT_CORNER,
	{size[0], 0}, FREE_CORNER,
	//upper row
	{0, c}, RIGHT_CORNER,
	{c, c}, FILLED_CORNER,
	{size[0] - c, c}, FILLED_CORNER,
	{size[0], c}, LEFT_CORNER,
	//upper row
	{0, size[1] - c}, LEFT_CORNER,
	{c, size[1] - c}, FILLED_CORNER,
	{size[0] - c, size[1] - c}, FILLED_CORNER,
	{size[0], size[1] - c}, RIGHT_CORNER,
	//upper row
	{0, size[1]}, FREE_CORNER,
	{c, size[1]}, RIGHT_CORNER,
	{size[0] - c, size[1]}, LEFT_CORNER,
	{size[0], size[1]}, FREE_CORNER
    };
    Index3D indicesData[ARRAYSIZE(indices)];

    Region indicesRegion;

    RegionSize newSize;

    const MeshID id = NFixedPip_NewMesh(&pip->base, MESH_SIZE, MESH_DATA_SIZE, &newSize);

    if (newSize) {
	reallocarr(pip->timestamps, newSize);
	reallocarr(pip->indices, newSize);
    }

    for (size_t i = 0; i < ARRAYSIZE(indices); i++) indicesData[i] = indices[i] + id;

    R_UploadIndices(&indicesRegion, ARRAYSIZE(indicesData), indicesData);

    pip->timestamps[id] = pip->time;
    pip->indices[id] = indicesRegion.position;

    GL_CHECK(GPUBuffer_SubData(pip->base.base.verticesBuffer, (GLsizeiptr)id * MESH_SIZE, sizeof(data), data));
}
void NFixedPipFrame_Run(NFixedPipFrame* const pip) {
    const float timemult = .0005f;

    pip->time += WH_GetDeltaTime() * timemult;

    for (MeshID i = 0; i < pip->base.base.numMeshes; i++) {
	if (pip->timestamps[i] != NAN && pip->time - pip->timestamps[i] >= 1) {
	    pip->base.base.numMeshes -= Arena_ReturnRegion(&pip->base.base.verticesArena, &(Region){.position = i, .size = 1});

	    pip->timestamps[i] = NAN;
	}
    }

    NFixedPip_Run(&pip->base);

    Uniform_Set_1F(pip->currentTimeUniform, pip->time);

    GL_CHECK(glDrawElements(GL_TRIANGLES, pip->base.base.numMeshes * ARRAYSIZE(indices), GL_UNSIGNED_INT, 0));
}
