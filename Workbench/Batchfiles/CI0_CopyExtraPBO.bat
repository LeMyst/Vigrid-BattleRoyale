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

for %%F in ("%~dp0..\ExtraPBOs\*.pbo" "%~dp0..\ExtraPBOs\*.bisign") do (
	echo !date! !time! Copying extra "%%~nxF"
	copy "%%~F" "%pboNamesFoldername%\addons\"
)

for %%F in ("%~dp0..\ExtraPBOs\*.bikey") do (
	echo !date! !time! Copying extra "%%~nxF"
	copy "%%~F" "%pboNamesFoldername%\keys\"
)

exit /B 0

:skip
echo %date% %time% CopyExtraPBOs is not 1 - skipping Workbench\ExtraPBOs, nothing extra was added to this build.
exit /B 0
