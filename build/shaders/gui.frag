#version 460 core

#extension GL_ARB_bindless_texture : require

layout (binding = 1, std430) readonly buffer textureHandlesBuffer {
    sampler2D textures[];
};

in vec2 texcoord;

out vec4 finalcolor;

void main() {
    const vec2 normTexCoord = vec2(texcoord.x - floor(texcoord.x), texcoord.y);

    finalcolor = vec4(
	vec3(texture(textures[uint(texcoord.x)], normTexCoord).g), texture(textures[uint(texcoord.x)], normTexCoord).r
    );
}
