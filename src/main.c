#define ANSI
#define _ANSI
#undef UNICODE
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <psapi.h>
#include <dbghelp.h>
#include "addr.c"
#include "exception.c"
#include "fsize.c"
#include "memrchr.c"
#include "optimize.h"

#define MAX_THREAD 64
#define MAX_DLL 16
#define GNU 2
#define LATENCY 99

#define SymOptions (SYMOPT_IGNORE_CVREC | SYMOPT_IGNORE_IMAGEDIR | SYMOPT_LOAD_ANYTHING | SYMOPT_NO_IMAGE_SEARCH | SYMOPT_DEFERRED_LOADS | SYMOPT_DISABLE_SYMSRV_AUTODETECT | SYMOPT_AUTO_PUBLICS | SYMOPT_INCLUDE_32BIT_MODULES)
#define NDebugSymOptions (SymOptions | SYMOPT_NO_CPP)
#define _DebugSymOptions (SymOptions | SYMOPT_LOAD_LINES)

#define GCXX_RUNTIME_EXCEPTION 541541187

#if defined(__GNUC__) || defined(__clang__)
#if defined(_DEBUG) && !defined(__OPTIMIZE__)
__attribute__((no_stack_protector, nothrow))
#else
__attribute__((leaf, no_stack_protector, nothrow))
#endif
#endif

VOID WINAPI CompletedWriteRoutine(DWORD dwErr, DWORD cbWritten, LPOVERLAPPED lpOverLap) {}

#if defined(__GNUC__) || defined(__clang__)
#if defined(_DEBUG) && !defined(__OPTIMIZE__)
__attribute__((access(read_only, 2), no_stack_protector, nothrow))
#else
__attribute__((access(read_only, 2), flatten, no_stack_protector, nothrow, simd))
#endif
#endif

#ifdef _M_ARM64
WINBASEAPI WINBOOL WINAPI IsWow64Process2 (HANDLE hProcess, USHORT *pProcessMachine, USHORT *pNativeMachine);
typedef BOOL (WINAPI * PFN_GETPROCESSINFORMATION)(HANDLE, PROCESS_INFORMATION_CLASS, LPVOID, DWORD);
#endif

int main(int argc, char *argv[])
{
    HANDLE hStderr;
    char buffer[65536];
    OVERLAPPED Overlapped = {};
    int temp, i, timeout = 0, debug = FALSE;
    BOOL breakpoint = FALSE, firstbreak = FALSE,
    output = FALSE,vexception = TRUE, verbose = FALSE, start = FALSE;
    Overlapped.Pointer = NULL;
    hStderr = GetStdHandle(STD_ERROR_HANDLE);
    for (i = 1; i < argc; ++i)
        if (argv[i][0] == '/')
        {
            if (argv[i][2] == '\0') switch(argv[i][1])
            {
                case 'B':
                    breakpoint = TRUE;
                    continue;
                case 'D':
                    debug = TRUE;
                    continue;
                case 'G':
                    debug = GNU;
                    continue;
                case 'Q':
                    vexception = FALSE;
                    continue;
                case 'O':
                    output = FALSE;
                    continue;
                case 'S':
                    start = TRUE;
                    continue;
                case 'T':
                    if (++i >= argc)
                    {
                        WriteFileEx(hStderr,
                            "ERROR: Invalid syntax. Value expected for '/T'\n"
                            "Type \"debug /?\" for usage.\n",
                            74, &Overlapped,
                            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                        return 1;
                    } else if ((timeout = atol(argv[i])) > 99999 || timeout < -1)
                    {
                        WriteFileEx(hStderr,
                            "ERROR: Invalid value for timeout (/T) specified. Valid range is -1 to 99999.\n",
                            76, &Overlapped,
                            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                        return 1;
                    }
                    continue;
                case 'V':
                    verbose = TRUE;
                    continue;
                case '?':
                    WriteFileEx(hStderr,
                        "Usage: debug [...] executable [...]\n\n"
                        "Description:\n"
                        "This tool is used to debug an executable on 64-bit Windows OS.\n\n"
                        "Parameter List:\n"
                        "/B Do not ignore breakpoints.\n"
                        "/D Load executable debug symbols.\n"
                        "/G Load executable debug symbols using GDB backend.\n"
                        "/Q Do not display verbose exception information.\n"
                        "/O Display OutputDebugString string.\n"
                        "/S Start executable with a new console.\n"
                        "/T Specify to wait for the specified time period (in seconds)\n"
                        "                                 or until any key is pressed.\n"
                        "/V Display verbose debug information. \n"
                        , 535, &Overlapped,
                        (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                    return 0;
            }
            memcpy(buffer, "ERROR: Invalid argument/option - '", 34);
            if (argv[i][2] == '\0')
            {
                temp = 2;
                buffer[34] = '/';
                buffer[35] = argv[i][1];
            }
            else
            {
                temp = strlen(argv[i]);
                memcpy(buffer + 34, argv[i], temp);
            }
            memcpy(buffer + 34 + temp, "'.\n"
                "Type \"debug /?\" for usage.\n", 30);
            WriteFileEx(hStderr, buffer, 34 + temp + 30, &Overlapped,
            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
            return 1;
        }
        else break;
    if (argc < 2 || i == argc)
    {
        WriteFileEx(hStderr,
            "ERROR: Invalid syntax.\n"
            "Type \"debug /?\" for usage.\n"
            , 50, &Overlapped,
            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
        return 1;
    }
    DWORD j, count;
    char _buffer[4096];
    j = SearchPathA(NULL, argv[i], ".exe", sizeof(_buffer), _buffer, NULL);
    if (j == 0)
    {
        WriteFileEx(hStderr,
            "ERROR: No such file or directory.\n"
            , 34, &Overlapped,
            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
        return 1;
    }
    if (GetBinaryTypeA(_buffer, &count) == 0)
    {
        WriteFileEx(hStderr,
            "ERROR: Exec format error.\n"
            , 26, &Overlapped,
            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
        return 1;
    }
    memcpy(buffer, _buffer, j);
    ++i;
    while (i < argc)
    {
        buffer[j] = ' ';
        ++j;
        temp = strlen(argv[i]);
        memcpy(&buffer[j], argv[i], temp);
        j += temp;
        ++i;
    }
    char *p;
    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo = {sizeof(startupInfo)};
    DEBUG_EVENT DebugEvent;
    LPVOID lpBaseOfDll[MAX_DLL];
    HANDLE hThread[MAX_THREAD], hFile[MAX_DLL];
    DWORD dwThreadId[MAX_THREAD] = {}, DLLInit[MAX_DLL] = {};
    buffer[j] = '\0';
    if (debug == GNU)
    {
        if (start) CreateProcessA(_buffer, buffer, NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE,
            NULL, NULL, &startupInfo, &processInfo);
        else CreateProcessA(_buffer, buffer, NULL, NULL, FALSE,
            CREATE_PRESERVE_CODE_AUTHZ_LEVEL,
            NULL, NULL, &startupInfo, &processInfo);
        DebugActiveProcess(processInfo.dwProcessId);
    }
    else
    {
        if (start) CreateProcessA(_buffer, buffer, NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE | DEBUG_ONLY_THIS_PROCESS,
            NULL, NULL, &startupInfo, &processInfo);
        else CreateProcessA(_buffer, buffer, NULL, NULL, FALSE,
            DEBUG_ONLY_THIS_PROCESS,
            NULL, NULL, &startupInfo, &processInfo);
    }
    WaitForDebugEvent(&DebugEvent, INFINITE);
    CloseHandle(DebugEvent.u.CreateProcessInfo.hProcess);
    CloseHandle(DebugEvent.u.CreateProcessInfo.hThread);
    ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, DBG_CONTINUE);
    if (verbose)
    {
        memcpy(buffer, "CreateProcess ", 14);
        _ultoa(DebugEvent.dwProcessId, buffer + 14, 10);
        temp = strlen(buffer + 14);
        buffer[14 + temp] = 'x';
        _ultoa(DebugEvent.dwThreadId, buffer + 15 + temp, 10);
        temp += strlen(buffer + 15 + temp);
        buffer[15 + temp] = '\n';
        WriteFileEx(hStderr, buffer, 16 + temp, &Overlapped,
            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
    }
    hThread[0] = processInfo.hThread;
    dwThreadId[0] = processInfo.dwThreadId;
    lpBaseOfDll[0] = DebugEvent.u.CreateProcessInfo.lpBaseOfImage;
    if (debug == TRUE) hFile[0] = DebugEvent.u.CreateProcessInfo.hFile;
    else CloseHandle(DebugEvent.u.CreateProcessInfo.hFile);
    while (TRUE)
    {
        WaitForDebugEvent(&DebugEvent, INFINITE);
        switch (DebugEvent.dwDebugEventCode)
        {
            case LOAD_DLL_DEBUG_EVENT:
                //Find storage position
                for (i = 1; i < MAX_DLL; ++i) if (!DLLInit[i])
                {
                    if (verbose)
                    {
                        memcpy(buffer, "LoadDll ", 8);
                        temp = GetFinalPathNameByHandleA(DebugEvent.u.LoadDll.hFile,
                            buffer + 8, sizeof(buffer), FILE_NAME_OPENED);
                        buffer[temp + 8] = '\n';
                        WriteFileEx(hStderr, buffer, temp + 9, &Overlapped,
                            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                    }
                    DLLInit[i] = 1;
                    hFile[i] = DebugEvent.u.LoadDll.hFile;
                    lpBaseOfDll[i] = DebugEvent.u.LoadDll.lpBaseOfDll;
                    break;
                }
                break;
            case UNLOAD_DLL_DEBUG_EVENT:
                //Find specific DLL
                for (i = 1; i < MAX_DLL; ++i)
                    if (DebugEvent.u.UnloadDll.lpBaseOfDll == lpBaseOfDll[i])
                {
                    if (verbose)
                    {
                        memcpy(buffer, "UnloadDll ", 10);
                        temp = GetFinalPathNameByHandleA(hFile[i],
                            buffer + 10, sizeof(buffer), FILE_NAME_OPENED);
                        buffer[temp + 10] = '\n';
                        WriteFileEx(hStderr, buffer, temp + 11, &Overlapped,
                            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                    }
                    CloseHandle(hFile[i]);
                    if (DLLInit[i] == 2) SymUnloadModule64(processInfo.hProcess,
                        (DWORD64) DebugEvent.u.UnloadDll.lpBaseOfDll);
                    DLLInit[i] = 0;
                    break;
                }
                break;
            case CREATE_THREAD_DEBUG_EVENT:
                //Find storage position
                for (i = 0; i < MAX_THREAD; ++i) if (!dwThreadId[i])
                {
                    if (verbose)
                    {
                        memcpy(buffer, "CreateThread ", 13);
                        _ultoa(DebugEvent.dwProcessId, buffer + 13, 10);
                        temp = strlen(buffer + 13);
                        buffer[13 + temp] = 'x';
                        _ultoa(DebugEvent.dwThreadId, buffer + 14 + temp, 10);
                        temp += strlen(buffer + 14 + temp);
                        buffer[14 + temp] = '\n';
                        WriteFileEx(hStderr, buffer, 15 + temp, &Overlapped,
                            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                    }
                    hThread[i] = DebugEvent.u.CreateThread.hThread;
                    dwThreadId[i] = DebugEvent.dwThreadId;
                    break;
                }
                break;
            case EXIT_THREAD_DEBUG_EVENT:
                //Find specific thread
                for (i = 0; i < MAX_THREAD; ++i) if (DebugEvent.dwThreadId == dwThreadId[i])
                {
                    CloseHandle(hThread[i]);
                    if (verbose)
                    {
                        memcpy(buffer, "ExitThread ", 11);
                        _ultoa(DebugEvent.dwProcessId, buffer + 11, 10);
                        temp = strlen(buffer + 11);
                        buffer[11 + temp] = 'x';
                        _ultoa(DebugEvent.dwThreadId, buffer + 12 + temp, 10);
                        temp += strlen(buffer + 12 + temp);
                        buffer[12 + temp] = '\n';
                        WriteFileEx(hStderr, buffer, 13 + temp, &Overlapped,
                            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                    }
                    dwThreadId[i] = 0;
                    break;
                }
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                if (verbose)
                {
                    memcpy(buffer, "ExitProcess ", 12);
                    _ultoa(DebugEvent.dwProcessId, buffer + 12, 10);
                    temp = strlen(buffer + 12);
                    buffer[12 + temp] = 'x';
                    _ultoa(DebugEvent.dwThreadId, buffer + 13 + temp, 10);
                    temp += strlen(buffer + 13 + temp);
                    buffer[13 + temp] = '\n';
                    WriteFileEx(hStderr, buffer, 13 + temp, &Overlapped,
                        (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                }
                for (i = 1; i < MAX_DLL; ++i) if (DLLInit[i] != 0)
                {
                    CloseHandle(hFile[i]); //May fail for first time
                    if (DLLInit[i] == 2) SymUnloadModule64(processInfo.hProcess,
                        (DWORD64) lpBaseOfDll[i]);
                }
                if (debug == TRUE)
                {
                    CloseHandle(hFile[0]);
                    if (DLLInit[0])
                    {
                        SymUnloadModule64(processInfo.hProcess,
                            (DWORD64) lpBaseOfDll[0]);
                        SymCleanup(processInfo.hProcess);
                    }
                }
                else if (DLLInit[0]) SymCleanup(processInfo.hProcess);
                CloseHandle(processInfo.hProcess);
                for (i = 0; i < MAX_THREAD; ++i) if (DebugEvent.dwThreadId == dwThreadId[i])
                {
                    CloseHandle(hThread[i]);
                    break;
                }
                if (timeout)
                {
                    /* ---------------- DECLARATION ---------------- */
                    HANDLE hStdin;
                    INPUT_RECORD InputRecord;
                    /* --------------------------------------------- */
                    WriteFileEx(hStderr,
                        "\nPress any key to continue ...",
                        30, &Overlapped,
                        (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                    hStdin = GetStdHandle(STD_INPUT_HANDLE);
                    if (timeout == -1)
                        while (TRUE)
                        {
                            ReadConsoleInputA(hStdin, &InputRecord, 1, &count);
                            if (InputRecord.EventType == KEY_EVENT &&
                                InputRecord.Event.KeyEvent.bKeyDown &&
                                InputRecord.Event.KeyEvent.wVirtualKeyCode != VK_MENU &&
                                InputRecord.Event.KeyEvent.wVirtualKeyCode != VK_CONTROL)
                                break;
                        }
                    else
                    {
                        /* ---------------- DECLARATION ---------------- */
                        DWORD ctime;
                        /* --------------------------------------------- */
                        ctime = GetTickCount() + (timeout << 10);
                        while (TRUE)
                        {
                            if (WaitForSingleObjectEx(hStdin, ctime - GetTickCount(), FALSE) == WAIT_TIMEOUT)
                                break;
                            ReadConsoleInputA(hStdin, &InputRecord, 1, &count);
                            if (InputRecord.EventType == KEY_EVENT &&
                                InputRecord.Event.KeyEvent.bKeyDown &&
                                InputRecord.Event.KeyEvent.wVirtualKeyCode != VK_MENU &&
                                InputRecord.Event.KeyEvent.wVirtualKeyCode != VK_CONTROL)
                            break;
                        }
                    }
                }
                return 0;
            case OUTPUT_DEBUG_STRING_EVENT:
                if (output == TRUE)
                {
                    SIZE_T NumberOfBytesRead;
                    ReadProcessMemory(processInfo.hProcess,
                        DebugEvent.u.DebugString.lpDebugStringData,
                        buffer, DebugEvent.u.DebugString.nDebugStringLength,
                        &NumberOfBytesRead);
                    WriteFileEx(hStderr,
                        buffer, NumberOfBytesRead, &Overlapped,
                        (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                }
                break;
            case EXCEPTION_DEBUG_EVENT:
                //ignore first-chance breakpoints && thread naming exception
                if (DebugEvent.u.Exception.ExceptionRecord.ExceptionCode == 541541187) //GCXX_RUNTIME_EXCEPTION
                    break;
                if ((breakpoint == FALSE && (
                DebugEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x80000003 || //EXCEPTION_BREAKPOINT
                DebugEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x4000001F || //STATUS_WX86_BREAKPOINT
                DebugEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x406D1388)) || //MS_VC_EXCEPTION
                (breakpoint == TRUE && ++firstbreak == TRUE))
                    break;
                for (i = 0; i < MAX_THREAD; ++i) if (DebugEvent.dwThreadId == dwThreadId[i])
                    break;
                if (DebugEvent.u.Exception.dwFirstChance == 0 ||
                    DebugEvent.dwThreadId != dwThreadId[i])
                {
                    ContinueDebugEvent(DebugEvent.dwProcessId,
                        DebugEvent.dwThreadId,
                        DBG_EXCEPTION_NOT_HANDLED);
                    continue;
                }
                char *name;
                BOOL bWow64;
                BOOL Console;
                CONTEXT Context;
                DWORD MachineType;
                DWORD Displacement;
                IMAGEHLP_LINE64 Line;
                DWORD64 Displacement64;
                STACKFRAME64 StackFrame;
                char Symbol[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
                PSYMBOL_INFO pSymbol = (PSYMBOL_INFO) Symbol;
                if (!DLLInit[0])
                {
                    DLLInit[0] = TRUE;
                    SymInitialize(processInfo.hProcess, NULL, FALSE);
                    if (debug == TRUE)
                    {
                        SymSetOptions(_DebugSymOptions);
                        SymLoadModuleEx(processInfo.hProcess,
                            hFile[0],
                            NULL,
                            NULL,
                            (DWORD64) lpBaseOfDll[0],
                            GetModuleSize(hFile[0]),
                            NULL,
                            0);
                    }
                    else SymSetOptions(NDebugSymOptions);
                }
                //Load all modules not loaded
                for (j = 1; j < MAX_DLL; ++j) if (DLLInit[j] == 1)
                {
                    SymLoadModuleEx(processInfo.hProcess,
                        hFile[j],
                        NULL,
                        NULL,
                        (DWORD64) lpBaseOfDll[j],
                        GetModuleSize(hFile[j]),
                        NULL,
                        0);
                    DLLInit[j] = 2;
                }
                memcpy(buffer, "Thread #", 8);
                _ultoa(i + 1, buffer + 8, 10);
                p = buffer + 8 + strlen(buffer + 8);
                memcpy(p, " caused ", 8);
                p += 8;
#ifndef _M_ARM64
                IsWow64Process(hProcess, &bWow64);
#endif
                p = FormatDebugException(&DebugEvent, p, _buffer, bWow64);
                *p = '\n';
                ++p;
                Console = GetFileType(hStderr) == FILE_TYPE_CHAR;
                if (vexception)
                {
                    if (Console)
                    {
                        *p = '\x1b';
                        ++p;
                        *p = '[';
                        ++p;
                        *p = '3';
                        ++p;
                        *p = '1';
                        ++p;
                        *p = 'm';
                        ++p;
                    }
                    p = FormatVerboseDebugException(p, 
                        DebugEvent.u.Exception.ExceptionRecord.ExceptionCode);
                    *p = '\n';
                    ++p;
                    if (Console)
                    {
                        *p = '\x1b';
                        ++p;
                        *p = '[';
                        ++p;
                        *p = 'm';
                        ++p;
                    }
                }
                //memset(&StackFrame, 0, sizeof(StackFrame));
#ifdef _M_ARM64
                USHORT processArch, nativeArch;
                IsWow64Process2(processInfo.hProcess, &processArch, &nativeArch);
                if (processArch == IMAGE_FILE_MACHINE_UNKNOWN)
                {
                    PROCESS_MACHINE_INFORMATION processMachineInfo = {};
                    HMODULE hKernelModule = GetModuleHandleA("kernel32");
                    PFN_GETPROCESSINFORMATION pfnGetProcessInformation =
                    (PFN_GETPROCESSINFORMATION)GetProcAddress(hKernelModule, "GetProcessInformation");
                    if (pfnGetProcessInformation(processInfo.hProcess, ProcessMachineTypeInfo, &processMachineInfo, sizeof processMachineInfo))
                    {
                        processArch = processMachineInfo.ProcessMachine;
                    } else processArch = nativeArch;
                }
                if (processArch == IMAGE_FILE_MACHINE_ARM64 ||
                    processArch == IMAGE_FILE_MACHINE_AMD64)
                {
                    // XXX: Unfortunate pContext is _not_ an AMD64 context, so StackWalk will fail
                    GetThreadContext(hThread[i], &Context);
                    MachineType = IMAGE_FILE_MACHINE_ARM64;
                    StackFrame.AddrPC.Offset = Context.Pc;
                    StackFrame.AddrStack.Offset = Context.Sp;
                    StackFrame.AddrFrame.Offset = Context.Fp;
                } else
                {
                    MachineType = IMAGE_FILE_MACHINE_I386;
                    Wow64GetThreadContext(hThread[i], (PWOW64_CONTEXT) &Context);
                    StackFrame.AddrPC.Offset = ((PWOW64_CONTEXT) &Context)->Eip;
                    StackFrame.AddrStack.Offset = ((PWOW64_CONTEXT) &Context)->Esp;
                    StackFrame.AddrFrame.Offset = ((PWOW64_CONTEXT) &Context)->Ebp;
                }
#else
                if (bWow64)
                {
                    ((PWOW64_CONTEXT) &Context)->ContextFlags = WOW64_CONTEXT_ALL;
                    Wow64GetThreadContext(hThread[i], (PWOW64_CONTEXT) &Context);
                    MachineType = IMAGE_FILE_MACHINE_I386;
                    StackFrame.AddrPC.Offset = ((PWOW64_CONTEXT) &Context)->Eip;
                    StackFrame.AddrStack.Offset = ((PWOW64_CONTEXT) &Context)->Esp;
                    StackFrame.AddrFrame.Offset = ((PWOW64_CONTEXT) &Context)->Ebp;
                }
                else
                {
                    Context.ContextFlags = CONTEXT_ALL;
                    GetThreadContext(hThread[i], &Context);
                    MachineType = IMAGE_FILE_MACHINE_AMD64;
                    StackFrame.AddrPC.Offset = Context.Rip;
                    StackFrame.AddrStack.Offset = Context.Rsp;
                    StackFrame.AddrFrame.Offset = Context.Rbp;
                }
#endif
                StackFrame.AddrPC.Mode = AddrModeFlat;
                StackFrame.AddrStack.Mode = AddrModeFlat;
                StackFrame.AddrFrame.Mode = AddrModeFlat;
                Line.SizeOfStruct = sizeof(Line);
                pSymbol->MaxNameLen = MAX_SYM_NAME;
                pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                count = 0;
                if (Console)
                {
                    while (TRUE)
                    {
                        if (!StackWalk64(MachineType, processInfo.hProcess, hThread[i], &StackFrame,
                            &Context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
                            break;
                        if (!SymFromAddr(processInfo.hProcess, StackFrame.AddrPC.Offset, &Displacement64, pSymbol))
                            break;
                        *p = '#';
                        ++p;
                        if (count < 10)
                        {
                            *p = '0' + count;
                            ++p;
                            *p = ' ';
                        }
                        else
                        {
                            *p = '0' + count / 10;
                            ++p;
                            *p = '0' + count % 10;
                        }
                        ++p;
                        *p = ' ';
                        ++p;
                        *p = '\x1b';
                        ++p;
                        *p = '[';
                        ++p;
                        *p = '3';
                        ++p;
                        *p = '4';
                        ++p;
                        *p = 'm';
                        ++p;
                        *p = '0';
                        ++p;
                        *p = 'x';
                        ++p;
                        if (MachineType == IMAGE_FILE_MACHINE_I386)
                        {
                            _ultoaddr((DWORD) StackFrame.AddrPC.Offset, p, _buffer);
                            p += 8;
                        }
                        else
                        {
                            ulltoaddr(StackFrame.AddrPC.Offset, p, _buffer);
                            p += 16;
                        }
                        *p = '\x1b';
                        ++p;
                        *p = '[';
                        ++p;
                        *p = 'm';
                        ++p;
                        *p = ' ';
                        ++p;
                        *p = 'i';
                        ++p;
                        *p = 'n';
                        ++p;
                        *p = ' ';
                        ++p;
                        *p = '\x1b';
                        ++p;
                        *p = '[';
                        ++p;
                        *p = '3';
                        ++p;
                        *p = '3';
                        ++p;
                        *p = 'm';
                        ++p;
                        temp = GetModuleFileNameExA(processInfo.hProcess,
                            (HMODULE) SymGetModuleBase64(processInfo.hProcess,
                                StackFrame.AddrPC.Offset),
                            _buffer, sizeof(_buffer));
                        name = (char *) memrchr(_buffer, '\\', temp) + 1;
                        j = (char *) memrchr(name, '.', _buffer + temp - name) - name;
                        memcpy(p, name, j);
                        p += j;
                        *p = '!';
                        ++p;
                        memcpy(p, pSymbol->Name, pSymbol->NameLen);
                        p += pSymbol->NameLen;
                        *p = '\x1b';
                        ++p;
                        *p = '[';
                        ++p;
                        *p = 'm';
                        ++p;
                        *p = ' ';
                        ++p;
                        *p = '(';
                        ++p;
                        *p = ')';
                        ++p;
                        *p = ' ';
                        ++p;
                        if (debug == TRUE && SymGetLineFromAddr64(processInfo.hProcess,
                            StackFrame.AddrPC.Offset, &Displacement, &Line))
                        {
                            *p = 'a';
                            ++p;
                            *p = 't';
                            ++p;
                            *p = ' ';
                            ++p;
                            *p = '\x1b';
                            ++p;
                            *p = '[';
                            ++p;
                            *p = '3';
                            ++p;
                            *p = '2';
                            ++p;
                            *p = 'm';
                            ++p;
                            p = FormatFileLine(&Line, p, Console, verbose);
                            *p = '\x1b';
                            ++p;
                            *p = '[';
                            ++p;
                            *p = 'm';
                            ++p;
                        }
                        else
                        {
                            *p = 'f';
                            ++p;
                            *p = 'r';
                            ++p;
                            *p = 'o';
                            ++p;
                            *p = 'm';
                            ++p;
                            *p = ' ';
                            ++p;
                            *p = '\x1b';
                            ++p;
                            *p = '[';
                            ++p;
                            *p = '3';
                            ++p;
                            *p = '2';
                            ++p;
                            *p = 'm';
                            ++p;
                            memcpy(p, _buffer, temp);
                            p += temp;
                            *p = '\x1b';
                            ++p;
                            *p = '[';
                            ++p;
                            *p = 'm';
                            ++p;
                            *p = '\n';
                            ++p;
                        }
                        ++count;
                    }
                    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
                }
                else
                {
                    while (TRUE)
                    {
                        if (!StackWalk64(MachineType, processInfo.hProcess, hThread[i], &StackFrame,
                            &Context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
                            break;
                        if (SymFromAddr(processInfo.hProcess,
                            StackFrame.AddrPC.Offset, &Displacement64, pSymbol) == 0)
                            break;
                        *p = '#';
                        ++p;
                        if (count < 10) *p = '0' + count;
                        else
                        {
                            *p = '0' + count / 10;
                            ++p;
                            *p = '0' + count % 10;
                        }
                        ++p;
                        *p = ' ';
                        ++p;
                        *p = ' ';
                        ++p;
                        *p = '0';
                        ++p;
                        *p = 'x';
                        ++p;
                        if (MachineType == IMAGE_FILE_MACHINE_I386)
                        {
                            _ultoaddr((DWORD) StackFrame.AddrPC.Offset, p, _buffer);
                            p += 8;
                        }
                        else
                        {
                            ulltoaddr(StackFrame.AddrPC.Offset, p, _buffer);
                            p += 16;
                        }
                        *p = ' ';
                        ++p;
                        *p = 'i';
                        ++p;
                        *p = 'n';
                        ++p;
                        *p = ' ';
                        ++p;
                        temp = GetModuleFileNameExA(processInfo.hProcess,
                            (HMODULE) SymGetModuleBase64(processInfo.hProcess,
                                StackFrame.AddrPC.Offset),
                            _buffer, sizeof(_buffer));
                        name = (char *) memrchr(_buffer, '\\', temp) + 1;
                        j = (char *) memrchr(name, '.', _buffer + temp - name) - name;
                        memcpy(p, name, j);
                        p += j;
                        *p = '!';
                        ++p;
                        memcpy(p, pSymbol->Name, pSymbol->NameLen);
                        p += pSymbol->NameLen;
                        *p = ' ';
                        ++p;
                        *p = '(';
                        ++p;
                        *p = ')';
                        ++p;
                        *p = ' ';
                        ++p;
                        if (debug == TRUE && SymGetLineFromAddr64(processInfo.hProcess,
                            StackFrame.AddrPC.Offset, &Displacement, &Line))
                        {
                            *p = 'a';
                            ++p;
                            *p = 't';
                            ++p;
                            *p = ' ';
                            ++p;
                            p = FormatFileLine(&Line, p, Console, verbose);
                        }
                        else
                        {
                            *p = 'f';
                            ++p;
                            *p = 'r';
                            ++p;
                            *p = 'o';
                            ++p;
                            *p = 'm';
                            ++p;
                            *p = ' ';
                            ++p;
                            memcpy(p, _buffer, temp);
                            p += temp;
                            *p = '\n';
                            ++p;
                        }
                        ++count;
                    }
                }
                if (debug == GNU)
                {
                    if (SearchPathA(NULL, "gdb.exe", NULL, sizeof(_buffer), _buffer, NULL))
                    {
                        BOOL isDebugged;
                        int iter, k;
                        DWORD dwRead;
                        PROCESS_INFORMATION GDBInfo;
                        char *str, *next, *ptr, *_ptr;
                        HANDLE hStdoutReadPipe, hStdoutWritePipe;
                        SECURITY_ATTRIBUTES saAttr = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
                        SuspendThread(hThread[i]);
                        ContinueDebugEvent(DebugEvent.dwProcessId,
                            DebugEvent.dwThreadId,
                            DBG_EXCEPTION_NOT_HANDLED);
                        DebugActiveProcessStop(DebugEvent.dwProcessId);
                        memcpy(p, "gdb.exe -q -x=", 14);
                        temp = GetTempPathA(4096, p + 14);
                        memcpy(p + 14 + temp, "gdbinit", 7);
                        if (GetFileAttributesA(p + 14) == INVALID_FILE_ATTRIBUTES)
                        {
                            FILE *fp = fopen(p + 14, "w");
                            fwrite("set print thread-events off\nset pagination off\nset style enabled on\nset backtrace limit 100\nset print frame-arguments all\nset print entry-values no\nset print object on\nset print pretty off\nset width 0\ncont\nbt", 208, 1, fp);
                            fclose(fp);
                        }
                        memcpy(p + 14 + temp + 7, " -p ", 4);
                        _ultoa(DebugEvent.dwProcessId, p + 14 + temp + 7 + 4, 10);
                        CreatePipe(&hStdoutReadPipe, &hStdoutWritePipe, &saAttr, 0);
                        startupInfo.hStdOutput = hStdoutWritePipe;
                        startupInfo.dwFlags = STARTF_USESTDHANDLES;
                        CreateProcessA(_buffer, p, NULL, NULL, TRUE,
                            CREATE_NO_WINDOW,
                            NULL, NULL, &startupInfo, &GDBInfo);
                        CloseHandle(hStdoutWritePipe);
                        CloseHandle(GDBInfo.hThread);
                        while (TRUE)
                        {
                            SleepEx(LATENCY, FALSE);
                            CheckRemoteDebuggerPresent(processInfo.hProcess,
                                &isDebugged);
                            if (isDebugged) break;
                        }
                        ResumeThread(hThread[i]);
                        iter = 0;
                        if (Console)
                        {
                            while (TRUE)
                            {
                                if ((ReadFile(hStdoutReadPipe, _buffer, sizeof(_buffer),
                                    &dwRead, NULL) == 0) || !dwRead) break;
                                _buffer[dwRead] = '\0';
                                str = strstr(_buffer, "\n#") + 1;
                                if (str == (char *) 1)
                                {
                                    if (_buffer[0] == '#')
                                        str = _buffer;
                                    else if ((*(unsigned long long *) _buffer & 177744406312) == 177744406312)
                                        break;
                                    else continue;
                                }
                                while (TRUE)
                                {
                                    next = (char *) memchr(str, '\n', _buffer + dwRead - str) + 1;
                                    if (memcmp(str, "(gdb)", 5) == 0) break;    
                                    if (iter >= count)
                                    {
                                        if (next > (char *) 1) temp = next - str;
                                        else temp = _buffer + dwRead - str;
                                        memcpy(p, str, 4);
                                        p += 4;
                                        *p = '\x1b';
                                        ++p;
                                        *p = '[';
                                        ++p;
                                        *p = '3';
                                        ++p;
                                        *p = '4';
                                        ++p;
                                        *p = 'm';
                                        ++p;
                                        if (*(str + 4) != '0') k = -1;
                                        else
                                        {
                                            k = 21;
                                            *p = '0';
                                            ++p;
                                            *p = 'x';
                                            ++p;
                                            if (bWow64)
                                            {
                                                k -= 8;
                                                memcpy(p, str + 6, 8);
                                                p += 8;
                                            } else
                                            {
                                                memcpy(p, str + 6, 16);
                                                p += 16;
                                            }
                                            *p = '\x1b';
                                            ++p;
                                            *p = '[';
                                            ++p;
                                            *p = 'm';
                                            ++p;
                                            *p = ' ';
                                            ++p;
                                            *p = 'i';
                                            ++p;
                                            *p = 'n';
                                            ++p;
                                            *p = ' ';
                                            ++p;
                                        }
                                        *p = '\x1b';
                                        ++p;
                                        *p = '[';
                                        ++p;
                                        *p = '3';
                                        ++p;
                                        *p = '3';
                                        ++p;
                                        *p = 'm';
                                        ++p;
                                        i = (char *) memchr(str + 5 + k, '(', temp - 5 - k) - str - 5 - k;
                                        memcpy(p, str + 5 + k, i);
                                        p += i;
                                        *p = '\x1b';
                                        ++p;
                                        *p = '[';
                                        ++p;
                                        *p = 'm';
                                        ++p;
                                        *p = '(';
                                        ++p;
                                        j = (char *) memrchr(str + 5 + k + i + 1, ')', temp - 5 - k - i - 1) - str - 5 - k - i;
                                        if (j > 1)
                                        {
                                            *p = '\x1b';
                                            ++p;
                                            *p = '[';
                                            ++p;
                                            *p = '3';
                                            ++p;
                                            *p = '6';
                                            ++p;
                                            *p = 'm';
                                            ++p;
                                            ptr = (char *) memchr(str + 5 + k + i + 1, '=', temp - 5 - k - i - 1);
                                            memcpy(p, str + 5 + k + i + 1, ptr - str - 5 - k - i - 1);
                                            p += ptr - str - 5 - k - i - 1;
                                            *p = '\x1b';
                                            ++p;
                                            *p = '[';
                                            ++p;
                                            *p = 'm';
                                            ++p;
                                            *p = '=';
                                            ++p;
                                            ++ptr;
                                            _ptr = (char *) memchr(ptr, ',', str + temp - ptr);
                                            if (_ptr == NULL)
                                            {
                                                _ptr = (char *) memrchr(ptr, ')', str + temp - ptr);
                                                memcpy(p, ptr, _ptr - ptr);
                                                p += _ptr - ptr;
                                            } else
                                            {
                                                memcpy(p, ptr, _ptr - ptr);
                                                p += _ptr - ptr;
                                                ptr = _ptr + 2;
                                                while (TRUE)
                                                {
                                                    *p = ',';
                                                    ++p;
                                                    *p = ' ';
                                                    ++p;
                                                    *p = '\x1b';
                                                    ++p;
                                                    *p = '[';
                                                    ++p;
                                                    *p = '3';
                                                    ++p;
                                                    *p = '6';
                                                    ++p;
                                                    *p = 'm';
                                                    ++p;
                                                    _ptr = (char *) memchr(ptr, '=', str + temp - ptr);
                                                    memcpy(p, ptr, _ptr - ptr);
                                                    p += _ptr - ptr;
                                                    *p = '\x1b';
                                                    ++p;
                                                    *p = '[';
                                                    ++p;
                                                    *p = 'm';
                                                    ++p;
                                                    if ((ptr = (char *) memchr(ptr, ',', str + temp - ptr) + 2)
                                                    == (char *) 2)
                                                    {
                                                        ptr = (char *) memchr(_ptr, ')', str + temp - ptr);
                                                        memcpy(p, _ptr, ptr - _ptr);
                                                        p += ptr - _ptr;
                                                        break;
                                                    }
                                                    memcpy(p, _ptr, ptr - _ptr - 2);
                                                    p += ptr - _ptr - 2;
                                                }
                                            }
                                        }
                                        *p = ')';
                                        ++p;
                                        if (*(str + 5 + k + i + 1 + j) == ' ')
                                        {
                                            *p = ' ';
                                            ++p;
                                            if (*(str + 5 + k + i + 1 + j + 1) == 'a')
                                            {
                                                *p = 'a';
                                                ++p;
                                                *p = 't';
                                                ++p;
                                                *p = ' ';
                                                ++p;
                                                *p = '\x1b';
                                                ++p;
                                                *p = '[';
                                                ++p;
                                                *p = '3';
                                                ++p;
                                                *p = '2';
                                                ++p;
                                                *p = 'm';
                                                ++p;
                                                ptr = str + 5 + k + i + 1 + j + 4;
                                                _ptr = (char *) memrchr(ptr, ':', next - ptr);
                                                memcpy(p, ptr, _ptr - ptr);
                                                p += _ptr - ptr;
                                                *p = '\x1b';
                                                ++p;
                                                *p = '[';
                                                ++p;
                                                *p = 'm';
                                                ++p;
                                                memcpy(p, _ptr, str + temp - _ptr);
                                                p += str + temp - _ptr;
                                                *_ptr = '\0';
                                                *(next - 1) = '\0';
                                                k = atoi(_ptr + 1);
                                                p = FormatSourceCode(ptr, k, p, verbose);
                                            }
                                            else
                                            {
                                                *p = 'f';
                                                ++p;
                                                *p = 'r';
                                                ++p;
                                                *p = 'o';
                                                ++p;
                                                *p = 'm';
                                                ++p;
                                                *p = ' ';
                                                ++p;
                                                *p = '\x1b';
                                                ++p;
                                                *p = '[';
                                                ++p;
                                                *p = '3';
                                                ++p;
                                                *p = '2';
                                                ++p;
                                                *p = 'm';
                                                ++p;
                                                memcpy(p, str + 5 + k + i + 1 + j + 6, next - (str + 5 + k + i + 1 + j + 6));
                                                p += next - (str + 5 + k + i + 1 + j + 6);
                                                *p = '\x1b';
                                                ++p;
                                                *p = '[';
                                                ++p;
                                                *p = 'm';
                                                ++p;
                                            }
                                        } else
                                        {
                                            *p = '\n';
                                            ++p;
                                        }
                                    }
                                    ++iter;
                                    if (*next != '#') break;
                                    str = next;
                                }
                            }
                        } else
                        {
                            while (TRUE)
                            {
                                if ((ReadFile(hStdoutReadPipe, _buffer, sizeof(_buffer),
                                    &dwRead, NULL) == 0) || !dwRead) break;
                                _buffer[dwRead] = '\0';
                                str = strstr(_buffer, "\n#") + 1;
                                if (str == (char *) 1)
                                {
                                    if (_buffer[0] == '#')
                                        str = _buffer;
                                    else if ((*(unsigned long long *) _buffer & 177744406312) == 177744406312)
                                        break;
                                    else continue;
                                }
                                while (TRUE)
                                {
                                    next = (char *) memchr(str, '\n', _buffer + dwRead - str) + 1;
                                    if (memcmp(str, "(gdb)", 5) == 0) break;    
                                    if (iter >= count)
                                    {
                                        temp = next - str;
                                        memcpy(p, str, temp);
                                        p += temp;
                                        if ((ptr = strstr(str, ") at ") + 5) > (char *) 5)
                                        {
                                            _ptr = (char *) memrchr(ptr, ':', next - ptr);
                                            *_ptr = '\0';
                                            *(next - 1) = '\0';
                                            k = atoi(_ptr + 1);
                                            p = FormatSourceCode(ptr, k, p, verbose);
                                        }
                                    }
                                    ++iter;
                                    if (*next != '#') break;
                                    str = next;
                                }
                            }
                        }
                        if (verbose)
                        {
                            memcpy(p, "ExitProcess ", 12);
                            p += 12;
                            _ultoa(DebugEvent.dwProcessId, p, 10);
                            p += strlen(p);
                            *p = 'x';
                            ++p;
                            _ultoa(DebugEvent.dwThreadId, p, 10);
                            p += strlen(p);
                            *p = '\n';
                            ++p;
                        }
                        WriteFileEx(hStderr,
                            buffer, p - buffer, &Overlapped,
                            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                        for (i = 1; i < MAX_DLL; ++i) if (DLLInit[i] != 0)
                        {
                            CloseHandle(hFile[i]); //May fail for first time
                            if (DLLInit[i] == 2) SymUnloadModule64(processInfo.hProcess,
                                (DWORD64) lpBaseOfDll[i]);
                        }
                        SymCleanup(processInfo.hProcess);
                        CloseHandle(processInfo.hProcess);
                        for (i = 0; i < MAX_THREAD; ++i) if (DebugEvent.dwThreadId == dwThreadId[i])
                        {
                            CloseHandle(hThread[i]);
                            break;
                        }
                        TerminateProcess(GDBInfo.hProcess, 0);
                        CloseHandle(GDBInfo.hProcess);
                        if (timeout)
                        {
                            /* ---------------- DECLARATION ---------------- */
                            HANDLE hStdin;
                            INPUT_RECORD InputRecord;
                            /* --------------------------------------------- */
                            WriteFileEx(hStderr,
                                "\nPress any key to continue ...",
                                30, &Overlapped,
                                (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                            hStdin = GetStdHandle(STD_INPUT_HANDLE);
                            if (timeout == -1)
                                while (TRUE)
                                {
                                    ReadConsoleInputA(hStdin, &InputRecord, 1, &count);
                                    if (InputRecord.EventType == KEY_EVENT &&
                                        InputRecord.Event.KeyEvent.wVirtualKeyCode != VK_CONTROL &&
                                        InputRecord.Event.KeyEvent.wVirtualKeyCode != VK_MENU)
                                        break;
                                }
                            else
                            {
                                /* ---------------- DECLARATION ---------------- */
                                DWORD ctime;
                                /* --------------------------------------------- */
                                ctime = GetTickCount() + (timeout << 10);
                                while (TRUE)
                                {
                                    if (WaitForSingleObjectEx(hStdin, ctime - GetTickCount(), FALSE) == WAIT_TIMEOUT)
                                        break;
                                    ReadConsoleInputA(hStdin, &InputRecord, 1, &count);
                                    if (InputRecord.EventType == KEY_EVENT &&
                                        InputRecord.Event.KeyEvent.wVirtualKeyCode != VK_CONTROL &&
                                        InputRecord.Event.KeyEvent.wVirtualKeyCode != VK_MENU)
                                        break;
                                }
                            }
                        }
                        return 0;
                    }
                    else if (verbose)
                    {
                        memcpy(p, "gdb.exe: No such file or directory.\n", 36);
                        p += 36;
                    }
                }
                WriteFileEx(hStderr,
                    buffer, p - buffer, &Overlapped,
                    (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedWriteRoutine);
                ContinueDebugEvent(DebugEvent.dwProcessId,
                    DebugEvent.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
                continue;
        }
        ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, DBG_CONTINUE);
    }
}