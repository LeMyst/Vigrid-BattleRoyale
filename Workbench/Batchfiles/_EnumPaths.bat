@echo off
setlocal enabledelayedexpansion

REM Usage: _EnumPaths <pattern> <outputFile>
REM Recursively lists <pattern> under the CURRENT directory into <outputFile>,
REM skipping any path containing a dot-prefixed folder (.claude, .git, .idea...).
REM Git worktrees under .claude\worktrees\ are complete copies of the mod, so
REM without this filter every worktree's config.cpp would be packed as its own PBO.
REM
REM The test applies to the part of each path BELOW the current directory only.
REM The build now runs from the checkout itself rather than through the P:
REM junction, and a worktree's own root is under .claude\worktrees\ - testing the
REM whole path would filter out every file a worktree build is meant to find.
REM Paths are matched without findstr, so a "!" in a path would be mangled by
REM delayed expansion; no path in this repo has one.

if "%~1"=="" (
	echo _EnumPaths - ERROR: pattern not given
	exit /B 1
)

if "%~2"=="" (
	echo _EnumPaths - ERROR: outputFile not given
	exit /B 1
)

set "base=%cd%"
if not "%base:~-1%"=="\" set "base=%base%\"

type nul > "%~2"

for /F "delims=" %%P in ('dir /B /S "%~1" 2^>NUL') do (
	set "full=%%P"
	set "rel=!full:%base%=!"
	set "keep=1"
	if not "!rel!"=="!rel:\.=!" set "keep=0"
	if "!rel:~0,1!"=="." set "keep=0"
	if "!keep!"=="1" echo %%P>> "%~2"
)

REM An empty result is not an error for callers.
exit /B 0
