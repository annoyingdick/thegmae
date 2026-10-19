#ifndef NPip_h_
#define NPip_h_

#include "Pip.h"

//Normal (non-instanced) pipeline struct derived from Pip.
typedef struct {
    Pip base;
    GPUBuffer meshesDataBuffer;

    MeshID meshesAllocationSize;
} NPip;

typedef struct {
    PipInitInfo baseInfo;

    size_t meshDataSize;
} NPipInitInfo;

void NPip_Init(NPip* pip, NPipInitInfo info);
MeshID NPip_NewMesh(NPip* pip, GLsizeiptr meshDataSize);
void NPip_Run(const NPip* pip);

#endif
