@echo off
setlocal

CALL "%~dp0SetupLaunch.bat"

if not defined AdditionalMPMods (
    echo AdditionalMPMods parameter was not set in the project.cfg, ignoring.
) else (
    set mods=%mods%;%AdditionalMPMods%
)

CALL "%~dp0SetupModList.bat"

call "%~dp0ClearLogs.bat" "%serverProfileDirectory%"

call "%~dp0ClearStorage.bat"

call "%~dp0LaunchSteamClient.bat" %PlayerSteamID% "%serverDirectory%" %serverEXE% %serverLaunchParams% "-config=%serverConfig%" -port=%port% "-profiles=%serverProfileDirectory%" "-mission=%MPMission%" "-mod=%modList%"

PING 127.0.0.1 -n 5 > nul

call "%~dp0ClearLogs.bat" "%ClientProfileDirectory%"

call "%~dp0LaunchSteamClient.bat" %PlayerSteamID% "%gameDirectory%" %clientEXE% %clientLaunchParams% "-connect=127.0.0.1" -port=%port% "-profiles=%ClientProfileDirectory%" "-name=%playerName%" %password% "-mod=%modList%"
