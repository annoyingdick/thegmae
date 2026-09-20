//using jsmn from cgltf
#define CGLTF_IMPLEMENTATION

#include <cglm/types.h>
#include <cglm/vec2.h>
#include <stb_image.h>
#include "PathHandler.h"
#include "Mesh.h"
#include "TexturesHandler.h"
#include "def.h"

typedef Index3D QuadIndices[3 + 3];
typedef vec4 Quad[4]; //two floats for position and other two for tex. coords

typedef struct {
    vec2 size;
    vec2 texCoords[4];
    float advance;
} Glyph;

//static Mesh testText;

static Glyph* characters;

static int tokenStrCmp(const char js[const], const char str[const], const jsmntok_t* const token) {
    return strncmp(js + token->start, str, token->end - token->start);
}
/*
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
*/
static void assertChar(const char character) {
    if (!character) {
	const char src[] = "lol";
	//const char src[] = "This character json object has an incorrect order of fields. Character: ";

	char str[sizeof(src) + 1];

	strcpy(str, src);

	str[sizeof(src) - 1] = character;
	str[sizeof(src)] = '\0';

	throwFatal("GUI error occurred!", str);
    }
}
static void checkJSMN(const int ret, const char path[const]) {
    switch (ret) {
    case JSMN_ERROR_INVAL:
	throwFatal("JSMN error: bad token", path);
	break;
    case JSMN_ERROR_NOMEM:
	throwFatal("JSMN error: string too large", path);
	break;
    case JSMN_ERROR_PART:
	throwFatal("JSMN error: string too short", path);
    }
}
static void initGlyphs() {
    const char path[] = "fonts\\bold.json";

    jsmn_parser parser;

    size_t fileSize;
    int width, height;
    char character, firstChar;

    char* const js = PH_OpenFile(path, sizeof(path), &fileSize);

    stbi_uc* const atlas = stbi_load("fonts\\bold.png", &width, &height, NULL, 3);

    jsmn_init(&parser);

    const int numTokens = jsmn_parse(&parser, js, fileSize - 1, NULL, 0);

    checkJSMN(numTokens, path);

    //reset parser
    jsmn_init(&parser);

    jsmntok_t* const tokens = mallocd(numTokens * sizeof(*tokens));

    checkJSMN(jsmn_parse(&parser, js, fileSize - 1, tokens, numTokens), path);

    character = 0;

    //count characters
    for (const jsmntok_t* token = tokens; token < tokens + numTokens;) {
	if (token->type == JSMN_STRING && !tokenStrCmp(js, "unicode", token)) {
	    token++;

	    const char c = (char)atoi(js + token->start);

	    if (!character) firstChar = c;

	    character = c;

	    token += 2;
	}
	else token++;
    }

    if (!character) throwFatal("There are no glyphs in this JSON!", path);

    mallocarr(characters, character - firstChar + 1);

    character = 0;

    for (const jsmntok_t* token = tokens; token < tokens + numTokens;) {
	if (token->type == JSMN_STRING) {
	    if (!tokenStrCmp(js, "unicode", token)) {
		token++;

		const char c = (char)atoi(js + token->start);

		if (!character) firstChar = c;

		character = c;
	    }
	    else if (!tokenStrCmp(js, "advance", token)) {
		assertChar(character);

		token++;

		//measured in em
		characters[character - firstChar].advance = strtof(js + token->start, NULL);
	    }
	}

	token++;
    }

    //const float texture = (float)TexturesHandler_LoadTextureRGB888(textureData, atlasSize, atlasSize, path);

    /*
    for (int i = 0; i < NUM_CHARACTERS; i++) {
	stbtt_aligned_quad quad;

	getQuad(packedChars, atlasSize, i, &quad);

	const float leftX = packedChars[i].xoff, topY = -packedChars[i].yoff, pad = (OUTLINE - 1) / (float)atlasSize;

	Glyph* const character = characters + i;

	character->coords[0] = leftX - OUTLINE;
	character->coords[1] = leftX + (float)(packedChars[i].x1 - packedChars[i].x0) + OUTLINE;
	character->coords[2] = topY - (float)(packedChars[i].y1 - packedChars[i].y0) - OUTLINE;
	character->coords[3] = topY + OUTLINE;

	character->texCoords[0][0] = character->texCoords[3][0] = quad.s1 + texture + pad;
	character->texCoords[0][1] = character->texCoords[1][1] = quad.t0 + texture - pad;
	character->texCoords[1][0] = character->texCoords[2][0] = quad.s0 + texture - pad;
	character->texCoords[2][1] = character->texCoords[3][1] = quad.t1 + texture + pad;
	
	character->advance = packedChars[i].xadvance + (2 * OUTLINE);
    }
    */

    free(js);
    free(tokens);

    stbi_image_free(atlas);
}

void GUI_Init() {
    initGlyphs();
    //initText(&testText, "Lorem ipsum dolor sit amet!");
}
void GUI_UpdateTexts() {
    //Mesh_DeleteInstance(&testText);
    //Mesh_Destroy(&testText);

    //initText(&testText, "Lorem ipsum dolor sit amet!");
}
void GUI_Loop() {

}
