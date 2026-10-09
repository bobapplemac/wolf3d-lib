@echo off
call "%~dp0scripts\windows\git-preflight.cmd"
if errorlevel 1 exit /b 1
if not "%~1"=="" goto invoke
call "%~dp0scripts\windows\legacy\configure-build.cmd"
exit /b %ERRORLEVEL%
:invoke
call "%~dp0scripts\windows\legacy\build.cmd" %*
exit /b %ERRORLEVEL%
