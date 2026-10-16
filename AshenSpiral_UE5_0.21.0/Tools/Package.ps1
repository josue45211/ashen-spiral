try {
    . (Join-Path $PSScriptRoot 'Common.ps1')
    $Engine=Find-UnrealEngine
    Build-Editor $Engine
    Import-Content $Engine $false
    $UAT=Join-Path $Engine 'Engine\Build\BatchFiles\RunUAT.bat'
    $Destination=Join-Path $ProjectRoot 'Builds\Windows'
    Invoke-LoggedNative $UAT @('BuildCookRun',('-project=' + $ProjectFile),'-noP4','-platform=Win64','-clientconfig=Development','-build','-cook','-map=/Game/Maps/AshenSpiral','-stage','-pak','-iostore','-archive',('-archivedirectory=' + $Destination),'-prereqs','-utf8output') 'Package-Windows.log'
    Write-Host ('Packaged build: ' + $Destination)
    Write-Host 'Share the WHOLE Windows build directory, not only its .exe. This command does not test gameplay.'
    exit 0
} catch { Write-Host $_.Exception.Message -ForegroundColor Red; exit 1 }
