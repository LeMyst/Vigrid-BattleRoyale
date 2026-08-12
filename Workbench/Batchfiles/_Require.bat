@echo off
REM Echoes a config value and fails if it is empty.
REM
REM Usage: call "%~dp0_Require.bat" ClientEXE
REM        if errorlevel 1 set /a failed=1
REM
REM Batch variable names are case insensitive, so the key may be spelled the same
REM way it appears in project.cfg / user.cfg.

if "%~1"=="" (
	echo ERROR: _Require.bat requires a config key name.
	exit /b 1
)

call set "_cfgVal=%%%~1%%"

echo %~1 is: "%_cfgVal%"

if not defined _cfgVal (
	echo ERROR: %~1 is not set. Set it in Workbench\user.cfg or Workbench\project.cfg.
	exit /b 1
)

exit /b 0
