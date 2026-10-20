# 0.24.2: two game instances on this PC - a listen host (test profile 90) and a guest (91) - each playing its half of the co-op check
# (Tools/DevScripts/verify_coop_host.txt and verify_coop_client.txt), then both quit. Logs: Saved\Logs\dev_coop_host.log / dev_coop_guest.log.
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tools\RunCoopCheck.ps1
param([int]$TimeoutSec=300,[int]$GuestDelaySec=30,[string]$Engine='C:\Program Files\Epic Games\UE_5.8')
$Project=Split-Path -Parent $PSScriptRoot
$Editor=Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor.exe'
$Uproject=Join-Path $Project 'AshenSpiral.uproject'
$HostLog=Join-Path $Project 'Saved\Logs\dev_coop_host.log';$GuestLog=Join-Path $Project 'Saved\Logs\dev_coop_guest.log'
foreach($L in @($HostLog,$GuestLog)){if(Test-Path $L){Remove-Item $L -Force}}
$HostArgs="`"$Uproject`" /Game/Maps/AshenSpiral?listen?Session=LOCAL2 -game -windowed -ResX=1280 -ResY=720 -WinX=10 -WinY=30 -ProfileSlot=90 -log -abslog=`"$HostLog`" -ExecCmds=`"as.dev.script Tools/DevScripts/verify_coop_host.txt`""
$H=Start-Process -FilePath $Editor -ArgumentList $HostArgs -PassThru
Start-Sleep -Seconds $GuestDelaySec # the host must be listening before the guest dials in
$GuestArgs="`"$Uproject`" 127.0.0.1:24567?Code=LOCAL2?Protocol=2402 -game -windowed -ResX=1280 -ResY=720 -WinX=640 -WinY=200 -ProfileSlot=91 -log -abslog=`"$GuestLog`" -ExecCmds=`"as.dev.script Tools/DevScripts/verify_coop_client.txt`""
$G=Start-Process -FilePath $Editor -ArgumentList $GuestArgs -PassThru
$Deadline=(Get-Date).AddSeconds($TimeoutSec)
while((Get-Date) -lt $Deadline -and (-not $H.HasExited -or -not $G.HasExited)){Start-Sleep -Seconds 2}
foreach($P in @($H,$G)){if(-not $P.HasExited){Stop-Process -Id $P.Id -Force;Write-Output "killed $($P.Id) at the timeout"}}
Write-Output "RunCoopCheck host exit=$($H.ExitCode) guest exit=$($G.ExitCode)"
