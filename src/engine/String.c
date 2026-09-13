#include "String.h"
#include "def.h"

void String_Init(String* const string) {
    string->length = 0;
    string->size = 2;

    string->cstring = callocd(string->size, sizeof(*string->cstring));
}
void String_InitWithData(String* string, const char data[const]) {
    string->length = (int)strlen(data);
    string->size = string->length + 2;

    string->cstring = mallocd(string->size);

    strcpy(string->cstring, data);
}
void String_Loop(String* const string) {
    //because we don't count null
    if (string->length == string->size - 1) {
	string->size += string->size / 2;

	string->cstring = reallocd(string->cstring, string->size);
    }
}
