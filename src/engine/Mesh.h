#ifndef Mesh_h_
#define Mesh_h_

#include "Render.h"
#include "Bone.h"
#include "Animation.h"

//#define DEBUG_MESH

#if defined printfd && defined REDEF_PRINTFD
#undef printfd
#ifdef DEBUG_MESH
#define printfd(...) printf(__FILE_NAME__" : " __VA_ARGS__)
#else
#define printfd(...)
#endif
#endif

//used for debug
#define VARS_MESH \
X(MeshID, id) \
X(InstanceID, numInstances) \
X(TextureID, numUsedTextures) \
X(PipID, pipId) \
X(AnimationID, numAnimations) \
X(BoneID, numBones)

typedef vec3 AABB[2];

typedef struct Mesh Mesh;

struct Mesh {
    Region verticesRegion, indicesRegion;

#define X(type, name) type name;
VARS_MESH
#undef X

    AABB bounding;

    Animation* animations;
    TextureID* usedTextures;
    Bone* bones;
    Mesh** selfInTask;
};

typedef struct {
    RegionSize verticesSize, numIndices;
    TextureID numUsedTextures;

    float* vertices;
    const Index3D* indices;
    const char* const* usedTextures;
} MeshInitWithDataInfo;

void Mesh_Init(Mesh* mesh, PipID pipId, const char fileName[]);
//void Mesh_InitWithData(Mesh* mesh, GraphicsPipelineID pipId, MeshInitWithDataInfo info);
bool Mesh_IsValid(const Mesh* mesh);
InstanceID Mesh_NewInstance(Mesh* mesh);
void Mesh_DeleteInstance(Mesh* mesh);
void Mesh_Destroy(Mesh* mesh);

void Mesh_DrawDebugGui(const Mesh* mesh, const char name[]);

#endif
