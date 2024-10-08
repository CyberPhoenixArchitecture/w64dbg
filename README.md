# w64dbg

## About

w64dbg is an open-source binary tool debugging live user-mode code for 64-bit Windows OS. It can read executable's debug information in __PDB__ format itself and __DWARF__ format generated using GNU C/C++ Compiler in backend.

## Download

* [Releases](https://github.com/CyberPhoenixArchitecture/w64dbg/releases)

* [Git repository](https://github.com/CyberPhoenixArchitecture/w64dbg)

## Installation

After download build binaries or build it yourself, you should run **setup.bat** to ensure working environment is fine.

## Usage

    
    Usage: debug [...] executable [...]
    
    Description:
    This tool is used to debug an executable on 64-bit Windows OS.
    
    Parameter List:
    /B Do not ignore breakpoints.
    /D Load executable debug symbols.
    /G Load executable debug symbols using GDB backend.
    /Q Do not display verbose exception information.
    /O Display OutputDebugString string.
    /S Start executable with a new console.
    /T Specify to wait for the specified time period (in seconds)
                                     or until any key is pressed.
    /V Display verbose debug information.
    

## Sample

    
    **********************************************************************
    ** Visual Studio 2022 Developer Command Prompt v17.11.1
    ** Copyright (c) 2022 Microsoft Corporation
    **********************************************************************
    [vcvarsall.bat] Environment initialized for: 'x64'
    
    C:\w64dbg>cl /Zi sample.cpp
    Microsoft (R) C/C++ Optimizing Compiler Version 19.41.34120 for x64
    Copyright (C) Microsoft Corporation.  All rights reserved.
    
    sample.cpp
    Microsoft (R) Incremental Linker Version 14.41.34120.0
    Copyright (C) Microsoft Corporation.  All rights reserved.
    
    /out:sample.exe
    /debug
    sample.obj
    
    C:\w64dbg>debug /D sample.exe
    Thread #01 caused access violation writing to location 0x0000000000000001
    The thread tried to read from or write to a virtual address for which it does not have the appropriate access
    #0  0x00007ff7bd942331 in sample!__crt_stdio_input::input_processor<char,__crt_stdio_input::string_input_adapter<char> >::write_integer () at minkernel\crts\ucrt\inc\corecrt_internal_stdio_input.h:1573
    #1  0x00007ff7bd93f285 in sample!__crt_stdio_input::input_processor<char,__crt_stdio_input::string_input_adapter<char> >::process_integer_specifier () at minkernel\crts\ucrt\inc\corecrt_internal_stdio_input.h:1523
    #2  0x00007ff7bd93ebc7 in sample!__crt_stdio_input::input_processor<char,__crt_stdio_input::string_input_adapter<char> >::process_conversion_specifier () at minkernel\crts\ucrt\inc\corecrt_internal_stdio_input.h:1226
    #3  0x00007ff7bd93f81f in sample!__crt_stdio_input::input_processor<char,__crt_stdio_input::string_input_adapter<char> >::process_state () at minkernel\crts\ucrt\inc\corecrt_internal_stdio_input.h:1144
    #4  0x00007ff7bd93e60d in sample!__crt_stdio_input::input_processor<char,__crt_stdio_input::string_input_adapter<char> >::process () at minkernel\crts\ucrt\inc\corecrt_internal_stdio_input.h:1100
    #5  0x00007ff7bd94276a in sample!__stdio_common_vsscanf () at minkernel\crts\ucrt\src\appcrt\stdio\input.cpp:122
    #6  0x00007ff7bd91850a in sample!_vsscanf_l () at C:\Program Files (x86)\Windows Kits\10\include\10.0.26100.0\ucrt\stdio.h:2156
     2156  |    }
    #7  0x00007ff7bd918569 in sample!sscanf () at C:\Program Files (x86)\Windows Kits\10\include\10.0.26100.0\ucrt\stdio.h:2251
     2251  |        _Result = _vsscanf_l(_Buffer, _Format, NULL, _ArgList);
    #8  0x00007ff7bd9183fc in sample!Exception () at C:\w64dbg\sample.cpp:6
       6   |    sscanf("12345", "%d", (int *) 1);
    #9  0x00007ff7bd91849b in sample!Example::RootException () at C:\w64dbg\sample.cpp:13
      13   |        Exception(i * 2, j, "Hello");
    #10 0x00007ff7bd91845b in sample!Example::CauseException () at C:\w64dbg\sample.cpp:18
      18   |        RootException(4, 5.6f);
    #11 0x00007ff7bd91841e in sample!main () at C:\w64dbg\sample.cpp:25
      25   |    example.CauseException();
    #12 0x00007ff7bd9187d0 in sample!__scrt_common_main_seh () at D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:288
    #13 0x00007ff87e62257d in KERNEL32!BaseThreadInitThunk () from C:\WINDOWS\System32\KERNEL32.dll
    #14 0x00007ff87f12af28 in ntdll!RtlUserThreadStart () from C:\WINDOWS\System32\ntdll.dll
    
    C:\w64dbg>
    
    ...
    
    Microsoft Windows [Version 10.0.22631.4037]
    (c) Microsoft Corporation. All rights reserved.
    
    C:\w64dbg>g++ -g sample.cpp -o sample.exe
    
    C:\w64dbg>debug /G sample.exe
    Thread #01 caused stack overflow
    The thread used up its stack
    #0  0x00007ff6a0ce2983 in Example::CauseException (this=0x5ffe9f) at sample.cpp:11
      11   |        RootException();
    #1  0x00007ff6a0ce2968 in Example::RootException (this=0x5ffe9f) at sample.cpp:6
       6   |        CauseException();
    #2  0x00007ff6a0ce2988 in Example::CauseException (this=0x5ffe9f) at sample.cpp:11
      11   |        RootException();
    #3  0x00007ff6a0ce2968 in Example::RootException (this=0x5ffe9f) at sample.cpp:6
       6   |        CauseException();
    #4  0x00007ff6a0ce2988 in Example::CauseException (this=0x5ffe9f) at sample.cpp:11
      11   |        RootException();
    #5  0x00007ff6a0ce2968 in Example::RootException (this=0x5ffe9f) at sample.cpp:6
       6   |        CauseException();
    #6  0x00007ff6a0ce2988 in Example::CauseException (this=0x5ffe9f) at sample.cpp:11
      11   |        RootException();
    #7  0x00007ff6a0ce2968 in Example::RootException (this=0x5ffe9f) at sample.cpp:6
       6   |        CauseException();
    #8  0x00007ff6a0ce2988 in Example::CauseException (this=0x5ffe9f) at sample.cpp:11
      11   |        RootException();
    #9  0x00007ff6a0ce2968 in Example::RootException (this=0x5ffe9f) at sample.cpp:6
       6   |        CauseException();
    #10 0x00007ff6a0ce2988 in Example::CauseException (this=0x5ffe9f) at sample.cpp:11
      11   |        RootException();
    #11 0x00007ff6a0ce2968 in Example::RootException (this=0x5ffe9f) at sample.cpp:6
       6   |        CauseException();
    #12 0x00007ff6a0ce2988 in Example::CauseException (this=0x5ffe9f) at sample.cpp:11
      11   |        RootException();
    #13 0x00007ff6a0ce2968 in Example::RootException (this=0x5ffe9f) at sample.cpp:6
       6   |        CauseException();
    #14 0x00007ff6a0ce2988 in Example::CauseException (this=0x5ffe9f) at sample.cpp:11
      11   |        RootException();
    #15 0x00007ff6a0ce2968 in Example::RootException (this=0x5ffe9f) at sample.cpp:6
       6   |        CauseException();
    #16 0x00007ff6a0ce2988 in Example::CauseException (this=0x5ffe9f) at sample.cpp:11
      11   |        RootException();
    #17 0x00007ff6a0ce2968 in Example::RootException (this=0x5ffe9f) at sample.cpp:6
       6   |        CauseException();
    #18 0x00007ff6a0ce2988 in Example::CauseException (this=0x5ffe9f) at sample.cpp:11
      11   |        RootException();
    #19 0x00007ff6a0ce2968 in Example::RootException (this=0x5ffe9f) at sample.cpp:6
       6   |        CauseException();
    #20 0x00007ff6a0ce2988 in Example::CauseException (this=0x5ffe9f) at sample.cpp:11
      11   |        RootException();
    #21 0x00007ff6a0ce2968 in Example::RootException (this=0x5ffe9f) at sample.cpp:6
       6   |        CauseException();
    #22 0x00007ff6a0ce2988 in Example::CauseException (this=0x5ffe9f) at sample.cpp:11
      11   |        RootException();
      
    ...
    

See [here](sample) for more details.

## DLL Dependency

|                  DLL                  |      Location       |               Package                |
| ------------------------------------- | ------------------- | ------------------------------------ |
| dbghelp.dll                           | C:\Windows\System32 | Debugging Tools For Windows          |
| kernel32.dll                          | C:\Windows\System32 | Windows kernel module                |
| ucrtbase.dll                          | C:\Windows\System32 | Universal C Runtime                  |
| api-ms-win-crt-convert-l1-1-0.dll     | C:\Windows\System32 | Microsoft Visual C++ Redistributable |
| api-ms-win-crt-environment-l1-1-0.dll | C:\Windows\System32 | Microsoft Visual C++ Redistributable |
| api-ms-win-crt-heap-l1-1-0.dll        | C:\Windows\System32 | Microsoft Visual C++ Redistributable |
| api-ms-win-crt-math-l1-1-0.dll        | C:\Windows\System32 | Microsoft Visual C++ Redistributable |
| api-ms-win-crt-private-l1-1-0.dll     | C:\Windows\System32 | Microsoft Visual C++ Redistributable |
| api-ms-win-crt-runtime-l1-1-0.dll     | C:\Windows\System32 | Microsoft Visual C++ Redistributable |
| api-ms-win-crt-stdio-l1-1-0.dll       | C:\Windows\System32 | Microsoft Visual C++ Redistributable |
| api-ms-win-crt-string-l1-1-0.dll      | C:\Windows\System32 | Microsoft Visual C++ Redistributable |
| api-ms-win-crt-time-l1-1-0.dll        | C:\Windows\System32 | Microsoft Visual C++ Redistributable |

**NOTE**: If your Windows is missing one of these DLL files, download [here](https://www.dll-files.com).

## Requirements

Minium supported OS: Windows Vista

Processor architecture: x64

## Limitations

w64dbg cannot handles more than **MAX_THREAD** and **MAX_DLL** (which is 64 and 16 defined [here](src/main.c#L15)) as it is abnormal for a process to overcome these limits. If you want to adjust them, just change the definition in source files and then rebuild on your own. By the way, w64dbg will not print out more than 100 thread context frames as the only way to reach that unbelievable number is infinite recursive.

## Frequently Asked Questions

### How can I get errno value of a specific thread?

Currently there is no way to get **errno** value as it's not a variable. It's definition is:

    
    _CRTIMP extern int *__cdecl _errno(void);
    #define errno (*_errno())
    

References to errno actually call the internal **_errno()** function, which returns the error value for the caller thread.

### Which options should I pass to MSVC when compiling?

* **`/DEBUG`** : generate debug information

See [this](https://learn.microsoft.com/en-us/cpp/build/reference/debug-generate-debug-info) for more details.

### Which options should I pass to GCC/Clang when compiling?

* **`-g`** : generate debug information

See [this](https://gcc.gnu.org/onlinedocs/gcc/Debugging-Options.html#index-g) for more details.

* **`-fno-omit-frame-pointer`** : Do not omit the frame pointer in functions that don’t need one

See [this](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html#index-fomit-frame-pointer) for more details.

## License

w64dbg is licensed under the BSD-3-Clause license.

See [LICENSE](LICENSE) for more details.
