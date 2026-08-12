@echo off

REM Usage: _EnumPaths <pattern> <outputFile>
REM Recursively lists <pattern> under the CURRENT directory into <outputFile>,
REM skipping any path containing a dot-prefixed folder (.claude, .git, .idea...).
REM Git worktrees under .claude\worktrees\ are complete copies of the mod, so
REM without this filter every worktree's config.cpp would be packed as its own PBO.

if "%~1"=="" (
	echo _EnumPaths - ERROR: pattern not given
	exit /B 1
)

if "%~2"=="" (
	echo _EnumPaths - ERROR: outputFile not given
	exit /B 1
)

dir /B /S "%~1" 2>NUL | findstr /V /R /C:"\\\." > "%~2"

REM findstr exits 1 when nothing matched; that is not an error for callers.
exit /B 0
