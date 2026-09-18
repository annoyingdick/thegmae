#include <cglm/cam.h>
#include "def.h"
#include "Uniform.h"
#include "PipStatic.h"
#include "PipDynamic.h"
#include "Camera.h"
#include "TexturesHandler.h"
#include "ModelsHandler.h"
#include "DebugGuiHandler.h"
#include "Render.h"

#define DO_OPENGL_DEBUG

//#define NOT_UPLOAD_INDICES
//#define NOT_SHOW_TEXTURES

#define INIT_NUM_INDICES 2

#define CHECKINIT(x) if (!indicesArena.size) throwFatal("Render is not initialized!", x)

//Compiler at any moment can rearrange any of the variables below so be careful!
//These buffers can be updated by CPU (GL_DYNAMIC_STORAGE)
static Arena indicesArena;
static GPUBuffer indicesBuffer, textureHandlesBuffer; //private
static PipStatic pipStatic;
static Uniform interpUniform; //private

static GLuint globalVao; //private
static RingBufferID readNormalIndex, writeNormalIndex, readInterpIndex, writeInterpIndex;

static mat4 pvMat;
static mat4 perspectiveMat;

static PipDynamic pipelines[NUM_DYNAMIC_PIPELINES];
static const GLsizeiptr pipelinesInstanceDataSizes[] = {
    [GRAPHICS_PIPELINE_NORMAL] = sizeof(mat4),
    [GRAPHICS_PIPELINE_INTERP] = 3 * sizeof(mat4),
    [GRAPHICS_PIPELINE_SKINNED] = MAX_BONES * sizeof(mat4),
    [GRAPHICS_PIPELINE_GUI] = 0,
    [GRAPHICS_PIPELINE_STATIC] = 2 * sizeof(mat4)
};
static const GLsizeiptr pipelinesVertexSizes[] = {
    [GRAPHICS_PIPELINE_NORMAL] = (
	VERTEX_POSITIONS_SIZE + VERTEX_TEXCOORDS_SIZE) * sizeof(float), 
    [GRAPHICS_PIPELINE_INTERP] = (
	VERTEX_POSITIONS_SIZE + VERTEX_TEXCOORDS_SIZE) * sizeof(float), 
    [GRAPHICS_PIPELINE_SKINNED] = (
	VERTEX_POSITIONS_SIZE + VERTEX_TEXCOORDS_SIZE + VERTEX_WEIGHTS_SIZE) * sizeof(float), 
    [GRAPHICS_PIPELINE_GUI] = (
	VERTEX2D_POSITIONS_SIZE + VERTEX2D_TEXCOORDS_SIZE) * sizeof(float), 
    [GRAPHICS_PIPELINE_STATIC] = (VERTEX_POSITIONS_SIZE + VERTEX_TEXCOORDS_SIZE) * sizeof(float)
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
    return pipId >= NUM_DYNAMIC_PIPELINES ? &pipStatic.base : &pipelines[pipId].base;
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
    const char* const vertexShaderNames[] = {
	"normal.vert", "interp.vert", "skinned.vert", "gui.vert"
    };
    const char* const piShaderNames[] = {
	"normal.comp", "interp.comp", "skinned.comp", "gui.comp"
    };
    
    for (PipID i = 0; i < NUM_DYNAMIC_PIPELINES; i++) {
	PipDynamic_Init(pipelines + i, (PipInitInfo){
	    .mainShaderInfo = {
		.vertexShaderSourceFileName = vertexShaderNames[i],
		.fragmentShaderSourceFileName = "normal.frag"
	    },
	    .processInstancesShaderInfo = {piShaderNames[i]},

	    .instanceDataSize = pipelinesInstanceDataSizes[i],
	    .vertexSize = pipelinesVertexSizes[i]
	});
    }

    PipStatic_Init(&pipStatic, (PipInitInfo){
	.mainShaderInfo = {
	    .vertexShaderSourceFileName = "static.vert",
	    .fragmentShaderSourceFileName = "normal.frag"
	},
	.processInstancesShaderInfo = {"static.comp"},

	.instanceDataSize = pipelinesInstanceDataSizes[GRAPHICS_PIPELINE_STATIC],
	.vertexSize = pipelinesVertexSizes[GRAPHICS_PIPELINE_STATIC]
    });
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

    Uniform_Init(
	&interpUniform, 
	pipelines[GRAPHICS_PIPELINE_INTERP].base.processInstancesProgram, "interp"
    );

    glPatchParameteri(GL_PATCH_VERTICES, NUM_VERTICES_PER_PATCH);
    glClearColor(.0f, .0f, .0f, 1.0f);
}
MeshID R_NewMesh(const PipID pipId) {
    return Pip_NewMesh(getBasePip(pipId));
}
InstanceID R_NewInstance(const PipID pipId, const NewInstanceInfo info) {
    if (pipId >= NUM_DYNAMIC_PIPELINES) {
	return PipStatic_NewInstance(&pipStatic, pipelinesInstanceDataSizes[pipId], info);
    }
    return PipDynamic_NewInstance(pipelines + pipId, pipelinesInstanceDataSizes[pipId], info);
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
void R_UploadStatic(const InstanceID id, const size_t size, const void* const data) {
    GL_CHECK(GPUBuffer_SubData(
	pipStatic.instancesDataBuffer, 
	id * pipelinesInstanceDataSizes[GRAPHICS_PIPELINE_STATIC], (GLsizeiptr)size, data
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

    void* const mapped = pipelines[pipId].instancesDataBufferPointers[index];

    return mapped + (id * pipelinesInstanceDataSizes[pipId]);
}
void R_UploadMesh(const PipID pipId, const UploadMeshInfo info) {
    Pip_UploadMesh(getBasePip(pipId), info);
}
void R_DeleteMesh(
    const PipID pipId, const MeshID meshId, 
    const Region* const restrict indicesRegion, const Region* const restrict verticesRegion
) {
    if (indicesRegion) Arena_ReturnRegion(&indicesArena, indicesRegion);

    Pip_DeleteMesh(getBasePip(pipId), meshId, verticesRegion);
}
void R_DeleteTexture(const TextureID id) {
    GL_CHECK(glClearNamedBufferSubData(
	textureHandlesBuffer.buf, GL_RG32UI, id * (GLintptr)sizeof(GLuint64), sizeof(GLuint64), GL_RG, GL_UNSIGNED_INT, NULL
    ));
}
void R_DeleteInstance(const PipID pipId, const DeleteInstanceInfo info) {
    Pip_DeleteInstance(getBasePip(pipId), info);
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
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(globalVao);

    GPUBuffer_Bind(indicesBuffer, GL_ELEMENT_ARRAY_BUFFER);

    ShaderProgram_Use(pipelines[GRAPHICS_PIPELINE_INTERP].base.processInstancesProgram);
    Uniform_Set_1F(interpUniform, interp);

    Pip_PreRun(pvMat);

    GL_CHECK(glFlushMappedNamedBufferRange(
	pipelines[GRAPHICS_PIPELINE_NORMAL].instancesDataBuffers[readNormalIndex].buf, 0, 
	pipelines[GRAPHICS_PIPELINE_NORMAL].base.numInstances * 
	pipelinesInstanceDataSizes[GRAPHICS_PIPELINE_NORMAL]
    ));
    PipDynamic_Run(pipelines + GRAPHICS_PIPELINE_NORMAL, readNormalIndex);
    PipDynamic_Run(pipelines + GRAPHICS_PIPELINE_INTERP, readInterpIndex);

    GL_CHECK(glFlushMappedNamedBufferRange(
	pipelines[GRAPHICS_PIPELINE_SKINNED].instancesDataBuffers[readNormalIndex].buf, 0, 
	pipelines[GRAPHICS_PIPELINE_SKINNED].base.numInstances * 
	pipelinesInstanceDataSizes[GRAPHICS_PIPELINE_SKINNED]
    ));
    PipDynamic_Run(pipelines + GRAPHICS_PIPELINE_SKINNED, readNormalIndex);
    PipDynamic_Run(pipelines + GRAPHICS_PIPELINE_GUI, readNormalIndex);

    PipStatic_Run(&pipStatic);

    renderSyncs[readNormalIndex] = getSync();

    glClientWaitSync(renderSyncs[writeNormalIndex], GL_SYNC_FLUSH_COMMANDS_BIT, UINT32_MAX);
    glDeleteSync(renderSyncs[writeNormalIndex]);
}
void R_FixedLoopOnce() {
    writeInterpIndex = readInterpIndex;
    readInterpIndex = (writeInterpIndex + 1) % NUM_RING_BUFFERS;

    //idk why this causes GL_INVALID_VALUE error in nsight, everything is in bounds, real wtf moment right here
    GL_CHECK(glFlushMappedNamedBufferRange(
	pipelines[GRAPHICS_PIPELINE_INTERP].instancesDataBuffers[readInterpIndex].buf, 0, 
	pipelines[GRAPHICS_PIPELINE_INTERP].base.numInstances * 
	pipelinesInstanceDataSizes[GRAPHICS_PIPELINE_INTERP]
    ));
}

void R_DrawDebugGui() {
    DGH_FIELD(&indicesArena);
    DGH_FIELD(&pipStatic.base);
    DGH_FIELD(readNormalIndex);
    DGH_FIELD(writeNormalIndex);
    DGH_FIELD(readInterpIndex);
    DGH_FIELD(writeInterpIndex);
    DGH_FIELD(pvMat);
    DGH_FIELD(perspectiveMat);
    DGH_ARRAYN(pipelines);
    DGH_ARRAYN(pipelinesInstanceDataSizes);
    DGH_ARRAYN(pipelinesVertexSizes);
}
