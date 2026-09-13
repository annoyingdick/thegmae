#ifndef ModelsHandler_h_
#define ModelsHandler_h_

#include <cglm/types.h>
#include "Render.h"
#include "Mesh.h"

//#define DEBUG_MH

#if defined printfd && defined REDEF_PRINTFD
#undef printfd
#ifdef DEBUG_MH
#define printfd(...) printf(__FILE_NAME__" : " __VA_ARGS__)
#else
#define printfd(...)
#endif
#endif

typedef struct {
    RegionSize numVertices, numIndices;

    vec3* vertices;
    Index3D* indices;
} ModelGeometry;

typedef struct {
    MeshID meshId;
    PipID pipId;

    char* fileName;
    Mesh* mesh;
} ModelLoadInfo;

void ModelsHandler_Init();
void ModelsHandler_LoadGeometry(const char fileName[], ModelGeometry* outGeometry);
Mesh** ModelsHandler_BeginLoadingTask(const ModelLoadInfo* info);
void ModelsHandler_Loop();

#endif
