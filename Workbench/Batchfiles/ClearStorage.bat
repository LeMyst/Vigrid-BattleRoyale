@echo off
REM Removes the persistence folders ("storage_*") under MPMission.

setlocal

if not defined MPMission (
	call "%~dp0_Config.bat" ClearStorage
	if errorlevel 1 exit /b 1
)

if not defined MPMission (
	echo ClearStorage: MPMission is not set, nothing to do.
	exit /b 0
)

for /d %%i in ("%MPMission%\storage_*") do (
	echo Removing folder "%%~i"
	rd /s /q "%%~i"
)

exit /b 0
