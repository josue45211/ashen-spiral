Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot 'AshenSpiral.uproject'
$Logs = Join-Path $ProjectRoot 'Saved\Logs'
New-Item -ItemType Directory -Force -Path $Logs | Out-Null

function Find-UnrealEngine {
    $Candidates = New-Object System.Collections.Generic.List[string]
    if ($env:UE_ROOT) { $Candidates.Add($env:UE_ROOT.Trim('"')) }
    $RegistryPaths = @('HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.8',
        'HKLM:\SOFTWARE\EpicGames\UnrealEngine\5.8',
        'HKLM:\SOFTWARE\WOW6432Node\EpicGames\Unreal Engine\5.8')
    foreach ($Key in $RegistryPaths) {
        if (Test-Path $Key) {
            $Value = Get-ItemProperty -Path $Key -Name InstalledDirectory -ErrorAction SilentlyContinue
            if ($Value -and $Value.InstalledDirectory) { $Candidates.Add($Value.InstalledDirectory) }
        }
    }
    $Launcher = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path $Launcher) {
        try {
            $Info = Get-Content -LiteralPath $Launcher -Raw | ConvertFrom-Json
            foreach ($Install in $Info.InstallationList) {
                if ($Install.AppName -eq 'UE_5.8') { $Candidates.Add($Install.InstallLocation) }
            }
        } catch { Write-Warning 'Could not read launcher install metadata; trying normal install locations.' }
    }
    $Candidates.Add((Join-Path $env:ProgramFiles 'Epic Games\UE_5.8'))
    foreach ($Candidate in $Candidates) {
        $VersionFile = Join-Path $Candidate 'Engine\Build\Build.version'
        if (-not (Test-Path -LiteralPath $VersionFile)) { continue }
        $Version = Get-Content -LiteralPath $VersionFile -Raw | ConvertFrom-Json
        if ($Version.MajorVersion -eq 5 -and $Version.MinorVersion -eq 8 -and $Version.PatchVersion -ge 3) {
            if (Test-Path (Join-Path $Candidate 'Engine\Binaries\Win64\UnrealEditor.exe')) {
                Write-Host ('Using Unreal {0}.{1}.{2}: {3}' -f $Version.MajorVersion,$Version.MinorVersion,$Version.PatchVersion,$Candidate)
                return $Candidate
            }
        }
    }
    throw 'Unreal Engine 5.8.3 or newer 5.8 hotfix not found. Install/update UE 5.8 in Epic Games Launcher, or set UE_ROOT to that engine folder. See START_HERE.md.'
}
function Invoke-LoggedNative([string]$Executable, [string[]]$Arguments, [string]$LogName) {
    $LogFile = Join-Path $Logs $LogName
    Write-Host ('Running: ' + $Executable)
    $OldPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try { & $Executable @Arguments 2>&1 | Tee-Object -FilePath $LogFile; $Exit = $LASTEXITCODE }
    finally { $ErrorActionPreference = $OldPreference }
    if ($Exit -ne 0) { throw ('Command failed with exit code {0}. Open {1}. See Docs\TROUBLESHOOTING.md.' -f $Exit,$LogFile) }
}
function Build-Editor([string]$Engine) {
    $Build = Join-Path $Engine 'Engine\Build\BatchFiles\Build.bat'
    Invoke-LoggedNative $Build @('AshenSpiralEditor','Win64','Development',('-Project=' + $ProjectFile),'-WaitMutex','-NoHotReloadFromIDE') 'Build-Editor.log'
}
function Import-Content([string]$Engine, [bool]$Force) {
    $Marker = Join-Path $ProjectRoot 'Saved\AshenBootstrap.json'
    $Script = Join-Path $ProjectRoot 'Tools\bootstrap_unreal.py'
    if (-not $Force -and (Test-Path $Marker) -and (Test-Path (Join-Path $ProjectRoot 'Content\Maps\AshenSpiral.umap'))) {
        try {
            $Data = Get-Content $Marker -Raw | ConvertFrom-Json
            $Hash = (Get-FileHash $Script -Algorithm SHA256).Hash.ToLowerInvariant()
            if ($Data.success -and $Data.bootstrap_sha256 -eq $Hash) { Write-Host 'Generated content already exists.'; return }
        } catch { Write-Warning 'Content marker needs refreshing.' }
    }
    if (Test-Path $Marker) { Remove-Item -LiteralPath $Marker }
    $Cmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    if ($Force) { $env:ASHEN_FORCE_IMPORT='1' } else { Remove-Item Env:\ASHEN_FORCE_IMPORT -ErrorAction SilentlyContinue }
    try {
        Invoke-LoggedNative $Cmd @($ProjectFile,'/Engine/Maps/Entry','-run=pythonscript',('-script=' + $Script),'-unattended','-nosplash','-NullRHI','-NoSound','-UTF8Output') 'Bootstrap.log'
    } finally { Remove-Item Env:\ASHEN_FORCE_IMPORT -ErrorAction SilentlyContinue }
    if (-not (Test-Path $Marker)) { throw 'Content bootstrap did not report success. Open Saved\Logs\Bootstrap.log. The game has NOT been marked ready.' }
    $Data=Get-Content $Marker -Raw | ConvertFrom-Json
    if (-not $Data.success) { throw 'Content import was not successful.' }
}
function Start-GameProcess([string]$Engine, [string[]]$Extra) {
    $Editor=Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor.exe'
    # Start-Process joins its arguments; quote every path explicitly for Windows PowerShell 5.1.
    $Args=@(('"' + $ProjectFile + '"')) + $Extra
    Start-Process -FilePath $Editor -ArgumentList $Args -WorkingDirectory $ProjectRoot | Out-Null
}
