#include <stdio.h>
#include "nuklearHandler.h"
#include "WindowHandler.h"
#include "PathHandler.h"
#include "InstancesHandler.h"
#include "NavigationHandler.h"
#include "CharactersHandler.h"
#include "DebugGuiHandler.h"

#define MAKE_GRAY(x) (struct nk_color){.r = x, .g = x, .b = x, .a = UINT8_MAX}
#define NK_WHITE MAKE_GRAY(UINT8_MAX)

#define ROW_HEIGHT 38

#define MAKE_MENU(h, title) if (openMenu(i++, title)) { h##_DrawDebugGui(); } nk_end(nkc)

#define MAX_CHARACTERS_64 21

static ArrayView selectedArray, wishArray;

static struct nk_context* nkc;

static nk_bool openMenu(const int menuId, const char title[const]) {
    const nk_flags flags = NK_WINDOW_BORDER + NK_WINDOW_SCALABLE + NK_WINDOW_MOVABLE + NK_WINDOW_MINIMIZABLE + NK_WINDOW_TITLE;
    const float width = 1000, height = 1000;
    const uint8_t titleHeight = 46;

    const nk_bool result = nk_begin(
	nkc, title, nk_rect(menuId % 2 ? width : 0, (float)(titleHeight * (menuId >> 1)), width, height), flags
    );

    return result;
}
static nk_bool openGroup(const char title[const], const size_t numElements) {
    return DGH_Begin(title, numElements);
}
static nk_bool openList() {
    const float x = 500;

    const nk_flags flags = NK_WINDOW_BORDER + NK_WINDOW_SCALABLE + NK_WINDOW_MOVABLE + NK_WINDOW_MINIMIZABLE +
    NK_WINDOW_TITLE + NK_WINDOW_CLOSABLE;

    return nk_begin(nkc, selectedArray.name, nk_rect(x, x, x, x), flags);
}
static void initSelectablesActiveStyle() {
    const struct nk_style_item selectableStyle = {.data.color = {.r = 100, .g = 149, .b = 237, .a = 255}};

    nkc->style.selectable.normal_active = nkc->style.selectable.hover_active = selectableStyle;
    nkc->style.selectable.pressed_active = selectableStyle;
}
static void initFont() {
    const float height = 28;

    struct nk_font_atlas* atlas;
    struct nk_font* font;

    nk_sdl_font_stash_begin(&atlas);

    font = nk_font_atlas_add_default(atlas, height, 0);

    nk_sdl_font_stash_end();

    if (font) nkc->style.font = &font->handle;
}
static void makeMenus() {
    int i;

    i = 0;

    MAKE_MENU(WH, "Window Handler");
    MAKE_MENU(PH, "Path Handler");
    MAKE_MENU(IH, "Instances Handler");
    MAKE_MENU(NH, "Navigation Handler");
    MAKE_MENU(R, "Render");
    MAKE_MENU(CH, "Characters Handler");
}
static void defLabel(const char text[const]) {
    nk_label(nkc, text, NK_TEXT_ALIGN_LEFT);
}
static void defRow(const int columns) {
    nk_layout_row_dynamic(nkc, ROW_HEIGHT, columns);
}
static void whiteLabel(const char text[const]) {
    nk_label_colored(nkc, text, NK_TEXT_ALIGN_LEFT, NK_WHITE);
}
static void defField(const char name[const restrict], const char typeStr[const restrict]) {
    char fullName[strlen(typeStr) + strlen(name) + 1];

    strcpy(fullName, typeStr);
    strcat(fullName, name);
    
    defRow(2);
    defLabel(fullName);
}
static void displayFloat(const float value) {
    const int size = snprintf(NULL, 0, "%f", value);

    char str[size];

    sprintf(str, "%f", value);

    whiteLabel(str);
}
static void displayU32(const uint32_t value) {
    const size_t maxChars = 11;

    char str[maxChars];

    sprintf(str, "%u", value);

    whiteLabel(str);
}
static void showI64(const int64_t value, const char name[const]) {
    char str[MAX_CHARACTERS_64];

    sprintf(str, "%lli", value);

    defField(name, "(int64)");
    whiteLabel(str);
}
static void showTriangle(const Triangle triangle, const char name[const]) {
    defRow(4);
    defLabel(name);

    for (int x = 0; x < 3; x++) displayU32(triangle[x]);
}
static void makeGui() {
    selectedArray = wishArray;

    makeMenus();

    if (selectedArray.array) {
	if (openList()) {
	    size_t numElements;

	    if (selectedArray.numElements || !selectedArray.sizeNum) {
		switch (selectedArray.sizeNum) {
		case sizeof(uint64_t):
		    numElements = *(uint64_t*)selectedArray.numElements;
		    break;
		case sizeof(uint32_t):
		    numElements = *(uint32_t*)selectedArray.numElements;
		    break;
		case sizeof(uint16_t):
		    numElements = *(uint16_t*)selectedArray.numElements;
		    break;
		case sizeof(uint8_t):
		    numElements = *(uint8_t*)selectedArray.numElements;
		    break;
		default:
		    numElements = 1;
		}
	    }
	    else numElements = selectedArray.sizeNum;

	    for (size_t i = 0; i < numElements; i++) {
		char name[MAX_CHARACTERS_64];

		sprintf(name, "%llu", i);

		switch (selectedArray.type) {
		case DGH_REGION_ARRAY:
		    DGH_FIELDNAME((const Region*)selectedArray.array + i, name);
		    break;
		case DGH_U32_ARRAY:
		    DGH_FIELDNAME(((uint32_t*)selectedArray.array)[i], name);
		    break;
		case DGH_U16_ARRAY:
		    DGH_FIELDNAME(((uint16_t*)selectedArray.array)[i], name);
		    break;
		case DGH_U8_ARRAY:
		    DGH_FIELDNAME(((uint8_t*)selectedArray.array)[i], name);
		    break;
		case DGH_I64_ARRAY:
		    showI64(((const int64_t*)selectedArray.array)[i], name);
		    break;
		case DGH_INSTANCE_ARRAY:
		    DGH_FIELDNAME((const Instance*)selectedArray.array + i, name);
		    break;
		case DGH_MESH_ARRAY:
		    DGH_FIELDNAME((const Mesh*)selectedArray.array + i, name);
		    break;
		case DGH_BONE_ARRAY:
		    DGH_FIELDNAME((const Bone*)selectedArray.array + i, name);
		    break;
		case DGH_ANIMATION_ARRAY:
		    DGH_FIELDNAME((const Animation*)selectedArray.array + i, name);
		    break;
		case DGH_VEC3_ARRAY:
		    DGH_Vec3(((vec3*)selectedArray.array)[i], name);
		    break;
		case DGH_TRIANGLE_ARRAY:
		    showTriangle(((Triangle*)selectedArray.array)[i], name);
		    break;
		case DGH_PIPDYNAMIC_ARRAY:
		    DGH_FIELDNAME(&((PipDynamic*)selectedArray.array)[i].base, name);
		    break;
		case DGH_CHARACTERPTR_ARRAY:
		    DGH_PTRNAME(((Character**)selectedArray.array)[i], name);
		    break;
		case DGH_CHARACTER_ARRAY:
		    DGH_FIELDNAME((Character*)selectedArray.array + i, name);
		    break;
		case DGH_ANIMATIONTRACK_ARRAY:
		    DGH_FIELDNAME((AnimationTrack*)selectedArray.array + i, name);
		}
	    }
	}
	else if (nk_window_is_hidden(nkc, selectedArray.name)) wishArray.array = NULL; //invalidate

	nk_end(nkc);
    }

    /*
    //MAKE_MENU(IH, "Physics Simulation");
    MAKE_MENU(NH, "Navigation Handler");
    MAKE_MENU(R, "Render");
    MAKE_MENU(CH, "Characters Handler");
    */
}
static void render() {
    const int maxVertexBuffer = 512 * 1024, maxElementBuffer = 128 * 1024;

    nk_sdl_render(NK_ANTI_ALIASING_ON, maxVertexBuffer, maxElementBuffer);
}

void DGH_Init(SDL_Window* const window) {
    nkc = nk_sdl_init(window);

    nkc->style.selectable.text_normal = nkc->style.selectable.text_normal_active = NK_WHITE;
    nkc->style.selectable.text_hover = nkc->style.selectable.text_hover_active = NK_WHITE;
    nkc->style.selectable.text_pressed = nkc->style.selectable.text_pressed_active = NK_WHITE;
    nkc->style.window.spacing.y = 0;

    initSelectablesActiveStyle();
    initFont();

    //some preparations
    DGH_Draw();

    nk_window_collapse(nkc, "Window Handler", NK_MINIMIZED);
    nk_window_collapse(nkc, "Path Handler", NK_MINIMIZED);
    nk_window_collapse(nkc, "Instances Handler", NK_MINIMIZED);
    nk_window_collapse(nkc, "Navigation Handler", NK_MINIMIZED);
    nk_window_collapse(nkc, "Render", NK_MINIMIZED);
    nk_window_collapse(nkc, "Characters Handler", NK_MINIMIZED);

    /*
    MakeDebugGui_Prepare(nkc);
    MakeDebugGui_Collapse(nkc);
    */
}
void DGH_InputBegin() {
    nk_input_begin(nkc);
}
void DGH_HandleEvent(SDL_Event* const event) {
    /*const float scale = .5f;

    switch (event->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        event->button.x *= scale;
        event->button.y *= scale;

        break;
    case SDL_EVENT_MOUSE_MOTION:
        event->motion.x *= scale;
        event->motion.y *= scale;

        break;
    }*/

    nk_sdl_handle_event(event);
}
void DGH_InputEnd() {
    nk_input_end(nkc);
}
void DGH_Draw() {
    makeGui();
    render();
}

void DGH_ArrayT(const voidArray* const array) {
    DGH_FIELD(array->numElements);
    DGH_FIELD(array->sizeInElements);
}
void DGH_Array(const ArrayView* const view) {
    const char firstStr[] = "See ";

    const int addressSize = 20;

    const int strSize = (int)(sizeof(firstStr) + strlen(view->name) + addressSize);

    char str[strSize];

    sprintf(str, "%s%s(0x%p)", firstStr, view->name, view->array);

    const bool isNameSame = selectedArray.name == view->name;

    defRow(1);

    const bool isSame = wishArray.array && isNameSame && wishArray.numElements == view->numElements;

    if (nk_button_text(nkc, str, strSize - 1) || isSame) wishArray = *view;
}
int DGH_Begin(const char name[const], const size_t num) {
    const float unknownMagic = 60;

    nk_layout_row_dynamic(nkc, ((float)num * (nkc->style.window.spacing.y + (float)ROW_HEIGHT)) + unknownMagic, 1);

    return nk_group_begin(nkc, name, NK_WINDOW_BORDER + NK_WINDOW_TITLE);
}
void DGH_End() {
    nk_group_end(nkc);
}

void DGH_U64(const uint64_t value, const char name[const]) {
    char str[MAX_CHARACTERS_64];

    sprintf(str, "%llu", value);

    defField(name, "(uint64)");
    whiteLabel(str);
}
void DGH_U32(const uint32_t value, const char name[const]) {
    defField(name, "(uint32)");
    displayU32(value);
}
void DGH_U16(const uint16_t value, const char name[const]) {
    const size_t maxChars = 6;

    char str[maxChars];

    sprintf(str, "%u", value);

    defField(name, "(uint16)");
    whiteLabel(str);
}
void DGH_U8(const uint8_t value, const char name[const]) {
    const size_t maxChars = 4;

    char str[maxChars];

    sprintf(str, "%u", value);

    defField(name, "(uint8)");
    whiteLabel(str);
}
void DGH_Float(const float value, const char name[const]) {
    defField(name, "(float)");
    displayFloat(value);
}
void DGH_Bool(bool value, const char name[]) {
    nkc->style.checkbox.cursor_hover.data.color = nkc->style.checkbox.cursor_normal.data.color = NK_WHITE;
    nkc->style.checkbox.hover.data.color = nkc->style.checkbox.normal.data.color = MAKE_GRAY(55);
    
    defField(name, "(bool)");
    nk_check_text(nkc, NULL, 0, value);
}
void DGH_Arena(const Arena* const arena, const char name[const]) {
    const size_t hard = 6;

    if (openGroup(name, hard)) {
	DGH_FIELD(arena->size);

	const struct nk_rect content = nk_window_get_content_region(nkc);

	const float yPos = content.y + (ROW_HEIGHT * 2);

	const float coof = content.w / (float)arena->size;

	struct nk_command_buffer* const canvas = nk_window_get_canvas(nkc);

	defRow(1);
	defLabel("Free regions:");

	nk_fill_rect(canvas, nk_rect(content.x, yPos, content.w, ROW_HEIGHT), 0, MAKE_GRAY(64));

	for (size_t i = 0; i < arena->freeRegions.numElements; i++) {
	    const Region* const freeLocation = arena->freeRegions.elements + i;

	    nk_fill_rect(canvas, nk_rect(
		content.x + ((float)freeLocation->position * coof), 
		yPos, 
		(float)freeLocation->size * coof,
		ROW_HEIGHT
	    ), 0, NK_WHITE);
	}

	//const RegionArray* const fr = &arena->freeRegions;
	
	defRow(1);

	DGH_ARRAYT(arena->freeRegions);

	nk_group_end(nkc);
    }
}
void DGH_Mat4(const mat4 mat, const char name[const]) {
    const int matrixDimensions = sizeof(mat4) / sizeof(vec4);

    if (DGH_Begin(name, matrixDimensions)) {
	for (int x = 0; x < matrixDimensions; x++) {
	    defRow(matrixDimensions);

	    for (int y = 0; y < matrixDimensions; y++) displayFloat(mat[x][y]);
	}

	DGH_End();
    }
}
void DGH_Vec3(const float* const vec, const char name[const]) {
    defRow(4);
    defLabel(name);
    
    for (int x = 0; x < 3; x++) displayFloat(vec[x]);
}
void DGH_Vec2(const float* vec, const char name[]) {
    defRow(3);
    defLabel(name);
    
    for (int x = 0; x < 2; x++) displayFloat(vec[x]);
}
void DGH_String(const char value[const restrict], const char name[const restrict]) {
    defField(name, "(char*)");

    if (value) whiteLabel(value);
    else defLabel("NULL");
}
