#ifndef Region_h_
#define Region_h_

#include <stdint.h>

typedef uint32_t RegionPosition;
typedef uint32_t RegionSize;

typedef struct {
    RegionPosition position;
    RegionSize size;
} Region;

RegionPosition Region_GetEnd(const Region* region);

void Region_DrawDebugGui(const Region* region, const char name[]);

#endif
