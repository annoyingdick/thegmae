#version 460 core

#include "commands.glsl"
#include "processVertices.glsl"

struct Vertex {
    float position[3];
    float texcoord[2];
};
struct InstanceData {
    mat4 inMat;
    mat4 outMat;
};

DECLARE_COMMANDS(readonly);
DECLARE_PROCESS_VERTICES_BUFFERS(InstanceData);

out float height;
out vec2 texcoord;

void main() {
    const vec4 pos = vec4(verts[gl_VertexID].position[0], verts[gl_VertexID].position[1], verts[gl_VertexID].position[2], 1.0);
    const uint instanceId = usedInstancesIds[commands[gl_DrawID].baseInstance + gl_InstanceID];

    texcoord = vec2(verts[gl_VertexID].texcoord[0], verts[gl_VertexID].texcoord[1]);
    gl_Position = instancesData[instanceId].outMat * pos;
}
