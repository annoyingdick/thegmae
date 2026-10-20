#ifndef NFixedPipFrame_h_
#define NFixedPipFrame_h_

#include "NFixedPip.h"
#include "Uniform.h"

typedef struct {
    NFixedPip base;
    Uniform currentTimeUniform;

    float texture, time;

    float* timestamps;
    RegionPosition* indices;
} NFixedPipFrame;

void NFixedPipFrame_Init(NFixedPipFrame* pip);
void NFixedPipFrame_NewFrame(NFixedPipFrame* pip, vec2 size);
void NFixedPipFrame_Run(NFixedPipFrame* pip);

#endif
