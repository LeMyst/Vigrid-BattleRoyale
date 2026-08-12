@echo off
setlocal enabledelayedexpansion

REM Usage: CI0_CopyExtraPBO <pboNamesFolder>
REM Drops the prebuilt PBOs from Workbench\ExtraPBOs into the packed mod folder.

set "pboNamesFoldername=%~1"
IF "%pboNamesFoldername%"=="" exit /B 1

rename "%pboNamesFoldername%\Addons" "addons"

for %%F in ("%~dp0..\ExtraPBOs\*.pbo" "%~dp0..\ExtraPBOs\*.bisign") do (
	echo !date! !time! Copying extra "%%~nxF"
	copy "%%~F" "%pboNamesFoldername%\addons\"
)

for %%F in ("%~dp0..\ExtraPBOs\*.bikey") do (
	echo !date! !time! Copying extra "%%~nxF"
	copy "%%~F" "%pboNamesFoldername%\keys\"
)
