#pragma once
#include <string.h>
#include <stdlib.h>

#if defined(__GNUC__) || defined(__clang__)
#if defined(_DEBUG) && !defined(__OPTIMIZE__)
__attribute__((no_stack_protector, nothrow))
#else
__attribute__((always_inline, flatten, no_stack_protector, nothrow))
#endif
#endif

static inline void _ultoaddr(unsigned long value, char *p, char *buffer)
{
    int temp;
    memset(p, '0', 8);
    _ultoa(value, buffer, 16);
    temp = strlen(buffer);
    memcpy(p + 8 - temp, buffer, temp);
}

#if defined(__GNUC__) || defined(__clang__)
#if defined(_DEBUG) && !defined(__OPTIMIZE__)
__attribute__((no_stack_protector, nothrow))
#else
__attribute__((always_inline, flatten, no_stack_protector, nothrow))
#endif
#endif

static inline void ulltoaddr(unsigned long long value, char *p, char *buffer)
{
    int temp;
    memset(p, '0', 16);
    ulltoa(value, buffer, 16);
    temp = strlen(buffer);
    memcpy(p + 16 - temp, buffer, temp);
}
