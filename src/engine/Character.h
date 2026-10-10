#ifndef Character_h_
#define Character_h_

#include <math.h>
#include "InstancesHandler.h"
#include "Geometry.h"
#include "vec3Array.h"
#include "AnimationTrack.h"
#include "GUI.h"

#define FLOAT_BINONE nextafterf(0, INFINITY)

typedef uint8_t SlotID;
typedef uint8_t StateID;

enum {
    SLOT_NOTHING,
    SLOT_PISTOL,
    SLOT_MAX_ENUM
};
enum : AnimationID {
    NOTHING_ANIMATION_IDLE,
    NOTHING_ANIMATION_WALK,
    NOTHING_ANIMATION_SPRINT,
    NOTHING_ANIMATION_DEATH_FROMFRONT,
    NOTHING_ANIMATION_DEATH_FROMBACK,
    NOTHING_ANIMATION_SCARED,
    WEAPON_ANIMATION_IDLE,
    WEAPON_ANIMATION_WALK,
    WEAPON_ANIMATION_SPRINT,
    WEAPON_ANIMATION_IDLE_STEADY,
    WEAPON_ANIMATION_SHOOT,
    WEAPON_ANIMATION_EQUIP,
    WEAPON_ANIMATION_UNEQUIP,
    ANIMATION_MAX_ENUM
};
enum {
    CHARACTER_STATE_NORMAL,
    CHARACTER_STATE_SPRINT,
    CHARACTER_STATE_AIM,
    CHARACTER_STATE_DEAD
};

#define VARS_CHARACTER \
X(float, fatigue) \
X(InstancePtrID, instance) \
X(InstancePtrID, weaponInstance) \
X(Ammo, ammoMag) \
X(Ammo, ammoLeft) \
X(SlotID, wishSlot) \
X(SlotID, currentSlot) \
X(StateID, state) \
X(bool, areAnimationsLoaded) \
X(bool, aimTaskLock) \
X(bool, hasGun)

typedef struct Character Character;

struct Character {
    vec3Array path;

#define X(type, name) type name;
VARS_CHARACTER
#undef X

    vec3 position, muzzlePosition;
    vec2 direction;

    Character* target;
    float* localMuzzlePosition;

    AnimationTrack tracks[ANIMATION_MAX_ENUM];
};

void Character_Init(Character* character, Mesh* weaponMesh);
bool Character_CanSeeDotCheck(Character* character, Character* them);
bool Character_ShouldProcessShot(const Character* character);
void Character_BeginSprinting(Character* character);
void Character_StopSprinting(Character* character);
void Character_ChooseSlot(Character* character, SlotID slot);
void Character_SwitchAim(Character* character, bool aim);
void Character_GoTo(Character* character, vec3 goal, TriangleID goalTri);
void Character_TryInitAnimations(Character* character);
void Character_HandleFatigue(Character* character);
//You'd better not use this function alone if your character can shoot. If you do anyway, please adapt this line:
//if (Character_ShouldProcessShot(character)) character->tracks[WEAPON_ANIMATION_SHOOT].weight = FLOAT_BINONE;
void Character_Loop(Character* character);

void Character_DrawDebugGui(const Character* character, const char name[]);

#endif
