#ifndef PipDynamic_h_
#define PipDynamic_h_

#include "Pip.h"

typedef struct {
    Pip base;

    GPUBuffer instancesDataBuffers[NUM_RING_BUFFERS];
    void* instancesDataBufferPointers[NUM_RING_BUFFERS];
} PipDynamic;

void PipDynamic_Init(PipDynamic* pip, PipInitInfo info);
MeshID PipDynamic_NewInstance(PipDynamic* pip, GLsizeiptr instanceDataSize, NewInstanceInfo info);
void PipDynamic_Run(const PipDynamic* pip, RingBufferID instancesDataBufferId);

#endif
