@echo off
REM Validates that a path is a safe target for a recursive delete.
REM
REM Usage: call "%~dp0_SafeDir.bat" "<path>"
REM        if errorlevel 2 goto :skip   REM shape is fine, directory absent - skip quietly
REM        if errorlevel 1 goto :fail   REM unsafe - refuse
REM        REM %brSafeDir% now holds the normalised path
REM
REM Exit codes:
REM   0  safe. brSafeDir = the normalised absolute path, no trailing backslash.
REM   1  unsafe: empty, not drive-letter absolute, a drive root, or a system directory.
REM      An error is echoed - the caller should abort.
REM   2  safe in shape but the directory does not exist. Silent: profile directories
REM      legitimately do not exist before the first launch, so this must not fail one.
REM
REM Deliberately no setlocal - brSafeDir has to reach the caller. Temporaries are
REM "_sd"-prefixed and cleared before returning.

set "brSafeDir="
set "_sdDenied="
set "_sdIn=%~1"

if not defined _sdIn (
	echo ERROR: _SafeDir.bat requires a path argument.
	goto :unsafe
)

REM Reject the two shapes that resolve somewhere other than where they look like
REM they point. A leading backslash is either the root of the current drive
REM ("\", "\DayZ") or a UNC path ("\\srv\share"); a bare "C:" is the *current
REM directory* on that drive, not its root.
if "%_sdIn:~0,1%"=="\" goto :unsafe
if "%_sdIn:~1%"==":" goto :unsafe

REM The ~f modifier resolves a relative path against the cwd, but it does NOT drop
REM a trailing backslash - "C:\foo\" stays "C:\foo\" - so strip those ourselves.
REM Looping via goto re-parses the line each pass, which is what makes it work
REM without delayed expansion.
for %%p in ("%_sdIn%") do set "_sdPath=%%~fp"

:strip
if not defined _sdPath goto :unsafe
if "%_sdPath:~-1%"=="\" (
	set "_sdPath=%_sdPath:~0,-1%"
	goto :strip
)

REM Must be absolute on a drive letter. This also rejects UNC paths, whose second
REM character is "\" rather than ":".
if not "%_sdPath:~1,1%"==":" goto :unsafe

REM A drive root is now two characters ("C:\" stripped down to "C:"), so anything
REM from the fourth character on has to be non-empty.
if "%_sdPath:~3%"=="" goto :unsafe

REM Exact matches only - "%LOCALAPPDATA%\DayZ" is a legitimate target and stays
REM allowed. One call per entry, at top level: %ProgramFiles(x86)% contains
REM parentheses that would terminate an enclosing ( ) block early.
call :deny "%SystemDrive%\"
call :deny "%SystemRoot%"
call :deny "%windir%"
call :deny "%USERPROFILE%"
call :deny "%LOCALAPPDATA%"
call :deny "%APPDATA%"
call :deny "%ProgramData%"
call :deny "%ProgramFiles%"
call :deny "%ProgramFiles(x86)%"
call :deny "%PUBLIC%"
call :deny "%TEMP%"
if defined _sdDenied goto :unsafe

if not exist "%_sdPath%\" (
	set "_sdIn="
	set "_sdPath="
	exit /b 2
)

set "brSafeDir=%_sdPath%"
set "_sdIn="
set "_sdPath="
exit /b 0

:unsafe
echo ERROR: refusing to delete from "%_sdIn%" - not a safe target directory.
set "_sdIn="
set "_sdPath="
set "_sdDenied="
exit /b 1

:deny
set "_sdD=%~1"
if not defined _sdD exit /b 0
if "%_sdD:~-1%"=="\" set "_sdD=%_sdD:~0,-1%"
if /i "%_sdD%"=="%_sdPath%" set "_sdDenied=1"
set "_sdD="
exit /b 0
