#ifndef TexturesHandler_h_
#define TexturesHandler_h_

#include <stdbool.h>
#include "TaskLoadTexture.h"

#define INIT_NUM_TEXTURES 2

#define PUBVARS_TexturesHandler \
X(Arena, texturesArena, FREELOCATIONARRAYS) \
X(Arena, tasksArena, FREELOCATIONARRAYS) \
X(TextureID, nextTextureId, INTEGER) \
X(TaskLoadTextureID, nextTaskId, INTEGER) \

void TexturesHandler_Init();
GLuint TexturesHandler_GetGLTexture(TextureID id);
TextureID TexturesHandler_BeginLoadingTask(const char name[], const char path[]);
TextureID TexturesHandler_(const char name[], const char path[]);
void TexturesHandler_UnloadTexture(TextureID id);
//returns true when this texture gets allocated storage
void TexturesHandler_Quit();
void TexturesHandler_Loop();

void* const* TexturesHandler_GetPublicVars();

#endif
