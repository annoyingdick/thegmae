#ifndef NavigationHandler_h_
#define NavigationHandler_h_

#include "Geometry.h"
#include "vec3Array.h"

#define ESCAPE_FROM_DANGER_MODE MAXT(TriangleID)

void NH_Init();
//Returns true if the path was found. If toTri is ESCAPE_FROM_POINT_MODE then 'to' argument is treated as danger point
bool NH_FindPath(vec3 from, vec3 to, TriangleID fromTri, TriangleID toTri, vec3Array* dest);
TriangleID NH_GetClosestTriangle(vec3 p);
TriangleID NH_GetRandomPoint(vec3 dest);
const Geometry* NH_GetGeometry();

void NH_DrawDebugGui();

#endif
