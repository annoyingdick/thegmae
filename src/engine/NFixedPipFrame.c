#include <math.h>
#include <cglm/vec2.h>
#include "TexturesHandler.h"
#include "Pip.h"
#include "NFixedPipFrame.h"

//positions (vec2) + texcoords (vec2), frames feature 16 vertices
#define MESH_SIZE 16 * sizeof(vec2) * 2
//timestamp (float)
#define MESH_DATA_SIZE sizeof(float)

//i don't know why it works and i'm too afraid to ask
#define FILLED_CORNER {1, 0}
#define FREE_CORNER {0, 0}
#define RIGHT_CORNER {0, 1}
#define LEFT_CORNER FILLED_CORNER

//oh yeah, an unmaintainable brick straight out of ass that holds together the entire program
static const Index3D indices[] = {
    0, 1, 4, 1, 4, 5,
    1, 5, 6, 1, 2, 6,
    2, 6, 7, 2, 3, 7,
    4, 7, 8, 7, 8, 11,
    8, 12, 13, 8, 9, 13,
    9, 10, 13, 10, 13, 14,
    10, 11, 14, 11, 14, 15
};

static void uploadVertices(const NFixedPipFrame* const pip, vec2 size, vec2 windowSize, const MeshID id) {
    const float cornerSize = 100, x = cornerSize / windowSize[0], y = cornerSize / windowSize[1];

    vec2 nsize;

    glm_vec2_div(size, windowSize, nsize);

    const vec2 data[] = {
	{0, 0}, FREE_CORNER,
	{x, 0}, LEFT_CORNER,
	{nsize[0] - x, 0}, RIGHT_CORNER,
	{nsize[0], 0}, FREE_CORNER,
	//upper row
	{0, y}, RIGHT_CORNER,
	{x, y}, FILLED_CORNER,
	{nsize[0] - x, y}, FILLED_CORNER,
	{nsize[0], y}, LEFT_CORNER,
	//upper row
	{0, nsize[1] - y}, LEFT_CORNER,
	{x, nsize[1] - y}, FILLED_CORNER,
	{nsize[0] - x, nsize[1] - y}, FILLED_CORNER,
	{nsize[0], nsize[1] - y}, RIGHT_CORNER,
	//upper row
	{0, nsize[1]}, FREE_CORNER,
	{x, nsize[1]}, RIGHT_CORNER,
	{nsize[0] - x, nsize[1]}, LEFT_CORNER,
	{nsize[0], nsize[1]}, FREE_CORNER
    };

    GL_CHECK(GPUBuffer_SubData(pip->base.base.verticesBuffer, (GLsizeiptr)id * MESH_SIZE, sizeof(data), data));
}

void NFixedPipFrame_Init(NFixedPipFrame* const pip) {
    const char texturePath[] = "textures/frame.png";

    NFixedPip_Init(&pip->base, (ShaderProgramInitInfo_VF){
	.vertexShaderSourceFileName = "gui.vert", .fragmentShaderSourceFileName = "frame.frag"
    }, MESH_SIZE, MESH_DATA_SIZE);

    Uniform_Init(&pip->currentTimeUniform, pip->base.base.program, "currentTime");
    Uniform_Init(&pip->textureUniform, pip->base.base.program, "textureHandle");

    mallocarr(pip->timestamps, INIT_NUM_MESHES);
    mallocarr(pip->indices, INIT_NUM_MESHES);
    mallocarr(pip->sizes, INIT_NUM_MESHES);

    pip->texture = TexturesHandler_BeginLoadingTask(texturePath, texturePath);
}
void NFixedPipFrame_NewFrame(NFixedPipFrame* const pip, vec2 size) {
    float windowX, windowY;

    WH_GetWindowSize(&windowX, &windowY);

    //raw data for the gpu
    Index3D indicesData[ARRAYSIZE(indices)];

    Region indicesRegion;

    RegionSize newSize;

    const MeshID id = NFixedPip_NewMesh(&pip->base, MESH_SIZE, MESH_DATA_SIZE, &newSize);

    if (newSize) {
	reallocarr(pip->timestamps, newSize);
	reallocarr(pip->indices, newSize);
	reallocarr(pip->sizes, newSize);
    }

    for (size_t i = 0; i < ARRAYSIZE(indices); i++) indicesData[i] = indices[i] + id;

    R_UploadIndices(&indicesRegion, ARRAYSIZE(indicesData), indicesData);

    pip->timestamps[id] = pip->time;
    pip->indices[id] = indicesRegion.position;

    glm_vec2_copy(size, pip->sizes[id]);

    uploadVertices(pip, size, (vec2){windowX, windowY}, id);
}
void NFixedPipFrame_UpdateFrames(const NFixedPipFrame* pip, vec2 windowSize) {
    for (MeshID i = 0; i < pip->base.base.numMeshes; i++) uploadVertices(pip, pip->sizes[i], windowSize, i);
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
    Uniform_Set_1UI(pip->textureUniform, pip->texture);

    GL_CHECK(glDrawElements(GL_TRIANGLES, pip->base.base.numMeshes * ARRAYSIZE(indices), GL_UNSIGNED_INT, 0));
}
