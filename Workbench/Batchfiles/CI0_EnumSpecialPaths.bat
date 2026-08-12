@echo off
setlocal enabledelayedexpansion

REM Usage: CI0_EnumSpecialPaths <tag>
REM Records the "special" (IGNORE?) file paths so the next build can diff them.
REM <tag> is the leading part of the list filename, not a mod name - each caller
REM picks the tag that matches the name it reads back later (CI.bat uses
REM "<PrefixLinkRoot>-ALL", CI1.bat uses "<modName>").

set /a failed=0

for %%k in (workDrive) do (
	call "%~dp0_Require.bat" %%k
	if errorlevel 1 set /a failed=1
)

if %failed%==1 exit /b 1

set "tag=%~1"

if not defined tag (
	echo ERROR: tag not defined
	exit /b 1
)

echo %date% %time% Getting "special" file paths...
call "%~dp0_EnumPaths.bat" "IGNORE?" "%workDrive%Temp\%tag%-specialpaths.list"
echo %date% %time% ...got "special" file paths
