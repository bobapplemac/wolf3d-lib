@echo off
setlocal
for %%I in ("%~dp0..\..") do set "WG_ROOT=%%~fI"

if "%WATCOM%"=="" (
    echo WATCOM is not set. Run the Open Watcom environment setup first.
    exit /b 2
)

set "INCLUDE=%WG_ROOT%\include;%WG_ROOT%\src;%WG_ROOT%\third_party\DBOPL;%WG_ROOT%\third_party\Nuked-OPL3;%INCLUDE%"
set "WCC386=-mf -w4 -dWOLF3D_STATIC -dWG_OPL_ENABLE_NUKED=1 -dWG_OPL_ENABLE_DBOPL=1 -dWG_OPL_ENABLE_SILENT=1 -dWG_OPL_ENABLE_ADLIB=1 %WCC386%"

if exist "%WATCOM%\binnt64\ide.exe" set "WG_IDE=%WATCOM%\binnt64\ide.exe"
if not defined WG_IDE if exist "%WATCOM%\binnt\ide.exe" set "WG_IDE=%WATCOM%\binnt\ide.exe"
if not defined WG_IDE if exist "%WATCOM%\binw\ide.exe" set "WG_IDE=%WATCOM%\binw\ide.exe"
if not defined WG_IDE (
    echo Open Watcom IDE.EXE was not found below %WATCOM%.
    exit /b 2
)

start "Open Watcom IDE - wolf3d-lib" "%WG_IDE%" "%~dp0wolf3d-lib.wpj"
