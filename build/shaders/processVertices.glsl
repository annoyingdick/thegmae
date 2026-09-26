#define DECLARE_PROCESS_VERTICES_BUFFERS(x) \
layout (binding = $BUFFER_BINDING_VERTICES$, std430) readonly buffer verticesBuffer { \
    Vertex verts[]; \
}; \
layout (binding = $BUFFER_BINDING_INSTANCES_DATA$, std430) readonly buffer instancesDataBuffer { \
    x instancesData[]; \
}; \
layout (binding = $BUFFER_BINDING_USED_INSTANCES_IDS$, std430) readonly buffer usedInstancesIdsBuffer { \
    uint usedInstancesIds[]; \
}
