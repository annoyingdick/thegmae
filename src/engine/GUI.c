#include <cglm/types.h>
#include <cglm/vec2.h>
#include "PathHandler.h"
#include "Mesh.h"
#include "TexturesHandler.h"
#include "def.h"

//it's implementation is already inside nuklear.h so don't care
#include <stb_truetype.h>

#define FIRST_CHARACTER ' '
#define NUM_CHARACTERS (0x7F - FIRST_CHARACTER)

typedef Index3D QuadIndices[3 + 3];
typedef vec4 Quad[4]; //two floats for position and other two for tex. coords

typedef struct {
    // (xLeft, xRight, yBottom, yTop)
    float coords[4]; //we don't need this to be aligned
    vec2 texCoords[4];
    float advance;
} Glyph;

static Glyph characters[NUM_CHARACTERS];

static unsigned char* beginPacking(stbtt_pack_context* const context, const int size) {
    const int padding = 1;

    unsigned char* const textureData = mallocd(sizeof(*textureData) * size * size);

    stbtt_PackBegin(context, textureData, size, size, 0, padding, NULL);

    return textureData;
}
static void initText(const char* text) {
    const size_t textLength = strlen(text);

    Mesh mesh;

    vec2 position = GLM_VEC2_ZERO_INIT;

    Quad vertices[textLength];
    QuadIndices indices[textLength];

    for (size_t i = 0; i < textLength; i++) {
	const Index3D order[] = {0, 1, 2, 0, 2, 3};

	for (size_t j = 0; j < sizeof(order) / sizeof(*order); j++) indices[i][j] = order[j] + (i * 4);

	if (*text >= FIRST_CHARACTER && *text < FIRST_CHARACTER + NUM_CHARACTERS) {
	    Glyph* const glyph = characters + text[i] - FIRST_CHARACTER;

	    vertices[i][0][0] = vertices[i][3][0] = glyph->coords[1] + position[0];
	    vertices[i][1][0] = vertices[i][2][0] = glyph->coords[0] + position[0];

	    vertices[i][0][1] = vertices[i][1][1] = glyph->coords[3] + position[1];
	    vertices[i][2][1] = vertices[i][3][1] = glyph->coords[2] + position[1];

	    for (size_t j = 0; j < 4; j++) glm_vec2_copy(glyph->texCoords[j], vertices[i][j] + 2);
	    //for (size_t j = 0; j < 4; j++) glm_vec2_zero(vertices[i][j] + 2);

	    position[0] += glyph->advance;
	}
    }

    Mesh_InitWithData(&mesh, GRAPHICS_PIPELINE_GUI, (MeshInitWithDataInfo){
	.verticesSize = textLength * 4,
	.numIndices = textLength * sizeof(QuadIndices) / sizeof(**indices),
	.vertices = vertices[0][0],
	.indices = indices[0]
    });
    puts("asf");
    Mesh_NewInstance(&mesh);
}
static void initFont(const unsigned char fontData[const]) {
    stbtt_fontinfo info;

    if(!stbtt_InitFont(&info, fontData, 0)) throwFatal("stb_truetype error occurred!", "Failed to initialize the font");
}
static void packRange(stbtt_pack_context* const context, const unsigned char fontData[const], stbtt_packedchar chars[const]) {
    const float fontSize = 64;

    stbtt_PackFontRange(context, fontData, 0, fontSize, FIRST_CHARACTER, NUM_CHARACTERS, chars);
}
static void getQuad(const stbtt_packedchar chars[const], const int size, const int index, stbtt_aligned_quad* const quad) {
    float unusedX, unusedY;

    stbtt_GetPackedQuad(chars, size, size, index, &unusedX, &unusedY, quad, 0);
}

void GUI_Init() {
    const char path[] = "fonts\\Arimo-Medium.ttf";

    const int atlasSize = 512;

    stbtt_pack_context ctx;

    stbtt_packedchar packedChars[NUM_CHARACTERS];

    size_t dataSize;

    unsigned char* const fontData = PH_OpenFile(path, sizeof(path), &dataSize);

    initFont(fontData);

    unsigned char* const textureData = beginPacking(&ctx, atlasSize);

    packRange(&ctx, fontData, packedChars);

    stbtt_PackEnd(&ctx);

    const float texture = (float)TexturesHandler_LoadTextureR8(textureData, atlasSize, atlasSize, path);

    for (int i = 0; i < NUM_CHARACTERS; i++) {
	stbtt_aligned_quad quad;

	float windowX, windowY;

	WH_GetWindowSize(&windowX, &windowY);

	getQuad(packedChars, atlasSize, i, &quad);

	const float leftX = packedChars[i].xoff / windowX, topY = -packedChars[i].yoff / windowY;

	Glyph* const character = characters + i;

	character->coords[0] = leftX;
	character->coords[1] = leftX + ((float)(packedChars[i].x1 - packedChars[i].x0) / windowX);
	character->coords[2] = topY - ((float)(packedChars[i].y1 - packedChars[i].y0) / windowY);
	character->coords[3] = topY;

	character->texCoords[0][0] = character->texCoords[3][0] = quad.s1 + texture;
	character->texCoords[0][1] = character->texCoords[1][1] = quad.t0 + texture;
	character->texCoords[1][0] = character->texCoords[2][0] = quad.s0 + texture;
	character->texCoords[2][1] = character->texCoords[3][1] = quad.t1 + texture;
	
	character->advance = packedChars[i].xadvance / windowX;
    }

    free(fontData);
    free(textureData);

    initText("Lorem ipsum dolor sit amet!");
}
void GUI_Loop() {

}
