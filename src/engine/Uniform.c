#include "Uniform.h"

void Uniform_Init(Uniform* const uniform, const ShaderProgram sp, const char name[const]) {
    uniform->location = glGetUniformLocation(sp.pro, name);
}
void Uniform_Set_1UI(const Uniform uniform, const GLuint value) {
    glUniform1ui(uniform.location, value);
}
void Uniform_Set_1F(const Uniform uniform, const GLfloat value) {
    glUniform1f(uniform.location, value);
}
void Uniform_Set_Mat4(const Uniform uniform, const mat4 value) {
    glUniformMatrix4fv(uniform.location, 1, GL_FALSE, value[0]);
}
