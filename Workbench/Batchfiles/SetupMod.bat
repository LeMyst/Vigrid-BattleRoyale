@echo off
REM One-time setup: creates the P:\<PrefixLinkRoot>\ junction pointing at this
REM checkout. The checkout folder name must equal PrefixLinkRoot.

setlocal enableextensions enabledelayedexpansion

cd /D "%~dp0"

call "%~dp0_Config.bat" SetupMod
if errorlevel 1 goto FAILED

set /a failed=0

for %%k in (WorkDrive PrefixLinkRoot GameDirectory ServerDirectory ModName ModBuildDirectory) do (
	call "%~dp0_Require.bat" %%k
	if errorlevel 1 set /a failed=1
)

if %failed%==1 goto FAILED

set rootDirectory=%cd%\..\..\..\

IF exist "%workDrive%%prefixLinkRoot%\" (
	echo Removing existing link "%workDrive%%prefixLinkRoot%\"
	rmdir "%workDrive%%prefixLinkRoot%\"
)

echo Creating link "%workDrive%%prefixLinkRoot%\"
mklink /J "%workDrive%%prefixLinkRoot%\" "%rootDirectory%%prefixLinkRoot%\"

REM Deploy the offline test missions. A mission has to live under GameDirectory to be launchable -
REM SPMission is resolved against the game's cwd - and it cannot be a junction into the checkout,
REM because a DayZ update wipes GameDirectory\Missions\ wholesale. So the masters live in
REM Workbench\Missions\ and are copied out here; re-run this script after a game update.
REM Sourced from %~dp0 rather than %rootDirectory% so it also works from a worktree.
REM Copies per folder and never deletes, so an unrelated mission already installed is left alone.
set "missionSource=%~dp0..\Missions"

if not exist "%missionSource%\" (
	echo No "Workbench\Missions\" folder found - no offline test mission to deploy.
	goto :DEPLOYED
)

for /D %%M in ("%missionSource%\*") do (
	echo Deploying mission "%%~nxM" to "%gameDirectory%Missions\%%~nxM\"
	xcopy /S /E /Y /I "%%M" "%gameDirectory%Missions\%%~nxM\" > NUL
	if errorlevel 1 echo WARNING: failed to deploy mission "%%~nxM" - offline launches will not work until it is in place.
)

:DEPLOYED

endlocal
exit /b 0

:FAILED
echo Failed to set up the mod.
endlocal
exit /b 1
