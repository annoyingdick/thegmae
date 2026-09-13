#version 460 core

struct Vertex {
    float position[3];
};

layout (binding = 0, std430) readonly buffer verticesBuffer {
    Vertex verts[];
};

layout (binding = 0) uniform uniforms {
    mat4 pvMat;
};

void main() {
    const vec4 pos = vec4(verts[gl_VertexID].position[0], verts[gl_VertexID].position[1], verts[gl_VertexID].position[2], 1.0);

    gl_Position = pvMat * pos;
}
