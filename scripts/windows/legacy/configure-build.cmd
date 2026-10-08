@echo off
setlocal
echo wolf3d-lib guided Windows XP build
echo Scanning VC6 through Visual Studio 2005...
echo.

set "DEFAULT_COMPILER="
call :detect vc6 "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\CL.EXE"
call :detect vs2002 "%ProgramFiles%\Microsoft Visual Studio .NET\Vc7\bin\cl.exe"
call :detect vs2003 "%ProgramFiles%\Microsoft Visual Studio .NET 2003\Vc7\bin\cl.exe"
call :detect vs2005 "%ProgramFiles%\Microsoft Visual Studio 8\VC\bin\cl.exe"
if "%DEFAULT_COMPILER%"=="" goto no_compiler
cmake --version >nul 2>&1
if errorlevel 1 goto no_cmake

echo.
set /p "COMPILER=Compiler [%DEFAULT_COMPILER%]: "
if "%COMPILER%"=="" set "COMPILER=%DEFAULT_COMPILER%"
call :validate_compiler %COMPILER%
if errorlevel 1 goto bad_compiler
set /p "ACTION=Result - package, test, or build [package]: "
if "%ACTION%"=="" set "ACTION=package"
set /p "CONFIG=Configuration - Release or Debug [Release]: "
if "%CONFIG%"=="" set "CONFIG=Release"
set /p "RUNTIME=Compiler runtime - static or dynamic [static]: "
if "%RUNTIME%"=="" set "RUNTIME=static"
set /p "DRIVERS=Compiled OPL drivers - all, a name, or hyphenated pair [all]: "
if "%DRIVERS%"=="" set "DRIVERS=all"
set /p "DEFAULT_OPL=Default OPL driver - nuked, dbopl, or silent [nuked]: "
if "%DEFAULT_OPL%"=="" set "DEFAULT_OPL=nuked"

echo.
echo Build plan:
echo   %COMPILER% x86, %CONFIG%, %ACTION%
echo   runtime=%RUNTIME%, drivers=%DRIVERS%, default=%DEFAULT_OPL%
echo.
echo Reproducible command:
echo   scripts\windows\legacy\build.cmd %COMPILER% %CONFIG% %DRIVERS% %DEFAULT_OPL% %RUNTIME% %ACTION%
echo.
set /p "CONFIRM=Run this build now? [Y/n]: "
if /I "%CONFIRM%"=="n" exit /b 0
if /I "%CONFIRM%"=="no" exit /b 0
call "%~dp0build.cmd" %COMPILER% %CONFIG% %DRIVERS% %DEFAULT_OPL% %RUNTIME% %ACTION%
exit /b %ERRORLEVEL%

:detect
if exist "%~2" (
  echo   %~1 ready - %~2
  set "DEFAULT_COMPILER=%~1"
) else echo   %~1 not found
exit /b 0

:validate_compiler
if /I "%~1"=="vc6" if exist "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\CL.EXE" exit /b 0
if /I "%~1"=="vs2002" if exist "%ProgramFiles%\Microsoft Visual Studio .NET\Vc7\bin\cl.exe" exit /b 0
if /I "%~1"=="vs2003" if exist "%ProgramFiles%\Microsoft Visual Studio .NET 2003\Vc7\bin\cl.exe" exit /b 0
if /I "%~1"=="vs2005" if exist "%ProgramFiles%\Microsoft Visual Studio 8\VC\bin\cl.exe" exit /b 0
exit /b 1

:no_compiler
echo No supported XP-era Visual C++ compiler was detected.
exit /b 2
:no_cmake
echo CMake was not found on PATH. See docs\building.md for legacy prerequisites.
exit /b 2
:bad_compiler
echo The selected compiler is not installed or is unsupported.
exit /b 2
