#ifndef GUI_h_
#define GUI_h_

#include "Hint.h"

typedef unsigned int Ammo;

void GUI_Init();
void GUI_UpdateAmmoMag(Ammo mag);
void GUI_UpdateAmmoBoth(Ammo mag, Ammo left);
//Use a string from either heap or static memory
void GUI_NewHint(Hint hint);
void GUI_ShowDeath();
//Called whenever window is resized
void GUI_UpdateTexts();
void GUI_Loop();

#endif
