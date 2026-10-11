#ifndef CharactersHandler_h_
#define CharactersHandler_h_

#include "Character.h"

void CH_Init();
void CH_AddCharacter(Character* character);
//be aware that this function also checks your character's field of view
Character* CH_FindClosestVisibleAimableArmedCharacter(Character* character);
Character* CH_FindClosestVisibleAimableCharacter(Character* character);
Mesh* CH_GetMesh();
void CH_Loop();

void CH_DrawDebugGui();

#endif
