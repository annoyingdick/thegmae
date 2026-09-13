#version 460 core

struct Vertex {
    float position[3];
    float texcoord[2];
};
struct DrawElementsIndirectCommand {
    uint count;
    uint instanceCount;
    uint firstIndex;
    int baseVertex;
    uint baseInstance;
};
struct InstanceData {
    mat4 inMat;
    mat4 outMat;
};

layout (binding = 0, std430) readonly buffer verticesBuffer {
    Vertex verts[];
};
layout (binding = 2, std430) readonly buffer commandsBuffer {
    DrawElementsIndirectCommand commands[];
};
layout (binding = 3, std430) readonly buffer instancesDataBuffer {
    InstanceData instancesData[];
};
layout (binding = 4, std430) readonly buffer usedInstancesIdsBuffer {
    uint usedInstancesIds[];
};

out float height;
out vec2 texcoord;

void main() {
    const vec4 pos = vec4(verts[gl_VertexID].position[0], verts[gl_VertexID].position[1], verts[gl_VertexID].position[2], 1.0);
    const uint instanceId = usedInstancesIds[commands[gl_DrawID].baseInstance + gl_InstanceID];

    texcoord = vec2(verts[gl_VertexID].texcoord[0], verts[gl_VertexID].texcoord[1]);
    gl_Position = instancesData[instanceId].outMat * pos;
}
