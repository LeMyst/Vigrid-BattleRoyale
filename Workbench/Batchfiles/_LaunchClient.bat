@echo off
REM Launches one local client slot and connects it to 127.0.0.1.
REM
REM Usage: call "%~dp0_LaunchClient.bat" <A|B|C>
REM
REM The slot letter is the suffix used by the config keys, with slot A spelled as
REM no suffix at all: PlayerSteamID / PlayerBSteamID / PlayerCSteamID,
REM PlayerName / PlayerBName / PlayerCName,
REM ClientProfileDirectory / ClientBProfileDirectory / ClientCProfileDirectory.
REM
REM Expects SetupLaunch.bat to have run first (gameDirectory, clientEXE,
REM clientLaunchParams, port and modList must already be in the environment).

set "slot=%~1"
if not defined slot set "slot=A"
if /i "%slot%"=="A" set "slot="

call set "brSteamID=%%Player%slot%SteamID%%"
call set "brPlayerName=%%Player%slot%Name%%"
call set "brProfileDir=%%Client%slot%ProfileDirectory%%"

if not defined brProfileDir (
	echo ERROR: Client%slot%ProfileDirectory is not set in Workbench\user.cfg.
	exit /b 1
)

REM Refuses to run if the configured profile dir is an unsafe delete target.
call "%~dp0ClearLogs.bat" "%brProfileDir%"
if errorlevel 1 exit /b 1

REM ConnectFromCLI (vanilla and MainMenu.c) reads -password. Left out entirely when unset:
REM an empty "" argument would end LaunchSteamClient.bat's argument loop early.
set "brPasswordArg="
if defined ServerPassword set brPasswordArg="-password=%ServerPassword%"

call "%~dp0LaunchSteamClient.bat" %brSteamID% "%gameDirectory%" %clientEXE% %clientLaunchParams% "-connect=127.0.0.1" -port=%port% "-profiles=%brProfileDir%" "-name=%brPlayerName%" "-mod=%modList%" %brPasswordArg%

exit /b 0
