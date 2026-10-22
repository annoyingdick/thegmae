#include <cglm/struct/vec3.h>
#include <cglm/struct/quat.h>
#include "Arena.h"
#include "DebugGuiHandler.h"
#include "CharactersHandler.h"

#define INIT_NUM_CHARACTERS 2

typedef unsigned int CharacterID;

static Arena arena;
static Geometry collision;
static Mesh mesh;
static CharacterID nextCharacterId;
static bool areAnimationsLoaded;

static Character** characters;

static bool isAimable(Character* const character) {
    const float time = .3f;

    return character->state != CHARACTER_STATE_DEAD || Character_GetDeathTrack(character)->time < time;
}
static void transformsToDelta(Animation* const animation) {
    for (BoneID i = 0; i < mesh.numBones; i++) {
	const KeyframeSetTranslation* const setTrans = animation->setsTrans + i;
	const KeyframeSetRotation* const setRot = animation->setsRot + i;

	versors* const origRot = setRot->rotationVersors + 0;

	for (KeyframeID j = 1; j < setTrans->numKeyframes; j++) {
	    setTrans->translationVectors[j] = glms_vec3_sub(setTrans->translationVectors[j], setTrans->translationVectors[0]);
	}

	setTrans->translationVectors[0] = glms_vec3_zero();

	*origRot = glms_quat_inv(*origRot);

	for (KeyframeID j = 1; j < setRot->numKeyframes; j++) {
	    setRot->rotationVersors[j] = glms_quat_mul(*origRot, setRot->rotationVersors[j]);
	}

	setRot->rotationVersors[0] = glms_quat_identity();
    }
}
static void processCharacter(
    Character* const restrict character, Character* const restrict c, float* const minDistance, Character** const result
) {
    const vec3s dir = glms_vec3_sub(c->position, character->position);

    const float distance = glms_vec3_norm2(dir);

    if (distance < *minDistance) {
	Ray ray = {.position = character->position, .direction = glms_vec3_normalize(dir)};

	ray.position.raw[1] += 1;

	//why not use the existing memory?
	const TriangleID tri = Geometry_Raycast(&collision, ray, &ray.position);

	if (ISINVALID(tri) || glms_vec3_distance2(character->position, ray.position) > distance) {
	    *minDistance = distance;
	    *result = c;
	}
    }
}

void CH_Init() {
    Arena_Init(&arena, INIT_NUM_CHARACTERS);
    Geometry_Init(&collision, "col.gltf");
    Mesh_Init(&mesh, GRAPHICS_PIPELINE_SKINNED, "boss.gltf");

    mallocarr(characters, INIT_NUM_CHARACTERS);
}
void CH_AddCharacter(Character* const character) {
    Region region;

    RegionSize newSize;

    if (Arena_RequestRegion(&arena, &region, &newSize, 1)) ++nextCharacterId;
    if (newSize) reallocarr(characters, newSize);

    characters[region.position] = character;
}
Character* CH_FindClosestVisibleAimableArmedCharacter(Character* const character) {
    float minDistance;

    Character* result;

    minDistance = INFINITY;
    result = NULL;

    foreach (Character* const* const c, characters, nextCharacterId)
	//can see armed characters behind the back
	//if (Character_CanSeeDotCheck(character, *c)) {
	    if (isAimable(*c) && (*c)->hasGun) processCharacter(character, *c, &minDistance, &result);
	//}
    forend

    return result;
}
Character* CH_FindClosestVisibleAimableCharacter(Character* const character) {
    float minDistance;

    Character* result;

    minDistance = INFINITY;
    result = NULL;

    foreach (Character* const* const c, characters, nextCharacterId)
	if (Character_CanSeeDotCheck(character, *c)) {
	    if (isAimable(*c)) processCharacter(character, *c, &minDistance, &result);
	}
    forend

    return result;
}
Mesh* CH_GetMesh() {
    return &mesh;
}
void CH_Loop() {
    //wait for mesh to load
    if (mesh.animations && !areAnimationsLoaded) {
	areAnimationsLoaded = true;

	foreach (Animation* const animation, mesh.animations, mesh.numAnimations)
	    if (!strcmp(animation->name, "pistol_shoot")) {
		transformsToDelta(animation);

		break;
	    }
	forend
    }
}

void CH_DrawDebugGui() {
    DGH_FIELD(&arena);
    DGH_FIELD(&collision);
    DGH_FIELD(&mesh);
    DGH_FIELD(nextCharacterId);
    DGH_FIELD(areAnimationsLoaded);
    DGH_ARRAY(characters, nextCharacterId);
}
