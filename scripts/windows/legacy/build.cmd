@echo off
setlocal

set "COMPILER=%~1"
set "CONFIG=%~2"
set "DRIVERS=%~3"
set "DEFAULT_OPL=%~4"
set "RUNTIME=%~5"
set "ACTION=%~6"

if "%COMPILER%"=="" goto usage
if "%CONFIG%"=="" set "CONFIG=Release"
if "%DRIVERS%"=="" set "DRIVERS=all"
if "%DEFAULT_OPL%"=="" set "DEFAULT_OPL=nuked"
if "%RUNTIME%"=="" set "RUNTIME=static"
if "%ACTION%"=="" set "ACTION=build"

set "GENERATOR="
if /I "%COMPILER%"=="vc6" set "GENERATOR=Visual Studio 6"
if /I "%COMPILER%"=="vs2002" set "GENERATOR=Visual Studio 7"
if /I "%COMPILER%"=="vs2003" set "GENERATOR=Visual Studio 7 .NET 2003"
if /I "%COMPILER%"=="vs2005" set "GENERATOR=Visual Studio 8 2005"
if "%GENERATOR%"=="" goto bad_compiler

if /I not "%CONFIG%"=="Debug" if /I not "%CONFIG%"=="Release" goto bad_config
set "ENABLE_NUKED=OFF"
set "ENABLE_DBOPL=OFF"
set "ENABLE_SILENT=OFF"
if /I "%DRIVERS%"=="all" set "ENABLE_NUKED=ON"
if /I "%DRIVERS%"=="all" set "ENABLE_DBOPL=ON"
if /I "%DRIVERS%"=="all" set "ENABLE_SILENT=ON"
if /I "%DRIVERS%"=="nuked" set "ENABLE_NUKED=ON"
if /I "%DRIVERS%"=="dbopl" set "ENABLE_DBOPL=ON"
if /I "%DRIVERS%"=="silent" set "ENABLE_SILENT=ON"
if /I "%DRIVERS%"=="nuked-dbopl" set "ENABLE_NUKED=ON"
if /I "%DRIVERS%"=="nuked-dbopl" set "ENABLE_DBOPL=ON"
if /I "%DRIVERS%"=="nuked-silent" set "ENABLE_NUKED=ON"
if /I "%DRIVERS%"=="nuked-silent" set "ENABLE_SILENT=ON"
if /I "%DRIVERS%"=="dbopl-silent" set "ENABLE_DBOPL=ON"
if /I "%DRIVERS%"=="dbopl-silent" set "ENABLE_SILENT=ON"
if "%ENABLE_NUKED%%ENABLE_DBOPL%%ENABLE_SILENT%"=="OFFOFFOFF" goto bad_drivers
if /I "%DEFAULT_OPL%"=="nuked" if /I not "%ENABLE_NUKED%"=="ON" goto bad_opl
if /I "%DEFAULT_OPL%"=="dbopl" if /I not "%ENABLE_DBOPL%"=="ON" goto bad_opl
if /I "%DEFAULT_OPL%"=="silent" if /I not "%ENABLE_SILENT%"=="ON" goto bad_opl
if /I not "%DEFAULT_OPL%"=="nuked" if /I not "%DEFAULT_OPL%"=="dbopl" if /I not "%DEFAULT_OPL%"=="silent" goto bad_opl
if /I not "%RUNTIME%"=="static" if /I not "%RUNTIME%"=="dynamic" goto bad_runtime
if /I not "%ACTION%"=="build" if /I not "%ACTION%"=="test" if /I not "%ACTION%"=="package" goto bad_action

set "STATIC_RUNTIME=ON"
if /I "%RUNTIME%"=="dynamic" set "STATIC_RUNTIME=OFF"

for %%I in ("%~dp0..\..\..") do set "ROOT=%%~fI"
set "BUILD_DIR=%ROOT%\build\legacy-%COMPILER%-%DRIVERS%-%DEFAULT_OPL%-%RUNTIME%"

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
    if errorlevel 1 goto failed
)

pushd "%BUILD_DIR%"
call "%~dp0cmake-driver.cmd" -G "%GENERATOR%" -DWG_ENABLE_OPL_NUKED=%ENABLE_NUKED% -DWG_ENABLE_OPL_DBOPL=%ENABLE_DBOPL% -DWG_ENABLE_OPL_SILENT=%ENABLE_SILENT% -DWG_DEFAULT_OPL_DRIVER=%DEFAULT_OPL% -DWG_STATIC_MSVC_RUNTIME=%STATIC_RUNTIME% "%ROOT%\cmake\legacy"
if errorlevel 1 goto failed_popd

if /I "%ACTION%"=="package" goto package
call "%~dp0cmake-driver.cmd" --build . --config %CONFIG%
if errorlevel 1 goto failed_popd

if /I "%ACTION%"=="test" (
    pushd "%CONFIG%"
    wolf3d-public-api-consumer.exe
    if errorlevel 1 goto failed_test
    popd
)

goto completed

:package
call "%~dp0cmake-driver.cmd" --build . --config %CONFIG% --target library_release
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
echo Usage: scripts\windows\legacy\build.cmd COMPILER [CONFIG] [DRIVERS] [DEFAULT_OPL] [RUNTIME] [ACTION]
echo.
echo   COMPILER  vc6, vs2002, vs2003, or vs2005
echo   CONFIG    Release ^(default^) or Debug
echo   DRIVERS   all ^(default^), nuked, dbopl, silent, or a hyphenated pair
echo   DEFAULT_OPL nuked ^(default^), dbopl, or silent; must be included
echo   RUNTIME   static ^(default^) or dynamic
echo   ACTION    build ^(default^), test, or package
exit /b 2

:bad_compiler
echo Unsupported compiler: %COMPILER%
goto usage

:bad_config
echo Unsupported configuration: %CONFIG%
goto usage

:bad_drivers
echo Unsupported driver set: %DRIVERS%
goto usage

:bad_opl
echo Default OPL driver is invalid or not compiled: %DEFAULT_OPL%
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
