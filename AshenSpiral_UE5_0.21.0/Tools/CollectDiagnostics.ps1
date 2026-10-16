Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$OutputRoot = Join-Path $ProjectRoot 'Saved\Diagnostics'
$CaptureRoot = Join-Path $OutputRoot ('Capture-' + $Stamp)
$Archive = Join-Path $OutputRoot ('AshenSpiral-Diagnostics-' + $Stamp + '.zip')

try {
    New-Item -ItemType Directory -Path $CaptureRoot -Force | Out-Null
    $Copied = New-Object System.Collections.Generic.List[string]
    # Copy only this project's diagnostics. Do not collect save profiles or machine-wide files.
    foreach ($Relative in @('Saved\Logs\AshenSpiral.log', 'Saved\Logs\Build-Editor.log',
            'Saved\Logs\Bootstrap.log', 'Saved\AshenBootstrap.json',
            'AshenSpiral.uproject', 'Config\DefaultEngine.ini', 'Config\DefaultGame.ini')) {
        $SourceFile = Join-Path $ProjectRoot $Relative
        if (Test-Path -LiteralPath $SourceFile -PathType Leaf) {
            $TargetFile = Join-Path $CaptureRoot $Relative
            New-Item -ItemType Directory -Path (Split-Path -Parent $TargetFile) -Force | Out-Null
            Copy-Item -LiteralPath $SourceFile -Destination $TargetFile
            $Copied.Add($Relative)
        }
    }
    $Summary = @(
        'Ashen Spiral 0.21.0 diagnostic collection',
        ('Captured: ' + (Get-Date -Format o)),
        'Runtime startup entries in AshenSpiral.log begin with AshenVisual.',
        'Please include a screenshot of the game window with this ZIP.',
        'No Unreal build, rendering, or performance test was run by this collector.',
        '',
        'Included project files:'
    ) + $Copied.ToArray()
    $Summary | Set-Content -LiteralPath (Join-Path $CaptureRoot 'Summary.txt') -Encoding UTF8
    Compress-Archive -Path (Join-Path $CaptureRoot '*') -DestinationPath $Archive
    Write-Host ('Created: ' + $Archive) -ForegroundColor Green
    if (-not (Test-Path (Join-Path $CaptureRoot 'Saved\Logs\AshenSpiral.log'))) {
        Write-Warning 'No runtime log was found. Run the game from this folder, reproduce the issue, close it, and collect again.'
    }
    Remove-Item -LiteralPath $CaptureRoot -Recurse -Force
    exit 0
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host ('Any collected files remain at: ' + $CaptureRoot)
    exit 1
}
