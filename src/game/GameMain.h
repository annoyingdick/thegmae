#ifndef GameMain_h_
#define GameMain_h_

#include <SDL3/SDL_keycode.h>
#include "engine/WindowHandler.h"

void GM_Init();
void GM_Mouse3(const NSC coords);
void GM_KeyDown(SDL_Keycode key);
void GM_KeyUp(SDL_Keycode key);
void GM_SetMetadata();
void GM_Loop(float interp);
void GM_R_PostLoop();
void GM_PSH_PreFixedLoop();
void GM_FixedLoop();
void GM_R_PreFixedLoopOnce();

#endif
