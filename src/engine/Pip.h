#ifndef Pip_h_
#define Pip_h_

#include <cglm/types.h>
#include "Arena.h"
#include "ShaderProgram.h"
#include "GPUBuffer.h"
#include "Render.h"

#define INIT_NUM_INSTANCES 2
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

#define VEC2XZ(x, a) (vec3){x[0] * a, 0, x[1] * a}
#define VEC3DUP(x) x[0], x[1], x[2]
#define VEC4DUP(x) x[0], x[1], x[2], x[3]
#define MAT4DUP(x) VEC4DUP(x[0]), VEC4DUP(x[1]), VEC4DUP(x[2]), VEC4DUP(x[3])

typedef struct {
    Arena verticesArena, commandsArena;
    ShaderProgram mainProgram, processInstancesProgram; //private
    GPUBuffer verticesBuffer, commandsBuffer, usedInstancesIdsBuffer, instancesMeshIdsBuffer; //private

    MeshID nextMeshId;
    InstanceID numInstances, instancesAllocationSize;

    InstanceID* baseInstances;
} Pip;

typedef struct {
    ShaderProgramInitInfo_VF mainShaderInfo;
    ShaderProgramInitInfo_Compute processInstancesShaderInfo;

    size_t instanceDataSize, vertexSize;
} PipInitInfo;

enum {
#define X(x) x,
    DECLARE_BUFFER_BINDINGS
#undef X
};

void Pip_PreInit();

void Pip_Init(Pip* pip, PipInitInfo info);
void Pip_UploadVertices(Pip* pip, GLsizeiptr vertexSize, UploadVerticesInfo info);
MeshID Pip_NewMesh(Pip* pip);
MeshID Pip_NewInstance(Pip* pip, NewInstanceInfo info);
void Pip_UploadMesh(const Pip* pip, UploadMeshInfo info);
void Pip_DeleteInstance(Pip* pip, DeleteInstanceInfo info);
void Pip_Run(const Pip* pip);

void Pip_DrawDebugGui(const Pip* pip, const char name[]);

#endif
