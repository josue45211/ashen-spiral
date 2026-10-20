# 0.24.2: plays one in-engine verification script (Tools/DevScripts/<name>.txt) in a standalone game window and waits for it to quit.
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tools\RunDevScript.ps1 -Script verify_elites
# Uses test profile 95 (your own profile is untouched). Screenshots land in Saved\Screenshots\Dev, the log in Saved\Logs\dev_<name>.log ("AshenDev:" lines are the script's own).
param([Parameter(Mandatory=$true)][string]$Script,[int]$TimeoutSec=300,[string]$Engine='C:\Program Files\Epic Games\UE_5.8')
$Project=Split-Path -Parent $PSScriptRoot
$Editor=Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor.exe'
$Log=Join-Path $Project "Saved\Logs\dev_$Script.log"
if(Test-Path $Log){Remove-Item $Log -Force}
$Args="`"$(Join-Path $Project 'AshenSpiral.uproject')`" /Game/Maps/AshenSpiral -game -windowed -ResX=1600 -ResY=900 -ProfileSlot=95 -log -abslog=`"$Log`" -ExecCmds=`"as.dev.script Tools/DevScripts/$Script.txt`""
$P=Start-Process -FilePath $Editor -ArgumentList $Args -PassThru
if(-not $P.WaitForExit($TimeoutSec*1000)){Stop-Process -Id $P.Id -Force;Write-Output "RunDevScript[$Script] TIMEOUT after $TimeoutSec s"} else {Write-Output "RunDevScript[$Script] exit=$($P.ExitCode)"}
Write-Output "log=$Log"
