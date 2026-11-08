@echo off
REM One-time setup: creates the P:\<PrefixLinkRoot>\ junction pointing at the primary
REM checkout, whichever checkout runs it.

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

REM Link the PRIMARY checkout, also from a worktree. The target used to be derived from %cd%,
REM which from a worktree named .claude\worktrees\ - so the working link was replaced by a dangling one.
set "linkTarget=%~dp0..\.."
if defined brPrimaryWorkbench set "linkTarget=%brPrimaryWorkbench%\.."
for %%I in ("%linkTarget%") do set "linkTarget=%%~fI"

if not exist "%linkTarget%\Workbench\project.cfg" (
	echo ERROR: "%linkTarget%" is not a checkout of this mod - leaving "%workDrive%%prefixLinkRoot%\" untouched.
	goto FAILED
)

IF exist "%workDrive%%prefixLinkRoot%\" (
	echo Removing existing link "%workDrive%%prefixLinkRoot%\"
	rmdir "%workDrive%%prefixLinkRoot%\"
)

echo Creating link "%workDrive%%prefixLinkRoot%\" to "%linkTarget%\"
mklink /J "%workDrive%%prefixLinkRoot%\" "%linkTarget%\"
if errorlevel 1 goto FAILED

REM Deploy the offline test missions. A mission has to live under GameDirectory to be launchable -
REM SPMission is resolved against the game's cwd - and it cannot be a junction into the checkout,
REM because a DayZ update wipes GameDirectory\Missions\ wholesale. So the masters live in
REM Workbench\Missions\ and are copied out here; re-run this script after a game update.
REM Sourced from %~dp0 rather than the link target, so a worktree deploys its own copy.
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
