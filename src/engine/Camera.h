#ifndef Camera_h_
#define Camera_h_

#include <cglm/types-struct.h>

#define PUBVARS_Camera \
X(float, yaw, FLOAT) \
X(float, pitch, FLOAT) \
X(float, distance, FLOAT) \
X(vec3s, lookAtPosition, VECTOR)

void Camera_Init();
float Camera_GetYaw();
float Camera_GetZoomDistance();
vec3s Camera_GetPosition();
mat4s Camera_GetPerspectiveCameraMatrix(mat4s perspectiveMat);
void Camera_SetLookAtPosition(vec3s position);
void Camera_MouseWheel(float scroll, float x, float y);
void Camera_MouseMotion(float xrel, float yrel);

#endif
