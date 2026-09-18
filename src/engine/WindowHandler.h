#ifndef WindowHandler_h_
#define WindowHandler_h_

#include <stdint.h>
#include <stdbool.h>

#define NANOSECONDS_IN_ONE_SECOND 1000000000
#define FIXED_LOOP_DELTA_TIME_NS 15625000

#define FIXED_LOOP_DELTA_TIME_S ((float)FIXED_LOOP_DELTA_TIME_NS / NANOSECONDS_IN_ONE_SECOND)

#define TO_NDC(x) x[0] = (x[0] * 2) - 1; x[1] = 1 - (x[1] * 2)

typedef uint64_t Time;
typedef uint8_t WHLoopResultCode;

//Normalized screen coordinates (0 to 1, 0;0 is top-left)
typedef float NSC[2];
//Normalized device coordinates from OpenGL (-1 to 1, -1;1 is top-left)
typedef float NDC[2];

typedef struct {
    float interp;
    WHLoopResultCode code;
} WHLoopResult;

enum {
    WH_LOOP_RESULT_NOTHING,
    WH_LOOP_RESULT_QUIT,
    WH_LOOP_RESULT_DO_FIXED_LOOP //should always be last
};

void WH_Init();
void WH_R_PostInit();
void WH_GetWindowSize(float* width, float* height);
float WH_GetDeltaTime();
bool WH_IsEditorOn();
void WH_ThrowError(const char title[], const char message[]);
void WH_Quit();
WHLoopResult WH_Loop();
void WH_R_PostLoop();

void WH_DrawDebugGui();

#endif
