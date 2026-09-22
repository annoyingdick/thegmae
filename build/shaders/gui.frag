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
vec4 paint(vec4 dst, const float threshold, const float dist, const float width, vec4 src) {
    const float opacity = clamp(src.a * clamp((threshold - dist) / width + 0.5, 0.0, 1.0) - dst.a, 0.0, 1.0);

    return opacity * src + (1.0 - opacity) * dst;
}
float opacity(const float threshold, const float width, const float tex) {
    return clamp((threshold - (1 - tex)) / width + 0.5, 0, 1);
}

void main() {
    const vec2 normTexCoord = vec2(texcoord.x - floor(texcoord.x), texcoord.y);

    const float tex = 1 - median(texture(textures[uint(texcoord.x)], normTexCoord).rgb);

    const float width = 0.25, thres = 0.3;

    finalColor = vec4(0);
    finalColor = paint(finalColor, thres, tex, width, vec4(1));
    finalColor = paint(finalColor, thres + 0.5, tex, width, vec4(vec3(0), 1));
}
