#ifndef PhysicsSimulationHandler_h_
#define PhysicsSimulationHandler_h_

#include <stdbool.h>
#include <stdint.h>

#define CHARACTER_SIZE_Y 1.8f

typedef uint32_t PhysBodyID;
typedef uint8_t RagdollJointID;

void PSH_Init();
PhysBodyID PSH_CreateBoxBody(bool isDynamic, const float* pos, const float* halfSize);
void* PSH_CreateRagdoll(void* settings, RagdollJointID numJoints, PhysBodyID outBodyIds[]);
void* PSH_CreateCharacter(const float* pos);
void PSH_ActivateBody(PhysBodyID id);
void PSH_DeleteBody(PhysBodyID id);
void PSH_GetCharacterPosition(void* character, float* dest);
void PSH_GetTransform(PhysBodyID id, void* dest);
void PSH_SetCharacterVelocity(void* character, const float* velocity);
void PSH_SetCharacterPosition(void* character, const float* position);
void PSH_SetGravity(const float* gravity);
void PSH_FixedLoop();

#endif
