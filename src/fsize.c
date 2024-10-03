#pragma once
#include <windows.h>

#if defined(__GNUC__) || defined(__clang__)
#if defined(_DEBUG) && !defined(__OPTIMIZE__)
__attribute__((no_stack_protector, nothrow))
#else
__attribute__((always_inline, flatten, no_stack_protector, nothrow))
#endif
#endif

static inline DWORD GetModuleSize(HANDLE hFile)
{
    DWORD Size;
    HANDLE hMapping;
    LPVOID lpBaseAddress;
    PIMAGE_DOS_HEADER dosHeader;
    PIMAGE_NT_HEADERS ntHeaders;
    hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    lpBaseAddress = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    dosHeader = (PIMAGE_DOS_HEADER)lpBaseAddress;
    ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)lpBaseAddress + dosHeader->e_lfanew);
    Size = ntHeaders->OptionalHeader.SizeOfImage;
    UnmapViewOfFile(lpBaseAddress);
    CloseHandle(hMapping);
    return Size;
}
