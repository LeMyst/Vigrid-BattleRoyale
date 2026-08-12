@echo off
REM Terminates every DayZ process this project can start.

for %%p in (
	DayZ_x64.exe
	DayZServer_x64.exe
	DZSALModServer.exe
	DayZDiag_x64.exe
	CrashReporter.exe
) do taskkill /F /IM %%p >nul 2>&1

exit /b 0
