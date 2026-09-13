#include <cglm/vec3.h>
#include <cglm/quat.h>
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

static void transformsToDelta(Animation* const animation) {
    for (BoneID i = 0; i < mesh.numBones; i++) {
	const KeyframeSetTranslation* const setTrans = animation->setsTrans + i;
	const KeyframeSetRotation* const setRot = animation->setsRot + i;

	float* const origRot = setRot->rotationVersors[0];

	for (KeyframeID j = 1; j < setTrans->numKeyframes; j++) {
	    float* const orig = setTrans->translationVectors[0];

	    glm_vec3_sub(setTrans->translationVectors[j], orig, setTrans->translationVectors[j]);
	}

	glm_vec3_zero(setTrans->translationVectors[0]);

	glm_quat_inv(origRot, origRot);

	for (KeyframeID j = 1; j < setRot->numKeyframes; j++) {
	    glm_quat_mul(origRot, setRot->rotationVersors[j], setRot->rotationVersors[j]);
	}

	glm_quat_identity(setRot->rotationVersors[0]);
    }
}
static void processCharacter(
    Character* const restrict character, Character* const restrict c, float* const minDistance, Character** const result
) {
    vec3 dir;

    glm_vec3_sub(c->position, character->position, dir);

    const float distance = glm_vec3_norm2(dir);

    if (distance < *minDistance) {
	Ray ray;

	glm_vec3_copy(character->position, ray[0]);

	ray[0][1] += 1;

	glm_vec3_normalize_to(dir, ray[1]);

	const TriangleID tri = Geometry_Raycast(&collision, ray, ray[0]);

	if (ISINVALID(tri) || glm_vec3_distance2(character->position, ray[0]) > distance) {
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
Character* CH_FindClosestVisibleAliveArmedCharacter(Character* const character) {
    float minDistance;

    Character* result;

    minDistance = INFINITY;
    result = NULL;

    foreach (Character* const* const c, characters, nextCharacterId)
	//if (Character_CanSeeDotCheck(character, *c)) {
	    if ((*c)->state != CHARACTER_STATE_DEAD && (*c)->hasGun) processCharacter(character, *c, &minDistance, &result);
	//}
    forend

    return result;
}
Character* CH_FindClosestVisibleAliveCharacter(Character* const character) {
    float minDistance;

    Character* result;

    minDistance = INFINITY;
    result = NULL;

    foreach (Character* const* const c, characters, nextCharacterId)
	if (Character_CanSeeDotCheck(character, *c)) {
	    if ((*c)->state != CHARACTER_STATE_DEAD) processCharacter(character, *c, &minDistance, &result);
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
