#ifndef NFixedPip_h_
#define NFixedPip_h_

#include "FixedPip.h"

typedef struct {
    FixedPip base;
    GPUBuffer meshesDataBuffer;

    MeshID meshesAllocationSize;
} NFixedPip;

void NFixedPip_Init(NFixedPip* pip, ShaderProgramInitInfo_VF shaderInfo, GLsizeiptr vertexSize, GLsizeiptr meshDataSize);
MeshID NFixedPip_NewMesh(NFixedPip* pip, GLsizeiptr vertexSize, GLsizeiptr meshDataSize, RegionSize* outNewSize);
void NFixedPip_Run(const NFixedPip* pip);

#endif
