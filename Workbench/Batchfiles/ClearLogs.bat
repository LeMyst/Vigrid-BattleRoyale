@echo off

call "..\project.cfg.bat"
call "..\user.cfg.bat"

del /s /q /f %serverProfileDirectory%\*.rpt
del /s /q /f %serverProfileDirectory%\*.log
del /s /q /f %serverProfileDirectory%\*.mdmp
del /s /q /f %serverProfileDirectory%\*.ADM
del /s /q /f %serverProfileDirectory%\EXTrace_Profiling_*.csv

del /s /q /f %ClientProfileDirectory%\*.rpt
del /s /q /f %ClientProfileDirectory%\*.log
del /s /q /f %ClientProfileDirectory%\*.mdmp
del /s /q /f %ClientProfileDirectory%\*.ADM
del /s /q /f %ClientProfileDirectory%\EXTrace_Profiling_*.csv

del /s /q /f %ClientBProfileDirectory%\*.rpt
del /s /q /f %ClientBProfileDirectory%\*.log
del /s /q /f %ClientBProfileDirectory%\*.mdmp
del /s /q /f %ClientBProfileDirectory%\*.ADM
del /s /q /f %ClientBProfileDirectory%\EXTrace_Profiling_*.csv

del /s /q /f %ClientCProfileDirectory%\*.rpt
del /s /q /f %ClientCProfileDirectory%\*.log
del /s /q /f %ClientCProfileDirectory%\*.mdmp
del /s /q /f %ClientCProfileDirectory%\*.ADM
del /s /q /f %ClientCProfileDirectory%\EXTrace_Profiling_*.csv

del /s /q /f %localappdata%\DayZ\*.rpt
del /s /q /f %localappdata%\DayZ\*.log
del /s /q /f %localappdata%\DayZ\*.mdmp
del /s /q /f %localappdata%\DayZ\*.ADM
del /s /q /f %localappdata%\DayZ\EXTrace_Profiling_*.csv

del /s /q /f "%localappdata%\DayZ Exp\*.rpt"
del /s /q /f "%localappdata%\DayZ Exp\*.log"
del /s /q /f "%localappdata%\DayZ Exp\*.mdmp"
del /s /q /f "%localappdata%\DayZ Exp\*.ADM"
del /s /q /f "%localappdata%\DayZ Exp\EXTrace_Profiling_*.csv"
