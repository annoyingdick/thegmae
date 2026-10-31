#include <stdlib.h>
#include "def.h"
#include "Render.h"
#include "Camera.h"
#include "game/GameMain.h"
#include "GUI.h"
#include "WindowHandler.h"
#include "DebugGuiHandler.h"

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900

#define SPEEDUP_FACTOR 1

//this is dumb but good enough
#define THROWERROR(t, m) if (!GUI_ThrowError(t, m)) fprintf(stderr, "The gmae has crashed!\n%s\n%s", t, m); \
    R_Loop(0); WH_R_PostLoop(); while (!glfwWindowShouldClose(window)) { glfwWaitEvents(); } exit(EXIT_FAILURE)

static Time currentTime, currentTimeFixed, deltaTime;
static int vsync = true;
static double mousex, mousey;
static bool isMouseFree = true;

static GLFWwindow* window;

static WHLoopResultCode pollEvents() {
    if (glfwWindowShouldClose(window)) return WH_LOOP_RESULT_QUIT;

    glfwPollEvents();

    return WH_LOOP_RESULT_NOTHING;
}
static WHLoopResult doTimeOperations() {
    const Time newTime = glfwGetTime();

    const Time fixedLoopAccum = newTime - currentTimeFixed;

    Time interp;
    WHLoopResultCode code;

    interp = fixedLoopAccum * FIXED_LOOP_HZ;
    code = WH_LOOP_RESULT_NOTHING;

    if (fixedLoopAccum >= 1. / FIXED_LOOP_HZ) {
	const int iterations = (int)interp;

	interp -= (float)iterations;
	code = WH_LOOP_RESULT_DO_FIXED_LOOP + iterations - 1;

	currentTimeFixed = newTime - fixedLoopAccum + (iterations / FIXED_LOOP_HZ);
    }

    deltaTime = newTime - currentTime;
    currentTime = newTime;

    return (WHLoopResult){
	.interp = (float)interp,
	.code = code
    };
}
static void glfwCallback(const int error, const char description[const]) {
    if (error) throwFatal("GLFW error occurred!", description);
}
static void resizeCallback(GLFWwindow* const window, const int width, const int height) {
    if (window) {}

    R_SetViewportSize(width, height);
    GUI_UpdateTexts();
}
static void keyCallback(GLFWwindow* const window, const int key, const int scancode, const int action, const int mods) {
    if (scancode && mods) {}

    switch (key) {
    case GLFW_KEY_ESCAPE:
	glfwSetWindowShouldClose(window, GLFW_TRUE);
	
	return;
    case GLFW_KEY_V:
	if (action == GLFW_PRESS) glfwSwapInterval(vsync = !vsync);
    }

    switch (action) {
    case GLFW_PRESS:
	GM_KeyDown(key);
	break;
    case GLFW_RELEASE:
	GM_KeyUp(key);
    }
}
static void mouse3() {
    float width, height;
    double x, y;

    WH_GetWindowSize(&width, &height);
    glfwGetCursorPos(window, &x, &y);

    GM_Mouse3((NSC){(float)x / width, (float)y / height});
}
static void mouseButtonCallback(GLFWwindow* const window, const int button, const int action, const int mods) {
    if (window && mods) {}

    if (action == GLFW_PRESS) {
	switch (button) {
	case GLFW_MOUSE_BUTTON_MIDDLE:
	    isMouseFree = false;

	    return;
	case GLFW_MOUSE_BUTTON_RIGHT:
	    mouse3();
	}
    }
    else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_MIDDLE) isMouseFree = true;
}
static void scrollCallback(GLFWwindow* const window, const double xoffset, const double yoffset) {
    if (xoffset) {}

    float width, height;
    double x, y;

    WH_GetWindowSize(&width, &height);
    glfwGetCursorPos(window, &x, &y);

    const float minS = width < height ? width : height;

    Camera_MouseWheel((float)yoffset, ((float)x - width / 2) / minS, ((float)y - height / 2) / minS);
}
static void mouseMoveCallback(GLFWwindow* const window, const double xpos, const double ypos) {
    if (window) {}

    if (!isMouseFree) Camera_MouseMotion((float)(xpos - mousex), (float)(ypos - mousey));

    mousex = xpos;
    mousey = ypos;
}
static void createWindow() {
    if (!(window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "THEGMAE", NULL, NULL))) {
	throwFatal("GLFW error occurred!", "Failed to create a window and OpenGL context!");
    }
}
static void setHints() {
    const int values[] = {
	//WINDOW
	GLFW_RESIZABLE, GLFW_TRUE,
	GLFW_VISIBLE, GLFW_TRUE,
	GLFW_DECORATED, GLFW_TRUE,
	GLFW_FOCUSED, GLFW_TRUE,
	GLFW_AUTO_ICONIFY, GLFW_FALSE,
	GLFW_FLOATING, GLFW_FALSE,
	GLFW_MAXIMIZED, GLFW_FALSE,
	GLFW_CENTER_CURSOR, GLFW_FALSE,
	GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE,
	GLFW_FOCUS_ON_SHOW, GLFW_FALSE,
	GLFW_SCALE_TO_MONITOR,  GLFW_FALSE,
	GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE,
	GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE,
	GLFW_POSITION_X, GLFW_ANY_POSITION,
	GLFW_POSITION_Y, GLFW_ANY_POSITION,
	//FRAMEBUFFER
	GLFW_RED_BITS, GLFW_DONT_CARE,
	GLFW_GREEN_BITS, GLFW_DONT_CARE,
	GLFW_BLUE_BITS, GLFW_DONT_CARE,
	GLFW_ALPHA_BITS, GLFW_DONT_CARE,
	GLFW_DEPTH_BITS, GLFW_DONT_CARE,
	GLFW_STENCIL_BITS, GLFW_DONT_CARE,
	//deprecated bruh
	GLFW_ACCUM_RED_BITS, GLFW_DONT_CARE,
	GLFW_ACCUM_GREEN_BITS, GLFW_DONT_CARE,
	GLFW_ACCUM_BLUE_BITS, GLFW_DONT_CARE,
	GLFW_ACCUM_ALPHA_BITS, GLFW_DONT_CARE,
	GLFW_AUX_BUFFERS, GLFW_DONT_CARE,
	//not so deprecated
	GLFW_STEREO, GLFW_FALSE,
	GLFW_SAMPLES, 0,
	GLFW_SRGB_CAPABLE, GLFW_FALSE,
	GLFW_DOUBLEBUFFER, GLFW_TRUE,
	//MONITOR
	GLFW_REFRESH_RATE, GLFW_DONT_CARE,
	//CONTEXT
	GLFW_CLIENT_API, GLFW_OPENGL_API,
	GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API,
	GLFW_CONTEXT_VERSION_MAJOR, 4,
	GLFW_CONTEXT_VERSION_MINOR, 6,
	GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE, //what kind of devil u should be to deprecate glLineWidth???
	GLFW_CONTEXT_DEBUG,
#ifdef DEBUG
	GLFW_TRUE
#else
	GLFW_FALSE
#endif
	,
	GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE,
	GLFW_CONTEXT_ROBUSTNESS,
#ifdef DEBUG
	GLFW_LOSE_CONTEXT_ON_RESET
#else
	GLFW_NO_ROBUSTNESS
#endif
	,
	GLFW_CONTEXT_RELEASE_BEHAVIOR, GLFW_RELEASE_BEHAVIOR_FLUSH,
	GLFW_CONTEXT_NO_ERROR,
#ifdef DEBUG
	GLFW_FALSE
#else
	GLFW_TRUE
#endif
    };

    for (size_t i = 0; i < ARRAYSIZE(values); i += 2) glfwWindowHint(values[i], values[i + 1]);
}

void WH_Init() {
    if (!glfwInit()) throwFatal("GLFW error occurred!", "Failed to initialize the library");

    glfwSetErrorCallback(glfwCallback);

    setHints();
    createWindow();

    glfwSetWindowSizeCallback(window, resizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(vsync);

    //please god forgive me for this cursed variable name :P
    /*
    for (int minor = lastGLMinorVersion; minor > 4; minor--) {
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);

	if ((glContext = SDL_GL_CreateContext(window))) break;
	
	if (minor == lastGLMinorVersion) {
	    printfd("OpenGL version 4.%i is not supported on this device, trying other versions...\n", minor);
	}
	else printfd("Version 4.%i: Not supported\n", minor);
    }
    */

    //SDL_CHECK(SDL_StartTextInput(window));
}
void WH_R_PostInit() {
#ifdef DEBUG
    DGH_Init(window);
#endif
}
float WH_GetWindowHeight() {
    float w, h;

    WH_GetWindowSize(&w, &h);

    return h;
}
void WH_GetWindowSize(float* const width, float* const height) {
    int w, h;

    WH_GetWindowSizeInt(&w, &h);

    *width = (float)w;
    *height = (float)h;
}
void WH_GetWindowSizeInt(int* const width, int* const height) {
    glfwGetWindowSize(window, width, height);
}
float WH_GetWindowInvRatio() {
    float w, h;

    WH_GetWindowSize(&w, &h);

    return h / w;
}
float WH_GetDeltaTime() {
    return (float)deltaTime * SPEEDUP_FACTOR;
}
bool WH_IsEditorOn() {
    return false;
}
void WH_ThrowError(const char title[const], const char message[const]) {
    THROWERROR(title, message);
}
void WH_Quit() {
    glfwTerminate();
}
WHLoopResult WH_Loop() {
    const WHLoopResultCode code = pollEvents();

    if (code) return (WHLoopResult){
	.interp = 0,
	.code = code
    };

    return doTimeOperations();
}
void WH_R_PostLoop() {
#ifdef DEBUG
    DGH_Loop();
#endif

    glfwSwapBuffers(window);
}
