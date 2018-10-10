REM NOTE: THIS  IS NOT THE OFFICIAL SERVER START SCRIPT
REM THIS IS PROVIDED FOR PRE-ALPHA TESTING

REM REMEMBER TO UPDATE THE SETTINGS BELOW

@echo off
title DayZ BR Development Script

set PORT=2302
set MOD=RPCFramework;dayzbr
set LOG=true
set CONFIG=serverDZ.cfg
set FREEZE=true
set PROFILE=D:\SteamLibrary\steamapps\common\DayZServer\profiles\Server


cls

:reboot
echo.      Killing DayZServer_x64.exe
echo.
taskkill /F /IM DayZServer_x64.exe /T
echo.
REM ask if the user would like to start the server up
echo.      Process killed. 
echo.
set /p startServer=Would you like to start the server[y/n]?:

IF NOT %startServer% == y (
	goto shutdown
)

REM Debug print of your config settings
echo.
echo.      Connection: %IP%:%PORT%
echo.      Config: %CONFIG%
echo.      Mods: %MOD%
IF %LOG% == true (
	echo.      Logs Enabled
)
IF %FREEZE% == true (
	echo.      Freeze Check Enabled
)


REM Handling input parameters (structuring the process arguments)
IF NOT "%MOD%" == "" (
	set MODLINE=-mod=%MOD%
) ELSE (
	set MODLINE=
)
IF %LOG% == true (
	set LOGLINE=-dologs -adminlog -scriptDebug=true
	set LOGTITLE=[Logs]
) ELSE (
	set LOGLINE=
	set LOGTITLE=[No Logs]
)
IF %FREEZE% == true (
	set FREEZELINE=-freezecheck
	set FREEZETITLE=[Freeze Check]
) ELSE (
	set FREEZELINE=
	set FREEZETITLE=[No Freeze Check]
)
IF NOT "%PROFILE%" == "" (
	set PROFILELINE=-profiles=%PROFILE%
) ELSE (
	set PROFILELINE=
)

set STARTLINE=DayZServer_x64.exe -config=%CONFIG% -port=%PORT% %PROFILELINE% %LOGLINE% %FREEZELINE% %MODLINE%

echo.      Starting DayZ Server... %LOGTITLE% %FREEZETITLE%
echo.      [DEBUG] %STARTLINE%
START %STARTLINE%

echo.
set /p restartServer=Would you like to kill the server[y/n]?:
REM if the user does not want to kill the server
IF NOT %restartServer% == y (
	goto shutdown
)
echo.
goto reboot


:shutdown
