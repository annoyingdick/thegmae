#version 460 core

#extension GL_ARB_bindless_texture : require

layout (binding = $BUFFER_BINDING_TEXTURE_HANDLES$, std430) readonly buffer textureHandlesBuffer {
    sampler2D textures[];
};

uniform uint textureHandle;

in vec2 texcoord;

out vec4 finalcolor;

void main() {
    finalcolor = vec4(vec3(0), texture(textures[textureHandle], texcoord) * 3 / 4);
}
