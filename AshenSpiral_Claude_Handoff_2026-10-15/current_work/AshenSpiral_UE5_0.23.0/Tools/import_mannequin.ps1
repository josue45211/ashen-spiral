# 0.24.2 Phase F: sets up the hero as the Unreal Mannequin on this machine (Josue after the 0.24.1 playtest: "use base UE base look for our character").
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tools\import_mannequin.ps1
# 1. Copies the engine's Third Person template Mannequins (Manny, Quinn, their skeleton, materials, the locomotion clips and ABP_Unarmed) into Content\Characters\Mannequins.
# 2. Imports the hero's move clips (attacks, dashes, flinches, the maul's leap, the weapon stances) from the Paragon pack (Tools\build_hero_animation.py, ASHEN_HERO_STEPS=clips).
# 3. Builds ABP_AshenHero from ABP_Unarmed with the upper-body layer (the AshenSpiralTools commandlet ASBuildHeroAnim; the editor target must be built first).
# Everything lands under Content\Characters, which git ignores: the template is Epic's sample content and the clips come from a Fab pack, so nothing here is committed. A machine
# without it draws the heroes with the 21-part armour rig, exactly as before (as.hero.skeletal 0 does the same on purpose).
param(
    [string]$Engine = 'C:\Program Files\Epic Games\UE_5.8',
    [switch]$SkipClips,
    [switch]$SkipBlueprint
)
$ErrorActionPreference = 'Stop'
$Project = Split-Path -Parent $PSScriptRoot
$Uproject = Join-Path $Project 'AshenSpiral.uproject'
$Template = Join-Path $Engine 'Templates\TemplateResources\High\Characters\Content\Mannequins'
$Target = Join-Path $Project 'Content\Characters\Mannequins'
$EditorCmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path $Template)) { throw "Missing the template Mannequins at $Template" }
New-Item -ItemType Directory -Force $Target | Out-Null
# the template's own folder layout is kept (its assets reference each other by path)
Copy-Item -Path (Join-Path $Template '*') -Destination $Target -Recurse -Force
Write-Output "Mannequins copied to $Target"
$Logs = Join-Path $Project 'Saved\Logs'
New-Item -ItemType Directory -Force $Logs | Out-Null
if (-not $SkipClips) {
    $env:ASHEN_HERO_STEPS = 'clips'
    & $EditorCmd $Uproject /Engine/Maps/Entry -run=pythonscript "-script=$(Join-Path $Project 'Tools\build_hero_animation.py')" -unattended -nosplash -NullRHI -NoSound "-abslog=$(Join-Path $Logs 'hero_clips.log')" | Out-Null
    Remove-Item Env:ASHEN_HERO_STEPS
    $Report = Get-Content (Join-Path $Project 'Saved\AshenHeroAnimation.json') -Raw | ConvertFrom-Json
    Write-Output ("Hero clips: {0} present, failed: {1}" -f $Report.clips.Count, ($Report.failed -join ', '))
}
if (-not $SkipBlueprint) {
    & $EditorCmd $Uproject -run=ASBuildHeroAnim -unattended -nosplash -NullRHI -NoSound "-abslog=$(Join-Path $Logs 'hero_anim_build.log')" | Out-Null
    $Built = Select-String -Path (Join-Path $Logs 'hero_anim_build.log') -Pattern 'AshenHeroAnim: .* built' | Select-Object -First 1
    if (-not $Built) { throw 'ABP_AshenHero was not built: see Saved\Logs\hero_anim_build.log' }
    Write-Output $Built.Line
}
