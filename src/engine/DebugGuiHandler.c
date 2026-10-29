#include "nuklearHandler.h"

static struct nk_context* nkc;

static void initAtlas() {
    struct nk_font_atlas* atlas;

    nk_glfw3_font_stash_begin(&atlas);
}

void DGH_Init(GLFWwindow* const window) {
    const int maxVertex = 512 * 1024, maxElement = 128 * 1024;

    nkc = nk_glfw3_init(window, NK_GLFW3_DEFAULT, maxVertex, maxElement);

    initAtlas();
    nk_glfw3_font_stash_end();
}
void DGH_Loop() {
    nk_glfw3_new_frame();

    if (nk_begin(nkc, "Demo", nk_rect(50, 50, 230, 250),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {

	}
    nk_end(nkc);

    nk_glfw3_render(NK_ANTI_ALIASING_OFF);
}
