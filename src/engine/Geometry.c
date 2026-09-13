#include <cglm/vec3.h>
#include <cglm/io.h>
#include <cglm/ray.h>
#include "def.h"
#include "ModelsHandler.h"
#include "DebugGuiHandler.h"
#include "Geometry.h"

void Geometry_Init(Geometry* const geometry, const char fileName[const]) {
    ModelGeometry gltfGeo;

    ModelsHandler_LoadGeometry(fileName, &gltfGeo);

    geometry->numVertices = gltfGeo.numVertices;
    geometry->numTriangles = gltfGeo.numIndices / 3;

    geometry->vertices = gltfGeo.vertices;
    geometry->triangles = (Triangle*)gltfGeo.indices;

    /*
    puts("vert start");
    foreach (vec3* const v, geometry->vertices, geometry->numVertices)
	glm_vec3_print(v[0], stdout);
    forend
    puts("vert end");
    */
    /*
    puts("ind start");
    for (TriangleID i = 0; i < geometry->numTriangles * 3; i++) {
	printf("%u\n", gltfGeo.indices[i]);
    }
    puts("ind end");
    */
}
float* Geometry_GetVertex(
    const Geometry* const geometry, const TriangleID triId, const Index3D index
) {
    return geometry->vertices[geometry->triangles[triId][index]];
}
TriangleID Geometry_Raycast(const Geometry* const geometry, Ray ray, vec3 dest) {
    float minDistance;
    TriangleID triangleId;

    minDistance = INFINITY;

    INVALIDATE(triangleId);

    for (TriangleID i = 0; i < geometry->numTriangles; i++) {
	float distance;

	float* const p0 = Geometry_GetVertex(geometry, i, 0);
	float* const p1 = Geometry_GetVertex(geometry, i, 1);
	float* const p2 = Geometry_GetVertex(geometry, i, 2);

	if (glm_ray_triangle(ray[0], ray[1], p0, p1, p2, &distance) && distance < minDistance) {
	    minDistance = distance;
	    triangleId = i;
	}
    }

    if (minDistance != INFINITY) {
	glm_vec3_scale(ray[1], minDistance, ray[1]);
	glm_vec3_add(ray[0], ray[1], dest);
    }

    return triangleId;
}

DGH_BEGIN(Geometry, geometry, 4) {
    DGH_FIELD(geometry->numVertices);
    DGH_FIELD(geometry->numTriangles);
    DGH_ARRAY(geometry->vertices, geometry->numVertices);
    DGH_ARRAY(geometry->triangles, geometry->numTriangles);
DGH_END }
