#include <khashl.h>
#include <cglm/vec3.h>
#include "HeapQueue.h"
#include "DebugGuiHandler.h"
#include "NavigationHandler.h"

#define DIR0 1
#define DIR1 -1

//one edge can only have two neigbouring triangles
typedef Index3D Edge[2];

typedef struct {
    TriangleID triangles[3];
    Edge edges[3];
    TriangleID numNeighbours;
} TriangleNeighbours;

typedef struct {
    Index3D id;
    vec3 newPosition;
} VertexTranslate;

typedef struct {
    Edge points;
    vec3 apex;
} Funnel;

typedef struct {
    TriangleID triId;
    Edge edge;
} Neighbour;

typedef union {
    uint64_t integer;
    Edge edge;
} uniEdge;

//we assume that indices are 32 bit integers   \/
KHASHL_MAP_INIT(
    KH_LOCAL, CameFromMap, CameFromMap, TriangleID, Neighbour, kh_hash_uint32, kh_eq_generic
)
KHASHL_MAP_INIT(KH_LOCAL, CostMap, CostMap, TriangleID, float, kh_hash_uint32, kh_eq_generic)
KHASHL_MAP_INIT(KH_LOCAL, EdgeMap, EdgeMap, uint64_t, TriangleID, kh_hash_uint64, kh_eq_generic)
KHASHL_MAP_INIT(
    KH_LOCAL, VertVertMap, VertVertMap, Index3D, Index3D, kh_hash_uint32, kh_eq_generic
)

DECLARE_ARRAY_TYPEDEF(Funnel)
DECLARE_ARRAY_IMPL(Funnel)

DECLARE_HEAPQUEUE_TYPEDEF(TriangleID)
DECLARE_HEAPQUEUE_IMPL(TriangleID)

static Geometry geometry;

static TriangleNeighbours* neighbours; //private
static vec3* centers;

//returns 0 if cdir is between dir0 and dir1
static int isOutsideDirs(vec3 dir0, vec3 dir1, vec3 cdir) {
    vec3 cross0, cross1;

    glm_vec3_cross(dir1, cdir, cross0);
    glm_vec3_cross(dir1, dir0, cross1);

    if (glm_vec3_dot(cross0, cross1) < 0) return DIR1;

    glm_vec3_cross(dir0, cdir, cross0);
    glm_vec3_cross(dir0, dir1, cross1);

    return glm_vec3_dot(cross0, cross1) < 0 ? DIR0 : 0;
}
//returns an id of a neighbour triangle and if there is no one returns 0xffffffff
static TriangleID processEdge(EdgeMap* const map, const TriangleID triangleId, uniEdge edge) {
    int absent;

    if (edge.edge[0] > edge.edge[1]) SWAP(Index3D, edge.edge[0], edge.edge[1]);

    const khint_t id = EdgeMap_put(map, edge.integer, &absent);

    if (absent) {
	kh_val(map, id) = triangleId;

	return MAXT(TriangleID);
    }

    const TriangleID neighbour = kh_val(map, id);

    EdgeMap_del(map, id);

    return neighbour;
}
static float dot2(vec3 v) {
    return glm_vec3_dot(v, v);
}
static float keepNorm(const float a) {
    return fmaxf(fminf(a, 1), 0);
}
static void handleEdge(
    EdgeMap* const edges, VertVertMap* const innerVertices, 
    const TriangleID triangleId, Edge edge
) {
    const TriangleID neighbourId = processEdge(edges, triangleId, (uniEdge){.edge = {edge[0], edge[1]}});

    if (!ISINVALID(neighbourId)) {
	TriangleNeighbours* const cur = neighbours + triangleId;
	TriangleNeighbours* const bour = neighbours + neighbourId;

	khint_t iter;
	int absent;

	if (edge[0] > edge[1]) {
	    cur->edges[cur->numNeighbours][0] = bour->edges[bour->numNeighbours][0] = edge[1];
	    cur->edges[cur->numNeighbours][1] = bour->edges[bour->numNeighbours][1] = edge[0];
	}
	else {
	    cur->edges[cur->numNeighbours][0] = bour->edges[bour->numNeighbours][0] = edge[0];
	    cur->edges[cur->numNeighbours][1] = bour->edges[bour->numNeighbours][1] = edge[1];
	}

	cur->triangles[cur->numNeighbours++] = neighbourId;
	bour->triangles[bour->numNeighbours++] = triangleId;

	iter = VertVertMap_put(innerVertices, edge[0], &absent);
	kh_val(innerVertices, iter) = edge[1];

	iter = VertVertMap_put(innerVertices, edge[1], &absent);
	kh_val(innerVertices, iter) = edge[0];
    }
}
static void lookDir(vec3 p0, vec3 p1, vec3 dest) {
    glm_vec3_sub(p1, p0, dest);
    glm_vec3_normalize(dest);
}
static void getFunnelDirs(Funnel* const funnel, vec3 dir0, vec3 dir1, const bool f) {
    lookDir(funnel->apex, geometry.vertices[funnel->points[f]], dir0);
    lookDir(funnel->apex, geometry.vertices[funnel->points[!f]], dir1);
}
static void pullString(vec3 point, Funnel* const funnel, vec3Array* const dest) {
    vec3 dir0, dir1, cdir;

    vec3* const f0 = geometry.vertices + funnel->points[0];
    vec3* const f1 = geometry.vertices + funnel->points[1];

    getFunnelDirs(funnel, dir0, dir1, 0);

    lookDir(funnel->apex, point, cdir);

    if (glm_vec3_eqv(dir0, dir1)) {
	if (glm_vec3_distance2(*f0, funnel->apex) < glm_vec3_distance2(*f1, funnel->apex)) {
	    vec3Array_InsertElement(dest, f0, 0);
	}
	else vec3Array_InsertElement(dest, f1, 0);
    }
    else {
	switch (isOutsideDirs(dir0, dir1, cdir)) {
	case DIR0:
	    vec3Array_InsertElement(dest, f0, 0);
	    break;
	case DIR1:
	    vec3Array_InsertElement(dest, f1, 0);
	}
    }
}
static Index3D narrowFunnel(
    const Edge portal, FunnelArray* const funnels,
    const bool f, const bool p, vec3Array* const dest
) {
    vec3 dir0, dir1, newCross, oldCross;

    Funnel* const lastFunnel = FunnelArray_GetLastElement(funnels);

    lookDir(lastFunnel->apex, geometry.vertices[portal[p]], dir0);
    lookDir(lastFunnel->apex, geometry.vertices[portal[!p]], dir1);

    glm_vec3_cross(dir0, dir1, newCross);

    const float newDot = glm_vec3_dot(dir0, dir1);

    getFunnelDirs(lastFunnel, dir0, dir1, f);

    glm_vec3_cross(dir0, dir1, oldCross);

    if (glm_vec3_dot(oldCross, newCross) < 0) {
	for (ArrayIndex i = 0; i < funnels->numElements - 1; i++) {
	    pullString(geometry.vertices[portal[p]], funnels->elements + i, dest);
	}

	const Index3D index = portal[!p];

	//our funnel twisted
	memcpy(funnels->elements[0].points, portal, sizeof(Edge));

	glm_vec3_copy(geometry.vertices[index], funnels->elements[0].apex);

	vec3Array_InsertElement(dest, geometry.vertices + index, 0);

	funnels->numElements = 1;

	return index;
    }

    if (newDot > glm_vec3_dot(dir0, dir1)) lastFunnel->points[f] = portal[p];
    else {
	Funnel funnel = {.points = {portal[p], portal[!p]}};

	glm_vec3_copy(geometry.vertices[lastFunnel->points[f]], funnel.apex);

	//SCARY, lastFunnel is a dangling pointer now
	FunnelArray_AppendElement(funnels, &funnel);
    }

    return MAXT(Index3D);
}
static void moveVertex(
    VertVertMap* const openVertices, 
    const VertVertMap* const innerVertices, VertexTranslate* const translates,
    size_t* const numTranslates, const Index3D main, const Index3D second
) {
    khint_t ovIter;
    int absent;

    ovIter = VertVertMap_put(openVertices, main, &absent);

    if (absent) kh_val(openVertices, ovIter) = second;
    else if (!ISINVALID(kh_val(openVertices, ovIter))) {
	const float shrinkDistance = .3f;

	vec3 dir0, dir1, shift;

	float* const apex = geometry.vertices[main];

	Funnel funnel;

	funnel.points[0] = second;
	funnel.points[1] = kh_val(openVertices, ovIter);

	INVALIDATE(kh_val(openVertices, ovIter));

	glm_vec3_copy(apex, funnel.apex);

	getFunnelDirs(&funnel, dir0, dir1, 0);

	glm_vec3_center(dir0, dir1, shift);

	const float shiftDistance = hypotf(
	    shrinkDistance, shrinkDistance / tanf(glm_vec3_angle(dir0, dir1) / 2)
	);

	//if this vertex is bound to only one triangle, we don't need to check our
	//interpolated dir because of the fact that every triangle is a convex polygon
	if ((ovIter = VertVertMap_get(innerVertices, main)) != kh_end(innerVertices)) {
	    vec3 cdir;

	    float* const inner = geometry.vertices[kh_val(innerVertices, ovIter)];

	    glm_vec3_sub(inner, apex, cdir);
	    
	    if (glm_vec3_eq(shift, 0)) {
		//glm_vec3_normalize(cdir);

		glm_vec3_cross(dir0, cdir, dir1);
		glm_vec3_cross(dir1, dir0, shift);
	    }
	    else if (isOutsideDirs(dir0, dir1, cdir)) glm_vec3_negate(shift);
	}

	glm_vec3_scale_as(shift, shiftDistance, shift);
	glm_vec3_add(apex, shift, translates[*numTranslates].newPosition);

	translates[(*numTranslates)++].id = main;
    }
}
static void shrinkGeometry(EdgeMap* const edges, VertVertMap* const innerVertices) {
    size_t numTranslates;
    khint_t iter;

    VertexTranslate translates[kh_size(edges)];

    VertVertMap* const openVertices = VertVertMap_init();

    numTranslates = 0;

    //all the remaining edges don't have any neighbours so they are open
    kh_foreach(edges, iter) {
	union Convert {
	    Edge edge;
	    uint64_t side;
	} conv;

	conv.side = kh_key(edges, iter);

	moveVertex(openVertices, innerVertices, translates, &numTranslates, conv.edge[0], conv.edge[1]);
	moveVertex(openVertices, innerVertices, translates, &numTranslates, conv.edge[1], conv.edge[0]);
    }

    foreach (VertexTranslate* const trans, translates, numTranslates)
	glm_vec3_copy(trans->newPosition, geometry.vertices[trans->id]);
    forend

    VertVertMap_destroy(openVertices);
}
static void reconstructPath(
    const CameFromMap* const cameFrom, const TriangleID fromTri,
    TriangleID current, vec3 from, vec3 to, vec3Array* const dest
) {
    FunnelArray funnels;

    Index3D apexId;

    INVALIDATE(apexId);

    dest->numElements = 1;

    FunnelArray_Init(&funnels);

    glm_vec3_copy(to, dest->elements[0]);
    glm_vec3_copy(to, funnels.elements[0].apex);

    for (size_t i = 0;; i++) {
	//pointer may be unaligned
	const Neighbour val = kh_val(cameFrom, CameFromMap_get(cameFrom, current));

	if (i && (ISINVALID(apexId) || (apexId != val.edge[0] && apexId != val.edge[1]))) {
	    const Index3D* const points = FunnelArray_GetLastElement(&funnels)->points;

	    if (points[0] == val.edge[0]) {
		apexId = narrowFunnel(val.edge, &funnels, 1, 1, dest);
	    }
	    else if (points[0] == val.edge[1]) {
		apexId = narrowFunnel(val.edge, &funnels, 1, 0, dest);
	    }
	    else if (points[1] == val.edge[0]) {
		apexId = narrowFunnel(val.edge, &funnels, 0, 1, dest);
	    }
	    else if (points[1] == val.edge[1]) {
		apexId = narrowFunnel(val.edge, &funnels, 0, 0, dest);
	    }
	}
	else {
	    memcpy(funnels.elements[0].points, val.edge, sizeof(Edge));

	    funnels.numElements = 1;
	}

	if ((current = val.triId) == fromTri) {
	    aforeach (Funnel* const funnel, &funnels) pullString(from, funnel, dest); forend

	    break;
	}
    }

    FunnelArray_Destroy(&funnels);
}
static void processNeighbours(
    TriangleIDHeapQueue* const open, CameFromMap* const cameFrom, 
    CostMap* const costs, const TriangleID current, vec3 to, const bool escapeMode
) {
    const TriangleNeighbours* const nrs = neighbours + current;

    for (TriangleID i = 0; i < nrs->numNeighbours; i++) {
	const TriangleID* const neighbour = nrs->triangles + i;

	const float graphCost = glm_vec3_distance2(centers[current], centers[*neighbour]);

	const float newCost = kh_val(costs, CostMap_get(costs, *neighbour)) + graphCost;

	const khint_t neighIter = CameFromMap_get(cameFrom, *neighbour);

	const float safeDistance2 = 4;

	if (safeDistance2 && escapeMode) {}

	//if (!escapeMode || glm_vec3_distance2(centers[*neighbour], to) > safeDistance2) {
	    if (neighIter == kh_end(cameFrom) || newCost < kh_val(costs, neighIter)) {
		int absent;

		khint_t iter = CameFromMap_put(cameFrom, *neighbour, &absent);

		kh_val(cameFrom, iter).triId = current;
		memcpy(kh_val(cameFrom, iter).edge, nrs->edges[i], sizeof(Edge));

		const float prio = newCost + glm_vec3_distance2(to, centers[*neighbour]);

		TriangleIDHeapQueue_InsertElement(open, prio, neighbour);

		iter = CostMap_put(costs, *neighbour, &absent);
		kh_val(costs, iter) = newCost;
	    }
	//}
    }
}

void NH_Init() {
    EdgeMap* const edges = EdgeMap_init();
    VertVertMap* const innerVertices = VertVertMap_init();

    Geometry_Init(&geometry, "nav.gltf");

    mallocarr(centers, geometry.numTriangles);
    callocarr(neighbours, geometry.numTriangles);

    for (TriangleID i = 0; i < geometry.numTriangles; i++) {
	const Index3D i0 = geometry.triangles[i][0], i1 = geometry.triangles[i][1];
	const Index3D i2 = geometry.triangles[i][2];

	glm_vec3_add(geometry.vertices[i0], geometry.vertices[i1], centers[i]);
	glm_vec3_add(centers[i], geometry.vertices[i2], centers[i]);
	glm_vec3_divs(centers[i], 3, centers[i]);

	handleEdge(edges, innerVertices, i, (Edge){i0, i1});
	handleEdge(edges, innerVertices, i, (Edge){i1, i2});
	handleEdge(edges, innerVertices, i, (Edge){i0, i2});
    }

    shrinkGeometry(edges, innerVertices);
    
    EdgeMap_destroy(edges);
    VertVertMap_destroy(innerVertices);
}
bool NH_FindPath(
    vec3 from, vec3 to, const TriangleID fromTri, const TriangleID toTri, vec3Array* const dest
) {
    if (ISINVALID(fromTri)) return false;
    if (fromTri == toTri) {
	dest->numElements = 1;

	glm_vec3_copy(to, dest->elements[0]);

	return true;
    }

    TriangleIDHeapQueue open;
    int absent;

    CameFromMap* const cameFrom = CameFromMap_init();
    CostMap* const costs = CostMap_init();

    vec3 farthest = {to[0], to[1], to[2]};
    TriangleID farthestTri;

    INVALIDATE(farthestTri);

    //printfd("Searching for a path from: %u, to: %u\n", fromTri, toTri);

    TriangleIDHeapQueue_Init(&open);
    TriangleIDHeapQueue_InsertElement(&open, 0, &fromTri);

    CameFromMap_put(cameFrom, fromTri, &absent);

    khint_t iter = CostMap_put(costs, fromTri, &absent);
    kh_val(costs, iter) = 0;

    dest->numElements = 0;

    while (!TriangleIDHeapQueue_IsEmpty(&open)) {
	TriangleID current;

	TriangleIDHeapQueue_PopElement(&open, &current);

	if (toTri == ESCAPE_FROM_DANGER_MODE && glm_vec3_distance2(centers[current], to) < 4) continue;

	if (glm_vec3_distance2(centers[current], to) > glm_vec3_distance2(farthest, to)) {
	    glm_vec3_copy(centers[current], farthest);

	    farthestTri = current;
	}

	if (current == toTri) {
	    reconstructPath(cameFrom, fromTri, current, from, to, dest);
	    
	    break;
	}

	processNeighbours(&open, cameFrom, costs, current, to, toTri == ESCAPE_FROM_DANGER_MODE);
    }

    if (toTri == ESCAPE_FROM_DANGER_MODE && !ISINVALID(farthestTri)) {
	reconstructPath(cameFrom, fromTri, farthestTri, from, farthest, dest);
    }

    TriangleIDHeapQueue_Destroy(&open);
    CameFromMap_destroy(cameFrom);
    CostMap_destroy(costs);

    return dest->numElements;
}
TriangleID NH_GetClosestTriangle(vec3 p) {
    float minDistance;
    TriangleID triangleId;

    minDistance = INFINITY;

    INVALIDATE(triangleId);

    for (TriangleID i = 0; i < geometry.numTriangles; i++) {
	float* const v1 = Geometry_GetVertex(&geometry, i, 0);
	float* const v2 = Geometry_GetVertex(&geometry, i, 1);
	float* const v3 = Geometry_GetVertex(&geometry, i, 2);

	//source: iquilezles.org/articles/triangledistance

	vec3 v21, v32, v13, p1, p2, p3, nor, n21, n32, n13;

	glm_vec3_sub(v2, v1, v21);
	glm_vec3_sub(v3, v2, v32);
	glm_vec3_sub(v1, v3, v13);
	glm_vec3_sub(p, v1, p1);
	glm_vec3_sub(p, v2, p2);
	glm_vec3_sub(p, v3, p3);
	glm_vec3_cross(v21, v13, nor);
	
	glm_vec3_cross(v21, nor, n21);
	glm_vec3_cross(v32, nor, n32);
	glm_vec3_cross(v13, nor, n13);

	glm_vec3_scale(v21, keepNorm(glm_vec3_dot(v21, p1) / dot2(v21)), v21);
	glm_vec3_sub(v21, p1, v21);
	glm_vec3_scale(v32, keepNorm(glm_vec3_dot(v32, p2) / dot2(v32)), v32);
	glm_vec3_sub(v32, p2, v32);
	glm_vec3_scale(v13, keepNorm(glm_vec3_dot(v13, p3) / dot2(v13)), v13);
	glm_vec3_sub(v13, p3, v13);

	const float distance = sqrtf( // inside/outside test    
	    (
	    glm_signf(glm_vec3_dot(n21,p1)) + 
	    glm_signf(glm_vec3_dot(n32,p2)) + 
	    glm_signf(glm_vec3_dot(n13,p3)) < 2
	    ) ? // 3 edges    
	    fminf(fminf(dot2(v21), dot2(v32)), dot2(v13)) : // 1 face    
	    glm_vec3_dot(nor,p1) * glm_vec3_dot(nor,p1) / dot2(nor)
	);

	if (distance < minDistance) {
	    minDistance = distance;
	    triangleId = i;
	}
    }

    return triangleId;
}
TriangleID NH_GetRandomPoint(vec3 dest) {
    //limited by RAND_MAX
    float r0, r1;

    vec3 dir0, dir1;

    const TriangleID triId = rand() % geometry.numTriangles;

    float* const p0 = geometry.vertices[geometry.triangles[triId][0]];
    float* const p1 = geometry.vertices[geometry.triangles[triId][1]];
    float* const p2 = geometry.vertices[geometry.triangles[triId][2]];

    r0 = (float)rand() / RAND_MAX;
    r1 = (float)rand() / RAND_MAX;

    if (r0 + r1 > 1) {
	r0 = 1 - r0;
	r1 = 1 - r1;
    }

    glm_vec3_sub(p1, p0, dir0);
    glm_vec3_sub(p2, p0, dir1);

    glm_vec3_scale(dir0, r0, dir0);
    glm_vec3_scale(dir1, r1, dir1);

    glm_vec3_add(p0, dir0, dest);
    glm_vec3_add(dest, dir1, dest);

    return triId;
}
const Geometry* NH_GetGeometry() {
    return &geometry;
}

void NH_DrawDebugGui() {
    DGH_FIELD(&geometry);
    DGH_ARRAY(centers, geometry.numTriangles);
}
