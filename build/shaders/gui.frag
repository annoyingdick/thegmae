#version 460 core

#extension GL_ARB_bindless_texture : require

layout (binding = 1, std430) readonly buffer textureHandlesBuffer {
    sampler2D textures[];
};

in vec2 texcoord;

out vec4 finalColor;

float median(vec3 rgb) {
    return max(min(rgb.r, rgb.g), min(max(rgb.r, rgb.g), rgb.b));
}
vec4 paint(vec4 dst, const float threshold, const float dist, const float inv, vec4 src) {
    const float opacity = clamp(src.a * clamp((threshold - dist) * inv + 0.5, 0, 1) - dst.a, 0, 1);

    return opacity * src + (1.0 - opacity) * dst;
}

void main() {
    const vec2 normTexCoord = vec2(texcoord.x - floor(texcoord.x), texcoord.y);
    
    //hardcoded
    const float fontSize = 100;

    const float tex = 1 - median(texture(textures[uint(texcoord.x)], normTexCoord).rgb);

    const float atlasSize = textureSize(textures[uint(texcoord.x)], 0).s;

    const float inv = 6.569366455078125 * fontSize / atlasSize;

    const float thres = 0.3;

    finalColor = vec4(0);
    finalColor = paint(finalColor, thres, tex, inv, vec4(1));
    finalColor = paint(finalColor, thres + 0.5, tex, inv, vec4(vec3(0), 1));
}
