#ifndef Arena_h_
#define Arena_h_

#include <stdbool.h>
#include "Region.h"
#include "Array.h"

//#define DEBUG_ARENA

#if defined printfd && defined REDEF_PRINTFD
#undef printfd
#ifdef DEBUG_ARENA
#define printfd(...) printf(__FILE_NAME__" : " __VA_ARGS__)
#else
#define printfd(...)
#endif
#endif

DECLARE_ARRAY_TYPEDEF(Region)

typedef struct {
    RegionArray freeRegions;

    RegionSize size;
} Arena;

void Arena_Init(Arena* arena, RegionSize initialSize);
//Returns boolean, that tells whether the returned region is last
//When expansion of a buffer is needed, a needed size is written to outNewArenaSize pointer.
bool Arena_RequestRegion(Arena* arena, Region* outRegion, RegionSize* outNewArenaSize, RegionSize reqSize);
//Returns a value that represents a shift of end pointer
RegionSize Arena_ReturnRegion(Arena* arena, const Region* region);
void Arena_Destroy(const Arena* arena);

#endif
