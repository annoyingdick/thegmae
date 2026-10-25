#version 460 core

#include "guibuffers.glsl"

#extension GL_ARB_bindless_texture : require

DECLARE_GPUBUFFERS(float);

uniform float currentTime;

in vec2 texcoord;
flat in uint meshId;

out vec4 finalcolor;

void main() {
    const vec2 normTexCoord = vec2(fract(texcoord.x), texcoord.y);

    finalcolor = vec4(vec3(0), texture(textures[uint(texcoord.x)], normTexCoord) * 3 / 4 * instancesData[meshId]);
    //texture(textures[textureHandle], texcoord)
}
