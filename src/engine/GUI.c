//using jsmn from cgltf
#define CGLTF_IMPLEMENTATION

#include <cglm/struct/vec2.h>
#include <stb_image.h>
#include "PathHandler.h"
#include "Mesh.h"
#include "TexturesHandler.h"
#include "def.h"
#include "Array.h"
#include "GUI.h"

#define SAFE_ZONE .015f
#define HINT_SPACE .015f
#define LINE_SPACING 1.25f

//i don't know why it works and i'm too afraid to ask
#define FILLED_CORNER 1 + framesTexture, 0
#define FREE_CORNER framesTexture, 0
#define RIGHT_CORNER framesTexture, 1
#define LEFT_CORNER FILLED_CORNER

typedef Index3D QuadIndices[3 + 3];
typedef vec4s Quad[4]; //two floats for position and other two for tex. coords

enum {
    LEFT,
    BOTTOM,
    RIGHT,
    TOP,
    INVALID_SIDE
};
typedef enum {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER
} HorizontalAlign;

DECLARE_ARRAY_TYPEDEF(Hint)
DECLARE_ARRAY_IMPL(Hint)

typedef struct {
    float coords[INVALID_SIDE];
    vec2s texCoords[4];
    float advance;
} Glyph;

static Mesh meshAmmo, deadText, hintText, hintFrame;
static HintArray hints;

static Ammo ammoMag = MAXT(ammoMag), ammoLeft;
static TextureID framesTexture;
static float currentHintLifeTime, timeBeingDead, fontSize;
static char firstChar;

static Glyph* characters;
static const Character* playableCharacter;

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
static float getInvFactor() {
    const float magic = 8;

    float width, height;

    WH_GetWindowSize(&width, &height);

    return height / magic;
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

    if (alignRight) position->raw[0] -= (glyph->coords[LEFT] + glyph->coords[RIGHT]) * size * ratio;
    if (alignTop) position->raw[1] -= (glyph->coords[BOTTOM] + glyph->coords[TOP]) * size;
}
static void updateTextData(const float outline, const float size, const InstanceID id) {
    const vec3 data = {1, outline, getInvFactor() * size};

    R_UploadStatic(GRAPHICS_PIPELINE_TEXT, id, sizeof(data), data);
}
static vec2s initText(
    Mesh* const mesh, const char text[const], vec2s position, const float size, const float outline, const float xmax,
    const HorizontalAlign hor, const bool alignTop
) {
    vec2s maxCoords;
    size_t i;

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
    maxCoords.raw[1] = origPos.raw[1] - (size / LINE_SPACING);

    alignText(&position, size, ratio, text[0], hor == ALIGN_RIGHT, alignTop);

    if (hor) i = hor == ALIGN_CENTER ? textLength / 2 : textLength - 1;
    else i = 0;
// hor ? !ISINVALID(i) : i < textLength
    for (;;) {
	const Index3D order[] = {0, 1, 2, 0, 2, 3};

	for (size_t j = 0; j < ARRAYSIZE(order); j++) indices[i][j] = order[j] + (i * 4);

	if (text[i] >= firstChar) {
	    Glyph* const glyph = characters + text[i] - firstChar;

	    float factor;

	    if (hor == ALIGN_CENTER) factor = i >= textLength / 2 ? 1 : -1;
	    else factor = hor ? -1.f : 1;

	    vertices[i][0].raw[0] = vertices[i][3].raw[0]
	    = (glyph->coords[factor > 0 ? RIGHT : LEFT] * size * ratio * factor) + position.raw[0];

	    vertices[i][1].raw[0] = vertices[i][2].raw[0]
	    = (glyph->coords[factor > 0 ? LEFT : RIGHT] * size * ratio * factor) + position.raw[0];

	    vertices[i][0].raw[1] = vertices[i][1].raw[1] = (glyph->coords[TOP] * size) + position.raw[1];
	    vertices[i][2].raw[1] = vertices[i][3].raw[1] = (glyph->coords[BOTTOM] * size) + position.raw[1];

	    if (text[i] == ' ' && position.raw[0] - origPos.raw[0] > xmax) {
		position.raw[0] = origPos.raw[0];
		position.raw[1] -= size * LINE_SPACING;

		if (position.raw[1] < maxCoords.raw[1]) maxCoords.raw[1] = position.raw[1];
	    }
	    else {
		for (size_t j = 0; j < 4; j++) glm_vec2_copy(
		    glyph->texCoords[j].raw, vertices[i][j].raw + VERTEX2D_TEXCOORDS_OFFSET
		);

		//for (size_t j = 0; j < 4; j++) glm_vec2_zero(vertices[i][j] + 2);
		position.raw[0] += glyph->advance * size * ratio * factor;

		if (position.raw[0] > maxCoords.raw[0]) maxCoords.raw[0] = position.raw[0];
	    }
	}

	if (hor) {
	    if (hor == ALIGN_CENTER) {
		i += i >= textLength / 2 ? 1 : -1;

		if (i == textLength) {
		    position.raw[0] = origPos.raw[0];

		    i = (textLength / 2) - 1;

//		    alignText(&position, size, ratio, text[i], true, false);
		}
	    }
	    else i -= 1;
	}
	else i += 1;

	if (hor) {
	    if (ISINVALID(i)) break;
	}
	else if (i >= textLength) break;
    }

    if (Mesh_IsValid(mesh)) Mesh_UpdateIndicesVertices(mesh, info);
    else Mesh_InitWithData(mesh, GRAPHICS_PIPELINE_TEXT, info);

    updateTextData(outline, size, mesh->id);

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
    const float atlasWidth, const float atlasHeight, const float texture, const bool recordingUV
) {
    const ptrdiff_t side = isTokenCoord(js, token);

    const float value = strtof(js + token[1].start, NULL);

    if (recordingUV) {
	const float w = (value / atlasWidth) + texture, h = 1 - (value / atlasHeight);

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
    float atlasWidth, atlasHeight;

    const float texture = (float)getAtlas(&atlasWidth, &atlasHeight);

    for (const jsmntok_t* token = tokens; token < tokens + numTokens - 1; token++) {
	if (!tokenStrCmp(js, "size", token)) fontSize = strtof(js + token[1].start, NULL);
	else if (!tokenStrCmp(js, "glyphs", token)) {
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
			else processGlyphValue(js, character, token, atlasWidth, atlasHeight, texture, recordingUV);
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
	&meshAmmo, string, glms_vec2_fill(1 - SAFE_ZONE), 
	fontSize, 1, INFINITY, ALIGN_RIGHT, true
    );
}
static void initFrame(Mesh* const mesh, const vec2s size) {
    const float cornerSize = .015f, ratio = WH_GetWindowInvRatio(), x = cornerSize * ratio, y = cornerSize;

    const float rightf = size.raw[0] * ratio, rightc = rightf - x, upperf = size.raw[1], upperc = upperf - y;

    //oh yeah, an unmaintainable brick straight out of ass that holds together the entire program
    const Index3D indices[] = {
	0, 1, 4, 1, 4, 5,
	1, 2, 5, 2, 5, 6,
	2, 6, 7, 2, 3, 7,
	4, 7, 8, 7, 8, 11,
	8, 12, 13, 8, 9, 13,
	9, 10, 13, 10, 13, 14,
	10, 11, 14, 11, 14, 15
    };

    float vertices[] = {
	0,      0, FREE_CORNER,
	x,      0, LEFT_CORNER,
	rightc, 0, RIGHT_CORNER,
	rightf, 0, FREE_CORNER,
	//upper row
	0,      y, RIGHT_CORNER,
	x,      y, FILLED_CORNER,
	rightc, y, FILLED_CORNER,
	rightf, y, LEFT_CORNER,
	//upper row
	0,      upperc, LEFT_CORNER,
	x,      upperc, FILLED_CORNER,
	rightc, upperc, FILLED_CORNER,
	rightf, upperc, RIGHT_CORNER,
	//upper row
	0,      upperf, FREE_CORNER,
	x,      upperf, RIGHT_CORNER,
	rightc, upperf, LEFT_CORNER,
	rightf, upperf, FREE_CORNER
    };

    for (size_t i = 0; i < ARRAYSIZE(vertices); i += 4) {
	vertices[i] += SAFE_ZONE - 1;
	vertices[i + 1] += 1 - upperf - SAFE_ZONE;
    }

    const MeshInitWithDataInfo info = {
	.verticesSize = ARRAYSIZE(vertices),
	.numIndices = ARRAYSIZE(indices),
	.vertices = vertices,
	.indices = indices
    };

    if (Mesh_IsValid(mesh)) Mesh_UpdateIndicesVertices(mesh, info);
    else {
	const float one = 1;

	Mesh_InitWithData(mesh, GRAPHICS_PIPELINE_GUIFRAMES, info);

	R_UploadStatic(GRAPHICS_PIPELINE_GUIFRAMES, mesh->id, sizeof(float), &one);
    }
}
static void initHint(const Hint text) {
    const float size = .04f, xmax = .5f;

    //return initText(&meshHint, "Press the right mouse button to move character. "
	//"Use the middle mouse button and WASD keys to relocate the camera", (vec2s){{-1, 1}}, size, 0, xmax, false, true);
	//"Use keys '0' and '1' to switch weapons. Your character will automatically try to shoot anybody "
	//"who comes into their view.", 
    initFrame(&hintFrame, glms_vec2_add(
	initText(
	    &hintText, text, (vec2s){{(HINT_SPACE * WH_GetWindowInvRatio()) + SAFE_ZONE - 1, 1 - HINT_SPACE - SAFE_ZONE}}, 
	    size, 0, xmax, ALIGN_LEFT, true
	), 
	glms_vec2_add(glms_vec2_fill(HINT_SPACE * 2), (vec2s){{0, size / 4}}) //magic division by 4
    ));
}

void GUI_Init() {
    framesTexture = TexturesHandler_BeginLoadingTask("framestexture", "textures/frame.png");

    HintArray_Init(&hints);

    initGlyphs();

    initAmmoText();

    //throwFatal(
    //"Oh man, your pc is fucked!", "Ur system32 folder's been filled with furry pron, send some bitcoin to me and i"
    //"'ll revert this shit");
}
const Character** GUI_GetCharacter() {
    return &playableCharacter;
}
bool GUI_ThrowError(const Hint title, const Hint description) {
    if (characters) {
	const float size = .1f, outline = .25f, xmax = 1.5f;

	Mesh tmesh, titmesh, dmesh, cmesh;

	float y;

	Mesh_Invalidate(&tmesh);
	Mesh_Invalidate(&titmesh);
	Mesh_Invalidate(&dmesh);
	Mesh_Invalidate(&cmesh);

	y = initText(
	    &tmesh, "The gmae crashed!", (vec2s){{-1, 1}}, size * 2, outline, xmax, ALIGN_LEFT, true
	).raw[1] + size;

	y += initText(&titmesh, title, (vec2s){{-1, 1 - y}}, size, outline, xmax, ALIGN_LEFT, true).raw[1] + size;
	y += initText(&dmesh, description, (vec2s){{-1, 1 - y}}, size, outline, xmax, ALIGN_LEFT, true).raw[1] + size;
	initText(
	    &cmesh, "Please open an issue on 'github.com/annoyingdick/thegmae' if the error persists", 
	    (vec2s){{-1, 1 - y}}, size / 2, outline, xmax, ALIGN_LEFT, true
	);

	return true;
    }

    return false;
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
void GUI_NewHint(const Hint hint) {
    HintArray_AppendElement(&hints, &hint);

    if (hints.numElements == 1) initHint(hints.elements[0]);
}
void GUI_ShowDeath() {
    const float size = .2f;

    initText(&deadText, "Got smoked!", glms_vec2_zero(), size, 1.f / 4, INFINITY, ALIGN_CENTER, false);
}
void GUI_HideDeath() {
    Mesh_Destroy(&deadText);

    timeBeingDead = 0;
}
void GUI_UpdateTexts() {
    initAmmoText();

    if (timeBeingDead) GUI_ShowDeath();

    if (hints.numElements) initHint(hints.elements[0]);
}
void GUI_Loop() {
    if (hints.numElements) {
	currentHintLifeTime += WH_GetDeltaTime();

	if (currentHintLifeTime > Hint_GetExpireTime(hints.elements[0])) {
	    const float fadeTime = .5f;

	    if (hints.numElements > 1) {
		currentHintLifeTime = 0;

		HintArray_RemoveElement(&hints, 0);

		initHint(hints.elements[0]);
	    }
	    else if (currentHintLifeTime > Hint_GetExpireTime(hints.elements[0])) {
		const float tr = 1 - ((currentHintLifeTime - Hint_GetExpireTime(hints.elements[0])) / fadeTime);

		R_UploadStatic(GRAPHICS_PIPELINE_GUIFRAMES, hintFrame.id, sizeof(tr), &tr);
		R_UploadStatic(GRAPHICS_PIPELINE_TEXT, hintText.id, sizeof(tr), &tr);
	    }
	}
    }
    if (playableCharacter->state == CHARACTER_STATE_DEAD) {
	timeBeingDead += WH_GetDeltaTime();

	const float opacity = fminf((timeBeingDead - 1) * 2, 1);

	R_UploadStatic(GRAPHICS_PIPELINE_TEXT, deadText.id, sizeof(opacity), &opacity);
    }
}
