@echo off
setlocal enabledelayedexpansion

REM Usage: CI0_CopyExtraPBO <pboNamesFolder>
REM Drops the prebuilt PBOs from Workbench\ExtraPBOs into the packed mod folder.
REM
REM OPT-IN, and off by default. Workbench\ExtraPBOs is gitignored (`*`), so its contents
REM differ per machine - copying it unconditionally meant a clone silently built a
REM DIFFERENT mod than the maintainer's, and shipped third-party PBOs signed by other
REM authors, plus their bikeys, inside @Vigrid-BattleRoyale. Set CopyExtraPBOs=1 in
REM Workbench\user.cfg to restore the old behaviour; read that folder's README first.

set "pboNamesFoldername=%~1"
IF "%pboNamesFoldername%"=="" exit /B 1

REM Unconditional: this is not part of the copy, and the build depends on it.
rename "%pboNamesFoldername%\Addons" "addons"

if /i not "%CopyExtraPBOs%"=="1" goto :skip

REM ExtraPBOs\ is gitignored apart from its README, so a git worktree's copy holds no PBO at all.
REM Read the primary checkout's then (brPrimaryWorkbench, from _Config.bat) - otherwise a worktree
REM build silently ships a different mod from a primary build, the very thing CopyExtraPBOs=1 is
REM meant to rule out.
set "extraDir=%~dp0..\ExtraPBOs"
if exist "%extraDir%\*.pbo" goto EXTRADIR_FOUND
if not defined brPrimaryWorkbench goto EXTRADIR_FOUND
if not exist "%brPrimaryWorkbench%\ExtraPBOs\*.pbo" goto EXTRADIR_FOUND
set "extraDir=%brPrimaryWorkbench%\ExtraPBOs"
echo %date% %time% Using the primary checkout's "%extraDir%" - this checkout has no extra PBOs.
:EXTRADIR_FOUND

for %%F in ("%extraDir%\*.pbo" "%extraDir%\*.bisign") do (
	echo !date! !time! Copying extra "%%~nxF"
	copy "%%~F" "%pboNamesFoldername%\addons\"
)

for %%F in ("%extraDir%\*.bikey") do (
	echo !date! !time! Copying extra "%%~nxF"
	copy "%%~F" "%pboNamesFoldername%\keys\"
)

exit /B 0

:skip
echo %date% %time% CopyExtraPBOs is not 1 - skipping Workbench\ExtraPBOs, nothing extra was added to this build.
exit /B 0
