#version 460 core

#extension GL_ARB_shader_draw_parameters : require

struct Vertex {
    float position[2];
    float texcoord[2];
};

layout (binding = $BUFFER_BINDING_VERTICES$, std430) readonly buffer verticesBuffer {
    Vertex verts[];
};

out vec2 texcoord;
flat out uint meshId;

void main() {
    const vec4 pos = vec4(verts[gl_VertexID].position[0], verts[gl_VertexID].position[1], 0, 1.0);

    meshId = gl_DrawID;

    texcoord = vec2(verts[gl_VertexID].texcoord[0], verts[gl_VertexID].texcoord[1]);
    gl_Position = pos;
}
