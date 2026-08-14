@echo off
REM Reports where the last server boot spent its time, and records it for comparison.
REM
REM Usage: BootTime.bat            reads the newest .RPT in the server profile directory
REM        BootTime.bat "<rpt>"    reads one specific .RPT
REM
REM Run it once the server has finished booting - NOT as part of the launch. LaunchSteamClient.bat
REM starts the server detached, so _LaunchServer.bat returns while the .RPT is still being written;
REM calling this from there would parse a half-finished boot, or the previous one.
REM
REM Every row is appended to Workbench\Logs\boottime.csv, which is what makes this worth running:
REM ClearLogs.bat deletes *.rpt and *.log from the profile directory at the start of every launch,
REM so without a copy outside that directory there is never anything to compare a slow boot against.
REM Boot time here has measured 1m48s to 3m36s on an identical build, so treat a single sample as
REM meaningless and collect several.

setlocal

if not "%~1"=="" (
	call :run --rpt %1
	exit /b %errorlevel%
)

call "%~dp0_Config.bat" BootTime
if errorlevel 1 exit /b 1

if not defined serverProfileDirectory (
	echo ERROR: ServerProfileDirectory is not set. Set it in Workbench\user.cfg.
	exit /b 1
)

REM ServerProfileDirectory conventionally ends in a backslash, and a trailing backslash inside a
REM quoted argument escapes the closing quote - the path reaches python as F:\ServerProfile" and the
REM directory is "not found". Same trim ClearStorage.bat does before joining a path.
set "brProfile=%serverProfileDirectory%"
if "%brProfile:~-1%"=="\" set "brProfile=%brProfile:~0,-1%"

call :run --profile "%brProfile%"
exit /b %errorlevel%

:run
python "%~dp0..\..\Tools\boottime.py" %*
if errorlevel 9009 (
	echo ERROR: python was not found on PATH. Tools\check.py needs it too - install Python 3.
	exit /b 1
)
exit /b %errorlevel%
