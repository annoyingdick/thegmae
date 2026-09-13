#include <stdlib.h>
#include "GPUBuffer.h"

void GPUBuffer_Init(GPUBuffer* const buffer, const GLsizeiptr size, const GLbitfield flags) {
    glCreateBuffers(1, &buffer->buf);
    glNamedBufferStorage(buffer->buf, size, NULL, flags);
}
void GPUBuffer_InitWithData(GPUBuffer* const buffer, const GLsizeiptr size, const void* const data) {
    glCreateBuffers(1, &buffer->buf);
    glNamedBufferStorage(buffer->buf, size, data, GL_NONE);
}
void GPUBuffer_MultiInit(
    GPUBuffer* const buffers, const GLsizei count, 
    const GLsizeiptr sizes[const], const GLbitfield flags[const]
) {
    glCreateBuffers(count, (GLuint*)buffers);

    for (GLsizei i = 0; i < count; i++) glNamedBufferStorage(buffers[i].buf, sizes[i], NULL, flags[i]);
}
void GPUBuffer_Realloc(GPUBuffer* const buffer, const GLsizeiptr oldSize, const GLsizeiptr newSize, const GLbitfield flags) {
    GLuint newBuffer;

    glCreateBuffers(1, &newBuffer);
    glNamedBufferStorage(newBuffer, newSize, NULL, flags);
    glCopyNamedBufferSubData(buffer->buf, newBuffer, 0, 0, oldSize);

    glDeleteBuffers(1, &buffer->buf);

    buffer->buf = newBuffer;
}
void GPUBuffer_Bind(const GPUBuffer buffer, const GLenum target) {
    glBindBuffer(target, buffer.buf);
}
void GPUBuffer_BindBase(const GPUBuffer buffer, const GLenum target, const GLuint index) {
    glBindBufferBase(target, index, buffer.buf);
}
void GPUBuffer_SubData(const GPUBuffer buffer, const GLintptr offset, const GLsizeiptr size, const void* const data) {
    glNamedBufferSubData(buffer.buf, offset, size, data);
}
void* GPUBuffer_Map(const GPUBuffer buffer, const GLsizeiptr size, const GLbitfield flags) {
    return glMapNamedBufferRange(buffer.buf, 0, size, flags);
}
void GPUBuffer_Destroy(const GPUBuffer buffer) {
    glDeleteBuffers(1, (const GLuint*)&buffer);
}
void GPUBuffer_MultiDestroy(const GPUBuffer* const buffers, const GLsizei count) {
    glDeleteBuffers(count, (const GLuint*)&buffers);
}
