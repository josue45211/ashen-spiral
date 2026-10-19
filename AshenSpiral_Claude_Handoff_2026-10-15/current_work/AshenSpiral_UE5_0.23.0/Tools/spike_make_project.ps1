# 0.24 skeletal spike helper: creates a throwaway content-only project (Saved\Scratch\spike_project) holding a copy of the engine's OWN Mannequin template content, so the
# spike can load it in the editor commandlet without putting Epic's content into the real project. Idempotent; copies only what the spike reads (about 93 MB).
# Usage: powershell -File Tools\spike_make_project.ps1        (set UE_ROOT to override the engine folder)
$Project = Split-Path -Parent $PSScriptRoot
$Engine = if ($env:UE_ROOT) { $env:UE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }
$Root = Join-Path $Project 'Saved\Scratch\spike_project'
$Src = Join-Path $Engine 'Templates\TemplateResources\High\Characters\Content\Mannequins'
$Dst = Join-Path $Root 'Content\Characters\Mannequins'
if (-not (Test-Path $Src)) { throw "Mannequin template content not found at $Src (set UE_ROOT to the UE 5.8 folder)." }
New-Item -ItemType Directory -Force $Dst | Out-Null
@'
{
  "FileVersion": 3,
  "EngineAssociation": "5.8",
  "Category": "Games",
  "Description": "Throwaway project for the Ashen Spiral 0.24 skeletal spike (never shipped)",
  "Plugins": [
    {"Name": "PythonScriptPlugin", "Enabled": true},
    {"Name": "EditorScriptingUtilities", "Enabled": true}
  ]
}
'@ | Set-Content -Encoding UTF8 (Join-Path $Root 'SpikeManny.uproject')
foreach ($d in 'Meshes','Materials','Rigs','Anims\Unarmed','Anims\Death','Anims\Rifle','Anims\Pistol') {
    $from = Join-Path $Src $d
    if (Test-Path $from) { robocopy $from (Join-Path $Dst $d) /E /NFL /NDL /NJH /NJS /NP | Out-Null }
}
$count = (Get-ChildItem $Dst -Recurse -File | Measure-Object).Count
$mb = [math]::Round((Get-ChildItem $Dst -Recurse -File | Measure-Object Length -Sum).Sum / 1MB, 1)
"spike project ready: $count files, $mb MB at $Root"
$global:LASTEXITCODE = 0 # robocopy reports 1 for "files copied"
