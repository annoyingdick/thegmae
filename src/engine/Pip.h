#ifndef Pip_h_
#define Pip_h_

#include "Arena.h"
#include "ShaderProgram.h"
#include "GPUBuffer.h"
#include "Render.h"

#define INIT_NUM_MESHES 2
#define NUM_RING_BUFFERS 2

#define DECLARE_BUFFER_BINDINGS \
X(BUFFER_BINDING_VERTICES) \
X(BUFFER_BINDING_TEXTURE_HANDLES) \
X(BUFFER_BINDING_COMMANDS) \
X(BUFFER_BINDING_INSTANCES_DATA) \
X(BUFFER_BINDING_USED_INSTANCES_IDS) \
X(BUFFER_BINDING_INSTANCES_MESH_IDS) \
X(BUFFER_BINDING_TERRAIN_VERTICES)

#ifdef DEBUG
#define GL_CHECK(x) do { glGetError(); x; if (glGetError() != GL_NO_ERROR) { \
    puts("\nWARNING: AN ERROR HAS OCCURRED IN THIS OPENGL FUNCTION:\n"#x" in "__FILE_NAME__"\n"); \
}} while (0)
#else
#define GL_CHECK(x) x
#endif

//Base pipeline struct. Useless on its own, because it doesn't upload any draw commands into the gpu.
typedef struct {
    Arena verticesArena, commandsArena;
    ShaderProgram mainProgram; //private
    GPUBuffer verticesBuffer, commandsBuffer; //private

    MeshID nextMeshId;
} Pip;

typedef struct {
    ShaderProgramInitInfo_VF mainShaderInfo;

    size_t vertexSize;
} PipInitInfo;

typedef struct {
    MeshID meshId;
    InstanceID baseInstance, numInstances;
    bool isCommandLast;
} PipUploadMeshFirstTimeInfo;

enum {
#define X(x) x,
    DECLARE_BUFFER_BINDINGS
#undef X
};

void Pip_Init(Pip* pip, PipInitInfo info);
void Pip_UploadVertices(Pip* pip, GLsizeiptr vertexSize, UploadVerticesInfo info);
MeshID Pip_NewMesh(Pip* pip, RegionSize* outNewSize, bool* outIsLast);
void Pip_UploadMesh(const Pip* pip, UploadMeshInfo info, InstanceID numInstances);
void Pip_UploadMeshFirstTime(Pip* pip, PipUploadMeshFirstTimeInfo info);
void Pip_Run(const Pip* pip);
void Pip_Draw(const Pip* pip);

#endif
