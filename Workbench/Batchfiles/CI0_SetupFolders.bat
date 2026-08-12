@echo off
setlocal enabledelayedexpansion

REM Usage: CI0_SetupFolders <modName>
REM Creates the PboNames\<modName> marker folder, the Mod\<modName> and
REM Temp\<modName> output folders, and copies the signing key into place.

echo CI0_SetupFolders %* running...

set "modName=%~1"

IF "%modName%"=="" (
	echo CI0_SetupFolders - ERROR: modName not given
	exit /B 1
)

set /a failed=0

for %%k in (workDrive modBuildDirectory keyDirectory keyName) do (
	call "%~dp0_Require.bat" %%k
	if errorlevel 1 set /a failed=1
)

if %failed%==1 exit /B 1

for %%d in (
	"%workDrive%Temp\PboNames\%modName%"
	"%modBuildDirectory%%modName%\addons\"
	"%modBuildDirectory%%modName%\keys\"
	"%workDrive%Temp\%modName%\"
	"%workDrive%Temp\%modName%\addons\"
) do (
	IF NOT exist %%d (
		echo Creating folder %%d
		mkdir %%d
	)
)

IF NOT exist "%modBuildDirectory%%modName%\addons\" (
	echo CI0_SetupFolders - ERROR: %modBuildDirectory%%modName%\addons\ does not exist
	exit /B 1
)

echo Copying over "%keyDirectory%%keyName%.bikey" to "%modBuildDirectory%%modName%\keys\"
copy "%keyDirectory%%keyName%.bikey" "%modBuildDirectory%%modName%\keys\"

echo CI0_SetupFolders done

exit /B 0
