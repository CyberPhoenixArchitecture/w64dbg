#pragma once
#include <string.h>

#ifdef __SSE__
#if defined(__GNUC__) || defined(__clang__)
#undef memcpy
#undef strcpy
#define memcpy(_Dst, _Src, _Size) \
({ \
    if (__builtin_constant_p(_Size) && _Size <= 8) \
        for (unsigned int _I = 0; _I < _Size; ++_I) \
            ((char *) _Dst)[_I] = ((const char *) _Src)[_I]; \
    else __builtin_memcpy(_Dst, _Src, _Size); \
    _Dst; \
})
#define memset(_Str, _C, _N) \
({ \
    if (__builtin_constant_p(_N) && _Size <= 8) \
        for (unsigned int _I = 0; _I < _N; ++_I) \
            ((char *) _Str)[_I] = _C; \
    else __builtin_memset(_Str, _C, _N); \
    _Str; \
})
#define strcpy(_Dst, _Src) \
({ \
    if (__builtin_constant_p(_Src)) \
    { \
        memcpy(_Dst, _Src, strlen(_Src)); \
        (_Dst)[strlen(_Src)] = '\0'; \
    } else __builtin_strcpy(_Dst, _Src); \
    _Dst; \
})
#elif defined(_MSC_VER) && defined(__cplusplus)
#include <type_traits>
using namespace std;
[[nodiscard]]
[[msvc::flatten]]
[[msvc::no_tls_guard]]
static inline void *quark_memcpy(void *_Dst, void *_Src, size_t _Size)
{
    if (__is_constant_evaluated() && _Size <= 8)
        for (unsigned int _I = 0; _I < _N; ++_I)
            ((char *) _Dst)[_I] = ((const char *) _Src)[_I];
    else memcpy(_Dst, _Src, _Size);
    return _Dst;
}
[[nodiscard]]
[[msvc::flatten]]
[[msvc::no_tls_guard]]
static inline void *quark_memset(void *_Str, int _C, size_t _N)
{
    if (__is_constant_evaluated() && _Size <= 8)
        for (unsigned int _I = 0; _I < _N; ++_I)
            ((char *) _Str)[_I] = _C; \
    else memset(_Str, _C, _N);
    return _Str;
}
[[nodiscard]]
[[msvc::flatten]]
[[msvc::no_tls_guard]]
static inline void *quark_strcpy(void *_Dst, void *_Src)
{
    if (__is_constant_evaluated())
        quark_memcpy(_Dst, _Src, strlen(_Src));
        (_Dst)[strlen(_Src)] = '\0';
    else strcpy(_Dst, _Src);
    return _Dst;
}
#undef memcpy
#undef strcpy
#define memcpy(_Dst, _Src, _Size) quark_memcpy(_Dst, _Src, _Size)
#define memset(_Str, _C, _N) quark_memset(_Str, _C, _N)
#define strcpy(_Dst, _Src, _Size) quark_strcpy(_Dst, _Src, _Size)
#endif
#endif
