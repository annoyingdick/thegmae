#ifndef Geometry_h_
#define Geometry_h_

#include <cglm/types-struct.h>
#include "Render.h"

typedef RegionSize TriangleID;
typedef Index3D Triangle[3];

typedef struct {
    Index3D numVertices;
    TriangleID numTriangles;

    vec3s* vertices;
    Triangle* triangles;
} Geometry;

void Geometry_Init(Geometry* geometry, const char fileName[]);
vec3s Geometry_GetVertex(const Geometry* geometry, TriangleID triId, Index3D index);
TriangleID Geometry_Raycast(const Geometry* geometry, Ray ray, vec3s* dest);

void Geometry_DrawDebugGui(const Geometry* geometry, const char name[]);

#endif
