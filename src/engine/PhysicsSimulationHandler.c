#include <float.h>
#include <Jolt/joltc.h>
#include "def.h"
#include "PhysicsSimulationHandler.h"

#define NON_MOVING 0
#define MOVING 1

#define DEFAULT_GRAVITY -9.81

static JPH_PhysicsSystem* physSystem;
static JPH_JobSystem* jobSystem;
static JPH_BodyInterface* bodyInterface;

static JPH_Vec3 simGravity = {0, DEFAULT_GRAVITY, 0};

void PSH_Init() {
    const uint32_t maxObjects = 1024;

    if (!JPH_Init()) throwFatal("Jolt physics error occurred!", "Initialization error");

    JPH_BroadPhaseLayerInterface* const bpli = JPH_BroadPhaseLayerInterfaceTable_Create(2, 2);
    JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(bpli, NON_MOVING, NON_MOVING);
    JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(bpli, MOVING, MOVING);

    JPH_ObjectLayerPairFilter* const olpf = JPH_ObjectLayerPairFilterTable_Create(2);
    JPH_ObjectLayerPairFilterTable_EnableCollision(olpf, NON_MOVING, MOVING);
    JPH_ObjectLayerPairFilterTable_EnableCollision(olpf, MOVING, NON_MOVING);
    JPH_ObjectLayerPairFilterTable_EnableCollision(olpf, MOVING, MOVING);

    JPH_ObjectVsBroadPhaseLayerFilter* const ovbplf = JPH_ObjectVsBroadPhaseLayerFilterTable_Create(bpli, 2, olpf, 2);

    const JPH_PhysicsSystemSettings pss = {
        .maxBodies = maxObjects,
        .maxBodyPairs = maxObjects,
        .maxContactConstraints = maxObjects,
        .numBodyMutexes = 0,
        .broadPhaseLayerInterface = bpli,
        .objectVsBroadPhaseLayerFilter = ovbplf,
        .objectLayerPairFilter = olpf
    };

    physSystem = JPH_PhysicsSystem_Create(&pss);
    jobSystem = JPH_JobSystemThreadPool_Create(NULL);
    bodyInterface = JPH_PhysicsSystem_GetBodyInterface(physSystem);
}
PhysBodyID PSH_CreateBoxBody(const bool isDynamic, const float* const pos, const float* const halfSize) {
    JPH_Shape* shape = (JPH_Shape*)JPH_BoxShape_Create((const JPH_Vec3*)halfSize, JPH_DEFAULT_CONVEX_RADIUS);

    JPH_BodyCreationSettings* const bcs = JPH_BodyCreationSettings_Create3(
	shape, (const JPH_RVec3*)pos, NULL, isDynamic ? JPH_MotionType_Dynamic : JPH_MotionType_Static, isDynamic
    );

    const JPH_BodyID id = JPH_BodyInterface_CreateAndAddBody(bodyInterface, bcs, JPH_Activation_Activate);

    JPH_BodyCreationSettings_Destroy(bcs);
    JPH_Shape_Destroy(shape);

    return id;
}
void* PSH_CreateRagdoll(void* const settings, const RagdollJointID numJoints, PhysBodyID outBodyIds[]) {
    JPH_Ragdoll* const ragdoll = JPH_RagdollSettings_CreateRagdoll(settings, physSystem, 0, 0);

    JPH_Ragdoll_AddToPhysicsSystem(ragdoll, JPH_Activation_Activate, true);

    for (RagdollJointID i = 0; i < numJoints; i++) outBodyIds[i] = JPH_Ragdoll_GetBodyID(ragdoll, i);

    return ragdoll;
}
void* PSH_CreateCharacter(const float* const pos) {
    JPH_Shape* const shape = (JPH_Shape*)JPH_CapsuleShape_Create((CHARACTER_SIZE_Y / 2) - .3f, .3f);

    JPH_Character* const character = JPH_Character_Create(&(JPH_CharacterSettings){
	.base = {
	    .up = {0, 1, 0},
	    .supportingVolume = {{0, 1, 0}, -1.0e10f},
	    .maxSlopeAngle = 50 * JPH_M_PI / 180,
	    .enhancedInternalEdgeRemoval = false,
	    .shape = shape
	},
	.layer = MOVING,
	.mass = 80,
	.friction = .2f,
	.gravityFactor = 1,
	.allowedDOFs = JPH_AllowedDOFs_TranslationX + JPH_AllowedDOFs_TranslationY + JPH_AllowedDOFs_TranslationZ
    }, (JPH_RVec3*)pos, &(JPH_Quat){0, 0, 0, 1}, 0, physSystem);

    JPH_Character_AddToPhysicsSystem(character, JPH_Activation_Activate, true);

    return character;
}
void PSH_ActivateBody(const PhysBodyID id) {
    JPH_BodyInterface_ActivateBody(bodyInterface, id);
}
void PSH_DeleteBody(const PhysBodyID id) {
    JPH_BodyInterface_RemoveAndDestroyBody(bodyInterface, id);
}
void PSH_GetCharacterPosition(void* const character, float* const dest) {
    JPH_Character_GetPosition(character, (JPH_RVec3*)dest, true);
}
void PSH_GetTransform(const PhysBodyID id, void* const dest) {
    JPH_BodyInterface_GetWorldTransform(bodyInterface, id, (JPH_RMat4*)dest);
}
void PSH_SetCharacterVelocity(void* const character, const float* const velocity) {
    JPH_Character_SetLinearVelocity(character, (JPH_Vec3*)velocity, true);
}
void PSH_SetCharacterPosition(void* const character, const float* const position) {
    JPH_Character_SetPosition(character, (JPH_RVec3*)position, true, true);
}
void PSH_SetGravity(const float* const gravity) {
    simGravity = *(const JPH_Vec3*)gravity;
}
void PSH_FixedLoop() {
    JPH_Vec3 grav;

    JPH_PhysicsSystem_GetGravity(physSystem, &grav);

    JPH_PhysicsSystem_SetGravity(physSystem, &simGravity);
    JPH_PhysicsSystem_Update(physSystem, (float)FIXED_LOOP_DELTA_TIME_NS / NANOSECONDS_IN_ONE_SECOND, 1, jobSystem);
}
