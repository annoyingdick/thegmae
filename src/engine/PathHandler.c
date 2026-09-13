#include "def.h"
#include "DebugGuiHandler.h"
#include "PathHandler.h"

static PathStringSize filesMainPathStrSize;

static char* filesMainPathStr;

static FILE* openFile(const char relPathStr[const], const PathStringSize relPathStrSize) {
    char path[PH_GetAbsolutePathStrSize(relPathStrSize)];

    PH_GetAbsolutePathStr(path, relPathStr);

    FILE* const file = fopen(path, "rb");

    if (!file) throwFatal("Failed to open a file!", path);

    return file;
}
static const char* findLastSlash(const char str[const]) {
    const char* const address = strrchr(str, '\\'); 

    if (!address) throwFatal("Path error occurred!", "Weird executable path");

    return address;
}

void PH_Init(const char exePath[const]) {
    filesMainPathStrSize = findLastSlash(exePath) - exePath + 2;

    filesMainPathStr = mallocd(filesMainPathStrSize);

    memcpy(filesMainPathStr, exePath, filesMainPathStrSize - 1);

    filesMainPathStr[filesMainPathStrSize - 1] = '\0';
}
PathStringSize PH_GetAbsolutePathStrSize(const PathStringSize relPathStrSize) {
    return filesMainPathStrSize + relPathStrSize - 1;
}
void PH_GetAbsolutePathStr(char dest[const restrict], const char relPathStr[const restrict]) {
    strcpy(dest, filesMainPathStr);
    strcat(dest, relPathStr);
}
char* PH_OpenFile(const char relPathStr[const], const PathStringSize relPathStrSize, size_t* const outFileSize) {
    FILE* const file = openFile(relPathStr, relPathStrSize);

    fseek(file, 0, SEEK_END);

    const size_t fileSize = ftell(file);

    char* const string = mallocd(fileSize);

    fseek(file, 0, SEEK_SET);
    fread(string, fileSize, 1, file);
    fclose(file);

    *outFileSize = fileSize;

    return string;
}

void PH_DrawDebugGui() {
    DGH_FIELD(filesMainPathStrSize);
    DGH_FIELD(filesMainPathStr);
}
