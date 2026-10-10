#include <cglm/cam.h>
#include "def.h"
#include "Uniform.h"
#include "PipStatic.h"
#include "PipDynamic.h"
#include "Camera.h"
#include "TexturesHandler.h"
#include "ModelsHandler.h"
#include "DebugGuiHandler.h"
#include "BillboardPip.h"
#include "Render.h"

#define DO_OPENGL_DEBUG

//#define NOT_UPLOAD_INDICES
//#define NOT_SHOW_TEXTURES

#define INIT_NUM_INDICES 2

#define UNIFORMS_SIZE (2 * sizeof(mat4))

#define CHECKINIT(x) if (!indicesArena.size) throwFatal("Render is not initialized!", x)
#define FLUSH_DYNAMIC_PIPELINE_BUFFER(x) GL_CHECK(glFlushMappedNamedBufferRange( \
    dynamics[x].instancesDataBuffers[readInterpIndex].buf, 0, \
    dynamics[x].base.numInstances * \
    pipelinesInstanceDataSizes[x] \
));

//Compiler at any moment can rearrange any of the variables below so be careful!
//These buffers can be updated by CPU (GL_DYNAMIC_STORAGE)
static Arena indicesArena;
static GPUBuffer indicesBuffer, textureHandlesBuffer, uniformsBuffer; //private
static Uniform interpUniform; //private
static BillboardPip billboardPip;

static GLuint globalVao; //private
static RingBufferID readNormalIndex, writeNormalIndex, readInterpIndex, writeInterpIndex;

static mat4 pvMat;
static mat4 perspectiveMat;

static PipDynamic dynamics[NUM_DYNAMIC_PIPELINES];
static PipStatic statics[GRAPHICS_PIPELINE_MAX_ENUM - NUM_DYNAMIC_PIPELINES];
static const GLsizeiptr pipelinesInstanceDataSizes[] = {
    [GRAPHICS_PIPELINE_NORMAL] = sizeof(mat4),
    [GRAPHICS_PIPELINE_INTERP] = 3 * sizeof(mat4),
    [GRAPHICS_PIPELINE_SKINNED] = MAX_BONES * sizeof(mat4),
    [GRAPHICS_PIPELINE_STATIC] = 2 * sizeof(mat4),
    [GRAPHICS_PIPELINE_GUI] = sizeof(float)
};
static const GLsizeiptr pipelinesVertexSizes[] = {
    [GRAPHICS_PIPELINE_NORMAL] = (
	VERTEX_POSITIONS_SIZE + VERTEX_TEXCOORDS_SIZE) * sizeof(float), 
    [GRAPHICS_PIPELINE_INTERP] = (
	VERTEX_POSITIONS_SIZE + VERTEX_TEXCOORDS_SIZE) * sizeof(float), 
    [GRAPHICS_PIPELINE_SKINNED] = (
	VERTEX_POSITIONS_SIZE + VERTEX_TEXCOORDS_SIZE + VERTEX_WEIGHTS_SIZE) * sizeof(float), 
    [GRAPHICS_PIPELINE_STATIC] = (VERTEX_POSITIONS_SIZE + VERTEX_TEXCOORDS_SIZE) * sizeof(float),
    [GRAPHICS_PIPELINE_GUI] = (VERTEX2D_POSITIONS_SIZE + VERTEX2D_TEXCOORDS_SIZE) * sizeof(float)
};

static GLsync renderSyncs[NUM_RING_BUFFERS];

#ifdef DO_OPENGL_DEBUG
static void message_callback(
    const GLenum source, const GLenum type, const GLuint id, const GLenum severity, const GLsizei length, 
    const GLchar* message, const void* userParam
) {
    const GLuint ignoreIds[] = {
	131185, //info: buffer map annoying message
	131154 //warning: nvidia texture uploads mess with 3d pipeline
    };

    const uint8_t srcStrSize = 16, typeStrSize = 20, sevStrSize = 13;

    char srcStr[srcStrSize], typeStr[typeStrSize], sevStr[sevStrSize];

    //SHUT THE FUCK UP SHUT THE FUCK UP SHUT THE FUCK UP SHUT THE FUCK UP SHUT THE FUCK UP 
    nforeach (const GLuint* const ignore, ignoreIds) if (id == *ignore) return; forend

    //fucking kill me
    switch (source) {
    case GL_DEBUG_SOURCE_API:
	strcpy_s(srcStr, srcStrSize, "OpenGL");

	break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
	strcpy_s(srcStr, srcStrSize, "Window system");

	break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
	strcpy_s(srcStr, srcStrSize, "Shader compiler");

	break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
	strcpy_s(srcStr, srcStrSize, "Third party");

	break;
    case GL_DEBUG_SOURCE_APPLICATION:
	strcpy_s(srcStr, srcStrSize, "Application");

	break;
    default:
	strcpy_s(srcStr, srcStrSize, "Unknown");
    }

    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
	strcpy_s(typeStr, typeStrSize, "Error");

	break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
	strcpy_s(typeStr, typeStrSize, "Deprecated behavior");

	break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
	strcpy_s(typeStr, typeStrSize, "Undefined behavior");

	break;
    case GL_DEBUG_TYPE_PORTABILITY:
	strcpy_s(typeStr, typeStrSize, "Portability");

	break;
    case GL_DEBUG_TYPE_PERFORMANCE:
	strcpy_s(typeStr, typeStrSize, "Performance");

	break;
    case GL_DEBUG_TYPE_MARKER:
	strcpy_s(typeStr, typeStrSize, "Marker");

	break;
    default:
	strcpy_s(typeStr, typeStrSize, "Unknown");
    }

    switch (severity) {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
	strcpy_s(sevStr, sevStrSize, "Notification");

	break;
    case GL_DEBUG_SEVERITY_LOW:
	strcpy_s(sevStr, sevStrSize, "Low");

	break;
    case GL_DEBUG_SEVERITY_MEDIUM:
	strcpy_s(sevStr, sevStrSize, "Medium");

	break;
    case GL_DEBUG_SEVERITY_HIGH:
	strcpy_s(sevStr, sevStrSize, "High");

	break;
    }

    //stupid
    if (userParam && length) {
	//printfd("haha\n");
    }

    printfd("%s : %s : %s : %u : %s\n", srcStr, typeStr, sevStr, id, message);
}
#endif

static GLsync getSync() {
    return glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}
static Pip* getBasePip(const PipID pipId) {
    return pipId >= NUM_DYNAMIC_PIPELINES ? &statics[pipId - NUM_DYNAMIC_PIPELINES].base : &dynamics[pipId].base;
}
static void initGlew() {
    const GLenum glewInitError = glewInit();

    if (glewInitError != GLEW_OK) {
	throwFatal("GLEW error occurred!", (const char*)glewGetErrorString(glewInitError));

	exit(EXIT_FAILURE);
    }
}
static void createGlobalVao() {
    glCreateVertexArrays(1, &globalVao);
}
static void initBuffers() {
    GPUBuffer_Init(&indicesBuffer, INIT_NUM_INDICES * sizeof(Index3D), GL_DYNAMIC_STORAGE_BIT);
    GPUBuffer_Init(&textureHandlesBuffer, INIT_NUM_TEXTURES * sizeof(GLuint64), GL_DYNAMIC_STORAGE_BIT);

    foreach (GLsync* const sync, renderSyncs, NUM_RING_BUFFERS) *sync = getSync(); forend
}
static void setPerspectiveMatrix(const float aspect) {
    const float nearZ = .1f, farZ = 1000;

    glm_perspective(M_PI_2, aspect, nearZ, farZ, perspectiveMat); 
}
static void initPipelines() {
    for (PipID i = 0; i < NUM_DYNAMIC_PIPELINES; i++) {
	const char* const vertexShaderNames[] = {
	    "normal.vert", "interp.vert", "skinned.vert"
	};
	const char* const piShaderNames[] = {
	    "normal.comp", "interp.comp", "skinned.comp"
	};

	PipDynamic_Init(dynamics + i, (PipInitInfo){
	    .mainShaderInfo = {
		.vertexShaderSourceFileName = vertexShaderNames[i],
		.fragmentShaderSourceFileName = "normal.frag"
	    },
	    .processInstancesShaderInfo = {piShaderNames[i]},

	    .instanceDataSize = pipelinesInstanceDataSizes[i],
	    .vertexSize = pipelinesVertexSizes[i]
	});
    }
    for (PipID i = 0; i < GRAPHICS_PIPELINE_MAX_ENUM - NUM_DYNAMIC_PIPELINES; i++) {
	const char* const vertexShaderNames[] = {
	    "static.vert", "gui.vert"
	};
	const char* const piShaderNames[] = {
	    "static.comp", "gui.comp"
	};

	PipStatic_Init(statics + i, (PipInitInfo){
	    .mainShaderInfo = {
		.vertexShaderSourceFileName = vertexShaderNames[i],
		.fragmentShaderSourceFileName = i == 1 ? "gui.frag" : "normal.frag"
	    },
	    .processInstancesShaderInfo = {piShaderNames[i]},

	    .instanceDataSize = pipelinesInstanceDataSizes[i + NUM_DYNAMIC_PIPELINES],
	    .vertexSize = pipelinesVertexSizes[i + NUM_DYNAMIC_PIPELINES]
	});
    }

    BillboardPip_Init(&billboardPip, "billboard.geom", "textures/muzzle.png", "textures/muzzleface.png");
}
static void updateUniforms() {
    mat4 mats[2];

    mat4 inv;

    glm_mat4_copy(pvMat, mats[0]);

    Camera_GetPerspectiveCameraMatrix(GLM_MAT4_IDENTITY, mats[1]);

    glm_mat4_inv(mats[1], inv);
    glm_vec3_copy(inv[3], mats[1][3]);

    //glm_mat4_print(mats[1], stdout);

    //mats[1][2][2] = -mats[1][2][2];

    GL_CHECK(GPUBuffer_SubData(uniformsBuffer, 0, sizeof(mats), mats));
}

void R_Init() {
    initGlew();

    printf("%s%s\n", "OpenGL version: ", glGetString(GL_VERSION));

#ifdef DO_OPENGL_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(message_callback, NULL);
#endif

    createGlobalVao();
    initBuffers();

    //Intel drivers have some issues with texture handles. Perhaps it's because drivers don't
    //initialize buffers with zeros
    R_DeleteTexture(0);
    GPUBuffer_BindBase(
	textureHandlesBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_TEXTURE_HANDLES
    );

    Arena_Init(&indicesArena, INIT_NUM_INDICES);

    //subsystems
    Camera_Init();
    TexturesHandler_Init();
    ModelsHandler_Init();

    printfd("Initializing graphics pipelines\n");

    Pip_PreInit();

    initPipelines();

    GL_CHECK(GPUBuffer_Init(&uniformsBuffer, UNIFORMS_SIZE, GL_DYNAMIC_STORAGE_BIT));
    GL_CHECK(GPUBuffer_BindBase(uniformsBuffer, GL_UNIFORM_BUFFER, 0));

    BillboardPip_NewBillboard(&billboardPip, (vec3){2, 0, -1}, GLM_XUP);

    Uniform_Init(
	&interpUniform, 
	dynamics[GRAPHICS_PIPELINE_INTERP].base.processInstancesProgram, "interp"
    );

    glPatchParameteri(GL_PATCH_VERTICES, NUM_VERTICES_PER_PATCH);
    glClearColor(.0f, .0f, .0f, 1.0f);
}
MeshID R_NewMesh(const PipID pipId) {
    return Pip_NewMesh(getBasePip(pipId));
}
InstanceID R_NewInstance(const PipID pipId, const NewInstanceInfo info) {
    if (pipId >= NUM_DYNAMIC_PIPELINES) {
	return PipStatic_NewInstance(statics + pipId - NUM_DYNAMIC_PIPELINES, pipelinesInstanceDataSizes[pipId], info);
    }
    return PipDynamic_NewInstance(dynamics + pipId, pipelinesInstanceDataSizes[pipId], info);
}
size_t R_GetVertexSizeByPipelineId(const PipID pipId) {
    return pipelinesVertexSizes[pipId];
}
void* R_GetPVmat() {
    return pvMat;
}
void R_NDCtoDirection(const NDC coords, float* const dest) {
    mat4 inv;

    glm_mat4_inv(R_GetPVmat(), inv);
    glm_mat4_mulv3(inv, (vec3){coords[0], coords[1], 1}, 1, dest);
}
void R_UploadIndices(Region* const outRegion, const RegionSize count, const Index3D indices[]) {
    CHECKINIT("Tried to upload indices");

    const RegionSize oldSize = indicesArena.size;

    RegionSize newSize;

    Arena_RequestRegion(&indicesArena, outRegion, &newSize, count);
    if (newSize) GPUBuffer_Realloc(
	&indicesBuffer, oldSize * (GLsizeiptr)sizeof(Index3D), newSize * (GLsizeiptr)sizeof(Index3D), GL_DYNAMIC_STORAGE_BIT
    );
#ifdef NOT_UPLOAD_INDICES
    return;
#endif
    GL_CHECK(GPUBuffer_SubData(
	indicesBuffer, outRegion->position * (GLintptr)sizeof(Index3D), count * (GLsizeiptr)sizeof(Index3D), indices
    ));
}
void R_UploadVertices(const PipID pipId, const UploadVerticesInfo info) {
    Pip_UploadVertices(getBasePip(pipId), pipelinesVertexSizes[pipId], info);
}
void R_UploadStatic(const PipID pipId, const InstanceID id, const size_t size, const void* const data) {
    GL_CHECK(GPUBuffer_SubData(
	statics[pipId - NUM_DYNAMIC_PIPELINES].instancesDataBuffer, 
	id * pipelinesInstanceDataSizes[pipId], (GLsizeiptr)size, data
    ));
}
void R_ShowTexture(const TextureID id) {
#ifdef NOT_SHOW_TEXTURES
    return;
#endif
    const GLuint64 handle = glGetTextureHandleARB(TexturesHandler_GetGLTexture(id));

    glMakeTextureHandleResidentARB(handle);

    GL_CHECK(GPUBuffer_SubData(textureHandlesBuffer, id * (GLintptr)sizeof(GLuint64), sizeof(GLuint64), &handle));
}
void* R_GetUploadPtr(const PipID pipId, const InstanceID id) {
    RingBufferID index;

    switch (pipId) {
    case GRAPHICS_PIPELINE_INTERP:
	index = writeInterpIndex;
	break;
    default:
	index = writeNormalIndex;
    }

    void* const mapped = dynamics[pipId].instancesDataBufferPointers[index];

    return mapped + (id * pipelinesInstanceDataSizes[pipId]);
}
void R_UploadMesh(const PipID pipId, const UploadMeshInfo info) {
    Pip_UploadMesh(getBasePip(pipId), info);
}
void R_DeleteMesh(const PipID pipId, const MeshID meshId) {
    Pip* const pip = getBasePip(pipId);

    pip->nextMeshId -= Arena_ReturnRegion(
	&pip->commandsArena, &(Region){.position = meshId, .size = 1}
    );
}
void R_DeleteTexture(const TextureID id) {
    GL_CHECK(glClearNamedBufferSubData(
	textureHandlesBuffer.buf, GL_RG32UI, id * (GLintptr)sizeof(GLuint64), sizeof(GLuint64), GL_RG, GL_UNSIGNED_INT, NULL
    ));
}
void R_DeleteInstance(const PipID pipId, const DeleteInstanceInfo info) {
    Pip_DeleteInstance(getBasePip(pipId), info);
}
void R_FreeIndicesVertices(const PipID pipId, const Region* const indicesRegion, const Region* const verticesRegion) {
    Arena_ReturnRegion(&indicesArena, indicesRegion);
    Arena_ReturnRegion(&getBasePip(pipId)->verticesArena, verticesRegion);
}
void R_ResizeTextureHandlesBuffer(const RegionSize oldSize, const RegionSize newSize) {
    GL_CHECK(GPUBuffer_Realloc(
	&textureHandlesBuffer, oldSize * (GLsizeiptr)sizeof(GLuint64), 
	newSize * (GLsizeiptr)sizeof(GLuint64), GL_DYNAMIC_STORAGE_BIT
    ));
    GPUBuffer_BindBase(textureHandlesBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_TEXTURE_HANDLES);
}
void R_SetViewportSize(const int width, const int height) {
    glViewport(0, 0, width, height);

    setPerspectiveMatrix((float)width / (float)height);
}
void R_Loop_UpdatePVMat() {
    Camera_GetPerspectiveCameraMatrix(perspectiveMat, pvMat);
}
void R_Loop(const float interp) {
    writeNormalIndex = readNormalIndex;
    readNormalIndex = (writeNormalIndex + 1) % NUM_RING_BUFFERS;

    TexturesHandler_Loop();
    ModelsHandler_Loop();
    
    //reset default bindings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(globalVao);

    GPUBuffer_Bind(indicesBuffer, GL_ELEMENT_ARRAY_BUFFER);

    updateUniforms();

    ShaderProgram_Use(dynamics[GRAPHICS_PIPELINE_INTERP].base.processInstancesProgram);
    Uniform_Set_1F(interpUniform, interp);

    FLUSH_DYNAMIC_PIPELINE_BUFFER(GRAPHICS_PIPELINE_NORMAL);
    PipDynamic_Run(dynamics + GRAPHICS_PIPELINE_NORMAL, readNormalIndex);
    PipDynamic_Run(dynamics + GRAPHICS_PIPELINE_INTERP, readInterpIndex);

    FLUSH_DYNAMIC_PIPELINE_BUFFER(GRAPHICS_PIPELINE_SKINNED);
    PipDynamic_Run(dynamics + GRAPHICS_PIPELINE_SKINNED, readNormalIndex);

    PipStatic_Run(statics + 0);

    //BillboardPip_Run(&quadPip);
    BillboardPip_Run(&billboardPip);
 
    //gui
    glClear(GL_DEPTH_BUFFER_BIT);
    PipStatic_Run(statics + 1);

    renderSyncs[readNormalIndex] = getSync();

    glClientWaitSync(renderSyncs[writeNormalIndex], GL_SYNC_FLUSH_COMMANDS_BIT, UINT32_MAX);
    glDeleteSync(renderSyncs[writeNormalIndex]);
}
void R_FixedLoopOnce() {
    writeInterpIndex = readInterpIndex;
    readInterpIndex = (writeInterpIndex + 1) % NUM_RING_BUFFERS;

    //idk why this causes GL_INVALID_VALUE error in nsight, everything is in bounds, real wtf moment right here
    FLUSH_DYNAMIC_PIPELINE_BUFFER(GRAPHICS_PIPELINE_INTERP);
}

void R_DrawDebugGui() {
    DGH_FIELD(&indicesArena);
    //DGH_FIELD(&pipStatic.base);
    DGH_FIELD(readNormalIndex);
    DGH_FIELD(writeNormalIndex);
    DGH_FIELD(readInterpIndex);
    DGH_FIELD(writeInterpIndex);
    DGH_FIELD(pvMat);
    DGH_FIELD(perspectiveMat);
    DGH_ARRAYN(dynamics);
    //DGH_ARRAYN(statics); TODO
    DGH_ARRAYN(pipelinesInstanceDataSizes);
    DGH_ARRAYN(pipelinesVertexSizes);
}
