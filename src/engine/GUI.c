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

static Mesh testText;

static unsigned char* beginPacking(stbtt_pack_context* const context, const int size) {
    const int padding = 1;

    unsigned char* const textureData = mallocd(sizeof(*textureData) * size * size);

    stbtt_PackBegin(context, textureData, size, size, 0, padding, NULL);

    return textureData;
}
static void initText(Mesh* const mesh, const char text[const]) {
    const size_t textLength = strlen(text);

    vec2 position = GLM_VEC2_ZERO_INIT;

    Quad vertices[textLength];
    QuadIndices indices[textLength];

    float windowX, windowY;

    WH_GetWindowSize(&windowX, &windowY);

    for (size_t i = 0; i < textLength; i++) {
	const Index3D order[] = {0, 1, 2, 0, 2, 3};

	for (size_t j = 0; j < sizeof(order) / sizeof(*order); j++) indices[i][j] = order[j] + (i * 4);

	if (*text >= FIRST_CHARACTER && *text < FIRST_CHARACTER + NUM_CHARACTERS) {
	    Glyph* const glyph = characters + text[i] - FIRST_CHARACTER;

	    vertices[i][0][0] = vertices[i][3][0] = (glyph->coords[1] / windowX) + position[0];
	    vertices[i][1][0] = vertices[i][2][0] = (glyph->coords[0] / windowX) + position[0];

	    vertices[i][0][1] = vertices[i][1][1] = (glyph->coords[3] / windowY) + position[1];
	    vertices[i][2][1] = vertices[i][3][1] = (glyph->coords[2] / windowY) + position[1];

	    for (size_t j = 0; j < 4; j++) glm_vec2_copy(glyph->texCoords[j], vertices[i][j] + 2);
	    //for (size_t j = 0; j < 4; j++) glm_vec2_zero(vertices[i][j] + 2);

	    position[0] += glyph->advance / windowX;
	}
    }

    Mesh_InitWithData(mesh, GRAPHICS_PIPELINE_GUI, (MeshInitWithDataInfo){
	.verticesSize = textLength * 4,
	.numIndices = textLength * sizeof(QuadIndices) / sizeof(**indices),
	.vertices = vertices[0][0],
	.indices = indices[0]
    });
    Mesh_NewInstance(mesh);
}
static void initFont(const unsigned char fontData[const]) {
    stbtt_fontinfo info;

    if(!stbtt_InitFont(&info, fontData, 0)) throwFatal("stb_truetype error occurred!", "Failed to initialize the font");
}
static void packRange(stbtt_pack_context* const context, const unsigned char fontData[const], stbtt_packedchar chars[const]) {
    const float fontSize = 128;

    stbtt_PackFontRange(context, fontData, 0, fontSize, FIRST_CHARACTER, NUM_CHARACTERS, chars);
}
static unsigned char* pack(const int size, const unsigned char fontData[const], stbtt_packedchar chars[const]) {
    stbtt_pack_context ctx;

    unsigned char* const textureData = beginPacking(&ctx, size);

    packRange(&ctx, fontData, chars);

    stbtt_PackEnd(&ctx);

    return textureData;
}
static void getQuad(const stbtt_packedchar chars[const], const int size, const int index, stbtt_aligned_quad* const quad) {
    float unusedX, unusedY;

    stbtt_GetPackedQuad(chars, size, size, index, &unusedX, &unusedY, quad, 0);
}
static void initGlyphs() {
    const char path[] = "fonts\\Arimo-Medium.ttf";

    const int atlasSize = 1024;

    stbtt_packedchar packedChars[NUM_CHARACTERS];

    unsigned char* const fontData = PH_OpenFile(path, sizeof(path), NULL);

    initFont(fontData);

    unsigned char* const textureData = pack(atlasSize, fontData, packedChars);

    const float texture = (float)TexturesHandler_LoadTextureR8(textureData, atlasSize, atlasSize, path);

    for (int i = 0; i < NUM_CHARACTERS; i++) {
	stbtt_aligned_quad quad;

	getQuad(packedChars, atlasSize, i, &quad);

	const float leftX = packedChars[i].xoff, topY = -packedChars[i].yoff;

	Glyph* const character = characters + i;

	character->coords[0] = leftX;
	character->coords[1] = leftX + (float)(packedChars[i].x1 - packedChars[i].x0);
	character->coords[2] = topY - (float)(packedChars[i].y1 - packedChars[i].y0);
	character->coords[3] = topY;

	character->texCoords[0][0] = character->texCoords[3][0] = quad.s1 + texture;
	character->texCoords[0][1] = character->texCoords[1][1] = quad.t0 + texture;
	character->texCoords[1][0] = character->texCoords[2][0] = quad.s0 + texture;
	character->texCoords[2][1] = character->texCoords[3][1] = quad.t1 + texture;
	
	character->advance = packedChars[i].xadvance;
    }

    free(fontData);
    free(textureData);
}

void GUI_Init() {
    initGlyphs();
    initText(&testText, "Lorem ipsum dolor sit amet!");
}
void GUI_UpdateTexts() {
    Mesh_DeleteInstance(&testText);
    Mesh_Destroy(&testText);

    initText(&testText, "Lorem ipsum dolor sit amet!");
}
void GUI_Loop() {

}
