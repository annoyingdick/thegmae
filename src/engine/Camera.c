#include <cglm/affine.h> // IWYU pragma: keep
#include <cglm/cam.h>
#include <cglm/mat4.h>
#include "Camera.h"

#define X(type, name, typeenum) static type name;
PUBVARS_Camera
#undef X
#define X(type, name, typeenum) &name,
static void* const pub[] = {PUBVARS_Camera};
#undef X

#define PITCH -M_PI_2

#define FPS_TYPE_CAMERA

/*
static void applyMove() {
#ifdef FPS_TYPE_CAMERA
    const float speed = .001f;
#else
    const float speed = positionNew[1] / 1000;
#endif

    const float x = (float)(moveLeft - moveRight) * speed, z = (float)(moveForward - moveBack) * speed;

#ifdef FPS_TYPE_CAMERA
    const float yawcos = cosf(yaw), yawsin = sinf(yaw);
    const float pitchcos = cosf(pitch), pitchsin = sinf(pitch);

    if (yawcos || yawsin || pitchcos || x) {}

    vec3 move = {(z * yawcos * pitchcos) + (x * yawsin), z * pitchsin, (z * yawsin * pitchcos) - (x * yawcos)};

    glm_vec3_add(velocity, move, velocity);
#else
    glm_vec3_sub(velocity, (vec3){x, 0, z}, velocity);
#endif
}
static void applyFriction() {
    const float factor = .98f;

    glm_vec3_scale(velocity, factor, velocity);
}
*/

void Camera_Init() {
    yaw = -(float)M_PI_2;
    pitch = -(float)M_PI_2 / 2;
    distance = 3;
    //pitch = PITCH;
}
float Camera_GetYaw() {
    return yaw;
}
float Camera_GetZoomDistance() {
    return distance;
}
void Camera_GetPosition(vec3 dest) {
    vec3 direction = {
	-cosf(yaw) * cosf(pitch) * distance,
	-sinf(pitch) * distance,
	-sinf(yaw) * cosf(pitch) * distance
    };

    glm_vec3_add(lookAtPosition, direction, dest);
}
void Camera_GetPerspectiveCameraMatrix(mat4 perspectiveMat, mat4 dest) {
    mat4 cameraMat = GLM_MAT4_IDENTITY_INIT;
    vec3 position;

    Camera_GetPosition(position);

    glm_lookat(position, lookAtPosition, GLM_YUP, cameraMat);
    //glm_look(positionInterp, (vec3){0, sinf(pitch), -cosf(pitch)}, (vec3){0, 0, -1}, cameraMat);

    glm_mat4_mul(perspectiveMat, cameraMat, dest);
}
void Camera_SetLookAtPosition(vec3 position) {
    glm_vec3_copy(position, lookAtPosition);
}
void Camera_MouseWheel(const float scroll, const float x, const float y) {
#ifdef FPS_TYPE_CAMERA
    if (x && y) {}

    distance = fmaxf(distance - scroll, 1);
#else
    const float observableHeight = 10000;

    positionNew[1] *= scroll > 0 ? 1.0f / 2 : 2;

    positionNew[1] = positionNew[1] > 1 ? positionNew[1] : 1;

    if (positionNew[1] < observableHeight) pitch = PITCH + ((observableHeight - positionNew[1]) / observableHeight * M_PI_2);
    else pitch = PITCH;

    if (scroll > 0) {
	positionNew[0] += x * positionNew[1] * 2 * scroll;
	positionNew[2] += y * positionNew[1] * 2 * scroll;
    }
#endif
}
void Camera_MouseMotion(const float xrel, const float yrel) {
#ifdef FPS_TYPE_CAMERA
    const float sensitivity = .001f;

    yaw += sensitivity * xrel;
    pitch -= sensitivity * yrel;

    while (yaw > M_PI) {
	yaw -= M_PI * 2;
    }
    while (yaw < -M_PI) {
	yaw += M_PI * 2;
    }
#else
    if (xrel && yrel) {}
#endif
}

void* const* Camera_GetPublicVars() {
    return pub;
}
