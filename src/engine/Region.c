#include "Region.h"

RegionPosition Region_GetEnd(const Region* const region) {
    return region->position + region->size;
}
