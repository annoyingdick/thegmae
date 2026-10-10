#ifndef BillboardPip_h_
#define BillboardPip_h_

#include <cglm/types.h>
#include "Arena.h"
#include "GPUBuffer.h"
#include "ShaderProgram.h"

typedef RegionPosition BillboardID;

typedef struct {
    Arena verticesArena;
    GPUBuffer verticesBuffer;

    float texture;
    ShaderProgram program;
    BillboardID numVertices;
} BillboardPip;

void BillboardPip_Init(BillboardPip* pip, const char geometrySource[], const char texturePath[], const char faceTexturePath[]);
BillboardID BillboardPip_NewBillboard(BillboardPip* pip, vec3 position, vec3 direction);
void BillboardPip_Run(const BillboardPip* pip);

#endif
