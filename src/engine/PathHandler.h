#ifndef PathHandler_h_
#define PathHandler_h_ 

#include <stdint.h>

typedef unsigned int PathStringSize;

void PH_Init(const char exePath[]);
PathStringSize PH_GetAbsolutePathStrSize(PathStringSize relPathStrSize);
void PH_GetAbsolutePathStr(char dest[], const char relPathStr[]);
char* PH_OpenFile(const char relPathStr[], PathStringSize relPathStrSize, size_t* outFileSize);

void PH_DrawDebugGui();

#endif
