@echo off
setlocal

set "CONFIG=%~1"
set "PLATFORM=%~2"
set "ACTION=%~3"
if "%CONFIG%"=="" set "CONFIG=Release"
if "%PLATFORM%"=="" set "PLATFORM=Win32"
if "%ACTION%"=="" set "ACTION=build"

for %%I in ("%~dp0..\..\..") do set "ROOT=%%~fI"
set "ARCH=x86"
if /I "%PLATFORM%"=="x64" set "ARCH=x64"
set "PRESET=windows-vs2008-dev-%ARCH%"
set "BUILD_DIR=%ROOT%\build\windows-vs2008-ide-%ARCH%"
set "CMAKE=%ROOT%\scripts\windows\cmake-driver.cmd"

if /I "%ACTION%"=="clean" goto clean
pushd "%ROOT%"
call "%CMAKE%" --preset %PRESET% -B "%BUILD_DIR%"
if errorlevel 1 goto failed

set "TARGET=wolf3d"
if /I "%ACTION%"=="publish" set "TARGET=library-release"
set "CLEAN_ARG="
if /I "%ACTION%"=="rebuild" set "CLEAN_ARG=--clean-first"
call "%CMAKE%" --build "%BUILD_DIR%" --config "%CONFIG%" --target %TARGET% %CLEAN_ARG%
if errorlevel 1 goto failed
popd
exit /b 0

:clean
if not exist "%BUILD_DIR%\CMakeCache.txt" exit /b 0
pushd "%ROOT%"
call "%CMAKE%" --build "%BUILD_DIR%" --config "%CONFIG%" --target clean
if errorlevel 1 goto failed
popd
exit /b 0

:failed
popd
exit /b 1
