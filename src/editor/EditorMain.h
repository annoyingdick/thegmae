#ifndef EditorMain_h_
#define EditorMain_h_

#include <SDL3/SDL_keycode.h>
#include "engine/InstancesHandler.h"

#define LEVEL_NAME_STRING_SIZE UINT8_MAX

void EM_Init();
int32_t* EM_GetSelectionBool(InstancePtrID ptrId);
char* EM_GetLevelNameString();
void EM_KeyDown(SDL_Keycode key);
void EM_MouseMotion(float xrel, float yrel);
void EM_Loop();

#endif
