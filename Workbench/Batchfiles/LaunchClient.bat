@echo off
REM Launches a single client that connects to a server on 127.0.0.1.
REM
REM Usage: LaunchClient.bat [A|B|C]   (defaults to A)

setlocal

call "%~dp0SetupLaunch.bat" MP
if errorlevel 1 exit /b 1

call "%~dp0_LaunchClient.bat" %1
