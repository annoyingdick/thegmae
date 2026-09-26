#version 460 core

#include "commands.glsl"

#define MAX_WEIGHTS 4

struct Vertex {
    float position[3];
    float texcoord[2];
    float weights[MAX_WEIGHTS];
};
struct InstanceData {
    mat4 bones[$MAX_BONES$];
};

DECLARE_COMMANDS(readonly);

layout (binding = 0, std430) readonly buffer verticesBuffer {
    Vertex verts[];
};
layout (binding = 3, std430) readonly buffer instancesDataBuffer {
    InstanceData instancesData[];
};
layout (binding = 4, std430) readonly buffer usedInstancesIdsBuffer {
    uint usedInstancesIds[];
};

layout (binding = 0) uniform uniforms {
    mat4 pvMat;
};

out float height;
out vec2 texcoord;

void main() {
    const vec4 vpos = vec4(verts[gl_VertexID].position[0], verts[gl_VertexID].position[1], verts[gl_VertexID].position[2], 1.0);
    const uint instanceId = usedInstancesIds[commands[gl_DrawID].baseInstance + gl_InstanceID];

    vec4 pos = vec4(0);

    for (uint i = 0; i < MAX_WEIGHTS; i++) {
	const float weight = verts[gl_VertexID].weights[i];

	pos += (instancesData[instanceId].bones[uint(weight)] * vpos) * (weight - floor(weight));
    }

    texcoord = vec2(verts[gl_VertexID].texcoord[0], verts[gl_VertexID].texcoord[1]);
    gl_Position = pos;
}
