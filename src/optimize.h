#include <string.h>

#if defined(__OPTIMIZE__) && defined(__has_attribute) && __has_builtin(__builtin_constant_p)
#undef memcpy
#if __has_builtin(__builtin_memcpy)
#define memcpy(_Dst, _Src, _Size) \
({ \
    if (__builtin_constant_p(_Src) || \
        __builtin_constant_p(_Size)) \
        for (unsigned int i = 0; i < _Size; ++i) \
            (_Dst)[i] = (_Src)[i]; \
    else __builtin_memcpy(_Dst, _Src, _Size); \
    _Dst; \
})
#else
#define memcpy(_Dst, _Src, _Size) \
({ \
    if (__builtin_constant_p(_Src) || \
        __builtin_constant_p(_Size)) \
        for (unsigned int i = 0; i < _Size; ++i) \
            (_Dst)[i] = (_Src)[i]; \
    else memcpy(_Dst, _Src, _Size); \
    _Dst; \
})
#endif
#if __has_builtin(__builtin_strlen)
#define strlen(...) __builtin_strlen(__VA_ARGS__)
#endif
#if __has_builtin(__builtin_memchr)
#define memchr(...) __builtin_memchr(__VA_ARGS__)
#endif
#if __has_builtin(__builtin_memcmp)
#define memcmp(...) __builtin_memcmp(__VA_ARGS__)
#endif
#if __has_builtin(__builtin_strstr)
#define strstr(...) __builtin_strstr(__VA_ARGS__)
#endif
#endif
