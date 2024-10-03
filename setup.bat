@echo off
set _NO_DEBUG_HEAP=1
set GDB_SUPPRESS_SOURCES_WARNING=1
del /f /q /ah %temp%\gdbinit
where gdb.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [WARNING] gdb.exe not found, cannot use /G options.
where dbghelp.dll >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [ERROR] dbghelp.dll missing. Install it here: https://www.dll-files.com/ucrtbase.dll.html
where ucrtbase.dll >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [ERROR] ucrtbase.dll missing. Install it here: https://www.dll-files.com/ucrtbase.dll.html
where api-ms-win-crt-convert-l1-1-0.dll >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [ERROR] api-ms-win-crt-convert-l1-1-0.dll missing. Install it here: https://www.dll-files.com/ucrtbase.dll.html
where api-ms-win-crt-environment-l1-1-0.dll >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [ERROR] api-ms-win-crt-environment-l1-1-0.dll missing. Install it here: https://www.dll-files.com/ucrtbase.dll.html
where api-ms-win-crt-heap-l1-1-0.dll >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [ERROR] api-ms-win-crt-heap-l1-1-0.dll missing. Install it here: https://www.dll-files.com/ucrtbase.dll.html
where api-ms-win-crt-math-l1-1-0.dll >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [ERROR] api-ms-win-crt-math-l1-1-0.dll missing. Install it here: https://www.dll-files.com/ucrtbase.dll.html
where api-ms-win-crt-private-l1-1-0.dll >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [ERROR] api-ms-win-crt-private-l1-1-0.dll missing. Install it here: https://www.dll-files.com/ucrtbase.dll.html
where api-ms-win-crt-runtime-l1-1-0.dll >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [ERROR] api-ms-win-crt-runtime-l1-1-0.dll missing. Install it here: https://www.dll-files.com/ucrtbase.dll.html
where api-ms-win-crt-stdio-l1-1-0.dll >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [ERROR] api-ms-win-crt-stdio-l1-1-0.dll missing. Install it here: https://www.dll-files.com/ucrtbase.dll.html
where api-ms-win-crt-string-l1-1-0.dll >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [ERROR] api-ms-win-crt-string-l1-1-0.dll missing. Install it here: https://www.dll-files.com/ucrtbase.dll.html
where api-ms-win-crt-time-l1-1-0.dll >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [ERROR] api-ms-win-crt-time-l1-1-0.dll. Install it here: https://www.dll-files.com/ucrtbase.dll.html
timeout -1
