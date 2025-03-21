@echo off

call "..\project.cfg.bat"
call "..\user.cfg.bat"

REM Remove all folders starting with "storage_" in the mpmissons folder
for /d %%i in ("%MPMission%\storage_*") do (
	echo Removing folder "%%i"
    rd /s /q "%%i"
)
