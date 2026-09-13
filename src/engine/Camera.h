#ifndef Camera_h_
#define Camera_h_

#include <cglm/types.h>

#define PUBVARS_Camera \
X(float, yaw, FLOAT) \
X(float, pitch, FLOAT) \
X(float, distance, FLOAT) \
X(vec3, lookAtPosition, VECTOR) \
X(vec3, velocity, VECTOR)

void Camera_Init();
float Camera_GetYaw();
float Camera_GetZoomDistance();
void Camera_GetPosition(vec3 dest);
void Camera_GetPerspectiveCameraMatrix(mat4 perspectiveMat, mat4 dest);
void Camera_SetLookAtPosition(vec3 position);
void Camera_MouseWheel(float scroll, float x, float y);
void Camera_MouseMotion(float xrel, float yrel);

void* const* Camera_GetPublicVars();

#endif
