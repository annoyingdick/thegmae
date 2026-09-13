#ifndef Uniform_h_
#define Uniform_h_

#include <cglm/types.h>
#include "ShaderProgram.h"

typedef struct {
    GLint location;
} Uniform;

void Uniform_Init(Uniform* uniform, ShaderProgram sp, const char name[]);
void Uniform_Set_1UI(Uniform uniform, GLuint value);
void Uniform_Set_1F(Uniform uniform, GLfloat value);
void Uniform_Set_Mat4(Uniform uniform, const mat4 value);

#endif
