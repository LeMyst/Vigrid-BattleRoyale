@echo off
REM Launches the dedicated server on its own.

setlocal

call "%~dp0SetupLaunch.bat" MP
if errorlevel 1 exit /b 1

call "%~dp0_LaunchServer.bat" 1
