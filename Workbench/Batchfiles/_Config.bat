@echo off
REM Loads Workbench\project.cfg then Workbench\user.cfg into the CALLER's environment.
REM user.cfg wins because it is loaded second - later "set" overwrites earlier.
REM
REM Usage: call "%~dp0_Config.bat" <tag>
REM        if errorlevel 1 exit /b 1
REM
REM <tag> names the generated artifact Workbench\combined.cfg.<tag>.bat so that
REM concurrent callers (CI.bat spawns CI_Build.bat in its own window) never write
REM the same file. Artifacts are gitignored via /Workbench/*.cfg.*.bat.
REM
REM Lines starting with ";" are skipped by the for /f eol option; lines starting
REM with "#" are skipped by the findstr test. Values are emitted as set "K=V" so
REM that characters like ( ) & survive - several real config values contain them.

set "cfgRoot=%~dp0.."
set "cfgOut=%cfgRoot%\combined.cfg.%~1.bat"

if "%~1"=="" (
	echo ERROR: _Config.bat requires a tag argument.
	exit /b 1
)

if not exist "%cfgRoot%\project.cfg" (
	echo ERROR: Workbench\project.cfg not found.
	exit /b 1
)

if not exist "%cfgRoot%\user.cfg" (
	echo ERROR: Workbench\user.cfg not found - copy Workbench\user_sample.cfg to it and fill it in.
	exit /b 1
)

if exist "%cfgOut%" del "%cfgOut%"

for %%f in ("%cfgRoot%\project.cfg" "%cfgRoot%\user.cfg") do (
	for /f "usebackq eol=; delims=" %%a in ("%%~f") do (
		echo %%a| findstr /b /c:"#" >nul
		if errorlevel 1 echo set "%%a">>"%cfgOut%"
	)
)

if not exist "%cfgOut%" (
	echo ERROR: no settings were parsed out of project.cfg / user.cfg.
	exit /b 1
)

call "%cfgOut%"
exit /b 0
