#include "GUI.h"
#include "Player.h"

void Player_Loop(Player* const player) {
    Character_Loop(&player->base);

    if (Character_ShouldProcessShot(&player->base)) {
	Character_GetWeaponTracks(&player->base, WEAPON_ANIMATION_SHOOT)->weight = FLOAT_BINONE;

	GUI_UpdateAmmoMag(player->base.ammoMag);
    }
    if (player->base.state == CHARACTER_STATE_DEAD) {
	const float respawnTime = 5;

	player->timeBeingDead += WH_GetDeltaTime();

	if (player->timeBeingDead > respawnTime) {
	    AnimationTrack* const t0 = player->base.tracks + NOTHING_ANIMATION_DEATH_FROMFRONT,
			  * const t1 = player->base.tracks + NOTHING_ANIMATION_DEATH_FROMBACK;

	    player->base.state = CHARACTER_STATE_NORMAL;

	    player->timeBeingDead = 0;

	    t0->weight = t0->time = t1->time = t1->weight = 0;

	    GUI_HideDeath();
	}
    }

    Character_HandleFatigue(&player->base);
}
