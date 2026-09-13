#ifndef debugdef_h_
#define debugdef_h_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "WindowHandler.h" // IWYU pragma: keep

#define DEBUG

#ifdef DEBUG
#define DEBUG_MESSAGES
#endif

#define FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#ifdef DEBUG_MESSAGES
#define printfd(...) printf(__FILE_NAME__" : " __VA_ARGS__)
#else
#define printfd(...)
#endif

#define BENCHMARK_BEGIN struct timespec _beg, _end; timespec_get(&_beg, TIME_UTC);
#define BENCHMARK_END \
timespec_get(&_end, TIME_UTC); \
printf("%llu nanoseconds\n", ((_end.tv_sec - _beg.tv_sec) * 1000000000) + (_end.tv_nsec - _beg.tv_nsec));

//i decided to hardcode this instead of using typeof which is a very new feature from c23 and 
//therefore it may not be widely supported
//Use for unsigned variables like: int foo; VARMAX(foo)
#define MAXT(t) (sizeof(t) == 8 ? UINT64_MAX : (sizeof(t) == 4 ? UINT32_MAX : (sizeof(t) == 2 ? UINT16_MAX : UINT8_MAX)))

#define INVALIDATE(x) x = -1
#define ISINVALID(x) (x == MAXT(x))

#define sqr(x) (x * x)

#define ARRAYSIZE(a) (sizeof(a) / sizeof(*a))

#define mallocarr(a, num) a = mallocd((num) * sizeof(*a))
#define callocarr(a, num) a = callocd(num, sizeof(*a))
#define reallocarr(a, num) a = reallocd(a, (num) * sizeof(*a))

//C needs it to be a native operator! I hope we'll see it in C29...
/*
#define foreach(item, array, num) for (size_t _i = 0, _keep = 1; _i < num; _i++, _keep = !_keep) \
for (item = array + _i; _keep; _keep = !_keep)
*/

#define foreach(item, array, num) for (size_t _i = 0; _i < num; _i++) { item = array + _i;
#define nforeach(item, array) for (size_t _i = 0; _i < ARRAYSIZE(array); _i++) { item = array + _i;
#define forend }

#define mallocd(size) mallocdebug(size, __FILE_NAME__, __func__)
#define reallocd(exmem, size) reallocdebug(exmem, size, __FILE_NAME__, __func__)
#define callocd(num, size) callocdebug(num, size, __FILE_NAME__, __func__)

//WH_ThrowError can be called even before the initalization of WH and SDL
#define throwFatal(title, message) WH_ThrowError(title, message)

void* mallocdebug(size_t size, const char fileName[], const char funcName[]);
void* reallocdebug(void* exmem, size_t size, const char fileName[], const char funcName[]);
void* callocdebug(size_t num, size_t size, const char fileName[], const char funcName[]);

#endif
