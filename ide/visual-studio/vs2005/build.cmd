@echo off
setlocal
set "CONFIG=%~1"
set "ACTION=%~2"
if "%CONFIG%"=="" set "CONFIG=Release"
if "%ACTION%"=="" set "ACTION=build"
for %%I in ("%~dp0..\..\..") do set "ROOT=%%~fI"
set "BUILD_DIR=%ROOT%\build\legacy-vs2005-standard-nuked-static"
if /I "%ACTION%"=="clean" goto clean
if /I not "%ACTION%"=="rebuild" goto after_rebuild
call :clean_build
if errorlevel 1 exit /b 1
:after_rebuild
set "DISPATCH_ACTION=build"
if /I "%ACTION%"=="publish" set "DISPATCH_ACTION=package"
call "%ROOT%\scripts\windows\legacy\build.cmd" vs2005 %CONFIG% standard nuked static %DISPATCH_ACTION%
exit /b %errorlevel%
:clean
call :clean_build
exit /b %errorlevel%
:clean_build
if not exist "%BUILD_DIR%\CMakeCache.txt" exit /b 0
pushd "%BUILD_DIR%"
cmake --build . --config %CONFIG% --target clean
set "RESULT=%errorlevel%"
popd
exit /b %RESULT%