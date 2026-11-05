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

REM brPrimaryWorkbench is the PRIMARY checkout's Workbench folder, whichever checkout asks: git's
REM common dir is the primary's .git from a worktree too. Undefined outside a git checkout.
REM Gitignored local state - user.cfg here, ExtraPBOs\ in CI0_CopyExtraPBO.bat - lives only in the
REM primary, so a fresh git worktree reads it from there instead of carrying copies, which would
REM then have to be deleted again before "git worktree remove".
REM The "=" in --path-format must be escaped: inside a for /f command cmd reads a bare "=" as an
REM argument separator, git receives a valueless --path-format, and the lookup silently fails.
set "brPrimaryWorkbench="
set "brGitCommon="
for /f "delims=" %%G in ('git -C "%cfgRoot%" rev-parse --path-format^=absolute --git-common-dir 2^>nul') do set "brGitCommon=%%G"
if not defined brGitCommon goto PRIMARY_DONE
set "brGitCommon=%brGitCommon:/=\%"
for %%I in ("%brGitCommon%\..\Workbench") do set "brPrimaryWorkbench=%%~fI"
:PRIMARY_DONE

REM project.cfg always comes from THIS checkout: it is tracked and may be what changed.
set "brUserCfg=%cfgRoot%\user.cfg"
if exist "%brUserCfg%" goto USERCFG_FOUND
if not defined brPrimaryWorkbench goto USERCFG_MISSING
set "brUserCfg=%brPrimaryWorkbench%\user.cfg"
if not exist "%brUserCfg%" goto USERCFG_MISSING
echo Using the primary checkout's "%brUserCfg%" - this checkout has no Workbench\user.cfg.
goto USERCFG_FOUND

:USERCFG_MISSING
echo ERROR: Workbench\user.cfg not found - copy Workbench\user_sample.cfg to it and fill it in.
exit /b 1

:USERCFG_FOUND
if exist "%cfgOut%" del "%cfgOut%"

for %%f in ("%cfgRoot%\project.cfg" "%brUserCfg%") do (
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

REM srcRoot is the checkout the build packs: the one these batch files live in, unless
REM BR_SOURCE_ROOT names another. The build used to read every source through the
REM P:\<PrefixLinkRoot> junction instead, so building a worktree meant re-pointing that one
REM global link - which Claude Code's sandbox refuses to remove, and which silently packed
REM the wrong checkout whenever it was left pointing elsewhere. The junction is still needed:
REM binarize resolves cross-PBO model and texture references through P:\.
if defined BR_SOURCE_ROOT (set "srcRoot=%BR_SOURCE_ROOT%") else (set "srcRoot=%cfgRoot%\..")
for %%I in ("%srcRoot%") do set "srcRoot=%%~fI"
if "%srcRoot:~-1%"=="\" set "srcRoot=%srcRoot:~0,-1%"
if not exist "%srcRoot%\Workbench\project.cfg" (
	echo ERROR: source root "%srcRoot%" is not a checkout of this mod - no Workbench\project.cfg in it.
	exit /b 1
)
exit /b 0
