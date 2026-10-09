@echo off
setlocal
if "%WOLF3D_GIT_CHECK%"=="0" exit /b 0
git --version >nul 2>&1
if errorlevel 1 goto no_git
rem MSYS2 git --exec-path is POSIX; try Bash next to the selected git.exe.
for %%G in (git.exe) do set "GIT_PROGRAM=%%~$PATH:G"
for %%G in ("%GIT_PROGRAM%") do set "GIT_DIRECTORY=%%~dpG"
set "GIT_BASH=%GIT_DIRECTORY%bash.exe"
if exist "%GIT_BASH%" goto run
set "GIT_BASH=%GIT_DIRECTORY%..\bin\bash.exe"
if exist "%GIT_BASH%" goto run
set "GIT_BASH=%GIT_DIRECTORY%..\usr\bin\bash.exe"
if exist "%GIT_BASH%" goto run
set "GIT_EXEC="
for /f "delims=" %%G in ('git --exec-path 2^>nul') do set "GIT_EXEC=%%G"
set "GIT_BASH=%GIT_EXEC%/../../../bin/bash.exe"
if exist "%GIT_BASH%" goto run
set "GIT_BASH=%GIT_EXEC%/../../../usr/bin/bash.exe"
if exist "%GIT_BASH%" goto run
set "GIT_BASH=%GIT_EXEC%/../../bin/bash.exe"
if exist "%GIT_BASH%" goto run
echo Bash for Git was not found; building existing sources without an update check.
exit /b 0
:no_git
echo Git was not found; building existing sources.
exit /b 0
:run
"%GIT_BASH%" "%~dp0..\git-preflight.sh" "%~dp0..\.."
exit /b %ERRORLEVEL%
