#pragma once
#include <stdint.h>

#if !defined(_DEBUG) || defined(__OPTIMIZE__)

#if defined(__GNUC__) || defined(__clang__)
#ifndef likely
#define likely(x)      __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)    __builtin_expect(!!(x), 0)
#endif
#else
#ifndef likely
#define likely(x)      (x)
#endif
#ifndef unlikely
#define unlikely(x)    (x)
#endif
#define __attribute__(...)
#endif

#ifdef __powerpc__
typedef enum {
    B_ADD, B_SUB, B_MUL, B_DIV, B_NEG, B_ABS, B_SQRT
} op_t;
#elif defined(__i386__)
typedef unsigned long int __attribute__ ((__may_alias__)) op_t;
#else
typedef unsigned long long int __attribute__ ((__may_alias__)) op_t;
#endif
#define ALIGN_DOWN(base, size)    ((base) & -((__typeof__ (base)) (size)))
#define ALIGN_UP(base, size)    ALIGN_DOWN ((base) + (size) - 1, (size))
#define PTR_ALIGN_DOWN(base, size) \
    ((__typeof__ (base)) ALIGN_DOWN ((uintptr_t) (base), (size)))
#define PTR_ALIGN_UP(base, size) \
    ((__typeof__ (base)) ALIGN_UP ((uintptr_t) (base), (size)))
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __PDP_ENDIAN    3412
#if defined(_M_X86) && (defined(__AARCH64EB__) || defined(__arc__) || defined(__ARMEB__) || defined(__hppa__) || defined(__MIPSEB) || defined(__nios2_big_endian__) || defined(__m68k__) || defined(__microblaze__) || defined(__or1k__) || defined(__powerpc__) || defined(__s390__) || defined(__sh__))
# define __BYTE_ORDER __BIG_ENDIAN
#else
# define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#ifdef __powerpc__
static inline __attribute__((always_inline, no_stack_protector, nothrow)) op_t
find_eq_all (op_t x1, op_t x2)
{
    return __builtin_cmpb (x1, x2);
}
static inline __attribute__((always_inline, no_stack_protector, nothrow)) op_t
find_zero_all (op_t x)
{
    return find_eq_all (x, 0);
}
#elif defined(__riscv_zbb) || defined(__riscv_xtheadbb)
static inline __attribute__((always_inline, no_stack_protector, nothrow)) op_t
find_zero_all (op_t x)
{
    find_t r;
#ifdef __riscv_xtheadbb
    asm ("th.tstnbz %0, %1" : "=r" (r) : "r" (x));
    return r;
#else
    asm ("orc.b %0, %1" : "=r" (r) : "r" (x));
    return ~r;
#endif
}
static inline __attribute__((always_inline, no_stack_protector, nothrow)) op_t
find_eq_all (op_t x1, op_t x2)
{
    return find_zero_all (x1 ^ x2);
}
#else
#if defined(__alpha)
static inline __attribute__((always_inline, no_stack_protector, nothrow)) op_t
find_zero_all (op_t x)
{
    return __builtin_alpha_cmpbge (0, x);
}
#elif defined(__ARM__)
static inline __attribute__((always_inline, no_stack_protector, nothrow)) op_t
find_zero_all (op_t x)
{
    op_t ret;
    op_t ones = ((op_t)-1 / 0xff) * 0x01;
    asm ("uqsub8 %0,%1,%2" : "=r"(ret) : "r"(ones), "r"(x));
    return ret;
}
#else
static inline __attribute__((always_inline, no_stack_protector, nothrow)) op_t
find_zero_all (op_t x)
{
    op_t m = ((op_t)-1 / 0xff) * 0x7f;
    return ~(((x & m) + m) | x | m);
}
#endif
static inline __attribute__((always_inline, no_stack_protector, nothrow)) op_t
find_eq_all (op_t x1, op_t x2)
{
    return find_zero_all (x1 ^ x2);
}
#endif
#ifdef __alpha
static inline __attribute__((always_inline, no_stack_protector, nothrow)) op_t
shift_find_last (op_t word, uintptr_t s)
{
    s = s % sizeof (op_t);
    if (s == 0)
        return word;
    return word & ~((op_t)-1 << s);
}
#else
static inline __attribute__((always_inline, no_stack_protector, nothrow)) op_t
shift_find_last (op_t word, uintptr_t s)
{
    s = s % sizeof (op_t);
    if (s == 0)
        return word;
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
        return word & ~(((op_t)-1) << (s * 8));
    else
        return word & ~(((op_t)-1) >> (s * 8));
}
#endif
#ifdef __alpha
static inline __attribute__((always_inline, no_stack_protector, nothrow)) unsigned int
index_last (op_t x)
{
#ifdef __alpha_cix__
    return __builtin_clzl (x) ^ 63;
#else
    unsigned r = 0;
    if (x & 0xf0)
        r += 4;
    if (x & (0xc << r))
        r += 2;
    if (x & (0x2 << r))
        ++r;
    return r;
#endif
}
#elif defined(__hppa__)
static inline __attribute__((always_inline, no_stack_protector, nothrow)) unsigned int
index_last (find_t c)
{
    unsigned int ret;
    asm ("extrw,u,= %1,15,8,%%r0\n\t"
        "ldi 1,%0\n\t"
        "extrw,u,= %1,23,8,%%r0\n\t"
        "ldi 2,%0\n\t"
        "extrw,u,= %1,31,8,%%r0\n\t"
        "ldi 3,%0"
        : "=r"(ret) : "r"(c), "0"(0));
    return ret;
}
#elif defined(__riscv)
static inline __attribute__((always_inline, no_stack_protector, nothrow)) unsigned int
index_last (find_t c)
{
    if (sizeof (op_t) == 8)
    {
        if (c & 0x8000000000000000UL)
            return 7;
        else if (c & 0x80000000000000UL)
            return 6;
        else if (c & 0x800000000000UL)
            return 5;
        else if (c & 0x8000000000UL)
            return 4;
    }
    if (c & 0x80000000U)
        return 3;
    else if (c & 0x800000U)
        return 2;
    else if (c & 0x8000U)
        return 1;
    return 0;
}
#else
#include "simd.h"
static inline __attribute__((always_inline, no_stack_protector, nothrow)) unsigned int
index_last (op_t c)
{
    int r;
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
        r = clz (c);
    else
        r = ctz (c);
    return sizeof (op_t) - 1 - (r >> 3);
}
#endif
#ifdef __hppa__
static inline __attribute__((always_inline, no_stack_protector, nothrow)) unsigned int
index_last_zero (op_t x)
{
    unsigned int ret;
    asm ("extrw,u,<> %1,15,8,%%r0\n\t"
        "ldi 1,%0\n\t"
        "extrw,u,<> %1,23,8,%%r0\n\t"
        "ldi 2,%0\n\t"
        "extrw,u,<> %1,31,8,%%r0\n\t"
        "ldi 3,%0"
        : "=r"(ret) : "r"(x), "0"(0));
    return ret;
}
#else
static inline __attribute__((always_inline, no_stack_protector, nothrow)) op_t
find_zero_low (op_t x)
{
    op_t lsb = ((op_t)-1 / 0xff) * 0x01;
    op_t msb = ((op_t)-1 / 0xff) * 0x80;
    return (x - lsb) & ~x & msb;
}
static inline __attribute__((always_inline, no_stack_protector, nothrow)) unsigned int
index_last_zero (op_t x)
{
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
        x = find_zero_all (x);
    else
        x = find_zero_low (x);
    return index_last (x);
}
#endif
#ifdef __alpha
static inline __attribute__((always_inline, no_stack_protector, nothrow)) _Bool
has_zero (op_t x)
{
    return __builtin_alpha_cmpbge (0, x) != 0;
}
static inline __attribute__((always_inline, no_stack_protector, nothrow)) _Bool
has_eq (op_t x1, op_t x2)
{
    return has_zero (x1 ^ x2);
}
#elif defined(__hppa__)
static inline __attribute__((always_inline, no_stack_protector, nothrow)) _Bool
has_eq (op_t x1, op_t x2)
{
    asm goto ("uxor,sbz %0,%1,%%r0\n\t"
        "b,n %l2" : : "r"(x1), "r"(x2) : : nbz);
    return 1;
  nbz:
    return 0;
}
#elif defined(__sh__)
static inline __attribute__((always_inline, no_stack_protector, nothrow)) _Bool
has_eq (op_t x1, op_t x2)
{
    int ret;
    asm("cmp/str %1,%2\n\t"
        "movt %0"
        : "=r" (ret) : "r" (x1), "r" (x2) : "t");
    return ret;
}
#else
static inline __attribute__((always_inline, no_stack_protector, nothrow)) _Bool
has_zero (op_t x)
{
    return find_zero_low (x) != 0;
}
static inline __attribute__((always_inline, no_stack_protector, nothrow)) _Bool
has_eq (op_t x1, op_t x2)
{
    return find_eq_all (x1, x2) != 0;
}
#endif
static inline __attribute__((always_inline, no_stack_protector, nothrow)) unsigned int
index_last_eq (op_t x1, op_t x2)
{
    return index_last_zero (x1 ^ x2);
}
static inline __attribute__((always_inline, no_stack_protector, nothrow)) void *
memrchr (const void *s, int c_in, size_t n)
{
    if (unlikely (n == 0))
        return NULL;
    const op_t *word_ptr = (const op_t *) PTR_ALIGN_UP (s + n, sizeof (op_t));
    uintptr_t s_int = (uintptr_t) s + n;
    op_t word = *--word_ptr;
    op_t repeated_c = ((op_t)-1 / 0xff) * c_in;
    const op_t *sword = (const op_t *) PTR_ALIGN_DOWN (s, sizeof (op_t));
    op_t mask = shift_find_last (find_eq_all (word, repeated_c), s_int);
    if (mask != 0)
    {
        char *ret = (char *) word_ptr + index_last (mask);
        return ret >= (char *) s ? ret : NULL;
    }
    if (word_ptr == sword)
        return NULL;
    word = *--word_ptr;
    while (word_ptr != sword)
    {
        if (has_eq (word, repeated_c))
            return (char *) word_ptr + index_last_eq (word, repeated_c);
        word = *--word_ptr;
    }
    if (has_eq (word, repeated_c))
    {
        char *ret = (char *) word_ptr + index_last_eq (word, repeated_c);
        if (ret >= (char *) s)
            return ret;
    }
    return NULL;
}

#endif