layout (binding = $BUFFER_BINDING_TEXTURE_HANDLES$, std430) readonly buffer textureHandlesBuffer {
    sampler2D textures[];
};
layout (binding = $BUFFER_BINDING_INSTANCES_DATA$, std430) readonly buffer instancesDataBuffer {
    float instancesData[];
};
