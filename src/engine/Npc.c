#include "Npc.h"

void Npc_Loop(Npc* const npc) {
    Character_Loop(npc);

    //remove this line if u want to enable the minigun mode for each npc bruh
    if (Character_ShouldProcessShot(npc)) Character_GetWeaponTracks(npc, WEAPON_ANIMATION_SHOOT)->weight = FLOAT_BINONE;

    Character_HandleFatigue(npc);
}
