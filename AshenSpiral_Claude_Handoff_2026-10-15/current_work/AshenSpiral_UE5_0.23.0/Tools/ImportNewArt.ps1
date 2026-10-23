# 0.26: imports named files from SourceArt into the project, without the whole-kit re-import Setup.ps1 does (Tools/import_new_art.py runs inside the editor).
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tools\ImportNewArt.ps1 -Files Meshes/SM_Weapon9.obj,Audio/S_ClawSwing.wav
# Close any running editor or game of this project first. The log is Saved\Logs\ImportNewArt.log.
param([Parameter(Mandatory=$true)][string[]]$Files,[string]$Engine='C:\Program Files\Epic Games\UE_5.8')
$Project=Split-Path -Parent $PSScriptRoot
$Cmd=Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Log=Join-Path $Project 'Saved\Logs\ImportNewArt.log'
$Files=@($Files|ForEach-Object{$_ -split ','}|Where-Object{$_.Trim()}) # (powershell -File passes -Files a,b as the one string "a,b")
$env:ASHEN_IMPORT=($Files -join ';')
$Script=Join-Path $PSScriptRoot 'import_new_art.py'
$P=Start-Process -FilePath $Cmd -ArgumentList @("`"$(Join-Path $Project 'AshenSpiral.uproject')`"",'/Engine/Maps/Entry','-run=pythonscript',"-script=`"$Script`"",'-unattended','-nosplash','-NullRHI','-NoSound','-UTF8Output',"-abslog=`"$Log`"") -PassThru -Wait -WindowStyle Hidden
$Done=Select-String -Path $Log -Pattern 'ASHEN IMPORT COMPLETE|ASHEN IMPORT FAILED' -ErrorAction SilentlyContinue | Select-Object -Last 1
Write-Output "ImportNewArt exit=$($P.ExitCode): $($Done.Line)"
if(-not $Done -or $Done.Line -match 'FAILED'){Select-String -Path $Log -Pattern 'Error|Traceback' -ErrorAction SilentlyContinue | Select-Object -Last 12 | ForEach-Object {$_.Line};exit 1}
