#include <stdlib.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_messagebox.h>
#include "def.h"
#include "Render.h"
#include "Camera.h"
#include "game/GameMain.h"
#include "editor/EditorMain.h"
#include "DebugGuiHandler.h"
#include "WindowHandler.h"

#define THROWERROR(t, m) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, t, m, window); exit(EXIT_FAILURE);

#define SDL_CHECK(x) do {if (!(x)) { \
    THROWERROR("SDL error occurred!", SDL_GetError()); \
}} while (0)

static Time currentTime, currentTimeFixed, deltaTime;
static bool isMouseFree;

static SDL_GLContext glContext;
static SDL_Window* window;

static WHLoopResultCode pollEvents() {
    SDL_Event event;

    int interval;

    while (SDL_PollEvent(&event)) {
	switch (event.type) {
	case SDL_EVENT_QUIT:
	    return WH_LOOP_RESULT_QUIT;
	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
	    R_SetViewportSize(event.window.data1, event.window.data2);

	    break;
	case SDL_EVENT_KEY_DOWN:
	    //hardcoded *literally* escape button
	    switch (event.key.key) {
	    case SDLK_ESCAPE:
		return WH_LOOP_RESULT_QUIT;
	    case SDLK_V:
		SDL_GL_GetSwapInterval(&interval);

		SDL_CHECK(SDL_GL_SetSwapInterval(!interval));

		break;
	    default:
		GM_KeyDown(event.key.key);
		EM_KeyDown(event.key.key);
	    }

	    break;
	case SDL_EVENT_KEY_UP:
	    GM_KeyUp(event.key.key);

	    break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	    switch (event.button.button) {
	    case 2:
		isMouseFree = false;

		break;
	    case 3:
		if(0){}

		int width, height;

		SDL_GetWindowSizeInPixels(window, &width, &height);

		GM_Mouse3((NSC){event.button.x / (float)width, event.button.y / (float)height});
	    }

	    break;
	case SDL_EVENT_MOUSE_BUTTON_UP:
	    if (event.button.button == 2) isMouseFree = true;

	    break;
	case SDL_EVENT_MOUSE_WHEEL:
	    if (true) {} // a fucking fuck named clangd says that, i cannot declare vars at here...

	    int width, height;

	    SDL_GetWindowSizeInPixels(window, &width, &height);
	
	    const float minS = (float)(width < height ? width : height);

	    Camera_MouseWheel(
		event.wheel.y, 
		(event.wheel.mouse_x - (float)width / 2) / minS, 
		(event.wheel.mouse_y - (float)height / 2) / minS
	    );

	    break;
	case SDL_EVENT_MOUSE_MOTION:
	    if (!isMouseFree) {
		Camera_MouseMotion(event.motion.xrel, event.motion.yrel);
	    }

	    EM_MouseMotion(event.motion.xrel, event.motion.yrel);

	    break;
	}

	if (isMouseFree) {
	    DGH_HandleEvent(&event);
	}
    }

    return WH_LOOP_RESULT_NOTHING;
}
static void createWindow() {
    const int defaultWindowWidth = 1600, defaultWindowHeight = 900;

    SDL_CHECK(window = SDL_CreateWindow(
	"Game", defaultWindowWidth, defaultWindowHeight, SDL_WINDOW_OPENGL + SDL_WINDOW_RESIZABLE
    ));
}
static WHLoopResult doTimeOperations() {
    const Time newTime = SDL_GetTicksNS();

    const Time fixedLoopAccum = newTime - currentTimeFixed;

    float interp;
    WHLoopResultCode code;

    interp = (float)fixedLoopAccum / FIXED_LOOP_DELTA_TIME_NS;
    code = WH_LOOP_RESULT_NOTHING;

    if (fixedLoopAccum >= FIXED_LOOP_DELTA_TIME_NS) {
	const uint8_t iterations = fixedLoopAccum / FIXED_LOOP_DELTA_TIME_NS;

	interp -= (float)iterations;
	code = WH_LOOP_RESULT_DO_FIXED_LOOP + iterations - 1;

	currentTimeFixed = newTime - fixedLoopAccum + ((Time)iterations * FIXED_LOOP_DELTA_TIME_NS);
    }

    deltaTime = newTime - currentTime;
    currentTime = newTime;

    return (WHLoopResult){
	.interp = interp,
	.code = code
    };
}

void WH_Init() {
    const int lastGLMinorVersion = 6;

    GM_SetMetadata();

    SDL_Init(SDL_INIT_VIDEO);

    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE));
    SDL_CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4));
    SDL_CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG));

    createWindow();

    //please god forgive me for this cursed variable name :P
    for (int minor = lastGLMinorVersion; minor > 4; minor--) {
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);

	if ((glContext = SDL_GL_CreateContext(window))) break;
	
	if (minor == lastGLMinorVersion) {
	    printfd("OpenGL version 4.%i is not supported on this device, trying other versions...\n", minor);
	}
	else printfd("Version 4.%i: Not supported\n", minor);
    }

    if (!glContext) {
	throwFatal("GPU error occurred!", 
	    "This GPU/driver doesn't support minimum required OpenGL version 4.5\n"
	    "If you have other GPUs on your system, try to switch to one of them\n"
	    "either via system settings or NVIDIA/AMD control panel."
	);
    }

    SDL_CHECK(SDL_GL_MakeCurrent(window, glContext));
    SDL_CHECK(SDL_GL_SetSwapInterval(1));

    SDL_CHECK(SDL_StartTextInput(window));

    isMouseFree = true;
}
void WH_R_PostInit() {
    DGH_Init(window);
}
void WH_GetWindowSize(float* const width, float* const height) {
    int w, h;

    SDL_GetWindowSizeInPixels(window, &w, &h);

    *width = (float)w;
    *height = (float)h;
}
float WH_GetDeltaTime() {
    return (float)deltaTime / NANOSECONDS_IN_ONE_SECOND;
}
bool WH_IsEditorOn() {
    return false;
}
void WH_ThrowError(const char title[], const char message[]) {
    THROWERROR(title, message);
}
void WH_Quit() {
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
}
WHLoopResult WH_Loop() {
    DGH_InputBegin();

    const WHLoopResultCode code = pollEvents();

    DGH_InputEnd();

    if (code) return (WHLoopResult){
	.interp = 0,
	.code = code
    };

    return doTimeOperations();
}
void WH_R_PostLoop() {
    DGH_Draw();

    SDL_CHECK(SDL_GL_SwapWindow(window));
}

void WH_DrawDebugGui() {
    DGH_FIELD(currentTime);
    DGH_FIELD(currentTimeFixed);
    DGH_FIELD(deltaTime);
    DGH_FIELD(isMouseFree);
}
