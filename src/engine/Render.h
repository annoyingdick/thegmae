#ifndef Render_h_
#define Render_h_

#include "WindowHandler.h"
#include "Region.h"

#define MAX_BONES 128
#define NUM_VERTICES_PER_PATCH 4

#define VERTEX2D_POSITIONS_OFFSET 0
#define VERTEX2D_POSITIONS_SIZE 2
#define VERTEX2D_TEXCOORDS_OFFSET VERTEX2D_POSITIONS_OFFSET
#define VERTEX2D_TEXCOORDS_SIZE 2

#define VERTEX_POSITIONS_OFFSET 0
#define VERTEX_POSITIONS_SIZE 3
#define VERTEX_TEXCOORDS_OFFSET VERTEX_POSITIONS_SIZE
#define VERTEX_TEXCOORDS_SIZE 2
#define VERTEX_WEIGHTS_OFFSET VERTEX_POSITIONS_SIZE + VERTEX_TEXCOORDS_SIZE
#define VERTEX_WEIGHTS_SIZE 4

typedef uint32_t Index3D;
typedef unsigned int TextureID;
typedef unsigned int MeshID;
typedef unsigned int InstanceID;
typedef unsigned int RingBufferID;

typedef enum {
    GRAPHICS_PIPELINE_NORMAL,
    GRAPHICS_PIPELINE_INTERP,
    GRAPHICS_PIPELINE_SKINNED,
    GRAPHICS_PIPELINE_GUI,
    GRAPHICS_PIPELINE_STATIC
} PipID;

#define NUM_DYNAMIC_PIPELINES GRAPHICS_PIPELINE_STATIC

typedef struct {
    RegionSize count;

    Region* outRegion; 
    const void* data;
} UploadVerticesInfo;

typedef struct {
    MeshID meshId;
    //Don't let numIndices to be anything but 0 if your mesh doesn't have any instances at the moment
    RegionPosition firstIndex, firstVertex, numIndices;
} UploadMeshInfo;

typedef struct {
    RegionSize numIndices;
    MeshID meshId;
    bool isFirstForThisMesh;
} NewInstanceInfo;

typedef struct {
    MeshID meshId;
    bool isLastForThisMesh;
} DeleteInstanceInfo;

void R_Init();
MeshID R_NewMesh(PipID pipId);
InstanceID R_NewInstance(PipID pipId, NewInstanceInfo info);
//Returns size in bytes
size_t R_GetVertexSizeByPipelineId(PipID pipId);
void* R_GetPVmat();
void R_NDCtoDirection(const NDC coords, float* dest);
void R_UploadIndices(Region* outRegion, RegionSize count, const Index3D indices[]);
void R_UploadVertices(PipID pipId, UploadVerticesInfo info);
void R_UploadStatic(InstanceID id, size_t size, const void* data);
void R_ShowTexture(TextureID id);
void* R_GetUploadPtr(PipID pipId, InstanceID id);
void R_UploadMesh(PipID pipId, UploadMeshInfo info);
void R_DeleteMesh(PipID pipId, MeshID meshId, const Region* indicesRegion, const Region* verticesRegion);
void R_DeleteTexture(TextureID id);
void R_DeleteInstance(PipID pipId, DeleteInstanceInfo info);
void R_ResizeTextureHandlesBuffer(RegionSize oldSize, RegionSize newSize);
void R_SetViewportSize(int width, int height);
void R_Loop_UpdatePVMat();
void R_Loop(float interp);
void R_FixedLoop();
void R_FixedLoopOnce();

void R_DrawDebugGui();

#endif
