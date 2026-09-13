#ifndef PipStatic_h_
#define PipStatic_h_

#include "Pip.h"

typedef struct {
    Pip base;
    GPUBuffer instancesDataBuffer;
} PipStatic;

void PipStatic_Init(PipStatic* pip, PipInitInfo info);
MeshID PipStatic_NewInstance(PipStatic* pip, GLsizeiptr instanceDataSize, NewInstanceInfo info);
void PipStatic_Run(const PipStatic* pip);

#endif
