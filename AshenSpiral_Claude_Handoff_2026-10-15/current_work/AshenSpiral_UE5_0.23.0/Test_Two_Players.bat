@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\TestTwoPlayers.ps1" 
if errorlevel 1 (
  echo.
  echo Setup stopped. Read the error above and START_HERE.md.
  pause
  exit /b 1
)
pause
