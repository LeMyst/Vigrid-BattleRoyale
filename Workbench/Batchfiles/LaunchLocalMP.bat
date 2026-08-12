@echo off
REM Launches the dedicated server plus N local clients.
REM
REM Usage: LaunchLocalMP.bat [1|2|3]   (defaults to 1)

setlocal

set "clientCount=%~1"
if not defined clientCount set "clientCount=1"
if %clientCount% LSS 1 set "clientCount=1"
if %clientCount% GTR 3 set "clientCount=3"

call "%~dp0SetupLaunch.bat" MP
if errorlevel 1 exit /b 1

call "%~dp0_LaunchServer.bat" %PlayerSteamID%

REM Give the server a head start before the first client connects.
timeout /t 5 /nobreak >nul

for /L %%i in (1,1,%clientCount%) do call :client %%i

exit /b 0

:client
if "%~1"=="1" set "slot=A"
if "%~1"=="2" set "slot=B"
if "%~1"=="3" set "slot=C"
call "%~dp0_LaunchClient.bat" %slot%
timeout /t 2 /nobreak >nul
exit /b 0
