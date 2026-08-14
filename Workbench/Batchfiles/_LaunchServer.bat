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

REM Both refuse to run if the configured path is an unsafe delete target. Abort the
REM launch rather than starting a server against a misconfigured profile/mission.
call "%~dp0ClearLogs.bat" "%serverProfileDirectory%"
if errorlevel 1 exit /b 1

REM KeepStorage=1 skips the persistence wipe. Wiping it forces the Central Economy to cold-spawn
REM the whole map every launch - measured at 33921 items and 51-73 s, over half of boot - so for an
REM iteration loop that is not testing loot, keeping storage is the single biggest saving available.
REM It is off by default because a wipe is what makes a run reproducible: with storage kept, players
REM rejoin their persisted character and vehicles stay where the last run left them.
REM Not a ( ) block - serverProfileDirectory can contain parentheses ("Program Files (x86)").
if /i "%KeepStorage%"=="1" goto :keepstorage

call "%~dp0ClearStorage.bat"
if errorlevel 1 exit /b 1
goto :launch

:keepstorage
echo KeepStorage=1 - leaving "storage_*" in place, skipping the cold CE loot spawn.

:launch

call "%~dp0LaunchSteamClient.bat" %brSteamID% "%serverDirectory%" %serverEXE% %serverLaunchParams% "-config=%serverConfig%" -port=%port% "-profiles=%serverProfileDirectory%" "-mission=%MPMission%" "-mod=%modList%"

exit /b 0
