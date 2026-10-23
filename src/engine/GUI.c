//using jsmn from cgltf
#define CGLTF_IMPLEMENTATION

#include <cglm/struct/vec2.h>
#include <stb_image.h>
#include "PathHandler.h"
#include "Mesh.h"
#include "TexturesHandler.h"
#include "def.h"
#include "GUI.h"

#define SAFE_ZONE .01f
#define HINT_SPACE .015f

typedef Index3D QuadIndices[3 + 3];
typedef vec4s Quad[4]; //two floats for position and other two for tex. coords

enum {
    LEFT,
    BOTTOM,
    RIGHT,
    TOP,
    INVALID_SIDE
};

typedef struct {
    float coords[INVALID_SIDE];
    vec2s texCoords[4];
    float advance;
} Glyph;

static Mesh meshAmmo, meshHint;

static Ammo ammoMag = MAXT(ammoMag), ammoLeft;

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
static TextureID getAtlas(float* const width, float* const height) {
    int w, h;

    const TextureID texture = TexturesHandler_LoadTextureRGB888(&w, &h, "fonts\\", "bold.png");

    *width = (float)w;
    *height = (float)h;

    return texture;
}
static void alignText(
    vec2s* const position, const float size, const float ratio, const char c, const bool alignRight, const bool alignTop
) {
    Glyph* const glyph = characters + c - firstChar;

    const float magic = .1f;

    if (alignRight) position->raw[0] -= (glyph->coords[LEFT] + glyph->coords[RIGHT] + magic) * size * ratio;
    if (alignTop) position->raw[1] -= (glyph->coords[BOTTOM] + glyph->coords[TOP] + magic) * size;
}
static vec2s initText(
    Mesh* const mesh, const char text[const], vec2s position, const float size, const float outline, const float xmax,
    const bool alignRight, const bool alignTop
) {
    vec2s maxCoords;

    const vec2s origPos = position;
    const size_t textLength = strlen(text);
    const float ratio = WH_GetWindowInvRatio();

    Quad vertices[textLength];
    QuadIndices indices[textLength];

    const MeshInitWithDataInfo info = {
	.verticesSize = textLength * 4,
	.numIndices = textLength * sizeof(QuadIndices) / sizeof(**indices),
	.vertices = vertices[0][0].raw,
	.indices = indices[0]
    };

    maxCoords.raw[0] = -INFINITY;
    maxCoords.raw[1] = INFINITY;

    alignText(&position, size, ratio, text[0], alignRight, alignTop);

    for (size_t i = alignRight ? textLength - 1 : 0; alignRight ? !ISINVALID(i) : i < textLength; i += alignRight ? -1 : 1) {
	const Index3D order[] = {0, 1, 2, 0, 2, 3};

	for (size_t j = 0; j < ARRAYSIZE(order); j++) indices[i][j] = order[j] + (i * 4);

	if (text[i] >= firstChar) {
	    Glyph* const glyph = characters + text[i] - firstChar;

	    vertices[i][0].raw[0] = vertices[i][3].raw[0] = (glyph->coords[RIGHT] * size * ratio) + position.raw[0];
	    vertices[i][1].raw[0] = vertices[i][2].raw[0] = (glyph->coords[LEFT] * size * ratio) + position.raw[0];

	    vertices[i][0].raw[1] = vertices[i][1].raw[1] = (glyph->coords[TOP] * size) + position.raw[1];
	    vertices[i][2].raw[1] = vertices[i][3].raw[1] = (glyph->coords[BOTTOM] * size) + position.raw[1];

	    if (text[i] == ' ' && position.raw[0] - origPos.raw[0] > xmax) {
		position.raw[0] = origPos.raw[0];
		position.raw[1] -= size * 4 / 3;

		if (position.raw[1] < maxCoords.raw[1]) maxCoords.raw[1] = position.raw[1];
	    }
	    else {
		for (size_t j = 0; j < 4; j++) glm_vec2_copy(
		    glyph->texCoords[j].raw, vertices[i][j].raw + VERTEX2D_TEXCOORDS_OFFSET
		);
		//for (size_t j = 0; j < 4; j++) glm_vec2_zero(vertices[i][j] + 2);
		position.raw[0] += glyph->advance * size * ratio * (alignRight ? -1.f : 1);

		if (position.raw[0] > maxCoords.raw[0]) maxCoords.raw[0] = position.raw[0];
	    }
	}
    }

    if (Mesh_IsValid(mesh)) Mesh_UpdateIndicesVertices(mesh, info);
    else {
	Mesh_InitWithData(mesh, GRAPHICS_PIPELINE_TEXT, info);

	R_UploadStatic(GRAPHICS_PIPELINE_TEXT, mesh->id, sizeof(outline), &outline);
    }

    const vec2s ret = glms_vec2_sub(maxCoords, origPos);// {{maxCoords.raw[0], maxCoords.raw[0]}};

    //printf("\n%s\n %f %f   %f %f\n", text, origPos.raw[0], origPos.raw[1], maxCoords.raw[0], maxCoords.raw[1]);
    //printf("\n%s\n %f %f\n", text, ret.raw[0], ret.raw[1]);

    return (vec2s){{ret.raw[0] / ratio, -ret.raw[1]}};
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
static void processGlyphValue(
    const char js[const], Glyph* const character, const jsmntok_t* const token, 
    const float width, const float height, const float texture, const bool recordingUV
) {
    const ptrdiff_t side = isTokenCoord(js, token);

    const float value = strtof(js + token[1].start, NULL);

    if (recordingUV) {
	const float w = (value / width) + texture, h = 1 - (value / height);

	switch (side) {
	case LEFT:
	    character->texCoords[1].raw[0] = character->texCoords[2].raw[0] = w;
	    break;
	case BOTTOM:
	    character->texCoords[2].raw[1] = character->texCoords[3].raw[1] = h;
	    break;
	case RIGHT:
	    character->texCoords[0].raw[0] = character->texCoords[3].raw[0] = w;
	    break;
	case TOP:
	    character->texCoords[0].raw[1] = character->texCoords[1].raw[1] = h;
	}
    }
    else character->coords[side] = value;
}
static void processTokens(jsmntok_t tokens[const], const char js[const], const char path[const], const int numTokens) {
    float width, height;

    const float texture = (float)getAtlas(&width, &height);

    for (const jsmntok_t* token = tokens; token < tokens + numTokens - 1; token++) {
	if (!tokenStrCmp(js, "glyphs", token)) {
	    const int numGlyphs = (++token)->size;

	    if (token->type != JSMN_ARRAY) throwFatal(path, "'glyphs' has been expected to be an array");
	    else if (!token->size) throwFatal(path, "There are no glyphs in this font json file");

	    bool recordingUV;

	    const jsmntok_t* glyph;
	    Glyph* character;
	    
	    //to stop compiler from bitchin
	    character = NULL;
	    recordingUV = false;

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
			else processGlyphValue(js, character, token, width, height, texture, recordingUV);
		    }

		    token += 2;
		}
	    }

	    break;
	}
    }
}
static void initGlyphs() {
    const char path[] = "fonts\\bold.json";

    jsmn_parser parser;

    size_t fileSize;
    char maxChar;

    char* const js = PH_OpenFile(path, sizeof(path), &fileSize);

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

    processTokens(tokens, js, path, numTokens);

    //before i fixed this, the space glyph had been filled with junk like 0xBAADF00D which caused severe artifacts
    memset(characters + ' ' - firstChar, UINT8_MAX, sizeof(*characters) - sizeof(characters->advance));
    
    free(js);
    free(tokens);
}
static void initAmmoText() {
    const char format[] = "%u  %u";

    const int size = snprintf(NULL, 0, format, ammoLeft, ammoMag);

    const float fontSize = .05f;

    char string[size];

    sprintf(string, format, ammoLeft, ammoMag);

    initText(
	&meshAmmo, string, (vec2s){{1 - (SAFE_ZONE * WH_GetWindowInvRatio()), 1 - SAFE_ZONE}}, 
	fontSize, 1, INFINITY, true, true
    );
}
static vec2s initHintText() {
    const float size = .04f, xmax = .5f, magic = 2.2f;

    //return initText(&meshHint, "Press the right mouse button to move character. "
	//"Use the middle mouse button and WASD keys to relocate the camera", (vec2s){{-1, 1}}, size, 0, xmax, false, true);
    return glms_vec2_add(initText(&meshHint, "Use keys '0' and '1' to switch weapons. Your character will automatically try to shoot anybody "
	"who comes into their view.", 
	(vec2s){{-1 + (HINT_SPACE * WH_GetWindowInvRatio()), 1 - HINT_SPACE}}, size, 0, xmax, false, true
    ), (vec2s){{HINT_SPACE * 2, HINT_SPACE * magic}});
}

void GUI_Init() {
    initGlyphs();

    initAmmoText();

    R_NewFrame(initHintText());
}
void GUI_UpdateAmmoMag(const Ammo mag) {
    ammoMag = mag;

    initAmmoText();
}
void GUI_UpdateAmmoBoth(const Ammo mag, const Ammo left) {
    ammoMag = mag;
    ammoLeft = left;

    initAmmoText();
}
void GUI_UpdateTexts() {
    initAmmoText();
    initHintText();
}
void GUI_Loop() {

}
