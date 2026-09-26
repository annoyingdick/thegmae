#include "commands.glsl"
#include "instancesMeshIds.glsl"

#define DECLARE_PROCESS_INSTANCES_BUFFERS(x) \
DECLARE_COMMANDS(); \
DECLARE_IMI(readonly); \
layout (binding = $BUFFER_BINDING_INSTANCES_DATA$, std430) buffer instancesDataBuffer { \
    x instancesData[]; \
}; \
layout (binding = $BUFFER_BINDING_USED_INSTANCES_IDS$, std430) writeonly buffer usedInstancesIdsBuffer { \
    uint usedInstancesIds[]; \
}; \
layout (binding = 0) uniform uniforms { \
    mat4 pvMat; \
}
