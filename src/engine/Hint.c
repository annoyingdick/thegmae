#include <string.h>
#include "Hint.h"

float Hint_GetExpireTime(const Hint hint) {
    const float charsPerSecond = 13 * 2;

    return (float)strlen(hint) / charsPerSecond;
}
