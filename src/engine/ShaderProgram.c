#include "def.h"
#include "PathHandler.h"

#include "ShaderProgram.h"

#define GLERROR_INFO_SIZE 512

typedef struct {
    GLint size;

    GLchar* string;
} FileReadResult;

static char* openFile(const char fileName[const], size_t* const outFileSize) {
    const char relPathPreStr[] = "shaders\\";

    const PathStringSize relPathStrSize = sizeof(relPathPreStr) + strlen(fileName); 

    char relPathStr[relPathStrSize];

    strcpy(relPathStr, relPathPreStr);
    strcat(relPathStr, fileName);

    return PH_OpenFile(relPathStr, relPathStrSize, outFileSize);
}
static void sourceShader(const GLuint shader, const char fileName[const]) {
    size_t fileSize;

    char* const string = openFile(fileName, &fileSize);

    //are we fucking deadass???
    glShaderSource(shader, 1, (const GLchar**)&string, (GLint*)&fileSize);

    free(string);
}
static void printErrorShader(const GLuint shader) {
    char errorInfo[GLERROR_INFO_SIZE];

    glGetShaderInfoLog(shader, GLERROR_INFO_SIZE, NULL, errorInfo);

    throwFatal("Shader compilation failed!", errorInfo);
}
static void checkShader(const GLuint shader, const GLenum pName) {
    GLint success;

    glGetShaderiv(shader, pName, &success);

    if (!success) printErrorShader(shader);
}
static GLuint createShader(const GLenum type, const char fileName[const]) {
    const GLuint shader = glCreateShader(type);

    sourceShader(shader, fileName);

    glCompileShader(shader);

    checkShader(shader, GL_COMPILE_STATUS);

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
