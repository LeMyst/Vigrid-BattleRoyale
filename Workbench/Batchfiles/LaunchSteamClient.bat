@echo off
REM Starts a DayZ executable, optionally under the Goldberg Steam emulator so that
REM several local clients can run side by side with distinct Steam identities.
REM
REM The emulator half is OPT-IN and off by default: it only runs when UseSteamEmu=1
REM is set in Workbench\user.cfg. With it off this script just starts the exe and
REM touches nothing outside the repo - nothing about building or launching the mod
REM depends on it. With it off you can still run one client at a time; what you lose
REM is several local clients holding distinct Steam identities, which is what stops
REM them kicking each other off a shared one.
REM
REM Usage: LaunchSteamClient.bat <SteamID> <GameDirectory> <GameEXE> <launch arguments...>

setlocal

REM Note: the argument checks below cannot be folded into a for loop - shift only
REM takes effect after a parenthesised block ends, so every iteration would read
REM the same %1.

if "%~1"=="" ( call :missing SteamID & exit /b 1 )
set "SteamID=%~1"
shift /1

if "%~1"=="" ( call :missing GameDirectory & exit /b 1 )
set "GameDirectory=%~1"
shift /1

if "%~1"=="" ( call :missing GameEXE & exit /b 1 )
set "GameEXE=%~1"
shift /1

if "%~1"=="" ( call :missing "launch arguments" & exit /b 1 )

:getargs
if "%~1"=="" goto main
if defined launchArgs (
	set launchArgs=%launchArgs% %1
) else (
	set launchArgs=%1
)
set arg=%~1
if "%arg:~0,5%"=="-name" set "AccountName=%arg:~6%"
shift /1
goto getargs

:main
if not defined AccountName set AccountName=Noob
echo AccountName: %AccountName%
echo SteamID: %SteamID%
REM Not a ( ) block - %APPDATA% and the account name can contain parentheses, and the
REM `echo | set /p` idiom below pipes, which is fragile inside one. Same reason
REM _LaunchServer.bat spells its KeepStorage branch out with labels.
if /i not "%UseSteamEmu%"=="1" goto :nosteamemu

echo UseSteamEmu=1 - writing the Goldberg identity for this slot.
if not exist "%APPDATA%\Goldberg SteamEmu Saves\settings" mkdir "%APPDATA%\Goldberg SteamEmu Saves\settings"
echo | set /p ="%AccountName%">"%APPDATA%\Goldberg SteamEmu Saves\settings\account_name.txt"
echo | set /p ="%SteamID%">"%APPDATA%\Goldberg SteamEmu Saves\settings\user_steam_id.txt"
goto :launch

:nosteamemu
echo UseSteamEmu is not 1 - skipping Steam emulation, launching directly.
echo Set UseSteamEmu=1 in Workbench\user.cfg to run several local clients under distinct Steam identities.

:launch
echo "%GameDirectory%%GameEXE%" %launchArgs%
start "" /D "%GameDirectory%" %GameEXE% %launchArgs%
exit /b 0

:missing
echo ERROR: No %~1 argument^!
echo Usage: %~nx0 ^<SteamID^> ^<GameDirectory^> ^<GameEXE^> ^<launch arguments^>
exit /b 0
