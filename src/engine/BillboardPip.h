#ifndef BillboardPip_h_
#define BillboardPip_h_

#include <cglm/types.h>
#include "Arena.h"
#include "GPUBuffer.h"
#include "ShaderProgram.h"
#include "Uniform.h"

#define BILLBOARD_LIFETIME 1 / 3.1415f

typedef RegionPosition BillboardID;

typedef struct {
    Arena verticesArena;
    GPUBuffer verticesBuffer;
    Uniform currentTimeUniform;

    float texture, time;
    ShaderProgram program;
    BillboardID numVertices;

    float* timestamps;
} BillboardPip;

void BillboardPip_Init(BillboardPip* pip, const char geometrySource[], const char texturePath[], const char faceTexturePath[]);
BillboardID BillboardPip_NewBillboard(BillboardPip* pip, vec3 position, vec3 direction);
void BillboardPip_Run(BillboardPip* pip);

#endif
