@echo off
setlocal enabledelayedexpansion

set skipPboCleanup=0
set killGame=1
set killWorkbench=0

:OPTIONS
REM parse command line options
if "%~1"=="" goto MAIN
if /I "%~1"=="rebuildAll" (
	REM handled by CI1.bat
) else if /I "%~1"=="skipPboCleanup" (
	echo Command line option skipPboCleanup found - not removing orphaned PBOs
	set skipPboCleanup=1
) else if /I "%~1"=="noIgnore" (
	REM handled by CI1.bat
) else if /I "%~1"=="killGame" (
	echo Command line option killGame found - will close all instances of game ^(client/server^) if running
	set killGame=1
) else if /I "%~1"=="noKillGame" (
	REM killGame is the default because a running game holds the PBOs open and the build cannot replace them.
	echo Command line option noKillGame found - leaving any running game ^(client/server^) alone
	set killGame=0
) else if /I "%~1"=="killWorkbench" (
	echo Command line option killWorkbench found - will close all instances of Workbench if running
	set killWorkbench=1
)
shift /1
goto OPTIONS

:MAIN

cd /D "%~dp0"

if %killGame%==1 CALL "%~dp0KillGame.bat"
if %killWorkbench%==1 taskkill /F /IM workbenchApp.exe /T

set success=1

if not exist "P:\" (
	echo P: drive is not mounted, exiting.
	exit /b 1
)

call "%~dp0_Config.bat" CI
if errorlevel 1 exit /b 1

REM Logs\ is gitignored, so it does not exist in a fresh checkout - create it
REM before writing the marker files.
if not exist "%~dp0..\Logs" mkdir "%~dp0..\Logs"

if exist "%~dp0..\Logs\*.failure" del "%~dp0..\Logs\*.failure"
if exist "%~dp0..\Logs\*.tmp" del "%~dp0..\Logs\*.tmp"
if exist "%~dp0..\Logs\*.success" del "%~dp0..\Logs\*.success"

REM Build + pack mod
if not exist CI_Build.bat (
	echo ERROR: %cd%\CI_Build.bat does not exist.>>"%~dp0..\Logs\Build.log"
	goto end
)
echo !date! !time! CI_Build.bat is starting
echo %time% > "%~dp0..\Logs\Build.tmp"
start "CI_Build" CI_Build.bat exitCmd %*

:loop
echo Waiting 1 seconds
ping -n 2 127.0.0.1 >NUL
set running=0
if exist "%~dp0..\Logs\Build.tmp" (
	set /a running+=1
	echo CI_Build.bat is running
)
if %running%==0 goto end
goto loop

:end
if exist "%~dp0..\Logs\Build.failure" (
	set success=0
)
if exist "%~dp0..\Logs\Build.failure" (
	echo /////////////////////////////////////////////////////////////>>"%~dp0..\Logs\Build.log"
	echo %date% %time% Something went wrong, check the log above.>>"%~dp0..\Logs\Build.log"
	echo /////////////////////////////////////////////////////////////>>"%~dp0..\Logs\Build.log"
) else (
	if not exist "%~dp0..\Logs\Build.success" (
		set success=0
		echo %date% %time% ERROR: Building Mod did not finish successfully.>>"%~dp0..\Logs\Build.log"
	)
)
if "!success!"=="1" echo %date% %time% Successfully packaged all mods.>>"%~dp0..\Logs\Build.log"
type "%~dp0..\Logs\Build.log"
if "!success!" NEQ "1" goto abort

endlocal
exit /B 0

:abort
endlocal
REM Has to be the last line so it sets the exitcode
exit /B 1