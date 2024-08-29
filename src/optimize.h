#include <string.h>

#ifdef __OPTIMIZE__

#undef memcpy
#undef strcpy

#ifdef __GNUC__
#define memcpy(_Dst, _Src, _Size) \
({ \
    if (__builtin_constant_p(_Src) || \
        __builtin_constant_p(_Size)) \
        for (unsigned int i = 0; i < _Size; ++i) \
            ((char *) __builtin_assume_aligned(_Dst, _Size << 3))[i] = \
                ((const char *) __builtin_assume_aligned(_Src, _Size << 3))[i]; \
    else __builtin_memcpy(_Dst, _Src, _Size); \
    _Dst; \
})
#define strcpy(_Dst, _Src) \
({ \
    if (__builtin_constant_p(_Src)) \
        memcpy(_Dst, _Src, strlen(_Src)); \
    else __builtin_strcpy(_Dst, _Src); \
    _Dst; \
})
#elif defined(__cplusplus)
#include <type_traits>
using namespace std;
#define memcpy(_Dst, _Src, _Size) \
({ \
    if (__is_constant_evaluated()) \
        for (unsigned int i = 0; i < _Size; ++i) \
            ((char *) _Dst)[i] = ((const char *) _Src)[i]; \
    else memcpy(_Dst, _Src, _Size); \
    _Dst; \
})
#define strcpy(_Dst, _Src) \
({ \
    if (__is_constant_evaluated()) \
        memcpy(_Dst, _Src, strlen(_Src)); \
    else strcpy(_Dst, _Src); \
    _Dst; \
})
#endif
