#version 460 core

#include "guibuffers.glsl"

#extension GL_ARB_bindless_texture : require

// R - opacity, G - outline, B - inv factor calculated on cpu, A - unused
DECLARE_GPUBUFFERS(vec4);

in vec2 texcoord;
flat in uint meshId;

out vec4 finalColor;

float median(const vec3 rgb) {
    return max(min(rgb.r, rgb.g), min(max(rgb.r, rgb.g), rgb.b));
}
vec4 paint(const vec4 dst, const float threshold, const float dist, const float inv, const vec4 src) {
    const float opacity = clamp((threshold - dist) * inv + 0.5, 0, 1) * instancesData[meshId].r;
    //const float opacity = clamp(src.a * clamp((threshold - dist) * inv + 0.5, 0, 1) - dst.a, 0, 1);

    return opacity * src + (1 - opacity) * dst;
}

void main() {
    const vec2 normTexCoord = vec2(fract(texcoord.x), texcoord.y);
    //const vec2 gradient = fwidth(normTexCoord), product = atlasSize * gradient;
    
    //hardcoded
    const float tex = 1 - median(texture(textures[uint(texcoord.x)], normTexCoord).rgb);

    //const float inv = 10 * max(dot(atlasSize, gradient) / (product.s * product.t) / 2, 1);

    const float thres = 0.9, wthres = thres - 0.4;
    finalColor = vec4(0);
    finalColor = paint(finalColor, mix(wthres, thres, instancesData[meshId].g), tex, instancesData[meshId].b, vec4(vec3(0), 1));
    finalColor = paint(finalColor, wthres, tex, instancesData[meshId].b, vec4(1));

    //finalColor.rgb = mix(sign(finalColor.rgb), finalColor.rgb, instancesData[meshId].g);

    //0.99999994
    gl_FragDepth = 1 - (finalColor.r + 0.0000002) * finalColor.a - 0.0000002;
}
