#ifndef GUI_h_
#define GUI_h_

typedef unsigned int Ammo;

void GUI_Init();
void GUI_UpdateAmmoMag(Ammo mag);
void GUI_UpdateAmmoBoth(Ammo mag, Ammo left);
void GUI_UpdateTexts();
void GUI_Loop();

#endif
