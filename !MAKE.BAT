@echo off
@echo Starting..
@echo system dos4g > _BUILD\LINK.LNK
@echo build errors > LOGS.LOG
FLTMECHK.exe !MAK gltest.exe
if not ERRORLEVEL 255 call _BUILD\MAKEMK.BAT
call wmake.exe /f _build\makefile /e /s /k
m logs.log