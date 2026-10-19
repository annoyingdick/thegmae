#ifndef FixedPip_h_
#define FixedPip_h_

#include "Arena.h"
#include "GPUBuffer.h"
#include "ShaderProgram.h"
#include "Render.h"

typedef struct {
    Arena verticesArena;
    GPUBuffer verticesBuffer;

    ShaderProgram program;
    MeshID numMeshes;
} FixedPip;

//Init the program urself lol
void FixedPip_Init(FixedPip* pip, GLsizeiptr vertexSize);
MeshID FixedPip_NewMesh(FixedPip* pip, GLsizeiptr vertexSize, RegionSize* outNewSize);
void FixedPip_Run(const FixedPip* pip);

#endif
