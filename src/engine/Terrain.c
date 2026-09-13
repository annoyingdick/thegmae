#include <cglm/vec3.h>
#include "Pip.h"
#include "Terrain.h"

#define TERRAIN_SIZE 20
#define NUM_VERTICES (size_t)NUM_VERTICES_PER_PATCH * TERRAIN_SIZE * TERRAIN_SIZE

static void initVertices(GPUBuffer* const buffer) {
    vec3 vertices[NUM_VERTICES];

    for (size_t x = 0; x < TERRAIN_SIZE; x++) {
	for (size_t z = 0; z < TERRAIN_SIZE; z++) {
	    vec3* const data = vertices + (x * NUM_VERTICES_PER_PATCH * TERRAIN_SIZE) + (z * NUM_VERTICES_PER_PATCH);

	    glm_vec3_copy((vec3){(float)x, sinf((float)(x + z)), (float)z}, data[0]);
	    glm_vec3_copy((vec3){(float)x + 1, sinf((float)(x + z + 1)), (float)z}, data[1]);
	    glm_vec3_copy((vec3){(float)x, sinf((float)(x + z + 1)), (float)z + 1}, data[2]);
	    glm_vec3_copy((vec3){(float)x + 1, sinf((float)(x + z + 2)), (float)z + 1}, data[3]);
	}
    }

    GPUBuffer_InitWithData(buffer, (GLsizeiptr)sizeof(vertices), vertices);
}

void Terrain_Init(Terrain* const terrain) {
    ShaderProgram_Init_VFTess(&terrain->mainProgram, (ShaderProgramInitInfo_VFTess){
	.vfInfo = {
	    .vertexShaderSourceFileName = "terrain.vert", .fragmentShaderSourceFileName = "terrain.frag"
	},
	.controlShaderSourceFileName = "terrain.tesc", .evaluationShaderSourceFileName = "terrain.tese"
    });

    /*ShaderProgram_Init_VF(&terrain->mainProgram, (ShaderProgramInitInfo_VF){
	.vertexShaderSourceFileName = "terrain.vert", .fragmentShaderSourceFileName = "terrain.frag"
    });*/

    initVertices(&terrain->verticesBuffer);

    GPUBuffer_BindBase(terrain->verticesBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_TERRAIN_VERTICES);
}
void Terrain_Loop(const Terrain* const terrain) {
    ShaderProgram_Use(terrain->mainProgram);

    glDrawArrays(GL_PATCHES, 0, NUM_VERTICES);
}
