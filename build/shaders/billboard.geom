#version 460 core

layout (points) in;
layout (triangle_strip, max_vertices = 8) out;

struct Vertex {
    float position[3], direction[3];
    float timestamp, tex;
};

layout (binding = 0, std430) readonly buffer verticesBuffer {
    Vertex verts[];
};

#include "uniforms.glsl"

uniform float currentTime;

out vec2 texcoord;

void makeMuzzle(const float face, const vec3 cam, const vec3 direction) {
    const Vertex vert = verts[gl_PrimitiveIDIn];

    const float progress = mod(currentTime - vert.timestamp, 1) * 3.1415, size = progress * progress * sin(progress) / 4;

    vec4 rl = vec4(cross(direction, mix(cam, vec3(0, 1, 0), face)), 0);

    //maagic
    const vec4 up = vec4(mix(direction * progress * (size + 1) / 4, cross(direction, rl.xyz) * size, face), 0);

    rl *= size;

    vec4 pos = gl_in[0].gl_Position - rl / 2 - mix(vec4(0), up / 2, face);

    texcoord = vec2(vert.tex + face, 0);
    gl_Position = pvMat * pos;
    EmitVertex();

    texcoord.t = 1;
    gl_Position = pvMat * (pos + rl);
    EmitVertex();

    pos += up;

    texcoord.t = 0;
    texcoord.s += 0.99; //the fucking floating point errors
    gl_Position = pvMat * pos;
    EmitVertex();

    texcoord.t = 1;
    //texcoord.s += 1;
    gl_Position = pvMat * (pos + rl);
    EmitVertex();

    EndPrimitive();
}

void main() {
    const Vertex vert = verts[gl_PrimitiveIDIn];

    const vec3 direction = vec3(vert.direction[0], vert.direction[1], vert.direction[2]);
    const vec3 cam = normalize(gl_in[0].gl_Position.xyz - viewMat[3].xyz);

    const float face = step(0, dot(cam, direction));

    makeMuzzle(1 - face, cam, direction);
    makeMuzzle(face, cam, direction);
}
