@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\Setup.ps1" -Mode Game
if errorlevel 1 (
  echo.
  echo Setup stopped. Read the error above and START_HERE.md.
  pause
  exit /b 1
)
pause
