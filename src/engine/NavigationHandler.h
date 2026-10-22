#ifndef NavigationHandler_h_
#define NavigationHandler_h_

#include "Geometry.h"
#include "vec3Array.h"

#define ESCAPE_FROM_DANGER_MODE MAXT(TriangleID)

void NH_Init();
//Returns true if the path was found. If toTri is ESCAPE_FROM_POINT_MODE then 'to' argument is treated as danger point
bool NH_FindPath(vec3s from, vec3s to, TriangleID fromTri, TriangleID toTri, vec3sArray* dest);
TriangleID NH_GetClosestTriangle(vec3s p);
TriangleID NH_GetRandomPoint(vec3s* dest);
const Geometry* NH_GetGeometry();

void NH_DrawDebugGui();

#endif
