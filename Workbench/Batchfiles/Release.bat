@echo off
REM Run any Launch*.bat against the RETAIL executables instead of the diag ones.
REM
REM Usage: Release.bat <Launcher.bat> [arguments]
REM
REM   Release.bat LaunchServer.bat            a real DayZServer_x64.exe boot - the one thing that
REM                                           proves the mod actually loads on a production server
REM   Release.bat LaunchLocalMP.bat 2         server + 2 retail clients
REM   Release.bat LaunchOffline.bat           single player on the retail client
REM
REM The whole local rig is DayZDiag_x64.exe with -newErrorsAreWarnings=1 on both sides, so a fatal
REM CfgPatches / addon error is demoted AND NOT LOGGED - a local run boots clean, plays fine, and
REM says nothing, while the same build kills a real server at addon load. Reach for this before
REM shipping anything that touches config.cpp, requiredAddons, defines[] or a #ifdef guard.
REM
REM Two things behave differently under a retail client, both expected:
REM   - the diag-only -br-autoconnect is compiled out, so clients stop at the main menu and have to
REM     be connected by hand (Play -> Direct connect, 127.0.0.1)
REM   - DIAG_DEVELOPER is undefined, so the whole diag menu and its fixtures are gone
REM
REM Sets BR_RELEASE=1, which SetupLaunch.bat turns into ReleaseEXE=1. setlocal keeps it out of the
REM caller's shell, so the next launch is a diag one again. For a persistent switch set ReleaseEXE=1
REM in Workbench\user.cfg instead.

setlocal

if "%~1"=="" (
	echo ERROR: Release.bat requires a launcher to run, e.g. Release.bat LaunchServer.bat
	exit /b 1
)

if not exist "%~dp0%~1" (
	echo ERROR: "%~1" is not a batch file in Workbench\Batchfiles.
	exit /b 1
)

set "BR_RELEASE=1"

REM %* keeps the launcher name; shift does not update it, so the arguments are rebuilt by hand.
set "brArgs=%*"
call set "brArgs=%%brArgs:*%~1=%%"

call "%~dp0%~1" %brArgs%
exit /b %errorlevel%
