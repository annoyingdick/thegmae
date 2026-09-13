#ifndef DrawElementsIndirectCommand_h_
#define DrawElementsIndirectCommand_h_

#include <GL/glew.h>

typedef struct {
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    GLint baseVertex;
    GLuint baseInstance;
} DrawElementsIndirectCommand;

#endif
