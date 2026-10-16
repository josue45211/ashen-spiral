try {
    . (Join-Path $PSScriptRoot 'Common.ps1')
    $Engine=Find-UnrealEngine
    Build-Editor $Engine
    Import-Content $Engine $false
    Start-GameProcess $Engine @('/Game/Maps/AshenSpiral?listen?Session=LOCAL2','-game','-windowed','-ResX=1000','-ResY=700','-WinX=10','-WinY=30','-ProfileSlot=90','-log')
    Write-Host 'When the host window reaches the hub, press Enter here to launch the second player.'
    Read-Host | Out-Null
    Start-GameProcess $Engine @('127.0.0.1:24567?Code=LOCAL2?Protocol=2100','-game','-windowed','-ResX=1000','-ResY=700','-WinX=1020','-WinY=30','-ProfileSlot=91','-log')
    Write-Host 'Use the Expedition panel in each window to ready up. Test slots 90 and 91 do not replace your normal profile.'
    exit 0
} catch { Write-Host $_.Exception.Message -ForegroundColor Red; exit 1 }
