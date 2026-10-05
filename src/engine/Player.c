#include "Player.h"

void Player_Loop(Player* const player) {
    Character_Loop(player);

    if (Character_ShouldProcessShot(player)) {
	player->tracks[WEAPON_ANIMATION_SHOOT].weight = FLOAT_BINONE;

	GUI_UpdateAmmoMag(player->ammoMag);
    }

    Character_HandleFatigue(player);
}
