Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
try {
    Add-Type -AssemblyName System.Windows.Forms
    $Picker = New-Object System.Windows.Forms.OpenFileDialog
    $Picker.Title = 'Select your old Ashen Spiral save or exported profile'
    $Picker.Filter = 'Ashen Spiral progress (*.sav;*.ashenprofile)|*.sav;*.ashenprofile'
    $Picker.CheckFileExists = $true
    if ($Picker.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) { exit 0 }
    $Selected = $Picker.FileName
    $TaskProject = Split-Path -Parent $PSScriptRoot
    $Destination = Join-Path $TaskProject 'Saved\Imports'
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    # Prepare both alternating slots when available. The game reads their revisions
    # and previews the newer compatible one; this never edits the old save folder.
    $Target = Join-Path $Destination 'SelectedProgress.sav'
    Copy-Item -LiteralPath $Selected -Destination $Target -Force
    $Companion = $null
    if ($Selected -match '_A\.sav$') { $Companion = $Selected -replace '_A\.sav$', '_B.sav' }
    if ($Selected -match '_B\.sav$') { $Companion = $Selected -replace '_B\.sav$', '_A.sav' }
    $OtherTarget = Join-Path $Destination 'SelectedProgress_Companion.sav'
    if (Test-Path -LiteralPath $OtherTarget) { Remove-Item -LiteralPath $OtherTarget }
    if ($Companion -and (Test-Path -LiteralPath $Companion)) { Copy-Item -LiteralPath $Companion -Destination $OtherTarget -Force }
    Write-Host 'Progress copied for review. In the offline hub: Overview > Progress > Review Import.'
    Write-Host 'Check the balances, then choose Restore. Your current profile is backed up first.'
    & (Join-Path $PSScriptRoot 'Setup.ps1') -Mode Game
} catch { Write-Host $_.Exception.Message -ForegroundColor Red; exit 1 }
