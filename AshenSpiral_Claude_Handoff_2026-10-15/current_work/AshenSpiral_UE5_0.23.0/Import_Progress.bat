@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Tools\ImportProgress.ps1"
if errorlevel 1 (
  echo Progress import preparation stopped. Read the error above.
)
pause
