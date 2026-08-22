@echo off
REM Swap the diag executables for the retail ones for a single launch.
REM
REM Usage: call "%~dp0_ReleaseEXE.bat" <MP|SP>
REM        if errorlevel 1 exit /b 1
REM
REM Called by SetupLaunch.bat when ReleaseEXE=1 (config) or BR_RELEASE=1 (environment).
REM Not an entry point - use Release.bat, or set ReleaseEXE=1 in Workbench\user.cfg.
REM
REM WHY THIS EXISTS. project.cfg points both ClientEXE and ServerEXE at DayZDiag_x64.exe and
REM user_sample.cfg puts -newErrorsAreWarnings=1 on both parameter strings, so EVERY local launch
REM demotes errors a retail build treats as fatal - and a fatal CfgPatches / addon failure is not
REM merely demoted, it is not logged at all. That is how a bad requiredAddons entry killed a live
REM server on 2026-08-22 while every local run booted clean with a full RPT. Only Tools\check.py
REM and an actual retail boot can catch that class of defect; this is the second one.
REM
REM Exports clientEXE, serverEXE, clientLaunchParams, serverLaunchParams, gameDirectory and
REM serverDirectory back into the caller, which is why the swap is invisible to every Launch*.bat.

setlocal enabledelayedexpansion

set "brMode=%~1"
if not defined brMode set "brMode=MP"

REM Substrings that identify a diag-only launch parameter. Any token containing one is dropped.
REM They carry no "=" on purpose: the needle in a !var:needle=! replacement is terminated by the
REM first "=", so a needle containing one silently matches the wrong thing.
set "brDiagFlags=newErrorsAreWarnings filePatching"

if not defined ReleaseClientEXE set "ReleaseClientEXE=DayZ_x64.exe"
if not defined ReleaseServerEXE set "ReleaseServerEXE=DayZServer_x64.exe"

set "brClientEXE=%ReleaseClientEXE%"
set "brServerEXE=%ReleaseServerEXE%"

REM The retail server exe usually lives in a different folder from the diag one, which is shared
REM with the client install. These two override that per side and are empty in every shipped config.
set "brGameDir=%gameDirectory%"
set "brServerDir=%serverDirectory%"
if defined ReleaseClientDirectory set "brGameDir=%ReleaseClientDirectory%"
if defined ReleaseServerDirectory set "brServerDir=%ReleaseServerDirectory%"

echo.
echo === RELEASE EXE MODE - errors are FATAL, as on a real server ===

set "brClientParams=%clientLaunchParams%"
set "brServerParams=%serverLaunchParams%"
call :strip brClientParams
call :strip brServerParams

set /a brMissing=0

if not exist "%brGameDir%%brClientEXE%" (
	echo ERROR: retail client not found at "%brGameDir%%brClientEXE%".
	echo        Set ReleaseClientEXE or ReleaseClientDirectory in Workbench\user.cfg.
	set /a brMissing=1
)

if /i not "%brMode%"=="SP" (
	if not exist "%brServerDir%%brServerEXE%" (
		echo ERROR: retail server not found at "%brServerDir%%brServerEXE%".
		echo        Set ReleaseServerEXE or ReleaseServerDirectory in Workbench\user.cfg.
		set /a brMissing=1
	)
)

if !brMissing!==1 exit /b 1

echo === a config or addon failure now shows as a MODAL and stops the process ===
echo.

endlocal & set "clientEXE=%brClientEXE%" & set "serverEXE=%brServerEXE%" & set "clientLaunchParams=%brClientParams%" & set "serverLaunchParams=%brServerParams%" & set "gameDirectory=%brGameDir%" & set "serverDirectory=%brServerDir%"
exit /b 0

REM ---------------------------------------------------------------------------
REM Rebuild a parameter string without its diag-only tokens.
REM
REM Rebuilt rather than string-replaced: "%VAR:a=b%" splits search from replacement at the first
REM "=", so it cannot express removing a token that itself contains one - which -newErrorsAreWarnings=1
REM does. Tokenising sidesteps that, and lets each dropped flag be named on stdout.
:strip
set "brIn=!%~1!"
set "brOut="
for %%p in (!brIn!) do (
	set "brTok=%%~p"
	set "brKeep=1"
	for %%d in (%brDiagFlags%) do (
		set "brProbe=!brTok:%%d=!"
		if not "!brProbe!"=="!brTok!" set "brKeep=0"
	)
	if "!brKeep!"=="1" (
		set "brSpace=!brTok: =!"
		if "!brSpace!"=="!brTok!" (
			set "brOut=!brOut! !brTok!"
		) else (
			set "brOut=!brOut! "!brTok!""
		)
	) else (
		echo   dropping diag-only launch parameter: !brTok!
	)
)
if defined brOut set "brOut=!brOut:~1!"
set "%~1=!brOut!"
exit /b 0
