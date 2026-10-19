# 0.24 skeletal spike helper. Usage: powershell -File Tools\spike_run.ps1 -Script Tools\spike_manny_export.py [-LogName spike_export]
# Runs a Python script inside the editor commandlet against the throwaway spike project (Saved\Scratch\spike_project, see spike_make_project.ps1): import and analysis only,
# no gameplay. Writes the engine log to Saved\Scratch\<LogName>.log and echoes the lines the script prefixed with SPIKE. Set UE_ROOT to override the engine folder.
param([Parameter(Mandatory=$true)][string]$Script, [string]$LogName = 'spike')
$Project = Split-Path -Parent $PSScriptRoot
$Engine = if ($env:UE_ROOT) { $env:UE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }
$Cmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Uproject = Join-Path $Project 'Saved\Scratch\spike_project\SpikeManny.uproject'
$ScriptPath = (Resolve-Path $Script).Path
$Log = Join-Path $Project ('Saved\Scratch\' + $LogName + '.log')
if (Test-Path $Log) { Remove-Item $Log -Force }
$sw = [Diagnostics.Stopwatch]::StartNew()
& $Cmd $Uproject '-run=pythonscript' ('-script=' + $ScriptPath) '-unattended' '-nosplash' '-NullRHI' '-NoSound' '-UTF8Output' ('-abslog=' + $Log) | Out-Null
$code = $LASTEXITCODE
"commandlet exit=$code elapsed=$([int]$sw.Elapsed.TotalSeconds)s log=$Log"
if (Test-Path $Log) {
    Select-String -Path $Log -Pattern 'SPIKE ' | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' } | Select-Object -First 80
    Select-String -Path $Log -Pattern 'Error:|Traceback' | Select-Object -First 15 | ForEach-Object { $_.Line }
}
exit $code
