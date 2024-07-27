#pragma once
#include <windows.h>

DWORD GetModuleSize(HANDLE hFile)
{
    DWORD Size;
    HANDLE hMapping;
    LPVOID lpBaseAddress;
    hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    lpBaseAddress = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)lpBaseAddress;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)lpBaseAddress + dosHeader->e_lfanew);
    Size = ntHeaders->OptionalHeader.SizeOfImage;
    UnmapViewOfFile(lpBaseAddress);
    CloseHandle(hMapping);
    return Size;
}
