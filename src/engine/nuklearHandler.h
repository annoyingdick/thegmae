#ifndef nuklearHandler_h_
#define nuklearHandler_h_

#include "def.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define STBTT_malloc(x, u) ((void)(u), mallocd(x))
#define STBTT_free(x, u) ((void)(u), free(x))

#include <nuklear.h>
#include <nuklear_sdl_gl3.h>

#endif
