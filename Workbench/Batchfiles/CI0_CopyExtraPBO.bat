@echo off
setlocal enabledelayedexpansion


set "pboNamesFoldername=%~1"
IF "%pboNamesFoldername%"=="" exit /B 1

rename "%pboNamesFoldername%\Addons" "addons"

for %%F in ("%~dp0..\ExtraPBOs\*.pbo") do (
	echo !date! !time! Copying extra PBO "%%~F"
	copy "%%~F" "%pboNamesFoldername%\addons\"
)

for %%F in ("%~dp0..\ExtraPBOs\*.bisign") do (
	echo !date! !time! Copying extra bisign "%%~F"
	copy "%%~F" "%pboNamesFoldername%\addons\"
)
