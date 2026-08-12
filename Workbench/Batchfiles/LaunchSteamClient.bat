@echo off
REM Starts a DayZ executable under the Goldberg Steam emulator so that several
REM local clients can run side by side with distinct Steam identities.
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
if not exist "%APPDATA%\Goldberg SteamEmu Saves\settings" mkdir "%APPDATA%\Goldberg SteamEmu Saves\settings"
echo | set /p ="%AccountName%">"%APPDATA%\Goldberg SteamEmu Saves\settings\account_name.txt"
echo | set /p ="%SteamID%">"%APPDATA%\Goldberg SteamEmu Saves\settings\user_steam_id.txt"

echo "%GameDirectory%%GameEXE%" %launchArgs%
start "" /D "%GameDirectory%" %GameEXE% %launchArgs%
exit /b 0

:missing
echo ERROR: No %~1 argument^!
echo Usage: %~nx0 ^<SteamID^> ^<GameDirectory^> ^<GameEXE^> ^<launch arguments^>
exit /b 0
