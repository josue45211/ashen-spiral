param([ValidateSet('Editor','Game','ContentOnly')][string]$Mode='Editor', [switch]$ForceAssets)
try {
    . (Join-Path $PSScriptRoot 'Common.ps1')
    $Engine=Find-UnrealEngine
    Write-Host 'Close any running Ashen Spiral editor before rebuilding. Build/import logs are written under Saved\Logs.'
    Build-Editor $Engine
    Import-Content $Engine ([bool]$ForceAssets)
    if ($Mode -eq 'Editor') { Start-GameProcess $Engine @('/Game/Maps/AshenSpiral'); Write-Host 'Open the AshenSpiral map and press Play. The hub is generated when Play begins.' }
    if ($Mode -eq 'Game') { Start-GameProcess $Engine @('/Game/Maps/AshenSpiral','-game','-windowed','-ResX=1600','-ResY=900') }
    exit 0
} catch { Write-Host $_.Exception.Message -ForegroundColor Red; exit 1 }
