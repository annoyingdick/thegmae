#ifndef IPipDynamic_h_
#define IPipDynamic_h_

#include "IPip.h"

//Instanced pipeline struct derived from IPip <- Pip. It's the best choice for persistently updated instances
typedef struct {
    IPip base;

    GPUBuffer instancesDataBuffers[NUM_RING_BUFFERS];
    void* instancesDataBufferPointers[NUM_RING_BUFFERS];
} IPipDynamic;

void IPipDynamic_Init(IPipDynamic* pip, IPipInitInfo info);
MeshID IPipDynamic_NewInstance(IPipDynamic* pip, GLsizeiptr instanceDataSize, NewInstanceInfo info);
void IPipDynamic_Run(const IPipDynamic* pip, RingBufferID instancesDataBufferId);

#endif
