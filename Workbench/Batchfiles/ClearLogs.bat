@echo off
REM Clears game logs (.rpt / .log / .mdmp / .ADM / EXTrace_Profiling_*.csv).
REM
REM Usage: ClearLogs.bat "<profile directory>"   clears just that directory
REM        ClearLogs.bat                         clears every known profile directory
REM
REM Every directory goes through _SafeDir.bat before anything is deleted, because
REM these paths come from the gitignored user.cfg and a "del /s /q /f" aimed at a
REM drive root or at %USERPROFILE% would take the whole tree with it.
REM
REM Exits 1 if a directory was rejected as an unsafe delete target. A directory
REM that merely does not exist yet is skipped quietly - profile directories are
REM not created until the first launch.

setlocal

if "%~1"=="" goto :sweep

call :clear "%~1"
exit /b %errorlevel%

:sweep
if not defined ClientProfileDirectory (
	call "%~dp0_Config.bat" ClearLogs
	if errorlevel 1 exit /b 1
)

set "brFailed="

for %%d in (
	"%serverProfileDirectory%"
	"%ClientProfileDirectory%"
	"%ClientBProfileDirectory%"
	"%ClientCProfileDirectory%"
	"%localappdata%\DayZ"
) do (
	call :clear %%d
	if errorlevel 1 set "brFailed=1"
)

if defined brFailed exit /b 1
exit /b 0

:clear
REM An unset config key reaches this as an empty argument. That is "not configured",
REM not a misconfiguration, so skip it before _SafeDir would reject it loudly.
if "%~1"=="" exit /b 0

call "%~dp0_SafeDir.bat" %1
REM Not a ( ) block: profile paths can contain parentheses ("Program Files (x86)")
REM and substituting one into a block breaks cmd's parenthesis matching.
if errorlevel 2 goto :nodir
if errorlevel 1 exit /b 1

echo Clearing logs in "%brSafeDir%"

REM One call per pattern, and NOT "for %%e in (*.rpt *.log ...)": a plain for set is
REM glob-expanded against the *current* directory and tokens that match nothing are
REM dropped, so that form deleted nothing at all from cwd Workbench\Batchfiles.
call :delpat "*.rpt"
call :delpat "*.log"
call :delpat "*.mdmp"
call :delpat "*.ADM"
call :delpat "EXTrace_Profiling_*.csv"
exit /b 0

:nodir
echo Skipping %1 - not a directory.
exit /b 0

:delpat
del /s /q /f "%brSafeDir%\%~1" >nul 2>&1
exit /b 0
