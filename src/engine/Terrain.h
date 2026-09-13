#ifndef Terrain_h_
#define Terrain_h_

#include "ShaderProgram.h"
#include "GPUBuffer.h"

typedef struct {
    ShaderProgram mainProgram;
    GPUBuffer verticesBuffer;
} Terrain;

void Terrain_Init(Terrain* terrain);
void Terrain_Loop(const Terrain* terrain);

#endif
