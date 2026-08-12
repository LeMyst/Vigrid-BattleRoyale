@echo off
REM Removes the persistence folders ("storage_*") under MPMission.
REM
REM MPMission goes through _SafeDir.bat first: it comes from the gitignored
REM user.cfg, and an "rd /s /q" under a drive root would delete every top-level
REM storage_* folder on that drive.

setlocal

if not defined MPMission (
	call "%~dp0_Config.bat" ClearStorage
	if errorlevel 1 exit /b 1
)

if not defined MPMission (
	echo ClearStorage: MPMission is not set, nothing to do.
	exit /b 0
)

set "brMission=%MPMission%"

REM A relative -mission= is resolved by the game against the server directory,
REM since LaunchSteamClient.bat starts it with /D "<GameDirectory>". Resolve it the
REM same way rather than against this script's cwd, which is Batchfiles.
if not "%brMission:~1,1%"==":" if defined serverDirectory call :join "%serverDirectory%" "%brMission%"

call "%~dp0_SafeDir.bat" "%brMission%"
if errorlevel 2 goto :nodir
if errorlevel 1 exit /b 1

for /d %%i in ("%brSafeDir%\storage_*") do (
	echo Removing folder "%%~i"
	rd /s /q "%%~i"
)

exit /b 0

:nodir
echo ClearStorage: "%brMission%" is not a directory, nothing to do.
exit /b 0

:join
set "_jBase=%~1"
if "%_jBase:~-1%"=="\" set "_jBase=%_jBase:~0,-1%"
set "brMission=%_jBase%\%~2"
set "_jBase="
exit /b 0
