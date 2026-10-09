#version 460 core

struct Vertex {
    float position[3], direction[3];
    float timestamp, tex;
};

layout (binding = 0, std430) readonly buffer verticesBuffer {
    Vertex verts[];
};

void main() {
    gl_Position = vec4(verts[gl_VertexID].position[0], verts[gl_VertexID].position[1], verts[gl_VertexID].position[2], 1.0);
}
