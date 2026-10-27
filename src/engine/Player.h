#ifndef Player_h_
#define Player_h_

#include "Character.h"

typedef struct {
    Character base;

    float timeBeingDead;
} Player;

void Player_Loop(Player* player);

#endif
