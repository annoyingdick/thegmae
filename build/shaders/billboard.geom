#version 460 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

struct Vertex {
    float position[3], direction[3];
    float timestamp;
};

layout (binding = 0, std430) readonly buffer verticesBuffer {
    Vertex verts[];
};

#include "uniforms.glsl"

out vec2 texcoord;

vec3 todirection(const Vertex vert) {
    return vec3(vert.direction[0], vert.direction[1], vert.direction[2]);
}

void main() {
    const vec4 right = vec4(normalize(cross(todirection(verts[gl_PrimitiveIDIn]), viewMat[2].xyz)), 0);

    const vec4 pos = gl_in[0].gl_Position - right / 2;

    texcoord = vec2(0);

    gl_Position = pvMat * pos;
    EmitVertex();

    gl_Position = pvMat * (pos + right);
    EmitVertex();

    gl_Position = pvMat * (pos + right / 2 + vec4(todirection(verts[gl_PrimitiveIDIn]), 0));
    EmitVertex();

    EndPrimitive();
}
