#ifndef ShaderProgram_h_
#define ShaderProgram_h_

#include <GL/glew.h>

typedef GLuint ShaderProgram;

typedef struct {
    const char* vertexShaderSourceFileName, *fragmentShaderSourceFileName;
} ShaderProgramInitInfo_VF;

typedef struct {
    ShaderProgramInitInfo_VF vfInfo;

    const char* controlShaderSourceFileName, *evaluationShaderSourceFileName;
} ShaderProgramInitInfo_VFTess;

typedef struct {
    ShaderProgramInitInfo_VF vfInfo;

    const char* geometryShaderSourceFileName;
} ShaderProgramInitInfo_VFG;

//i think there aren't VFCompute shaders because when i'd added
//them months ago, something went horribly wrong and i don't have idea what exactly happened
typedef struct {
    const char* sourceFileName;
} ShaderProgramInitInfo_Compute;

void ShaderProgram_Init_VF(ShaderProgram* sp, ShaderProgramInitInfo_VF info);
void ShaderProgram_Init_VFTess(ShaderProgram* sp, ShaderProgramInitInfo_VFTess info);
void ShaderProgram_Init_VFG(ShaderProgram* sp, ShaderProgramInitInfo_VFG info);
void ShaderProgram_Init_Compute(ShaderProgram* sp, ShaderProgramInitInfo_Compute info);
void ShaderProgram_Use(ShaderProgram sp);

#endif
