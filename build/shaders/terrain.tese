#version 460 core

layout (quads, fractional_odd_spacing, ccw) in;

layout (binding = 0) uniform uniforms {
    mat4 pvMat;
};

out float height;

void main() {
    const float u = gl_TessCoord.x;
    const float v = gl_TessCoord.y;

    const vec4 p00 = gl_in[0].gl_Position;
    const vec4 p01 = gl_in[1].gl_Position;
    const vec4 p10 = gl_in[2].gl_Position;
    const vec4 p11 = gl_in[3].gl_Position;

    const vec4 p0 = (p01 - p00) * u + p00;
    const vec4 p1 = (p11 - p10) * u + p10;
    const vec4 p = (p1 - p0) * v + p0;

    height = p.y;

    gl_Position = pvMat * p;
}
