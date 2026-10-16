#ifndef IPipStatic_h_
#define IPipStatic_h_

#include "IPip.h"

typedef struct {
    IPip base;
    GPUBuffer instancesDataBuffer;
} IPipStatic;

void IPipStatic_Init(IPipStatic* pip, IPipInitInfo info);
MeshID IPipStatic_NewInstance(IPipStatic* pip, GLsizeiptr instanceDataSize, NewInstanceInfo info);
void IPipStatic_Run(const IPipStatic* pip);

#endif
