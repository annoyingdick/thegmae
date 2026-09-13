#ifndef Array_h_
#define Array_h_

#include "def.h"

#define ARRAY_INITIAL_SIZE 2

typedef size_t ArrayIndex;

#define DECLARE_ARRAY_TYPEDEF(type) typedef struct { \
    type* elements; ArrayIndex numElements, sizeInElements; \
} type##Array;

#define ARRAY_NEW_ELEMENT_RESIZE(array) if (++array->numElements > array->sizeInElements) { \
    array->sizeInElements += array->sizeInElements / 2; \
    reallocarr(array->elements, array->sizeInElements); \
}

#define aforeach(item, array) for (ArrayIndex _i = 0; _i < (array)->numElements; _i++) { item = (array)->elements + _i;

#define DECLARE_ARRAY_IMPL(type) \
void type##Array_Init(type##Array* const array) { \
    mallocarr(array->elements, ARRAY_INITIAL_SIZE); \
    array->numElements = 0; \
    array->sizeInElements = ARRAY_INITIAL_SIZE; \
} \
bool type##Array_IsEmpty(const type##Array* const array) { return !array->numElements; } \
type* type##Array_GetLastElement(const type##Array* const array) { return array->elements + array->numElements - 1; } \
void type##Array_RemoveElement(type##Array* const array, const size_t id) { \
    --array->numElements; \
    for (size_t i = id; i < array->numElements; i++) memcpy(array->elements + i, array->elements + i + 1, sizeof(type)); \
} \
void type##Array_InsertElement(type##Array* const array, const type* const element, const size_t id) { \
    ARRAY_NEW_ELEMENT_RESIZE(array) \
    for (size_t i = array->numElements - 1; i > id; i--) memcpy(array->elements + i, array->elements + i - 1, sizeof(type)); \
    memcpy(array->elements + id, element, sizeof(type)); \
} \
ArrayIndex type##Array_AppendElement(type##Array* array, const type* element) { \
    ARRAY_NEW_ELEMENT_RESIZE(array) \
    memcpy(array->elements + array->numElements - 1, element, sizeof(type)); \
    return array->numElements - 1; \
} \
void type##Array_Destroy(const type##Array* const array) { free(array->elements); }

#define DECLARE_ARRAY(type) \
void type##Array_Init(type##Array* array); \
bool type##Array_IsEmpty(const type##Array* array); \
type* type##Array_GetLastElement(const type##Array* array); \
void type##Array_RemoveElement(type##Array* array, size_t id); \
void type##Array_InsertElement(type##Array* array, const type* element, size_t id); \
ArrayIndex type##Array_AppendElement(type##Array* array, const type* element); \
void type##Array_Destroy(const type##Array* array);

#endif
