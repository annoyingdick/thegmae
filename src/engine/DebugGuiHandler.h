#ifndef DebugGuiHandler_h_
#define DebugGuiHandler_h_

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include "Arena.h"
#include "Mesh.h"
#include "Geometry.h"
#include "InstancesHandler.h"
#include "Pip.h"
#include "PipDynamic.h"
#include "Character.h"

#define DGH_FIELDNAME(x, n) _Generic(x, \
    uint64_t: DGH_U64, \
    uint32_t: DGH_U32, \
    uint16_t: DGH_U16, \
    uint8_t: DGH_U8, \
    float: DGH_Float, \
    bool: DGH_Bool, \
    Arena*: DGH_Arena, const Arena*: DGH_Arena, \
    Mesh*: Mesh_DrawDebugGui, const Mesh*: Mesh_DrawDebugGui, \
    const Region*: Region_DrawDebugGui, \
    const Animation*: Animation_DrawDebugGui, \
    const Instance*: Instance_DrawDebugGui, \
    const Bone*: Bone_DrawDebugGui, \
    Geometry*: Geometry_DrawDebugGui, \
    Character*: Character_DrawDebugGui, \
    Pip*: Pip_DrawDebugGui, \
    AnimationTrack*: AnimationTrack_DrawDebugGui, \
    vec4*: DGH_Mat4, const vec4*: DGH_Mat4, \
    char*: DGH_String \
)(x, n)

#define DGH_FIELD(x) DGH_FIELDNAME(x, #x":")

#define DGH_ARRAYTYPE(x) _Generic(x, \
    Region*: DGH_REGION_ARRAY, \
    uint32_t*: DGH_U32_ARRAY, \
    uint16_t*: DGH_U16_ARRAY, \
    uint8_t*: DGH_U8_ARRAY, \
    const int64_t*: DGH_I64_ARRAY, \
    Instance*: DGH_INSTANCE_ARRAY, \
    Mesh*: DGH_MESH_ARRAY, \
    Bone*: DGH_BONE_ARRAY, \
    Animation*: DGH_ANIMATION_ARRAY, const Animation*: DGH_ANIMATION_ARRAY, \
    vec3*: DGH_VEC3_ARRAY, \
    Triangle*: DGH_TRIANGLE_ARRAY, \
    PipDynamic*: DGH_PIPDYNAMIC_ARRAY, \
    Character*: DGH_CHARACTER_ARRAY, \
    Character**: DGH_CHARACTERPTR_ARRAY, \
    const AnimationTrack*: DGH_ANIMATIONTRACK_ARRAY \
)

#define DGH_ARRAY(x, n) DGH_Array(&(ArrayView){ \
    .sizeNum = sizeof(n), \
    .type = DGH_ARRAYTYPE(x), \
    .array = x, \
    .numElements = &n, \
    .name = #x \
})

#define DGH_PTR(x) DGH_Array(&(ArrayView){ \
    .sizeNum = 0, \
    .type = DGH_ARRAYTYPE(x), \
    .array = x, \
    .numElements = NULL, \
    .name = #x \
})

#define DGH_PTRNAME(x, n) DGH_Array(&(ArrayView){ \
    .sizeNum = 0, \
    .type = DGH_ARRAYTYPE(x), \
    .array = x, \
    .numElements = NULL, \
    .name = n \
})

#define DGH_ARRAYN(x) DGH_Array(&(ArrayView){ \
    .sizeNum = ARRAYSIZE(x), \
    .type = DGH_ARRAYTYPE(x), \
    .array = x, \
    .numElements = NULL, \
    .name = #x \
})

#define DGH_STRUCTFUNC(x) void x##_DrawDebugGui(const x* obj, const char name[]);
#define DGH_BEGIN(x, nam, num) void x##_DrawDebugGui(const x* const nam, const char name[const]) { if (DGH_Begin(name, num))
#define DGH_END DGH_End(); }

#define DGH_ARRAYT(a) DGH_ArrayT((voidArray*)&a); DGH_ARRAY(a.elements, a.numElements)

DECLARE_ARRAY_TYPEDEF(void)

typedef enum {
    DGH_REGION_ARRAY,
    DGH_U32_ARRAY,
    DGH_U16_ARRAY,
    DGH_U8_ARRAY,
    DGH_I64_ARRAY,
    DGH_INSTANCE_ARRAY,
    DGH_MESH_ARRAY,
    DGH_BONE_ARRAY,
    DGH_ANIMATION_ARRAY,
    DGH_VEC3_ARRAY,
    DGH_TRIANGLE_ARRAY,
    DGH_PIPDYNAMIC_ARRAY,
    DGH_CHARACTERPTR_ARRAY,
    DGH_CHARACTER_ARRAY,
    DGH_ANIMATIONTRACK_ARRAY
} ArrayType;

typedef struct {
    unsigned int sizeNum;
    ArrayType type;

    const void* array, *numElements;
    const char* name;
} ArrayView;

void DGH_Init(SDL_Window* window);
void DGH_InputBegin();
void DGH_HandleEvent(SDL_Event* event);
void DGH_InputEnd();
void DGH_Draw();

void DGH_ArrayT(const voidArray* array);
void DGH_Array(const ArrayView* view);
int DGH_Begin(const char name[], size_t num);
void DGH_End();

void DGH_U64(uint64_t value, const char name[]);
void DGH_U32(uint32_t value, const char name[]);
void DGH_U16(uint16_t value, const char name[]);
void DGH_U8(uint8_t value, const char name[]);
void DGH_Float(float value, const char name[]);
void DGH_Bool(bool value, const char name[]);
void DGH_Arena(const Arena* arena, const char name[]);
void DGH_Mat4(const mat4 mat, const char name[]);
void DGH_Vec3(const float* vec, const char name[]);
void DGH_Vec2(const float* vec, const char name[]);
void DGH_String(const char value[], const char name[]);

#endif
