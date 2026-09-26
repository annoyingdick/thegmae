#version 460 core

#include "processVertices.glsl"

struct Vertex {
    float position[2];
    float texcoord[2];
};

DECLARE_PROCESS_VERTICES_BUFFERS(mat4);

out vec2 texcoord;

void main() {
    const vec4 pos = vec4(verts[gl_VertexID].position[0], verts[gl_VertexID].position[1], 0, 1.0);

    texcoord = vec2(verts[gl_VertexID].texcoord[0], verts[gl_VertexID].texcoord[1]);
    gl_Position = pos;
}
