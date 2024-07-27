#define ANSI
#define _ANSI
#undef UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <psapi.h>
#include <dbghelp.h>
#include "addr.c"
#include "exception.c"
#include "fsize.c"

#define MAX_THREAD 64
#define MAX_DLL 64
#define GNU 2
#define LATENCY 99

#define SymOptions (SYMOPT_DEFERRED_LOADS | SYMOPT_INCLUDE_32BIT_MODULES)
#define _DebugSymOptions (SymOptions | SYMOPT_LOAD_LINES)
#define NDebugSymOptions (SymOptions | SYMOPT_NO_CPP)

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fwrite(
            "ERROR: Invalid syntax.\n"
            "Type \"debug /?\" for usage.\n"
            , 1, 50, stderr);
        return 1;
    }
    char buffer[4096];
    int temp, i, debug = FALSE, verbose = TRUE, start = FALSE;
    for (i = 1; i < argc; ++i)
        if (argv[i][0] == '/') switch(argv[i][1])
        {
            case 'D':
                if (argv[i][2] == '\0')
                {
                    debug = TRUE;
                    break;
                }
            case 'G':
                if (argv[i][2] == '\0')
                {
                    debug = GNU;
                    break;
                }
            case 'Q':
                if (argv[i][2] == '\0')
                {
                    verbose = FALSE;
                    break;
                }
            case 'S':
                if (argv[i][2] == '\0')
                {
                    start = TRUE;
                    break;
                }
            case '?':
                if (argv[i][2] == '\0')
                {
                    fwrite(
                        "Usage: debug [/D] [/G] [/Q] [/S] executable [...]\n\n"
                        "Description:\n"
                        "This tool is used to debug an executable on 64-bit Windows OS.\n\n"
                        "Parameter List:\n"
                        "/D Load debug symbols.\n"
                        "/G Load debug symbols in DWARF format.\n"
                        "/Q Do not display verbose information.\n"
                        "/S Start executable with a new console.\n"
                        , 1, 285, stderr);
                }
                return 0;
            default:
                printf("ERROR: Invalid argument/option - '%s'\n"
                    "Type \"debug /?\" for usage.\n", argv[i]);
                return 1;
        }
        else break;
    int j, count;
    count = i;
    j = strlen(argv[i]);
    if (!memchr(argv[i], '.', j))
    {
        memcpy(&argv[i][j], ".exe", 5);
        j += 4;
    }
    memcpy(buffer, argv[i], j);
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
    char *p, _buffer[4096];
    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo = {sizeof(startupInfo)};
    if (debug == GNU)
    {
        DWORD dwRead;
        BOOL isDebugged;
        char *str, *next, *ptr, *_ptr;
        PROCESS_INFORMATION GDBInfo;
        HANDLE hStdoutReadPipe, hStdoutWritePipe;
        SECURITY_ATTRIBUTES saAttr = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
        _searchenv("gdb.exe", "PATH", _buffer);
        CreatePipe(&hStdoutReadPipe, &hStdoutWritePipe, &saAttr, 0);
        if (start) CreateProcessA(argv[count], buffer, NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE | CREATE_SUSPENDED,
            NULL, NULL, &startupInfo, &processInfo);
        else CreateProcessA(argv[count], buffer, NULL, NULL, FALSE,
            CREATE_SUSPENDED,
            NULL, NULL, &startupInfo, &processInfo);
        startupInfo.hStdOutput = hStdoutWritePipe;
        startupInfo.dwFlags = STARTF_USESTDHANDLES;
        memcpy(buffer, "gdb.exe -q -batch -ex=cont -ex=bt -p ", 37);
        _ultoa(processInfo.dwProcessId, buffer + 37, 10);
        CreateProcessA(_buffer, buffer, NULL, NULL, TRUE, 0,
            NULL, NULL, &startupInfo, &GDBInfo);
        CloseHandle(hStdoutWritePipe);
        CloseHandle(GDBInfo.hThread);
        CloseHandle(GDBInfo.hProcess);
        while (TRUE)
        {
            Sleep(LATENCY);
            CheckRemoteDebuggerPresent(processInfo.hProcess,
                &isDebugged);
            if (isDebugged) break;
        }
        ResumeThread(processInfo.hThread);
        CloseHandle(processInfo.hThread);
        WaitForSingleObjectEx(processInfo.hProcess, INFINITE, 0);
        CloseHandle(processInfo.hProcess);
        if (!ReadFile(hStdoutReadPipe, buffer, sizeof(buffer), &dwRead, NULL) ||
            dwRead == 0) return 0;
        if (verbose) printf(buffer);
        else
        {
            str = _buffer;
            p = strstr(buffer, "\n#") + 1;
            if (p == (char *) 1) p = buffer;
            while (TRUE)
            {
                next = (char *) memchr(p, '\n', buffer + dwRead - p) + 1;
                temp = next - p;
                ptr = (char *) memchr(p, '!', temp);
                _ptr = ptr + 1;
                if (ptr && *_ptr == '.')
                {
                    i = _ptr - p;
                    memcpy(str, p, i);
                    str += i;
                    *str = '_';
                    ++str;
                    *str = '_';
                    ++str;
                    ++i;
                    memcpy(str, _ptr + 1, temp - i);
                    str += temp - i;
                }
                else
                {
                    memcpy(str, p, temp);
                    str += temp;
                    *(next - 1) = '\0';
                    ptr = strrchr(p, ')') + 1;
                    if (strncmp(ptr, " at ", 4) == 0)
                    {
                        ptr += 4;
                        _ptr = (char *) strrchr(ptr, ':');
                        *_ptr = '\0';
                        str = FormatSourceCode(ptr, atoi(_ptr + 1), str);
                    }
                }
                if (*next != '#') break;
                p = next;
            }
        }
        CloseHandle(hStdoutReadPipe);
        if (!verbose) fwrite(_buffer, 1, str - _buffer, stderr);
        return 0;
    }
    DEBUG_EVENT DebugEvent;
    LPVOID lpBaseOfDll[MAX_DLL] = {};
    HANDLE hThread[MAX_THREAD] = {}, hFile[MAX_DLL] = {};
    DWORD dwThreadId[MAX_THREAD] = {}, DLLInit[MAX_DLL] = {};
    if (start) CreateProcessA(argv[count], buffer, NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE | DEBUG_ONLY_THIS_PROCESS,
        NULL, NULL, &startupInfo, &processInfo);
    else CreateProcessA(argv[count], buffer, NULL, NULL, FALSE,
        DEBUG_ONLY_THIS_PROCESS,
        NULL, NULL, &startupInfo, &processInfo);
    WaitForDebugEvent(&DebugEvent, INFINITE);
    CloseHandle(DebugEvent.u.CreateProcessInfo.hProcess);
    CloseHandle(DebugEvent.u.CreateProcessInfo.hThread);
    ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, DBG_CONTINUE);
    hThread[0] = processInfo.hThread;
    dwThreadId[0] = processInfo.dwThreadId;
    hFile[0] = DebugEvent.u.CreateProcessInfo.hFile;
    lpBaseOfDll[0] = DebugEvent.u.CreateProcessInfo.lpBaseOfImage;
    while (TRUE)
    {
        WaitForDebugEvent(&DebugEvent, INFINITE);
        switch (DebugEvent.dwDebugEventCode)
        {
            case LOAD_DLL_DEBUG_EVENT:
                //Find storage position
                for (i = 1; i < MAX_DLL; ++i) if (DLLInit[i] == 0)
                {
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
                    CloseHandle(hFile[i]);
                    if (DLLInit[i] == 2) SymUnloadModule64(processInfo.hProcess,
                        (DWORD64) DebugEvent.u.UnloadDll.lpBaseOfDll);
                    DLLInit[i] = 0;
                    break;
                }
                break;
            case CREATE_THREAD_DEBUG_EVENT:
                //Find storage position
                for (i = 0; i < MAX_THREAD; ++i) if (dwThreadId[i] == 0)
                {
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
                    dwThreadId[i] = 0;
                    break;
                }
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                for (i = 1; i < MAX_DLL; ++i) if (DLLInit[i] != 0)
                {
                    CloseHandle(hFile[i]); //May fail for first time
                    if (DLLInit[i] == 2) SymUnloadModule64(processInfo.hProcess,
                        (DWORD64) lpBaseOfDll[i]);
                }
                CloseHandle(hFile[0]);
                if (DLLInit[0])
                {
                    if (debug) SymUnloadModule64(processInfo.hProcess,
                        (DWORD64) lpBaseOfDll[0]);
                    SymCleanup(processInfo.hProcess);
                }
                CloseHandle(processInfo.hProcess);
                for (i = 0; i < MAX_THREAD; ++i) if (DebugEvent.dwThreadId == dwThreadId[i])
                {
                    CloseHandle(hThread[i]);
                    break;
                }
                return 0;
            case EXCEPTION_DEBUG_EVENT:
                //ignore first-chance breakpoints && thread naming exception
                if (DebugEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x80000003 || //EXCEPTION_BREAKPOINT
                DebugEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x4000001F || //STATUS_WX86_BREAKPOINT
                DebugEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x406D1388) //MS_VC_EXCEPTION
                    break;
                ContinueDebugEvent(DebugEvent.dwProcessId,
                    DebugEvent.dwThreadId,
                    DBG_EXCEPTION_NOT_HANDLED);
                //Terminate and exit
                if (!DebugEvent.u.Exception.dwFirstChance) break;
                for (i = 0; i < MAX_THREAD; ++i) if (DebugEvent.dwThreadId == dwThreadId[i])
                    break;
                if (DebugEvent.dwThreadId != dwThreadId[i]) continue;
                char *name;
                BOOL bWow64;
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
                    if (debug)
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
                memcpy(buffer, "Thread #.. caused ", 18);
                temp = i + 1;
                buffer[8] = '0' + temp / 10;
                buffer[9] = '0' + temp % 10;
                p = buffer + 18;
                IsWow64Process(processInfo.hProcess, &bWow64);
                p = FormatDebugException(&DebugEvent, p, _buffer, bWow64);
                *p = '\n';
                ++p;
                if (verbose)
                {
                    p = FormatVerboseDebugException(p, 
                        DebugEvent.u.Exception.ExceptionRecord.ExceptionCode);
                    *p = '\n';
                    ++p;
                }
                //memset(&StackFrame, 0, sizeof(StackFrame));
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
                StackFrame.AddrPC.Mode = AddrModeFlat;
                StackFrame.AddrStack.Mode = AddrModeFlat;
                StackFrame.AddrFrame.Mode = AddrModeFlat;
                Line.SizeOfStruct = sizeof(Line);
                pSymbol->MaxNameLen = MAX_SYM_NAME;
                pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                count = 0;
                while (TRUE)
                {
                    StackWalk64(MachineType, processInfo.hProcess, hThread[i], &StackFrame,
                        &Context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL);
                    if (!SymFromAddr(processInfo.hProcess,
                        StackFrame.AddrPC.Offset, &Displacement64, pSymbol))
                        break;
                    *p = '#';
                    ++p;
                    if (count < 10) *p = '0' + count;
                    else
                    {
                        *p = '0' + count / 10;
                        ++p;
                        *p = '0' + count % 10;
                        ++p;
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
                    temp = K32GetModuleFileNameExA(processInfo.hProcess,
                        (HMODULE) SymGetModuleBase64(processInfo.hProcess,
                            StackFrame.AddrPC.Offset),
                        _buffer, 4096);
                    name = (char *) strrchr(_buffer, '\\') + 1;
                    j = strrchr(name, '.') - name;
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
                    if (debug && SymGetLineFromAddr64(processInfo.hProcess,
                        StackFrame.AddrPC.Offset, &Displacement, &Line))
                    {
                        *p = ' ';
                        ++p;
                        *p = 'a';
                        ++p;
                        *p = 't';
                        ++p;
                        *p = ' ';
                        ++p;
                        p = FormatFileLine(&Line, p);
                    }
                    *p = '\n';
                    ++p;
                    ++count;
                }
                fwrite(buffer, 1, p - buffer, stderr);
                continue;
        }
        ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, DBG_CONTINUE);
    }
}
