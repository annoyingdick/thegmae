#ifndef NFixedPipFrame_h_
#define NFixedPipFrame_h_

#include "NFixedPip.h"
#include "Uniform.h"

typedef struct {
    NFixedPip base;
    Uniform currentTimeUniform, textureUniform;

    TextureID texture;
    float time;

    float* timestamps;
    RegionPosition* indices;
    vec2s* sizes;
} NFixedPipFrame;

void NFixedPipFrame_Init(NFixedPipFrame* pip);
//IMPORTANT MESSAGE: NFixedPipFrame allows only one frame at a time
void NFixedPipFrame_NewFrame(NFixedPipFrame* pip, vec2s size);
void NFixedPipFrame_UpdateFrames(const NFixedPipFrame* pip, float ratio);
void NFixedPipFrame_Run(NFixedPipFrame* pip);

#endif
