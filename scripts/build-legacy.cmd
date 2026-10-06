@echo off
setlocal

set "COMPILER=%~1"
set "CONFIG=%~2"
set "AUDIO=%~3"
set "OPL=%~4"
set "RUNTIME=%~5"
set "ACTION=%~6"

if "%COMPILER%"=="" goto usage
if "%CONFIG%"=="" set "CONFIG=Release"
if "%AUDIO%"=="" set "AUDIO=standard"
if "%OPL%"=="" set "OPL=nuked"
if "%RUNTIME%"=="" set "RUNTIME=static"
if "%ACTION%"=="" set "ACTION=build"

set "GENERATOR="
if /I "%COMPILER%"=="vc6" set "GENERATOR=Visual Studio 6"
if /I "%COMPILER%"=="vs2002" set "GENERATOR=Visual Studio 7"
if /I "%COMPILER%"=="vs2003" set "GENERATOR=Visual Studio 7 .NET 2003"
if /I "%COMPILER%"=="vs2005" set "GENERATOR=Visual Studio 8 2005"
if "%GENERATOR%"=="" goto bad_compiler

if /I not "%CONFIG%"=="Debug" if /I not "%CONFIG%"=="Release" goto bad_config
if /I not "%AUDIO%"=="standard" if /I not "%AUDIO%"=="silent" goto bad_audio
if /I not "%OPL%"=="nuked" if /I not "%OPL%"=="dbopl" goto bad_opl
if /I not "%RUNTIME%"=="static" if /I not "%RUNTIME%"=="dynamic" goto bad_runtime
if /I not "%ACTION%"=="build" if /I not "%ACTION%"=="test" if /I not "%ACTION%"=="package" goto bad_action

set "STATIC_RUNTIME=ON"
if /I "%RUNTIME%"=="dynamic" set "STATIC_RUNTIME=OFF"

for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "BUILD_DIR=%ROOT%\build\legacy-%COMPILER%-%AUDIO%-%OPL%-%RUNTIME%"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if errorlevel 1 goto failed

pushd "%BUILD_DIR%"
cmake -G "%GENERATOR%" -DWG_AUDIO_BACKEND=%AUDIO% -DWG_OPL_BACKEND=%OPL% -DWG_STATIC_MSVC_RUNTIME=%STATIC_RUNTIME% "%ROOT%\cmake\legacy"
if errorlevel 1 goto failed_popd

if /I "%ACTION%"=="package" goto package
cmake --build . --config %CONFIG%
if errorlevel 1 goto failed_popd

if /I "%ACTION%"=="test" (
    pushd "%CONFIG%"
    wolf3d-public-api-consumer.exe
    if errorlevel 1 goto failed_test
    popd
)

goto completed

:package
cmake --build . --config %CONFIG% --target library_release
if errorlevel 1 goto failed_popd

:completed
popd
echo.
echo Legacy %COMPILER% %CONFIG% %ACTION% completed:
echo   %BUILD_DIR%\%CONFIG%
exit /b 0

:failed_test
popd
goto failed_popd

:usage
echo Usage: scripts\build-legacy.cmd COMPILER [CONFIG] [AUDIO] [OPL] [RUNTIME] [ACTION]
echo.
echo   COMPILER  vc6, vs2002, vs2003, or vs2005
echo   CONFIG    Release ^(default^) or Debug
echo   AUDIO     standard ^(default^) or silent
echo   OPL       nuked ^(default^) or dbopl
echo   RUNTIME   static ^(default^) or dynamic
echo   ACTION    build ^(default^), test, or package
exit /b 2

:bad_compiler
echo Unsupported compiler: %COMPILER%
goto usage

:bad_config
echo Unsupported configuration: %CONFIG%
goto usage

:bad_audio
echo Unsupported audio backend: %AUDIO%
goto usage

:bad_opl
echo Unsupported OPL backend: %OPL%
goto usage

:bad_runtime
echo Unsupported runtime: %RUNTIME%
goto usage

:bad_action
echo Unsupported action: %ACTION%
goto usage

:failed_popd
popd
:failed
echo Legacy build failed.
exit /b 1
