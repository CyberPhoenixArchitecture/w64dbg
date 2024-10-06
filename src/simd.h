#pragma once
#ifndef BUILTIN_H
#define BUILTIN_H

#if defined(__GNUC__) || defined(__clang__)
#ifdef __cplusplus
#undef ffs
template <typename T>
static inline constexpr int ffs(T x)
{
    if (sizeof(typeof(x)) == 8) return __builtin_ffsll(x);
    else return __builtin_ffs(x);
}
template <typename T>
static inline constexpr int clz(T x)
{
    if (sizeof(typeof(x)) == 8) return __builtin_clzll(x);
    else return __builtin_clz(x);
}
template <typename T>
static inline constexpr int ctz(T x)
{
    if (sizeof(typeof(x)) == 8) return __builtin_ctzll(x);
    else return __builtin_ctz(x);
}
template <typename T>
static inline constexpr int clrsb(T x)
{
    if (sizeof(typeof(x)) == 8) return __builtin_clrsbll(x);
    else return __builtin_clrsb(x);
}
template <typename T>
static inline constexpr int popcount(T x)
{
    if (sizeof(typeof(x)) == 8) return __builtin_popcountll(x);
    else return __builtin_popcount(x);
}
template <typename T>
static inline constexpr int parity(T x)
{
    if (sizeof(typeof(x)) == 8) return __builtin_parityll(x);
    else return __builtin_parity(x);
}
#else
#undef ffs
#define ffs(x) _Generic((x), \
    long long: __builtin_ffsll, \
    unsigned long long: __builtin_ffsll, \
    default: __builtin_ffs \
)(x)
#define clz(x) _Generic((x), \
    long long: __builtin_clzll, \
    unsigned long long: __builtin_clzll, \
    default: __builtin_clz \
)(x)
#define ctz(x) _Generic((x), \
    long long: __builtin_ctzll, \
    unsigned long long: __builtin_ctzll, \
    default: __builtin_ctz \
)(x)
#define clrsb(x) _Generic((x), \
    long long: __builtin_clrsbll, \
    unsigned long long: __builtin_clrsbll, \
    default: __builtin_clrsb \
)(x)
#define popcount(x) _Generic((x), \
    long long: __builtin_popcountll, \
    unsigned long long: __builtin_popcountll, \
    default: __builtin_popcount \
)(x)
#define parity(x) _Generic((x), \
    long long: __builtin_parityll, \
    unsigned long long: __builtin_parityll, \
    default: __builtin_parity \
)(x)
#endif
#else
#ifdef __cplusplus
#undef ffs
template <typename T>
static inline constexpr int ffs(T x)
{
    if (sizeof(typeof(x)) == 8) return _BitScanForward64(x) + 1;
    else return _BitScanForward(x) + 1;
}
template <typename T>
static inline constexpr int clz(T x)
{
    if (sizeof(typeof(x)) == 8) return __lzcnt64(x); //_lzcnt_u64
    else if (sizeof(typeof(x)) == 4) return __lzcnt(x); //_lzcnt_u32
    else return __lzcnt16(x);
}
template <typename T>
static inline constexpr int ctz(T x)
{
    if (sizeof(typeof(x)) == 8) 64 - _BitScanForward64(x);
    else return sizeof(typeof(x)) * 8 - _BitScanForward(x);
}
template <typename T>
static inline constexpr int clrsb(T x)
{
    if (sizeof(typeof(x)) == 8) return __lzcnt64(~x);
    else if (sizeof(typeof(x)) == 4) return __lzcnt(~x);
    else return __lzcnt16(~x);
}
template <typename T>
static inline constexpr int popcount(T x)
{
    if (sizeof(typeof(x)) == 8) return _mm_popcnt_u64(x);
    else return _mm_popcnt_u32(x);
}
template <typename T>
static inline constexpr int parity(T x)
{
    if (sizeof(typeof(x)) == 8) return _mm_popcnt_u64(x) & 1;
    else return _mm_popcnt_u32(x) & 1;
}
#else
#undef ffs
#define ffs(x) _Generic((x), \
    long long: _BitScanForward64, \
    unsigned long long: _BitScanForward64, \
    default: _BitScanForward \
)(x) + 1
#define clz(x) _Generic((x), \
    short: __lzcnt16, \
    unsigned short: __lzcnt16, \
    int: __lzcnt, \
    unsigned int: __lzcnt, \
    long: __lzcnt, \
    unsigned long: __lzcnt, \
    long long: __lzcnt64, \
    unsigned long long: __lzcnt64 \
)(x)
#define ctz(x) sizeof(x) * 8 - _Generic((x), \
    long long: _BitScanForward64, \
    unsigned long long: _BitScanForward64, \
    default: _BitScanForward \
)(x)
#define clrsb(x) _Generic((x), \
    short: __lzcnt16, \
    unsigned short: __lzcnt16, \
    int: __lzcnt, \
    unsigned int: __lzcnt, \
    long: __lzcnt, \
    unsigned long: __lzcnt, \
    long long: __lzcnt64, \
    unsigned long long: __lzcnt64 \
)(~x)
#define popcount(x) _Generic((x), \
    long long: _mm_popcnt_u64, \
    unsigned long long: _mm_popcnt_u64, \
    default: _mm_popcnt_u32 \
)(x)
#define parity(x) _Generic((x), \
    long long: _mm_popcnt_u64, \
    unsigned long long: _mm_popcnt_u64, \
    default: _mm_popcnt_u32 \
)(x) & 1
#endif
#endif

#endif