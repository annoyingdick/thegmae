#include "def.h"
#include "PathHandler.h"
#include "ShaderProgram.h"

#define STB_INCLUDE_LINE_NONE
#define STB_INCLUDE_IMPLEMENTATION
#include <stb_include.h>

#define GLERROR_INFO_SIZE 512

typedef struct {
    GLint size;

    GLchar* string;
} FileReadResult;

static char* openFile(const char fileName[const], const char includesStr[const]) {
    const PathStringSize filePathSize = strlen(includesStr) + strlen(fileName) + 1;

    char filePath[filePathSize];

    size_t fileSize;

    strcpy(filePath, includesStr);
    strcat(filePath, fileName);

    char* const file = PH_OpenFile(filePath, filePathSize, &fileSize);

    //make it null terminated
    file[fileSize - 1] = '\0';

    return file;
}
static char* includeFile(const char fileName[const]) {
    const char includesStr[] = "shaders\\";

    char includesPath[PH_GetAbsolutePathStrSize(sizeof(includesStr))], error[UINT8_MAX + 1];
    
    PH_GetAbsolutePathStr(includesPath, includesStr);

    char* const cleanFile = openFile(fileName, includesStr);
    char* const file = stb_include_string(cleanFile, NULL, includesPath, NULL, error);

    free(cleanFile);

    if (!file) throwFatal("stb_include error has occurred!", error);

    return file;
}
static void sourceShader(const GLuint shader, const char fileName[const]) {
    char* const string = includeFile(fileName);

    //are we fucking deadass???
    glShaderSource(shader, 1, (const GLchar**)&string, NULL);

    free(string);
}
static void printErrorShader(const GLuint shader, const char fileName[const]) {
    char errorInfo[GLERROR_INFO_SIZE];

    glGetShaderInfoLog(shader, GLERROR_INFO_SIZE, NULL, errorInfo);

    throwFatal(fileName, errorInfo);
}
static void checkShader(const GLuint shader, const GLenum pName, const char fileName[const]) {
    GLint success;

    glGetShaderiv(shader, pName, &success);

    if (!success) printErrorShader(shader, fileName);
}
static GLuint createShader(const GLenum type, const char fileName[const]) {
    const GLuint shader = glCreateShader(type);

    sourceShader(shader, fileName);

    glCompileShader(shader);

    checkShader(shader, GL_COMPILE_STATUS, fileName);

    return shader;
}
static void printErrorProgram(const GLuint program) {
    char errorInfo[GLERROR_INFO_SIZE];

    glGetProgramInfoLog(program, GLERROR_INFO_SIZE, NULL, errorInfo);

    throwFatal("Shader program linking failed!", errorInfo);
}
static void checkProgram(const GLuint program) {
    GLint success;

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) printErrorProgram(program);
}
static void finishProgram(const GLuint program) {
    glLinkProgram(program);

    checkProgram(program);
}

void ShaderProgram_Init_VF(ShaderProgram* const sp, const ShaderProgramInitInfo_VF info) {
    const GLuint vertShader = createShader(GL_VERTEX_SHADER, info.vertexShaderSourceFileName);
    const GLuint fragShader = createShader(GL_FRAGMENT_SHADER, info.fragmentShaderSourceFileName);

    sp->pro = glCreateProgram();

    glAttachShader(sp->pro, vertShader);
    glAttachShader(sp->pro, fragShader);

    finishProgram(sp->pro);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
}
void ShaderProgram_Init_VFTess(ShaderProgram* const sp, const ShaderProgramInitInfo_VFTess info) {
    const GLuint shaders[] = {
	createShader(GL_VERTEX_SHADER, info.vfInfo.vertexShaderSourceFileName),
	createShader(GL_FRAGMENT_SHADER, info.vfInfo.fragmentShaderSourceFileName),
	createShader(GL_TESS_CONTROL_SHADER, info.controlShaderSourceFileName),
	createShader(GL_TESS_EVALUATION_SHADER, info.evaluationShaderSourceFileName)
    };

    sp->pro = glCreateProgram();

    nforeach (const GLuint* const shader, shaders) glAttachShader(sp->pro, *shader); forend

    finishProgram(sp->pro);

    nforeach (const GLuint* const shader, shaders) glDeleteShader(*shader); forend
}
void ShaderProgram_Init_Compute(ShaderProgram* const sp, const ShaderProgramInitInfo_Compute info) {
    const GLuint shader = createShader(GL_COMPUTE_SHADER, info.sourceFileName);

    sp->pro = glCreateProgram();

    glAttachShader(sp->pro, shader);

    finishProgram(sp->pro);

    glDeleteShader(shader);
}
void ShaderProgram_Use(const ShaderProgram sp) {
    glUseProgram(sp.pro);
}
