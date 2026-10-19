#ifndef FixedPipMuzzle_h_
#define FixedPipMuzzle_h_

#include "Uniform.h"
#include "FixedPip.h"

//#define BILLBOARD_LIFETIME 1 / 3.1415f

typedef struct {
    FixedPip base;
    Uniform currentTimeUniform;

    float texture, time;

    float* timestamps;
} FixedPipMuzzle;

void FixedPipMuzzle_Init(
    FixedPipMuzzle* pip, const char geometrySource[], const char texturePath[], const char faceTexturePath[]
);
void FixedPipMuzzle_NewMuzzle(FixedPipMuzzle* pip, vec3 position, vec3 direction);
void FixedPipMuzzle_Run(FixedPipMuzzle* pip);

#endif
