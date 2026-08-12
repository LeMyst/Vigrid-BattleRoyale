REM Shared preamble for every Launch*.bat: load config, validate it, build the mod
REM list and kill any running game.
REM
REM Usage: call "%~dp0SetupLaunch.bat" <MP|SP>
REM        if errorlevel 1 exit /b 1
REM
REM Deliberately has no setlocal - it exports the config, "mods" and "modList"
REM into the calling launcher.

if not exist "P:\" (
	echo P: drive is not mounted, exiting.
	exit /b 1
)

call "%~dp0_Config.bat" SetupLaunch
if errorlevel 1 exit /b 1

REM Only validate the keys this launch mode actually uses - requiring MP keys for
REM an SP launch (and vice versa) rejects perfectly valid configs.
REM Both mod-list keys hold the same value in every shipped config, but they stay
REM separate so SP and MP can diverge.
if /i "%~1"=="SP" (
	set "brRequired=ClientEXE GameDirectory SPMission ClientProfileDirectory"
	set "brModsKey=AdditionalSPMods"
	set "mods=%AdditionalSPMods%"
) else (
	set "brRequired=ClientEXE ServerEXE GameDirectory ServerDirectory ServerProfileDirectory MPMission"
	set "brModsKey=AdditionalMPMods"
	set "mods=%AdditionalMPMods%"
)

set /a failed=0

for %%k in (%brRequired%) do (
	call "%~dp0_Require.bat" %%k
	if errorlevel 1 set /a failed=1
)

echo ClientLaunchParams is: "%clientLaunchParams%"
echo ServerLaunchParams is: "%serverLaunchParams%"
echo PlayerName is: "%playerName%"

if %failed%==1 exit /b 1

if not defined mods echo %brModsKey% parameter was not set in the project.cfg, ignoring.

call "%~dp0SetupModList.bat"

call "%~dp0KillGame.bat"

exit /b 0
