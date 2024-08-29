#include <string.h>

#if defined(__OPTIMIZE__) && defined(__has_builtin)
#undef memcpy
#undef strcpy
#if __has_builtin(__builtin_constant_p)
#define memcpy(_Dst, _Src, _Size) \
({ \
    if (__builtin_constant_p(_Src) || \
        __builtin_constant_p(_Size)) \
    { \
        char *_Dst = \
            (char *) __builtin_assume_aligned(_Dst, _Size << 3); \
        const char *_Src = \
            (const char *) __builtin_assume_aligned(_Src, _Size << 3); \
        for (unsigned int i = 0; i < _Size; ++i) \
            _Dst[i] = _Src[i]; \
    } \
    else __builtin_memcpy(_Dst, _Src, _Size); \
    _Dst; \
})
#define strcpy(_Dst, _Src) \
({ \
    if (__builtin_constant_p(_Src)) \
        memcpy(_Dst, _Src, strlen(_Src)); \
    else __builtin_strcpy(_Dst, _Src, _Size); \
    _Dst; \
})
#else
#define memcpy(_Dst, _Src, _Size) \
({ \
    if (__is_constant_evaluated(_Src) || \
        __is_constant_evaluated(_Size)) \
    { \
        char *_Dst = \
            (char *) assume_aligned<_Size << 3>(_Dst); \
        const char *_Src = \
            (const char *) assume_aligned<_Size << 3>(_Src); \
        for (unsigned int i = 0; i < _Size; ++i) \
            _Dst[i] = _Src[i]; \
    } \
    else memcpy(_Dst, _Src, _Size); \
    _Dst; \
})
#define strcpy(_Dst, _Src) \
({ \
    if (__is_constant_evaluated(_Src)) \
        memcpy(_Dst, _Src, strlen(_Src)); \
    else strcpy(_Dst, _Src, _Size); \
    _Dst; \
})
#endif
#endif
