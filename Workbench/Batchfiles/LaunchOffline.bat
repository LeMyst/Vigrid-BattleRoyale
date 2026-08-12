@echo off
REM Launches a single-player session with SPMission.
REM Deliberately bypasses LaunchSteamClient.bat - no Steam emulation is needed
REM for offline play.

setlocal

call "%~dp0SetupLaunch.bat" SP
if errorlevel 1 exit /b 1

@echo on
start /D "%gameDirectory%" %clientEXE% %clientLaunchParams% "-mod=%modList%" "-profiles=%ClientProfileDirectory%" "-name=%playerName%" "-mission=%SPMission%"
@echo off
