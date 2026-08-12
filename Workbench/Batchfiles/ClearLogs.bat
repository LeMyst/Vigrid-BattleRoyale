@echo off
REM Clears game logs (.rpt / .log / .mdmp / .ADM / EXTrace_Profiling_*.csv).
REM
REM Usage: ClearLogs.bat "<profile directory>"   clears just that directory
REM        ClearLogs.bat                         clears every known profile directory
REM
REM Every directory is guarded and quoted - an unset variable used to degrade to
REM "del /s /q /f \*.log" at the root of the current drive.

setlocal

if not "%~1"=="" (
	call :clear "%~1"
	exit /b 0
)

if not defined ClientProfileDirectory (
	call "%~dp0_Config.bat" ClearLogs
	if errorlevel 1 exit /b 1
)

for %%d in (
	"%serverProfileDirectory%"
	"%ClientProfileDirectory%"
	"%ClientBProfileDirectory%"
	"%ClientCProfileDirectory%"
	"%localappdata%\DayZ"
) do call :clear %%d

exit /b 0

:clear
set "logDir=%~1"
if not defined logDir exit /b 0
if not exist "%logDir%\" (
	echo Skipping "%logDir%" - not a directory.
	exit /b 0
)
echo Clearing logs in "%logDir%"
for %%e in (*.rpt *.log *.mdmp *.ADM EXTrace_Profiling_*.csv) do del /s /q /f "%logDir%\%%e" >nul 2>&1
exit /b 0
