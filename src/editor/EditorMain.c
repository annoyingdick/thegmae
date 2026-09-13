#include <cglm/affine.h>
#include <cglm/box.h>
#include <cglm/mat3.h>
#include <cglm/mat4.h>
#include "engine/def.h"
#include "engine/LevelHandler.h"
#include "engine/ShaderProgram.h"
#include "engine/Pip.h"
#include "EditorMain.h"

#define NUM_VERTICES_PER_BOX 8
#define NUM_INDICES_PER_BOX 24
#define VERTICES_SIZE_PER_BOX (GLsizeiptr)(NUM_VERTICES_PER_BOX * sizeof(vec3))
#define INDICES_SIZE_PER_BOX (GLsizeiptr)(NUM_INDICES_PER_BOX * sizeof(GLushort))

static ShaderProgram mainProgram;
static GPUBuffer selectionVerticesBuffer, selectionIndicesBuffer;

static GLsizeiptr selectionBuffersSize = 2;

static bool translateMode;

static int32_t* selections;

static vec3 translated;

static char levelName[LEVEL_NAME_STRING_SIZE];

static void specifyLinesWidth() {
    const GLfloat linesWidth = 10;

    glLineWidth(linesWidth);
}
static void updateData(const InstanceID ptrId, const InstanceID selId, mat4 transform) {
    Mesh* const mesh = LH_GetInstanceMesh(ptrId);

    AABB box;

    const float* const min = box[0], *const max = box[1];

    glm_aabb_transform(mesh->bounding, transform, box);

    const float vertices[] = {
	min[0], min[1], min[2],
	max[0], min[1], min[2],
	min[0], max[1], min[2],
	min[0], min[1], max[2],

	max[0], max[1], max[2],
	min[0], max[1], max[2],
	max[0], min[1], max[2],
	max[0], max[1], min[2]
    };
    const GLushort cubeIndices[] = {
	0, 1,
	0, 2,
	0, 3,
	4, 5,
	4, 6,
	4, 7,
	1, 6,
	1, 7,
	2, 5,
	2, 7,
	3, 5,
	3, 6
    };
    GLushort indices[NUM_INDICES_PER_BOX];

    for (GLushort i = 0; i < NUM_INDICES_PER_BOX; i++) indices[i] = cubeIndices[i] + (selId * NUM_VERTICES_PER_BOX);

    GPUBuffer_SubData(selectionVerticesBuffer, selId * VERTICES_SIZE_PER_BOX, VERTICES_SIZE_PER_BOX, vertices);
    GPUBuffer_SubData(selectionIndicesBuffer, selId * INDICES_SIZE_PER_BOX, INDICES_SIZE_PER_BOX, indices);
}

void EM_Init() {
    ShaderProgram_Init_VF(&mainProgram, (ShaderProgramInitInfo_VF){
	.vertexShaderSourceFileName = "lines.vert", .fragmentShaderSourceFileName = "lines.frag"
    });

    GPUBuffer_Init(&selectionVerticesBuffer, selectionBuffersSize * VERTICES_SIZE_PER_BOX, GL_DYNAMIC_STORAGE_BIT);
    GPUBuffer_Init(&selectionIndicesBuffer, selectionBuffersSize * INDICES_SIZE_PER_BOX, GL_DYNAMIC_STORAGE_BIT);

    specifyLinesWidth();

    selections = callocd(LH_GetInstancesCount(), sizeof(*selections));
}
int32_t* EM_GetSelectionBool(const InstancePtrID ptrId) {
    return selections + ptrId;
}
char* EM_GetLevelNameString() {
    return levelName;
}
void EM_KeyDown(const SDL_Keycode key) {
    if (key == SDLK_G) translateMode = !translateMode;
}
void EM_MouseMotion(const float xrel, const float yrel) {
    bool notEmpty;

    mat3 pv3x3;

    glm_mat4_pick3(R_GetPVmat(), pv3x3);

    notEmpty = false;

    for (size_t i = 0; i < sizeof(mat3) / sizeof(float); i++) {
	if (pv3x3[0][i] != .0f) {
	    notEmpty = true;

	    break;
	}
    }

    if (notEmpty) {
	int width, height;

	mat3 pv3x3inverse;
	vec3 translation;

	WH_GetWindowSize(&width, &height);

	translation[0] = xrel / (float)width;
	translation[1] = -yrel / (float)height;
	translation[2] = 0;

	glm_mat3_inv(pv3x3, pv3x3inverse);
	glm_mat3_mulv(pv3x3inverse, translation, translation);

	glm_vec3_add(translated, translation, translated);
    }
}
void EM_Loop() {
    InstanceID numSelected;

    numSelected = 0;

    for (InstanceID i = 0; i < LH_GetInstancesCount(); i++) {
	if (selections[i]) {
	    mat4 mat;

	    if (++numSelected > selectionBuffersSize) {
		const GLsizeiptr oldSize = selectionBuffersSize;

		GPUBuffer_Realloc(
		    &selectionVerticesBuffer, oldSize * VERTICES_SIZE_PER_BOX, 
		    (selectionBuffersSize += selectionBuffersSize / 2) * VERTICES_SIZE_PER_BOX, GL_DYNAMIC_STORAGE_BIT
		);
		GPUBuffer_Realloc(
		    &selectionIndicesBuffer, oldSize * INDICES_SIZE_PER_BOX, 
		    selectionBuffersSize * INDICES_SIZE_PER_BOX, GL_DYNAMIC_STORAGE_BIT
		);
	    }

	    glm_translated_to(LH_GetInstanceTransform(i), translated, mat);
	    glm_mat4_copy(mat, R_GetUploadPtr(GRAPHICS_PIPELINE_STATIC, i));

	    updateData(i, numSelected - 1, mat);
	}
    }

    ShaderProgram_Use(mainProgram);

    GPUBuffer_BindBase(selectionVerticesBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_VERTICES);
    GPUBuffer_Bind(selectionIndicesBuffer, GL_ELEMENT_ARRAY_BUFFER);

    glDepthFunc(GL_ALWAYS);
    glDrawElements(GL_LINES, numSelected * NUM_INDICES_PER_BOX, GL_UNSIGNED_SHORT, NULL);
}
