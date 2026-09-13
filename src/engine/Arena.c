#define REDEF_PRINTFD
#include "Arena.h"

DECLARE_ARRAY_IMPL(Region)

static bool isRegionTouchingEnd(const Arena* const arena, const Region* const region) {
    return Region_GetEnd(region) == arena->size;
}
//Returns a bool that tells whether this region is touching the end of the arena. 
//outRegion.size will be zero if there is no space
static bool fitInRegion(
    Arena* const arena, Region* const outRegion, 
    const ArrayIndex id, const RegionSize reqSize
) {
    Region* const fr = arena->freeRegions.elements + id;

    //can we fit in here?
    if (fr->size >= reqSize) {
	const bool isTouchingEnd = isRegionTouchingEnd(arena, fr);

	outRegion->size = reqSize;
	outRegion->position = fr->position;

	printfd("off=%u : siz=%u\n", fr->position, reqSize);

	if (fr->size > reqSize) {
	    printfd("Found a free region bigger than requested. Requested size: %u : Total size: %u\n", reqSize, fr->size);

	    fr->position += reqSize;
	    fr->size -= reqSize;
	}
	else {
	    printfd("Found a free region with exact size: %u\n", reqSize); 

	    RegionArray_RemoveElement(&arena->freeRegions, id);
	}

	return isTouchingEnd;
    }

    //too big
    return false;
}
static RegionSize getNewArenaSize(const Arena* const arena, const RegionSize reqSize) {
    const Region* const lastFreeRegion = RegionArray_GetLastElement(&arena->freeRegions);
    const bool isTouchingEnd = !RegionArray_IsEmpty(&arena->freeRegions) && isRegionTouchingEnd(arena, lastFreeRegion);

    RegionSize estSize, freeSize;

    estSize = arena->size;

    do {
	estSize += estSize / 2; 
	freeSize = estSize - arena->size + (isTouchingEnd ? lastFreeRegion->size : 0);
    } while (freeSize < reqSize);

    return estSize;
}

void Arena_Init(Arena* const arena, const RegionSize initialSize) {
    RegionArray_Init(&arena->freeRegions);
    RegionArray_AppendElement(&arena->freeRegions, &(Region){.position = 0, .size = initialSize});

    arena->size = initialSize;
}
bool Arena_RequestRegion(
    Arena* const arena, Region* const outRegion, RegionSize* const outNewArenaSize, const RegionSize reqSize
) {
    printfd("Requested a region with size of %u\n", reqSize);

    outRegion->size = *outNewArenaSize = 0;

    for (ArrayIndex i = 0; i < arena->freeRegions.numElements; i++) {
	const bool isTouchingEnd = fitInRegion(arena, outRegion, i, reqSize);

	if (outRegion->size) return isTouchingEnd;
    }
    
    //no space was found

    *outNewArenaSize = getNewArenaSize(arena, reqSize);

    Region* const lastFreeRegion = RegionArray_GetLastElement(&arena->freeRegions);

    const RegionSize sizeDiff = *outNewArenaSize - arena->size;

    if (!RegionArray_IsEmpty(&arena->freeRegions) && isRegionTouchingEnd(arena, lastFreeRegion)) {
	lastFreeRegion->size += sizeDiff;
    }
    else RegionArray_AppendElement(&arena->freeRegions, &(Region){.position = arena->size, .size = sizeDiff});

    arena->size = *outNewArenaSize;

    printfd("Expanding array: new size=%u\n", *outNewArenaSize);

    fitInRegion(arena, outRegion, arena->freeRegions.numElements - 1, reqSize);

    return true; //we've expanded the array, so the chosen region will always be last
}
RegionSize Arena_ReturnRegion(Arena* const arena, const Region* const region) {
#ifdef DEBUG
    printfd("Returning region: off=%u : siz=%u\n", region->position, region->size);

    if (!region->position && !region->size) {
	throwFatal("Arena error occurred!", "Some code area tried to return a zero region");
    }
#endif

    for (ArrayIndex i = 0; i < arena->freeRegions.numElements; i++) {
	Region* const fr = arena->freeRegions.elements + i;

	if (Region_GetEnd(fr) == region->position) {
	    //we are at the end of this free region
	    //merge if possible
	    RegionSize ret;

	    if (i == arena->freeRegions.numElements - 1) {
		ret = isRegionTouchingEnd(arena, region) ? fr->size + region->size : 0;
	    }
	    else if (Region_GetEnd(region) == fr[1].position && isRegionTouchingEnd(arena, fr + 1)) {
		ret = fr->size + region->size;
	    }
	    else ret = 0;

	    if (fr[1].position == fr->position + (fr->size += region->size)) {
		fr->size += fr[1].size;

		RegionArray_RemoveElement(&arena->freeRegions, i + 1);
	    }

	    printfd("Index sub: %u\n", ret);
	    return ret;
	}
	if (fr->position == region->position + region->size) {
	    //we are at the beginning of this free region
	    fr->position -= region->size;
	    fr->size += region->size;

	    //we cannot merge here, because the previous region would've done it for us

	    printfd("Index sub: %u\n", i == arena->freeRegions.numElements - 1);

	    return isRegionTouchingEnd(arena, fr) ? region->size : 0;
	}
    }

    //we couldn't merge with other free regions, so create one
    bool insertionEvaded;

    insertionEvaded = true;

    for (ArrayIndex i = 0; i < arena->freeRegions.numElements; i++) {
	if (region->position > arena->freeRegions.elements[i].position) {
	    RegionArray_InsertElement(&arena->freeRegions, region, i + 1);

	    insertionEvaded = false;

	    break;
	}
    }

    if (insertionEvaded) RegionArray_InsertElement(&arena->freeRegions, region, 0);

    //printfd("Return region... islast=%u\n", isLocationOpenAndLast(fla, location)) ? location->size : 0;

    return isRegionTouchingEnd(arena, region) ? region->size : 0;
}
void Arena_Destroy(const Arena* const arena) {
    RegionArray_Destroy(&arena->freeRegions);
}
