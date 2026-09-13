#include "DebugGuiHandler.h"
#include "Region.h"

RegionPosition Region_GetEnd(const Region* const region) {
    return region->position + region->size;
}

DGH_BEGIN(Region, region, 2) {
    DGH_FIELD(region->position);
    DGH_FIELD(region->size);
DGH_END }
