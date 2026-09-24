#version 460 core

struct Vertex {
    float position[2];
    float texcoord[2];
};

layout (binding = 0, std430) readonly buffer verticesBuffer {
    Vertex verts[];
};
layout (binding = 3, std430) readonly buffer instancesDataBuffer {
    mat4 instancesData[];
};
layout (binding = 4, std430) readonly buffer usedInstancesIdsBuffer {
    uint usedInstancesIds[];
};

out vec2 texcoord;

void main() {
    const vec4 pos = vec4(verts[gl_VertexID].position[0], verts[gl_VertexID].position[1], 0, 1.0);

    texcoord = vec2(verts[gl_VertexID].texcoord[0], verts[gl_VertexID].texcoord[1]);
    gl_Position = pos;
}
