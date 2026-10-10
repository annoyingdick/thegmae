#version 460 core

#extension GL_ARB_bindless_texture : require

layout (binding = $BUFFER_BINDING_TEXTURE_HANDLES$, std430) readonly buffer textureHandlesBuffer {
    sampler2D textures[];
};

in vec2 texcoord;

out vec4 finalcolor;

void main() {
    const vec2 normTexCoord = vec2(fract(texcoord.x), texcoord.y);

    if (texcoord.x == 0 && texcoord.y == 0) finalcolor = vec4(1);
    else finalcolor = texture(textures[uint(texcoord.x)], normTexCoord);

    //if (finalcolor.a < 0.8) discard;
}
