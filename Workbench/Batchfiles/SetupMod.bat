@echo off
REM One-time setup: creates the P:\<PrefixLinkRoot>\ junction pointing at this
REM checkout. The checkout folder name must equal PrefixLinkRoot.

setlocal enableextensions enabledelayedexpansion

cd /D "%~dp0"

call "%~dp0_Config.bat" SetupMod
if errorlevel 1 goto FAILED

set /a failed=0

for %%k in (WorkDrive PrefixLinkRoot GameDirectory ServerDirectory ModName ModBuildDirectory) do (
	call "%~dp0_Require.bat" %%k
	if errorlevel 1 set /a failed=1
)

if %failed%==1 goto FAILED

set rootDirectory=%cd%\..\..\..\

IF exist "%workDrive%%prefixLinkRoot%\" (
	echo Removing existing link "%workDrive%%prefixLinkRoot%\"
	rmdir "%workDrive%%prefixLinkRoot%\"
)

echo Creating link "%workDrive%%prefixLinkRoot%\"
mklink /J "%workDrive%%prefixLinkRoot%\" "%rootDirectory%%prefixLinkRoot%\"

endlocal
exit /b 0

:FAILED
echo Failed to set up the mod.
endlocal
exit /b 1
