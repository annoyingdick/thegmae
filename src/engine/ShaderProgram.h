#ifndef ShaderProgram_h_
#define ShaderProgram_h_

#include <GL/glew.h>

typedef struct {
    GLuint pro;
} ShaderProgram;

typedef struct {
    const char* vertexShaderSourceFileName, *fragmentShaderSourceFileName;
} ShaderProgramInitInfo_VF;

typedef struct {
    ShaderProgramInitInfo_VF vfInfo;

    const char* controlShaderSourceFileName, *evaluationShaderSourceFileName;
} ShaderProgramInitInfo_VFTess;

typedef struct {
    const char* sourceFileName;
} ShaderProgramInitInfo_Compute;

void ShaderProgram_Init_VF(ShaderProgram* sp, ShaderProgramInitInfo_VF info);
void ShaderProgram_Init_VFTess(ShaderProgram* sp, ShaderProgramInitInfo_VFTess info);
void ShaderProgram_Init_Compute(ShaderProgram* sp, ShaderProgramInitInfo_Compute info);
void ShaderProgram_Use(ShaderProgram sp);

#endif
