#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "def.h"

//This factor can be used for memory leaks detection! Enter an extremely
//large number like 10000 and just see if the program's memory usage on a real-time graph (Process Hacker for ex.) is growing.
//Our boomer, skinny jeans, virgin moustache, large glasses grandpas used to do this at least...
#define ALLOC_FACTOR 1

static void throwAllocError(
    const size_t size, const size_t lastTitleSize, const char lastTitle[], const char fileName[], const char funcName[]
) {
    const uint8_t charsToRepresentU64 = 20;

    const char sepStr[] = " : ";
    const char firstMsgStr[] = "The application failed to request memory from the OS. Requested size: ";
    const char lastMsgStr[] = " bytes";

    char title[strlen(fileName) + sizeof(sepStr) - 1 + strlen(funcName) + sizeof(sepStr) - 1 + lastTitleSize];
    char msg[sizeof(firstMsgStr) - 1 + charsToRepresentU64 + sizeof(lastMsgStr)];

    sprintf(title, "%s%s%s%s%s", fileName, sepStr, funcName, sepStr, lastTitle);
    sprintf(msg, "%s%llu%s", firstMsgStr, size, lastMsgStr);

    throwFatal(title, msg);
}

void* mallocdebug(const size_t size, const char fileName[const restrict], const char funcName[const restrict]) {
    void* const mem = malloc(size * ALLOC_FACTOR);

    if (!mem) {
	const char lastTitle[] = "Malloc error occurred!";

	throwAllocError(size, sizeof(lastTitle), lastTitle, fileName, funcName);
    }

    return mem;
}
void* reallocdebug(
    void* const exmem, const size_t size, const char fileName[const restrict], const char funcName[const restrict]
) {
    void* const mem = realloc(exmem, size * ALLOC_FACTOR);

    if (!mem) {
	const char lastTitle[] = "Realloc error occurred!";

	throwAllocError(size, sizeof(lastTitle), lastTitle, fileName, funcName);
    }

    return mem;
}
void* callocdebug(
    const size_t num, const size_t size, const char fileName[const restrict], const char funcName[const restrict]
) {
    void* const mem = calloc(num, size * ALLOC_FACTOR);

    if (!mem) {
	const char lastTitle[] = "Calloc error occurred!";

	throwAllocError(num * size, sizeof(lastTitle), lastTitle, fileName, funcName);
    }

    return mem;
}
