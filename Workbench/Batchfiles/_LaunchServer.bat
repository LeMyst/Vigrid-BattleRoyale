@echo off
REM Clears the server profile logs and storage, then launches the dedicated server.
REM
REM Usage: call "%~dp0_LaunchServer.bat" <SteamID>
REM
REM Expects SetupLaunch.bat to have run first (serverDirectory, serverEXE,
REM serverLaunchParams, serverConfig, serverProfileDirectory, port, MPMission and
REM modList must already be in the environment).

set "brSteamID=%~1"
if not defined brSteamID set "brSteamID=1"

call "%~dp0ClearLogs.bat" "%serverProfileDirectory%"

call "%~dp0ClearStorage.bat"

call "%~dp0LaunchSteamClient.bat" %brSteamID% "%serverDirectory%" %serverEXE% %serverLaunchParams% "-config=%serverConfig%" -port=%port% "-profiles=%serverProfileDirectory%" "-mission=%MPMission%" "-mod=%modList%"

exit /b 0
