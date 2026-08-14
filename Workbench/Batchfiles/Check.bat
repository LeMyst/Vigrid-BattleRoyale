@echo off
REM Runs the static source checks in Tools/check.py.
REM
REM Unlike every other batch file here this one loads no config and touches no
REM build output - it only reads tracked sources - so it needs neither user.cfg
REM nor a mounted P:. It is the same command CI runs; see .github/workflows/checks.yml.
REM
REM These do NOT compile anything. The real validation loop is still
REM Deploy.bat -> LaunchOffline.bat -> read the .rpt. What this catches is the
REM class of defect that fails silently at runtime with a clean .rpt.
REM
REM   Check.bat              run everything
REM   Check.bat --list       name every check
REM   Check.bat --only rpc   run one
REM   Check.bat -W           treat warnings as errors

setlocal

set "brRepoRoot=%~dp0..\.."

where python >nul 2>&1
if errorlevel 1 (
	echo Check: python was not found on PATH. Install Python 3 - stdlib only, no packages needed.
	exit /b 1
)

pushd "%brRepoRoot%"
python Tools\check.py %*
set "brResult=%errorlevel%"
popd

exit /b %brResult%
