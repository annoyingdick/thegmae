#version 460 core

#extension GL_ARB_bindless_texture : require

layout (binding = 1, std430) readonly buffer textureHandlesBuffer {
    sampler2D textures[];
};

in vec2 texcoord;

out vec3 finalcolor;

void main() {
    const vec2 normTexCoord = vec2(texcoord.x - floor(texcoord.x), texcoord.y);

    if (texcoord.x == 0 && texcoord.y == 0) finalcolor = vec3(1);
    else finalcolor = texture(textures[uint(texcoord.x)], normTexCoord).rgb;
}
