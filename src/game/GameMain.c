#include <SDL3/SDL_Init.h>
#include <cglm/mat4.h>
#include "engine/Character.h"
#include "engine/Camera.h"
#include "engine/PhysicsSimulationHandler.h"
#include "engine/Pip.h"
#include "engine/Geometry.h"
#include "engine/NavigationHandler.h"
#include "TestMeshStreaming.h"

#define NUM_TEST_CUBES 1
#define NUM_TEST_CHARACTERS 1

static Mesh arrowMesh, cubeMesh, cubeMeshDyn, tlt, navMesh;
static Mesh weaponMesh;
static Character character, angry[NUM_TEST_CHARACTERS];
static ShaderProgram linesProgram;
static GPUBuffer linesVerticesBuffer, linesIndicesBuffer;

static InstancePtrID testCubes[NUM_TEST_CUBES];
static PhysBodyID testCubesPhys[NUM_TEST_CUBES];
static mat4 testCubesTrans[NUM_TEST_CUBES];
static vec3 cameraPos;
static NDC ndc;

static float moveForward, moveBack, moveRight, moveLeft;
static InstancePtrID tlti;
static bool tltspace, shift, mouse3;

/*
static void createStaticCube(vec3 pos, vec3 halfSize) {
    InstancePtrID ptrId;

    mat4 mat = GLM_MAT4_IDENTITY_INIT;

    PSH_CreateBoxBody(false, pos, halfSize);

    ptrId = IH_NewInstance(&cubeMesh);

    glm_translate(mat, pos);
    glm_scale(mat, halfSize);

    IH_UploadStatic(ptrId, sizeof(mat), mat);
}
*/
static InstancePtrID createDynamicCube(vec3 pos, vec3 halfSize, PhysBodyID* const outPhysId) {
    *outPhysId = PSH_CreateBoxBody(true, pos, halfSize);

    return IH_NewInstance(&cubeMeshDyn);
}
static void specifyLinesWidth() {
    const GLfloat linesWidth = 10;

    glLineWidth(linesWidth);
}

//IMPORTANT LIMITATION: it is not guaranteed that your instances data on gpu will be safe and 
//not erased! When you create an instance of an old mesh, the data of every instance of a newer 
//mesh get shifted inside a buffer. Usually this doesn't cause any issues with dynamic data, 
//but static one can be ruined. To avoid issues, try to follow the order of creation of meshes.
//Create instances only once a mesh is initialized, not after.

void GM_Init() {
    const Geometry* const navgeo = NH_GetGeometry();

    Mesh_Init(&cubeMesh, GRAPHICS_PIPELINE_STATIC, "cube.gltf");
    Mesh_Init(&cubeMeshDyn, GRAPHICS_PIPELINE_INTERP, "deagle.gltf");
    Mesh_Init(&arrowMesh, GRAPHICS_PIPELINE_INTERP, "arrow.gltf");
    Mesh_Init(&navMesh, GRAPHICS_PIPELINE_STATIC, "nav.gltf");
    Mesh_Init(&weaponMesh, GRAPHICS_PIPELINE_NORMAL, "deagle.gltf");

    Character_Init(&character, &weaponMesh);

    character.hasGun = true;

    nforeach (Character* const an, angry)
	Character_Init(an, &weaponMesh);
	NH_GetRandomPoint(an->position);

	an->hasGun = an - angry > 40;

	Character_ChooseSlot(an, 1);
    forend

    ShaderProgram_Init_VF(
	&linesProgram, (ShaderProgramInitInfo_VF){"lines.vert", "lines.frag"}
    );

    GPUBuffer_InitWithData(
	&linesVerticesBuffer, navgeo->numVertices * (GLsizeiptr)sizeof(*navgeo->vertices), 
	navgeo->vertices
    );
    GPUBuffer_InitWithData(
	&linesIndicesBuffer, navgeo->numTriangles * (GLsizeiptr)sizeof(*navgeo->triangles), 
	navgeo->triangles
    );
    
    specifyLinesWidth();

    //createStaticCube((vec3){0, -1, 0}, (vec3){10, 1, 10});
    //createStaticCube((vec3){0, 0, -1}, (vec3){10, 1, 0.01f});

    for (size_t i = 0; i < NUM_TEST_CUBES; i++) {
	testCubes[i] = createDynamicCube((vec3){100, 100, 0}, GLM_VEC3_ONE, testCubesPhys + i);
    }

    const mat4 mat = GLM_MAT4_IDENTITY_INIT;

    IH_UploadStatic(IH_NewInstance(&navMesh), sizeof(mat), mat);

    //if (lol) {}

    //glm_mat4_identity(IH_GetUploadPtr(lol));
}
void GM_Mouse3(const NSC coords) {
    //const float rayDistance = 100;

    memcpy(ndc, coords, sizeof(ndc));

    TO_NDC(ndc);

    mouse3 = true;

    //printf("%f %f %f\n", result[0], result[1], result[2]);
}
void GM_KeyDown(const SDL_Keycode key) {
    switch (key) {
    case SDLK_W:
	moveForward = true;
	break;
    case SDLK_S:
	moveBack = true;
	break;
    case SDLK_D:
	moveRight = true;
	break;
    case SDLK_A:
	moveLeft = true;
	break;
    case SDLK_SPACE:
	tltspace = true;
	break;
    case SDLK_UP:
	Character_ChooseSlot(&character, character.currentSlot + 1);
	break;
    case SDLK_DOWN:
	Character_ChooseSlot(&character, character.currentSlot - 1);
	break;
    case SDLK_1:
	Character_ChooseSlot(&character, 0);
	break;
    case SDLK_2:
	Character_ChooseSlot(&character, 1);
	break;
    case SDLK_X:
	Character_SwitchAim(&character, character.state != CHARACTER_STATE_AIM);
	break;
    case SDLK_C:
	if (character.state == CHARACTER_STATE_SPRINT) Character_StopSprinting(&character);
	else Character_BeginSprinting(&character);

	break;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
	shift = true;
    }
}
void GM_KeyUp(const SDL_Keycode key) {
    switch (key) {
    case SDLK_W:
	moveForward = false;
	break;
    case SDLK_S:
	moveBack = false;
	break;
    case SDLK_D:
	moveRight = false;
	break;
    case SDLK_A:
	moveLeft = false;
	break;
    case SDLK_SPACE:
	tltspace = false;
	break;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
	shift = false;
    }
}
void GM_SetMetadata() {
    SDL_SetAppMetadata("Game", "0.1.0", "com.annoyingdick.game");
}
void GM_Loop(const float interp) {
    //as soon as there are other instances that were created before tlt, tlti must be non zero
    if (true) {
	if (tltspace && !Mesh_IsValid(&tlt)) {//!Mesh_IsValid(&tlt)) {
	    Mesh_Init(&tlt, GRAPHICS_PIPELINE_STATIC, "mosquitoinamber.gltf");

	    tlti = IH_NewInstance(&tlt);
	}
	else if (!tltspace && Mesh_IsValid(&tlt)) {
	    IH_DeleteInstance(tlti);

	    Mesh_Destroy(&tlt);
	}
    }
    if (mouse3) {
	Ray ray;

	R_NDCtoDirection(ndc, ray[1]);
	Camera_GetPosition(ray[0]);

	const TriangleID goalTri = Geometry_Raycast(NH_GetGeometry(), ray, ray[0]);

	if (!ISINVALID(goalTri)) Character_GoTo(&character, ray[0], goalTri);

	mouse3 = false;
    }

    if (interp) {}

    Character_Loop(&character);

    nforeach (Character* const an, angry)
	if (rand() < 100 && an->hasGun) {
	    vec3 goal;

	    const TriangleID goalTri = NH_GetRandomPoint(goal);

	    Character_GoTo(an, goal, goalTri);
	}
	
	Character_SwitchAim(an, true);
	Character_Loop(an);
    forend

    //camera shenaningans
    const float moveX = moveLeft - moveRight, moveZ = moveForward - moveBack;
    const float speed = Camera_GetZoomDistance() * WH_GetDeltaTime();

    cameraPos[0] += (sinf(Camera_GetYaw()) * moveX + cosf(Camera_GetYaw()) * moveZ) * speed;
    cameraPos[2] += (sinf(Camera_GetYaw()) * moveZ - cosf(Camera_GetYaw()) * moveX) * speed;

    Camera_SetLookAtPosition(cameraPos);

    TestMeshStreaming_Loop();

    if (Mesh_IsValid(&tlt)) {
	const mat4 mat = GLM_MAT4_IDENTITY_INIT;

	IH_UploadStatic(tlti, sizeof(mat), mat);
    }

    //Camera_SetLookAtPosition((vec3){5, 0, -5});
}
void GM_R_PostLoop() {
    ShaderProgram_Use(linesProgram);

    GPUBuffer_BindBase(linesVerticesBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_VERTICES);
    GPUBuffer_Bind(linesIndicesBuffer, GL_ELEMENT_ARRAY_BUFFER);

    glDepthFunc(GL_ALWAYS);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(
	GL_TRIANGLES, (GLsizei)NH_GetGeometry()->numTriangles * 3, GL_UNSIGNED_INT, NULL
    );
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
void GM_PSH_PreFixedLoop() {

}
void GM_FixedLoop() {

}
void GM_R_PreFixedLoopOnce() {
    //PSH_SetGravity((vec3){(float)(rand() - RAND_MAX / 2) / RAND_MAX, 0, 0});

    for (size_t i = 0; i < NUM_TEST_CUBES; i++) {
	glm_mat4_copy(testCubesTrans[i], IH_GetUploadPtr(testCubes[i]));

	PSH_GetTransform(testCubesPhys[i], testCubesTrans[i]);

	glm_mat4_copy(testCubesTrans[i], IH_GetUploadPtr(testCubes[i]) + sizeof(mat4));
    }
}
