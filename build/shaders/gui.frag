#version 460 core

#extension GL_ARB_bindless_texture : require

#include "instancesMeshIds.glsl"

DECLARE_IMI(readonly);

layout (binding = $BUFFER_BINDING_TEXTURE_HANDLES$, std430) readonly buffer textureHandlesBuffer {
    sampler2D textures[];
};
layout (binding = $BUFFER_BINDING_INSTANCES_DATA$, std430) readonly buffer instancesDataBuffer {
    float instancesData[];
};

in vec2 texcoord;
flat in uint meshId;

out vec4 finalColor;

float median(const vec3 rgb) {
    return max(min(rgb.r, rgb.g), min(max(rgb.r, rgb.g), rgb.b));
}
vec4 paint(const vec4 dst, const float threshold, const float dist, const float inv, const vec4 src) {
    const float opacity = clamp(src.a * clamp((threshold - dist) * inv + 0.5, 0, 1) - dst.a, 0, 1);

    return opacity * src + (1.0 - opacity) * dst;
}

void main() {
    const vec2 normTexCoord = vec2(fract(texcoord.x), texcoord.y), atlasSize = textureSize(textures[uint(texcoord.x)], 0);
    const vec2 gradient = fwidth(normTexCoord), product = atlasSize * gradient;
    
    //hardcoded
    const float tex = 1 - median(texture(textures[uint(texcoord.x)], normTexCoord).rgb);

    const float inv = 2 * max(dot(atlasSize, gradient) / (product.s * product.t) / 2, 1);

    const float thres = 0.3;

    finalColor = vec4(0);
    finalColor = paint(finalColor, thres, tex, inv, vec4(1));
    finalColor = paint(finalColor, thres + instancesData[instancesMeshIds[meshId]], tex, inv, vec4(vec3(0), 1));
}
