#pragma once
#include <string.h>
#include <stdlib.h>

static inline void _ultoaddr(unsigned long value, char *p, char *buffer)
{
    int temp;
    memset(p, '0', 8);
    _ultoa(value, buffer, 16);
    temp = strlen(buffer);
    memcpy(p + 8 - temp, buffer, temp);
}
static inline void ulltoaddr(unsigned long long value, char *p, char *buffer)
{
    int temp;
    memset(p, '0', 16);
    ulltoa(value, buffer, 16);
    temp = strlen(buffer);
    memcpy(p + 16 - temp, buffer, temp);
}
