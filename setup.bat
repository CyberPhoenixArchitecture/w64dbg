@echo off
set _NO_DEBUG_HEAP=1
where gdb.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 echo [WARNING] gdb.exe not found, cannot use /G options.
timeout -1
