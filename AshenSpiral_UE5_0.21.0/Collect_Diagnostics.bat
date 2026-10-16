@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\CollectDiagnostics.ps1"
if errorlevel 1 echo Diagnostic collection failed. Read the message above.
pause
