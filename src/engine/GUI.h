#ifndef GUI_h_
#define GUI_h_

#include "Hint.h"
#include "Character.h"

void GUI_Init();
const Character** GUI_GetCharacter();
void GUI_UpdateAmmoMag(Ammo mag);
void GUI_UpdateAmmoBoth(Ammo mag, Ammo left);
//Use a string from either heap or static memory
void GUI_NewHint(Hint hint);
void GUI_ShowDeath();
void GUI_HideDeath();
//Called whenever window is resized
void GUI_UpdateTexts();
void GUI_Loop();

#endif
