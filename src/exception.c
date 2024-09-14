#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <dbghelp.h>
#include "addr.c"

#if defined(__GNUC__) || defined(__clang__)
#if defined(_DEBUG) && !defined(__OPTIMIZE__)
__attribute__((no_stack_protector, nothrow))
#else
__attribute__((always_inline, flatten, no_stack_protector, nothrow))
#endif
#endif

static inline const char *GetExceptionMessage(DWORD ExceptionCode)
{
    switch (ExceptionCode)
    {
        case 0xC0000005: // EXCEPTION_ACCESS_VIOLATION
            return "access violation";
        case 0xC0000006: // EXCEPTION_IN_PAGE_ERROR
            return "in page error";
        case 0xC0000008: // EXCEPTION_INVALID_HANDLE
            return "invalid handle";
        case 0xC000001D: // EXCEPTION_ILLEGAL_INSTRUCTION
            return "illegal instruction";
        case 0xC0000025: // EXCEPTION_NONCONTINUABLE_EXCEPTION
            return "cannot continue";
        case 0xC0000026: // EXCEPTION_INVALID_DISPOSITION
            return "invalid disposition";
        case 0xC000008C: // EXCEPTION_ARRAY_BOUNDS_EXCEEDED
            return "array bounds exceeded";
        case 0xC000008D: // EXCEPTION_FLT_DENORMAL_OPERAND
            return "floating-point denormal operand";
        case 0xC000008E: // EXCEPTION_FLT_DIVIDE_BY_ZERO
            return "floating-point division by zero";
        case 0xC000008F: // EXCEPTION_FLT_INEXACT_RESULT
            return "floating-point inexact result";
        case 0xC0000090: // EXCEPTION_FLT_INVALID_OPERATION
            return "floating-point invalid operation";
        case 0xC0000091: // EXCEPTION_FLT_OVERFLOW
            return "floating-point overflow";
        case 0xC0000092: // EXCEPTION_FLT_STACK_CHECK
            return "floating-point stack check";
        case 0xC0000093: // EXCEPTION_FLT_UNDERFLOW
            return "floating-point underflow";
        case 0xC0000094: // EXCEPTION_INT_DIVIDE_BY_ZERO
            return "integer division by zero";
        case 0xC0000095: // EXCEPTION_INT_OVERFLOW
            return "integer overflow";
        case 0xC0000096: // EXCEPTION_PRIV_INSTRUCTION
            return "privileged instruction";
        case 0xC00000FD: // EXCEPTION_STACK_OVERFLOW
            return "stack overflow";
        case 0xC0000194: // EXCEPTION_POSSIBLE_DEADLOCK
            return "possible deadlock condition";
        case 0xC0000409: // STATUS_STACK_BUFFER_OVERRUN
            //https://devblogs.microsoft.com/oldnewthing/20190108-00/?p=100655
            return "fast fail";
        case 0xC000041D: // STATUS_FATAL_USER_CALLBACK_EXCEPTION
            return "fatal user callback exception";
        case 0xC0000420: // STATUS_ASSERTION_FAILURE
            return "assertion failure";
        case 0xE0434f4D: // STATUS_CLR_EXCEPTION
            return "CLR exception";
        case 0xE06D7363: // STATUS_CPP_EH_EXCEPTION
            return "C++ exception handling exception";
        case 0x80000001: // EXCEPTION_GUARD_PAGE
            return "guard page exception";
        case 0x80000002: // EXCEPTION_DATATYPE_MISALIGNMENT
            return "alignment fault";
        case 0x80000003: // EXCEPTION_BREAKPOINT
            return "breakpoint";
        case 0x80000004: // EXCEPTION_SINGLE_STEP
            return "single step";
        case 0x4000001F: // STATUS_WX86_BREAKPOINT
            return "breakpoint";
        case 0x40000015: // STATUS_FATAL_APP_EXIT
            return "fast application exit";
        case 0x40010003: // DBG_TERMINATE_THREAD
            return "terminate thread";
        case 0x40010004: // DBG_TERMINATE_PROCESS
            return "terminate process";
        case 0x40010005: // DBG_CONTROL_C
            return "CONTROL+C";
        case 0x40010008: // DBG_CONTROL_BREAK
            return "CONTROL+BREAK";
        case 0x406D1388:
            return "thread name exception";
        case 1717: //RPC_S_UNKNOWN_IF
            return "unknown interface";
        case 1722: //RPC_S_SERVER_UNAVAILABLE
            return "server unavailable";
        default:
            return "unknown exception";
    }
}

#if defined(__GNUC__) || defined(__clang__)
#if defined(_DEBUG) && !defined(__OPTIMIZE__)
__attribute__((no_stack_protector, nothrow))
#else
__attribute__((always_inline, flatten, no_stack_protector, nothrow))
#endif
#endif

static inline char *FormatDebugException(DEBUG_EVENT *DebugEvent, char *buffer, char *p, BOOL bWow64)
{
    int temp;
    const char *ptr = GetExceptionMessage(DebugEvent->u.Exception.ExceptionRecord.ExceptionCode);
    temp = strlen(ptr);
    memcpy(buffer, ptr, temp);
    buffer += temp;
    if (DebugEvent->u.Exception.ExceptionRecord.ExceptionCode == 0xC0000005 ||
        DebugEvent->u.Exception.ExceptionRecord.ExceptionCode == 0xC0000006)
    {
        *buffer = ' ';
        ++buffer;
        switch (DebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[0])
        {
            case 0:
                memcpy(buffer, "reading from", 12);
                buffer += 12;
                break;
            case 1:
                memcpy(buffer, "writing to", 10);
                buffer += 10;
                break;
            case 8:
                memcpy(buffer, "DEP violation at", 16);
                buffer += 16;
                break;
        }
        memcpy(buffer, " location 0x", 12);
        buffer += 12;
        if (bWow64) _ultoaddr((ULONGLONG) DebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[1], buffer, p);
        else ulltoaddr((ULONGLONG) DebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[1], buffer, p);
        buffer += 16;
    }
    return buffer;
}

#define FormatVerboseDebugException(p, ExceptionCode) \
({ \
    switch (ExceptionCode) \
    { \
        case 0xC0000005: \
            memcpy(p, "The thread tried to read from or write to a virtual address for which it does not have the appropriate access", 109); \
            p += 109; \
            break; \
        case 0xC0000006: \
            memcpy(p, "The thread tried to access a page that was not present, and the system was unable to load the page", 98); \
            p += 98; \
            break; \
        case 0xC0000008: \
            memcpy(p, "The thread used a handle to a kernel object that was invalid", 60); \
            p += 60; \
            break; \
        case 0xC000001D: \
            memcpy(p, "The thread tried to execute an invalid instruction", 50); \
            p += 50; \
            break; \
        case 0xC0000025: \
            memcpy(p, "The thread tried to continue execution after a noncontinuable exception occurred", 80); \
            p += 80; \
            break; \
        case 0xC0000026: \
            memcpy(p, "An exception handler returned an invalid disposition to the exception dispatcher", 80); \
            p += 80; \
            break; \
        case 0xC000008C: \
            memcpy(p, "The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking", 118); \
            p += 118; \
            break; \
        case 0xC000008D: \
            memcpy(p, "One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value", 152); \
            p += 152; \
            break; \
        case 0xC000008E: \
            memcpy(p, "The thread tried to divide a floating-point value by a floating-point divisor of zero", 85); \
            p += 85; \
            break; \
        case 0xC000008F: \
            memcpy(p, "The result of a floating-point operation cannot be represented exactly as a decimal fraction", 92); \
            p += 92; \
            break; \
        case 0xC0000090: \
            memcpy(p, "This exception represents any floating-point exception not included", 67); \
            p += 67; \
            break; \
        case 0xC0000091: \
            memcpy(p, "The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type", 106); \
            p += 106; \
            break; \
        case 0xC0000092: \
            memcpy(p, "The stack overflowed or underflowed as the result of a floating-point operation", 79); \
            p += 79; \
            break; \
        case 0xC0000093: \
            memcpy(p, "The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type", 103); \
            p += 103; \
            break; \
        case 0xC0000094: \
            memcpy(p, "The thread tried to divide an integer value by an integer divisor of zero", 73); \
            p += 73; \
            break; \
        case 0xC0000095: \
            memcpy(p, "The result of an integer operation caused a carry out of the most significant bit of the result", 95); \
            p += 95; \
            break; \
        case 0xC0000096: \
            memcpy(p, "The thread tried to execute an instruction whose operation is not allowed in the current machine mode", 101); \
            p += 101; \
            break; \
        case 0xC00000FD: \
            memcpy(p, "The thread used up its stack", 28); \
            p += 28; \
            break; \
        case 0xC0000194: \
            memcpy(p, "The wait operation on the critical section times out", 52); \
            p += 52; \
            break; \
        case 0xC0000409: \
            memcpy(p, "The system detected an overrun of a stack-based buffer in this application", 74); \
            p += 74; \
            break; \
        case 0xC000041D: \
            memcpy(p, "An unhandled exception was encountered during a user callback", 61); \
            p += 61; \
            break; \
        case 0xC0000420: \
            memcpy(p, "An assertion failure has occurred", 33); \
            p += 33; \
            break; \
        case 0xE0434f4D: \
            memcpy(p, "A managed code exception was encountered within .NET Common Language Runtime", 76); \
            p += 76; \
            break; \
        case 0xE06D7363: \
            memcpy(p, "A C++ exception has been thrown and is being handled or caught", 62); \
            p += 62; \
            break; \
        case 0x80000001: \
            memcpy(p, "The thread accessed memory allocated with the PAGE_GUARD modifier", 65); \
            p += 65; \
            break; \
        case 0x80000002: \
            memcpy(p, "The thread tried to read or write data that is misaligned on hardware that does not provide alignment", 101); \
            p += 101; \
            break; \
        case 0x80000003: \
            memcpy(p, "A breakpoint was encountered", 28); \
            p += 28; \
            break; \
        case 0x80000004: \
            memcpy(p, "A trace trap or other single-instruction mechanism signaled that one instruction has been executed", 98); \
            p += 98; \
            break; \
        case 0x4000001F: \
            memcpy(p, "A Win32 x86 breakpoint was encountered", 38); \
            p += 38; \
            break; \
        case 0x40000015: \
            memcpy(p, "The application caused an unhandled runtime exception during shutdown", 69); \
            p += 69; \
            break; \
        case 0x40010003: \
            memcpy(p, "The debugger terminated thread", 30); \
            p += 30; \
            break; \
        case 0x40010004: \
            memcpy(p, "The debugger terminated process", 31); \
            p += 31; \
            break; \
        case 0x40010005: \
            memcpy(p, "The debugger got control C", 26); \
            p += 26; \
            break; \
        case 0x40010008: \
            memcpy(p, "The debugger received control break", 35); \
            p += 35; \
            break; \
        case 0x406D1388: \
            memcpy(p, "The thread set its own name by raising exception", 48); \
            p += 48; \
            break; \
        case 1717: \
            memcpy(p, "The interface is unknown", 24); \
            p += 24; \
            break; \
        case 1722: \
            memcpy(p, "The RPC server is unavailable", 29); \
            p += 29; \
            break; \
    } \
    p; \
})

#if defined(__GNUC__) || defined(__clang__)
#if defined(_DEBUG) && !defined(__OPTIMIZE__)
__attribute__((no_stack_protector, nothrow))
#else
__attribute__((always_inline, flatten, no_stack_protector, nothrow))
#endif
#endif

static inline char *FormatSourceCode(char *fname, int line, char *p, BOOL verbose)
{
    int temp;
    char buffer[4096];
    FILE *fp;
    temp = 0;
    fp = fopen(fname, "r");
    if (fp == NULL && verbose)
    {
        p += sprintf(p, "%s: No such file or directory.\n", fname);
        return p;
    }
    while (fgets(buffer, sizeof(buffer), fp))
    {
        ++temp;
        if (temp == line)
        {
            p += sprintf(p, "%8u | %s", line, buffer);
            break;
        }
    }
    fclose(fp);
    return p;
}

#if defined(__GNUC__) || defined(__clang__)
#if defined(_DEBUG) && !defined(__OPTIMIZE__)
__attribute__((no_stack_protector, nothrow))
#else
__attribute__((always_inline, flatten, no_stack_protector, nothrow))
#endif
#endif

static inline char *FormatFileLine(IMAGEHLP_LINE64 *lpLine, char *p, BOOL Console, BOOL verbose)
{
    int temp;
    temp = strlen(lpLine->FileName);
    memcpy(p, lpLine->FileName, temp);
    p += temp;
    if (Console)
    {
        *p = '\x1b';
        ++p;
        *p = '[';
        ++p;
        *p = 'm';
        ++p;
    }
    *p = ':';
    ++p;
    _ultoa(lpLine->LineNumber, p, 10);
    p += strlen(p);
    *p = '\n';
    ++p;
    return FormatSourceCode(lpLine->FileName, lpLine->LineNumber, p, verbose);
}
