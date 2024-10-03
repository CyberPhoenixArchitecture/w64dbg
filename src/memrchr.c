#include <string.h>
#include <limits.h>

/* Nonzero if X is not aligned on a "long" boundary.  */
#ifdef __i386__
#define unaligned(X) ((unsigned long)(X) & (sizeof(long) - 1))
#else
#define unaligned(X) ((unsigned long long)(X) & (sizeof(long) - 1))
#endif

/* How many bytes are loaded each iteration of the word copy loop.  */
#define LBLOCKSIZE (sizeof (long))

/* Threshhold for punting to the bytewise iterator.  */
#define TOO_SMALL(LEN)  ((LEN) < LBLOCKSIZE)

#if LONG_MAX == 2147483647L
#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)
#else
#if LONG_MAX == 9223372036854775807L
/* Nonzero if X (a long int) contains a NULL byte. */
#define DETECTNULL(X) (((X) - 0x0101010101010101) & ~(X) & 0x8080808080808080)
#else
#error long int is not a 32bit or 64bit type.
#endif
#endif

#ifndef DETECTNULL
#error long int is not a 32bit or 64bit byte
#endif

/* DETECTCHAR returns nonzero if (long)X contains the byte used
   to fill (long)MASK. */
#define DETECTCHAR(X,MASK) (DETECTNULL(X ^ MASK))

#if defined(__GNUC__) || defined(__clang__)
__attribute__((access(read_only, 1), always_inline, no_stack_protector, nonnull(1), nothrow, pure))
#endif

static inline void *memrchr(const void *src_void, int c, size_t length)
{
    if (!length) return NULL;
#ifdef _MSC_VER
    const unsigned char * __restrict src = (const unsigned char * __restrict)
#else
    const unsigned char * __restrict__ src = (const unsigned char * __restrict__)
#endif
    src_void + length - 1;
    while (unaligned(src))
    {
        if (!length)
            return 0;
        if (*src == c)
            return (void *) src;
        --length;
        --src;
    }
    unsigned long mask;
    unsigned long *asrc;
    if (length >= LBLOCKSIZE)
    {
        asrc = (unsigned long *) (src - LBLOCKSIZE + 1);
        mask = c << 8 | c;
        mask = mask << 16 | mask;
        while (length >= LBLOCKSIZE)
        {
            if (DETECTCHAR(*asrc, mask))
                break;
            length -= LBLOCKSIZE;
            --asrc;
        }
        src = (unsigned char *) asrc + LBLOCKSIZE - 1;
    }
    while (length)
    {
        if (*src == c)
            return (void *) src;
        --length;
        --src;
    }
    return 0;
}
