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
#include "memrchr.c"

#define MAX_THREAD 256
#define MAX_DLL 64
#define GNU 2
#define LATENCY 99

#define SymOptions (SYMOPT_DEFERRED_LOADS | SYMOPT_INCLUDE_32BIT_MODULES)
#define _DebugSymOptions (SymOptions | SYMOPT_LOAD_LINES)
#define NDebugSymOptions (SymOptions | SYMOPT_NO_CPP)

#define GCXX_RUNTIME_EXCEPTION 541541187

int main(int argc, char *argv[])
{
    char buffer[65536];
    int temp, i, breakpoint = FALSE, firstbreak = FALSE, debug = FALSE,
    output = FALSE, timeout = 0, vexception = TRUE, verbose = FALSE, start = FALSE;
    for (i = 1; i < argc; ++i)
        if (argv[i][0] == '/') switch(argv[i][1])
        {
            case 'B':
                if (argv[i][2] == '\0')
                {
                    breakpoint = TRUE;
                    break;
                }
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
                    vexception = FALSE;
                    break;
                }
            case 'O':
                if (argv[i][2] == '\0')
                {
                    output = TRUE;
                    break;
                }
            case 'S':
                if (argv[i][2] == '\0')
                {
                    start = TRUE;
                    break;
                }
            case 'T':
                if (argv[i][2] == '\0')
                {
                    if (++i >= argc)
                    {
                        printf("ERROR: Invalid syntax. Value expected for '/T'\n"
                            "Type \"debug /?\" for usage.\n");
                        exit(1);
                    } else if ((argv[i][0] == '-' && !isdigit(argv[i][1])) ||
                        (argv[i][0] != '-' && !isdigit(argv[i][0])) ||
                        (timeout = atoi(argv[i])) > 99999 ||
                        timeout < -1)
                    {
                        printf("ERROR: Invalid value for timeout (/T) specified. Valid range is -1 to 99999.\n");
                        exit(1);
                    }
                    timeout = atoi(argv[i]);
                    break;
                }
            case 'V':
                if (argv[i][2] == '\0')
                {
                    verbose = TRUE;
                    break;
                }
            case '?':
                if (argv[i][2] == '\0')
                {
                    printf(
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
                        "/V Display verbose debug information. \n");
                    exit(0);
                }
            default:
                printf("ERROR: Invalid argument/option - '%s'\n"
                    "Type \"debug /?\" for usage.\n", argv[i]);
                exit(1);
        }
        else break;
    if (argc < 2 || i == argc)
    {
        printf(
            "ERROR: Invalid syntax.\n"
            "Type \"debug /?\" for usage.\n");
        exit(1);
    }
    int j, count;
    char _buffer[4096];
    j = SearchPathA(NULL, argv[i], ".exe", sizeof(_buffer), _buffer, NULL);
    if (j == 0)
    {
        printf("ERROR: No such file or directory.\n");
        exit(1);
    }
    if (GetBinaryTypeA(_buffer, (LPDWORD) &count) == 0)
    {
        printf("ERROR: Exec format error.\n");
        exit(1);
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
                if (verbose)
                {
                    GetFinalPathNameByHandleA(DebugEvent.u.LoadDll.hFile,
                        buffer, sizeof(buffer), FILE_NAME_OPENED);
                    printf("LoadDll %s\n", buffer);
                }
                //Find storage position
                for (i = 1; i < MAX_DLL; ++i) if (!DLLInit[i])
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
                    if (verbose)
                    {
                        GetFinalPathNameByHandleA(hFile[i],
                            buffer, sizeof(buffer), FILE_NAME_OPENED);
                        printf("UnloadDll %s\n", buffer);
                    }
                    CloseHandle(hFile[i]);
                    if (DLLInit[i] == 2) SymUnloadModule64(processInfo.hProcess,
                        (DWORD64) DebugEvent.u.UnloadDll.lpBaseOfDll);
                    DLLInit[i] = 0;
                    break;
                }
                break;
            case CREATE_THREAD_DEBUG_EVENT:
                if (verbose)
                {
                    printf("CreateThread %ux%u\n",
                        DebugEvent.dwProcessId,
                        DebugEvent.dwThreadId);
                }
                //Find storage position
                for (i = 0; i < MAX_THREAD; ++i) if (!dwThreadId[i])
                {
                    hThread[i] = DebugEvent.u.CreateThread.hThread;
                    dwThreadId[i] = DebugEvent.dwThreadId;
                    break;
                }
                break;
            case EXIT_THREAD_DEBUG_EVENT:
                if (verbose)
                {
                    printf("ExitThread %ux%u\n",
                        DebugEvent.dwProcessId,
                        DebugEvent.dwThreadId);
                }
                //Find specific thread
                for (i = 0; i < MAX_THREAD; ++i) if (DebugEvent.dwThreadId == dwThreadId[i])
                {
                    CloseHandle(hThread[i]);
                    dwThreadId[i] = 0;
                    break;
                }
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                if (verbose)
                {
                    printf("ExitProcess %ux%u\n",
                        DebugEvent.dwProcessId,
                        DebugEvent.dwThreadId);
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
                    sprintf(buffer, "timeout %d", timeout);
                    system(buffer);
                }
                exit(0);
            case OUTPUT_DEBUG_STRING_EVENT:
                if (output == TRUE)
                {
                    SIZE_T NumberOfBytesRead;
                    ReadProcessMemory(processInfo.hProcess,
                        DebugEvent.u.DebugString.lpDebugStringData,
                        buffer, DebugEvent.u.DebugString.nDebugStringLength,
                        &NumberOfBytesRead);
                    printf(buffer);
                }
                break;
            case EXCEPTION_DEBUG_EVENT:
                //ignore first-chance breakpoints && thread naming exception
                //GCXX_RUNTIME_EXCEPTION
                if (DebugEvent.u.Exception.ExceptionRecord.ExceptionCode == 541541187)
                    break;
                if ((breakpoint == FALSE && (
                DebugEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x80000003 || //EXCEPTION_BREAKPOINT
                DebugEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x4000001F || //STATUS_WX86_BREAKPOINT
                DebugEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x406D1388)) || //MS_VC_EXCEPTION
                (breakpoint == TRUE && ++firstbreak == TRUE))
                    break;
                //Terminate and exit
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
                p = buffer + sprintf(buffer, "Thread #%02u caused ", temp + 1);
                IsWow64Process(processInfo.hProcess, &bWow64);
                p = FormatDebugException(&DebugEvent, p, _buffer, bWow64);
                *p = '\n';
                ++p;
                Console = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR;
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
                        memcpy(p, "gdb.exe -q -batch -x=", 21);
                        temp = GetTempPathA(4096, p + 21);
                        memcpy(p + 21 + temp, "gdbinit", 7);
                        if (GetFileAttributesA(p + 21) == INVALID_FILE_ATTRIBUTES)
                        {
                            FILE *fp = fopen(p + 21, "w");
                            fwrite("set print thread-events off\nset pagination off\nset style enabled on\nset backtrace limit 100", 91, 1, fp);
                            fclose(fp);
                        }
                        memcpy(p + 21 + temp + 7, " -ex=cont -ex=bt -p ", 20);
                        _ultoa(DebugEvent.dwProcessId, p + 21 + temp + 7 + 20, 10);
                        CreatePipe(&hStdoutReadPipe, &hStdoutWritePipe, &saAttr, 0);
                        startupInfo.hStdOutput = hStdoutWritePipe;
                        startupInfo.dwFlags = STARTF_USESTDHANDLES;
                        CreateProcessA(_buffer, p, NULL, NULL, TRUE,
                            CREATE_NO_WINDOW,
                            NULL, NULL, &startupInfo, &GDBInfo);
                        CloseHandle(hStdoutWritePipe);
                        CloseHandle(GDBInfo.hThread);
                        CloseHandle(GDBInfo.hProcess);
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
                                str = (char *) memchr(_buffer, '#', dwRead);
                                if (str == NULL) continue;
                                while (TRUE)
                                {
                                    next = (char *) memchr(str, '\n', _buffer + dwRead - str) + 1;
                                    if (iter >= count)
                                    {
                                        temp = next - str;
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
                                            memcpy(p, str + 4, 18);
                                            p += 18;
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
                                        j = (char *) memchr(str + 5 + k + i + 1, ')', temp - 5 - k - i - 1) - str - 5 - k - i;
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
                                                _ptr = (char *) memchr(ptr, ')', str + temp - ptr);
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
                                str = (char *) memchr(_buffer, '#', dwRead);
                                if (str == NULL) continue;
                                while (TRUE)
                                {
                                    next = (char *) memchr(str, '\n', _buffer + dwRead - str) + 1;
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
                    }
                    else if (verbose)
                    {
                        memcpy(p, "gdb.exe: No such file or directory.\n", 36);
                        p += 36;
                    }
                } else ContinueDebugEvent(DebugEvent.dwProcessId,
                    DebugEvent.dwThreadId,
                    DBG_EXCEPTION_NOT_HANDLED);
                *p = '\0';
                printf(buffer);
                if (debug == GNU) exit(0);
                continue;
        }
        ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, DBG_CONTINUE);
    }
}
