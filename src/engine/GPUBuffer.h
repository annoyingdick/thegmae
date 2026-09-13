#ifndef GPUBuffer_h_
#define GPUBuffer_h_

#include <GL/glew.h>

typedef struct {
    GLuint buf;
} GPUBuffer;

void GPUBuffer_Init(GPUBuffer* buffer, GLsizeiptr size, GLbitfield flags);
void GPUBuffer_InitWithData(GPUBuffer* buffer, GLsizeiptr size, const void* data);
void GPUBuffer_MultiInit(GPUBuffer* buffers, GLsizei count, const GLsizeiptr sizes[], const GLbitfield flags[]);
void GPUBuffer_Realloc(GPUBuffer* buffer, GLsizeiptr oldSize, GLsizeiptr newSize, GLbitfield flags);
void GPUBuffer_Bind(GPUBuffer buffer, GLenum target);
void GPUBuffer_BindBase(GPUBuffer buffer, GLenum target, GLuint index);
void GPUBuffer_SubData(GPUBuffer buffer, GLintptr offset, GLsizeiptr size, const void* data);
void* GPUBuffer_Map(GPUBuffer buffer, GLsizeiptr size, GLbitfield flags);
void GPUBuffer_Destroy(GPUBuffer buffer);
void GPUBuffer_MultiDestroy(const GPUBuffer* buffers, GLsizei count);

#endif
