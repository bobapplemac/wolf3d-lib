@echo off
setlocal
rem VS2002-2005 may have a different PATH from an ordinary command prompt.
if not "%WOLF3D_LEGACY_CMAKE%"=="" (
    set "LEGACY_CMAKE=%WOLF3D_LEGACY_CMAKE%"
    goto run
)
set "LEGACY_CMAKE="
rem Reuse the exact CMake that configured an existing build tree.
if exist CMakeCache.txt for /f "tokens=1,* delims==" %%A in ('%SystemRoot%\System32\findstr.exe /b /c:"CMAKE_COMMAND:INTERNAL=" CMakeCache.txt') do set "LEGACY_CMAKE=%%B"
if defined LEGACY_CMAKE if exist "%LEGACY_CMAKE%" goto run
for %%C in (cmake.exe) do set "LEGACY_CMAKE=%%~$PATH:C"
if defined LEGACY_CMAKE goto run
for %%C in (
    "%ProgramFiles%\CMake\bin\cmake.exe"
    "%ProgramFiles(x86)%\CMake\bin\cmake.exe"
    "%ProgramW6432%\CMake\bin\cmake.exe"
    "%ProgramFiles%\CMake 3.5\bin\cmake.exe"
    "%ProgramFiles(x86)%\CMake 3.5\bin\cmake.exe"
) do if exist "%%~C" (
    set "LEGACY_CMAKE=%%~C"
    goto run
)
echo ERROR: Legacy CMake was not found. Install CMake 3.5 or set 1>&2
echo WOLF3D_LEGACY_CMAKE to the full path of its cmake.exe. 1>&2
exit /b 1
:run
if not exist "%LEGACY_CMAKE%" (
    echo ERROR: CMake executable does not exist: "%LEGACY_CMAKE%" 1>&2
    exit /b 1
)
"%LEGACY_CMAKE%" %*
exit /b %ERRORLEVEL%
