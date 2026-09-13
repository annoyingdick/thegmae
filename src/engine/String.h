#ifndef String_h_
#define String_h_

#include <stdint.h>

typedef struct {
    int length, size;
    char* cstring;
} String;

void String_Init(String* string);
void String_InitWithData(String* string, const char data[]);
void String_Loop(String* string);

#endif
