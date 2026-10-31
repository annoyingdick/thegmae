#include "NavigationHandler.h"
#include "Pip.h"
#include "nuklearHandler.h"

#define WINDOW_HEIGHT 900

static ShaderProgram linesProgram;
static GPUBuffer linesVerticesBuffer, linesIndicesBuffer;
static nk_bool showNavmesh;

static struct nk_context* nkc;

static nk_bool openMenu() {
    const float y = 500, w = 325, h = 200;

    return nk_begin(
	nkc, "The cheerful menu of joy and happiness!!1!", nk_rect(0, y, w, h),
	NK_WINDOW_BORDER + NK_WINDOW_MOVABLE + NK_WINDOW_SCALABLE + NK_WINDOW_MINIMIZABLE + NK_WINDOW_TITLE
    );
}
static void initContext(GLFWwindow* const window) {
    const int maxVertex = 512 * 1024, maxElement = 128 * 1024;

    nkc = nk_glfw3_init(window, NK_GLFW3_DEFAULT, maxVertex, maxElement);
}
static void initAtlas() {
    struct nk_font_atlas* atlas;

    nk_glfw3_font_stash_begin(&atlas);
}
static void newFrame() {
    nk_glfw3_new_frame(WINDOW_HEIGHT / WH_GetWindowHeight(), WINDOW_HEIGHT);
}
static void render() {
    float w, h;

    WH_GetWindowSize(&w, &h);

    nk_glfw3_render(NK_ANTI_ALIASING_ON, WINDOW_HEIGHT * w / h, WINDOW_HEIGHT);
}
static void initLines() {
    const Geometry* const navgeo = NH_GetGeometry();

    GPUBuffer_InitWithData(
	&linesVerticesBuffer, navgeo->numVertices * (GLsizeiptr)sizeof(*navgeo->vertices), 
	navgeo->vertices
    );
    GPUBuffer_InitWithData(
	&linesIndicesBuffer, navgeo->numTriangles * (GLsizeiptr)sizeof(*navgeo->triangles), 
	navgeo->triangles
    );
}

void DGH_Init(GLFWwindow* const window) {
    initContext(window);
    initAtlas();

    nk_glfw3_font_stash_end();

    ShaderProgram_Init_VF(
	&linesProgram, (ShaderProgramInitInfo_VF){"lines.vert", "lines.frag"}
    );

    initLines();
}
void DGH_Loop() {
    newFrame();

    if (openMenu()) {
	const float row = 20;

	nk_layout_row_dynamic(nkc, row, 1);
	nk_checkbox_label(nkc, "Show navmesh", &showNavmesh);
    }
    nk_end(nkc);

    if (showNavmesh) {
	ShaderProgram_Use(linesProgram);

	GPUBuffer_BindBase(linesVerticesBuffer, GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_VERTICES);
	GPUBuffer_Bind(linesIndicesBuffer, GL_ELEMENT_ARRAY_BUFFER);

	glDepthFunc(GL_ALWAYS);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, (GLsizei)NH_GetGeometry()->numTriangles * 3, GL_UNSIGNED_INT, NULL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    render();
}
