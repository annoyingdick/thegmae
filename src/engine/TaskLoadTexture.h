#ifndef Task_h_
#define Task_h_

#include <stdbool.h>
#include <GL/glew.h>
#include "Render.h"

typedef uint8_t TaskLoadTextureID;
typedef uint16_t UploadBatchID;

typedef struct {
    GLsync uploadSync;

    int width, height;
    UploadBatchID nextBatchId, numBatches;
    TextureID textureId;
    TaskLoadTextureID loadId;

    char* path;
    void* bufferPointer;
} TaskLoadTexture;

//Returns true if cpu has done loading an image from a disk
bool TaskLoadTexture_IsLoaded(const TaskLoadTexture* task);
//Returns true if texture is fully uploaded to gpu
bool TaskLoadTexture_IsUploaded(const TaskLoadTexture* task);
bool TaskLoadTexture_IsValid(const TaskLoadTexture* task);

#endif
