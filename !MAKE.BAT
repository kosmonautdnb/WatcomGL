@echo off
@echo Starting.. (needs RamDisk at R:)
rem @echo system dos4g > R:\LINK.LNK
@echo system pmodew > R:\LINK.LNK
@echo build errors > R:\LOGS.LOG
_BUILD\FLTMECHK.exe !MAK R:\gltest.exe
if not ERRORLEVEL 255 call _BUILD\MAKEMK.BAT
call wmake.exe /f R:\makefile /e /s /k
m R:\logs.log