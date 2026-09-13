#include <float.h>
#include "def.h"
#include "PhysicsSimulationHandler.h"

#define NON_MOVING 0
#define MOVING 1

#define DEFAULT_GRAVITY -9.81

void PSH_Init() {}

PhysBodyID PSH_CreateBoxBody(const bool isDynamic, const float* const pos, const float* const halfSize) {
    if (isDynamic && pos && halfSize) {}
    return 0;
}
void* PSH_CreateRagdoll(void* const settings, const RagdollJointID numJoints, PhysBodyID outBodyIds[]) {
    if (settings && numJoints && outBodyIds) {}
    return NULL;
}
void* PSH_CreateCharacter(const float* const pos) {
    if (pos) {}

    return NULL;
}
void PSH_ActivateBody(const PhysBodyID id) {
    if (id) {}
}
void PSH_DeleteBody(const PhysBodyID id) {
    if (id) {}
}
void PSH_GetCharacterPosition(void* const character, float* const dest) {
    if (character && dest) {}
}
void PSH_GetTransform(const PhysBodyID id, void* const dest) {
    if (id && dest) {}
}
void PSH_SetCharacterVelocity(void* const character, const float* const velocity) {
    if (character && velocity) {}
}
void PSH_SetCharacterPosition(void* const character, const float* const position) {
    if (character && position) {}
}
void PSH_SetGravity(const float* const gravity) {
    if (gravity) {}
}
void PSH_FixedLoop() {}
