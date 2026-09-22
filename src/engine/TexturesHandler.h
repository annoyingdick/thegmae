#ifndef TexturesHandler_h_
#define TexturesHandler_h_

#include <stdbool.h>
#include "TaskLoadTexture.h"

#define INIT_NUM_TEXTURES 2

#define PUBVARS_TexturesHandler \

void TexturesHandler_Init();
GLuint TexturesHandler_GetGLTexture(TextureID id);
TextureID TexturesHandler_BeginLoadingTask(const char name[], const char path[]);
TextureID TexturesHandler_LoadTextureRGB888(const unsigned char data[], int width, int height, const char name[]);
void TexturesHandler_UnloadTexture(TextureID id);
//returns true when this texture gets allocated storage
void TexturesHandler_Quit();
void TexturesHandler_Loop();

#endif
