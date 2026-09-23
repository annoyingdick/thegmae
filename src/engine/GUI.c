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

enum {
    LEFT,
    BOTTOM,
    RIGHT,
    TOP,
    INVALID_SIDE
};

typedef struct {
    float coords[INVALID_SIDE];
    vec2 texCoords[4];
    float advance;
} Glyph;

static Mesh testText;

static char firstChar;

static Glyph* characters;

static ptrdiff_t isTokenCoord(const char js[const], const jsmntok_t* const token) {
    if (token->type == JSMN_STRING) {
	const char* const names[] = {
	    [LEFT] = "left", 
	    [BOTTOM] = "bottom", 
	    [RIGHT] = "right", 
	    [TOP] = "top"
	};

	nforeach (const char* const* const name, names)
	    if (!strncmp(js + token->start, *name, token->end - token->start)) return name - names;
	forend
    }

    return INVALID_SIDE;
}
static int tokenStrCmp(const char js[const], const char str[const], const jsmntok_t* const token) {
    return token->type == JSMN_STRING ? strncmp(js + token->start, str, token->end - token->start) : 1;
}
static void initText(Mesh* const mesh, const char text[const]) {
    const float size = 100;

    const size_t textLength = strlen(text);

    vec2 position = GLM_VEC2_ZERO_INIT;

    Quad vertices[textLength];
    QuadIndices indices[textLength];

    float windowX, windowY;

    WH_GetWindowSize(&windowX, &windowY);

    windowX /= size;
    windowY /= size;

    for (size_t i = 0; i < textLength; i++) {
	const Index3D order[] = {0, 1, 2, 0, 2, 3};

	for (size_t j = 0; j < ARRAYSIZE(order); j++) indices[i][j] = order[j] + (i * 4);

	if (text[i] >= firstChar) {
	    Glyph* const glyph = characters + text[i] - firstChar;

	    vertices[i][0][0] = vertices[i][3][0] = (glyph->coords[RIGHT] / windowX) + position[0];
	    vertices[i][1][0] = vertices[i][2][0] = (glyph->coords[LEFT] / windowX) + position[0];

	    vertices[i][0][1] = vertices[i][1][1] = (glyph->coords[TOP] / windowY) + position[1];
	    vertices[i][2][1] = vertices[i][3][1] = (glyph->coords[BOTTOM] / windowY) + position[1];

	    for (size_t j = 0; j < 4; j++) glm_vec2_copy(glyph->texCoords[j], vertices[i][j] + VERTEX2D_TEXCOORDS_OFFSET);
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
    char maxChar;

    char* const js = PH_OpenFile(path, sizeof(path), &fileSize);

    stbi_uc* const atlas = stbi_load("fonts\\bold.png", &width, &height, NULL, 3);

    jsmn_init(&parser);

    const int numTokens = jsmn_parse(&parser, js, fileSize - 1, NULL, 0);

    checkJSMN(numTokens, path);

    //reset parser
    jsmntok_t* const tokens = mallocd(numTokens * sizeof(*tokens));

    jsmn_init(&parser);

    checkJSMN(jsmn_parse(&parser, js, fileSize - 1, tokens, numTokens), path);

    firstChar = CHAR_MAX;
    maxChar = CHAR_MIN;

    //count characters
    for (const jsmntok_t* token = tokens; token < tokens + numTokens;) {
	if (!tokenStrCmp(js, "unicode", token)) {
	    const char c = (char)atoi(js + (++token)->start);

	    if (c < firstChar) firstChar = c;
	    if (c > maxChar) maxChar = c;

	    //token += 2;
	}
	else token++;
    }

    if (!maxChar) throwFatal("There are no glyphs in this font json file!", path);

    mallocarr(characters, maxChar - firstChar + 1);

    const float texture = (float)TexturesHandler_LoadTextureRGB888(atlas, width, height, path);

    for (const jsmntok_t* token = tokens; token < tokens + numTokens - 1; token++) {
	if (!tokenStrCmp(js, "glyphs", token)) {
	    const int numGlyphs = (++token)->size;

	    if (token->type != JSMN_ARRAY) throwFatal(path, "'glyphs' has been expected to be an array");
	    else if (!token->size) throwFatal(path, "There are no glyphs in this font json file");

	    bool recordingUV;

	    const jsmntok_t* glyph;
	    Glyph* character;

	    glyph = ++token;

	    for (int i = 0; i < numGlyphs;) {
		if (token->type == JSMN_OBJECT) {
		    ++i;

		    glyph = ++token;

		    character = NULL;
		}
		else if (!tokenStrCmp(js, "unicode", token) && !character) {
		    character = characters + atoi(js + token[1].start) - firstChar;

		    token = glyph;
		}
		else {
		    if (character) {
			if (!tokenStrCmp(js, "advance", token)) character->advance = strtof(js + token[1].start, NULL);
			else if (!tokenStrCmp(js, "planeBounds", token)) recordingUV = false;
			else if (!tokenStrCmp(js, "atlasBounds", token)) recordingUV = true;
			else {
			    const ptrdiff_t side = isTokenCoord(js, token);

			    const float value = strtof(js + token[1].start, NULL);

			    if (recordingUV) {
				const float w = (value / (float)width) + texture, h = 1 - (value / (float)height);

				switch (side) {
				case LEFT:
				    character->texCoords[1][0] = character->texCoords[2][0] = w;
				    break;
				case BOTTOM:
				    character->texCoords[2][1] = character->texCoords[3][1] = h;
				    break;
				case RIGHT:
				    character->texCoords[0][0] = character->texCoords[3][0] = w;
				    break;
				case TOP:
				    character->texCoords[0][1] = character->texCoords[1][1] = h;
				}
			    }
			    else character->coords[side] = value;
			}
		    }

		    token += 2;
		}
	    }

	    /*
	    for (++token; token <= glyphs + glyphs->size; token++) {
		const jsmntok_t* const glyph = token;

		Glyph* character;

		character = NULL;

		printf("%i\n", glyph->size);

		for (++token; token <= glyph + glyph->size; token += 2) {
		    if (!tokenStrCmp(js, "unicode", token)) {
			character = characters + atoi(js + token[1].start) - firstChar;

			break;
		    }
		}

		if (!character) {
		    throwFatal(path, "This font json file has a glyph object which isn't associated with any character");
		}

		token = glyph;

		for (token++; token <= glyph + glyph->size; token += 2) {
		    if (!tokenStrCmp(js, "advance", token)) character->advance = strtof(js + token[1].start, NULL);
		}
	    }
	    */

	    /*
	    if (!tokenStrCmp(js, "unicode", token)) {
		token++;

		const char c = (char)atoi(js + token->start);

		if (!character) firstChar = c;

		character = c;
	    }
	    else if (!tokenStrCmp(js, "advance", token)) {
		assertChar(character);

		//measured in em
		characters[character - firstChar].advance = strtof(js + (++token)->start, NULL);
	    }
	    else if (!tokenStrCmp(js, "planeBounds", token)) {
		Glyph* const glyph = characters + character - firstChar;

		assertChar(character);

		//measured in em
		glyph->coords[0] = strtof(js + (token += 3)->start, NULL);
		glyph->coords[1] = strtof(js + (token += 2)->start, NULL);
		glyph->coords[2] = strtof(js + (token += 2)->start, NULL);
		glyph->coords[3] = strtof(js + (token += 2)->start, NULL);
	    }
	    else if (!tokenStrCmp(js, "atlasBounds", token)) {
		Glyph* const glyph = characters + character - firstChar;

		assertChar(character);

		//measured in em
		glyph->coords[0] = strtof(js + (token += 3)->start, NULL);
		glyph->coords[1] = strtof(js + (token += 2)->start, NULL);
		glyph->coords[2] = strtof(js + (token += 2)->start, NULL);
		glyph->coords[3] = strtof(js + (token += 2)->start, NULL);
	    }
	    */

	    break;
	}
    }

    for (int i = 0; i < maxChar - firstChar; i++) {
	const float a = (characters[i].coords[RIGHT] - characters[i].coords[LEFT]) * width;
	const float b = (characters[i].texCoords[0][0] - characters[i].texCoords[1][0]) * width;
	printf("%c %f %f %.20f\n", i + firstChar, a, b, a / b);
    }

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
    initText(&testText, "Lorem ipsum dolor sit amet!");
}
void GUI_UpdateTexts() {
    Mesh_DeleteInstance(&testText);
    Mesh_Destroy(&testText);

    initText(&testText, "Lorem ipsum dolor sit amet!");
}
void GUI_Loop() {

}
