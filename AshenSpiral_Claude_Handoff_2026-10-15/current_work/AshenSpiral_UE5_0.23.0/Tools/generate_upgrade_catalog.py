"""Regenerate the human-readable catalog from the runtime's authoritative tables."""
from pathlib import Path
import re
import json

ROOT = Path(__file__).resolve().parents[1]

def generate():
    traits = re.findall(r'\{"([^"]+)","([^"]+)",(\d+),(\d+),([^,]+),(\d+),(\d+),(true|false)\}',
                        (ROOT/'Source/AshenSpiral/Public/ASBuildRules.h').read_text())
    assert len(traits) == 66, 'Catalog parser must cover all runtime traits'
    data = (ROOT/'Source/AshenSpiral/Private/ASData.cpp').read_text().split('const FASRunUpgrade RunUpgrades')[1].split('};')[0]
    stats = re.findall(r'\{TEXT\("([^"]+)"\),TEXT\("([^"]+)"\)\}', data)
    assert len(stats) == 6
    lines = ['# Upgrade catalog — 0.22.0', '',
             'Generated from `ASBuildRules.h` and `ASData.cpp`. All listed picks are run-only; normal rank caps are shown. Evolution can add one effective rank to eligible repeatable traits without increasing their normal pick cap.', '',
             '## Six core stat upgrades', '', '| Upgrade | Per-pick effect | Normal cap |', '|---|---|---|']
    for name, description in stats:
        lines.append(f'| {name} | {description} | 5 picks; effect cap may be reached earlier |')
    for category, title in enumerate(['Damage', 'Survival', 'Modifiers and economy']):
        lines += ['', '## '+title, '', '| Trait | Effect | Max picks | Rarity | Eligible weapons |', '|---|---|---|---|---|']
        for name, description, maximum, kind, tags, rarity, weapons, instant in traits:
            if int(kind) != category or name == "Vault Sense": continue
            weapon_label = {'511':'All nine weapons', '35':'Sword, hammer, scythe', '460':'Bow, glaive, firearms', '220':'Bow, glaive, staff, scattergun, rotary', '476':'All projectile weapons'}[weapons]
            lines.append(f'| {name} | {description} | {maximum} | '+['Common','Rare','Legendary'][int(rarity)]+f' | {weapon_label} |')
    lines += ['', 'The existing 24 relics and ten Spiral styles are separate from these 71 available upgrade definitions (65 traits + six core upgrades); retired Vault Sense retains its internal ID but cannot appear. Focus is the skill resource; Cinders are the shared run currency. See `BUILDCRAFT_0.14_NOTES.md` for proc restrictions, controls, new encounters and testing limits.', '']
    (ROOT/'Docs/UPGRADE_CATALOG.md').write_text('\n'.join(lines), encoding='utf-8')
    print('Generated catalog: 65 selectable traits + 6 core upgrades; one retired trait ID.')

def full_catalog():
    """Export every current content table, retaining stable retired IDs explicitly."""
    data = (ROOT/'Source/AshenSpiral/Private/ASData.cpp').read_text()
    arsenal = (ROOT/'Source/AshenSpiral/Public/ASArsenalRules.h').read_text()
    career = (ROOT/'Source/AshenSpiral/Public/ASCareerRules.h').read_text()
    def block(name):
        return re.search(r'const \w+ '+name+r'\[[^\]]+\]\s*=\s*\{(.*?)\n\};',data,re.S).group(1)
    def rows(name):
        return [re.findall(r'TEXT\("([^"]*)"\)',r) for r in re.findall(r'\{([^{}]+)\}',block(name))]
    def values(name,label='Values'):
        body=arsenal.split('inline '+('int' if name=='BaseCapacity' else 'float')+' '+name+'(',1)[1]
        return [float(v.strip().rstrip('f')) for v in re.search(label+r'\[\]=\{([^}]+)\}',body).group(1).split(',')]
    rates,capacities,reloads,damage=[values(n) for n in ['Rate','BaseCapacity','ReloadSeconds','Damage']]
    weapons=[]
    for i,r in enumerate(re.findall(r'\{([^{}]+)\}',block('Weapons'))):
        name,desc=re.findall(r'TEXT\("([^"]*)"\)',r)
        cost=int(re.search(r'\),\s*(\d+),',r).group(1))
        weapons.append(dict(id=i,name=name,description=desc,ash_cost=cost,base_damage_per_attack=damage[i],
                            attack_interval_seconds=rates[i],capacity=int(capacities[i]),reload_seconds=reloads[i]))
    for w in weapons:
        i=w['id'];w['uses_magazine']=i not in (0,1,5)
        if i in (0,1,5):
            w['combo_steps']=3 if i in (0,1) else 1
            w['third_hit_damage_multiplier']=2.6 if i==1 else (1.3 if i==0 else 1.0)
            w['third_hit_time_multiplier']=1.65 if i==1 else (1.25 if i==0 else 1.0)
    weapons[1]['hammer_022']=dict(combo_damage=[62,62,161.2],combo_seconds=[.72,.72,1.188],combo_window_seconds=1.2,
        finisher='planted frontal; committed',leap_cooldown_seconds=7.5,leap_base_damage=100,leap_radius_cm=370,
        leap_startup_seconds=.16,leap_flight_seconds=1.04,leap_recovery_seconds=.22,leap_height_cm=208.35,
        leap_dash_distance_multiplier=1.15,leap_ash_cost=0,hitstop_seconds=[.06,.06,.14])
    relics=[dict(id=i,key=r[0],name=r[1],description=r[2]) for i,r in enumerate(rows('Relics'))]
    abilities=[dict(id=i,name=r[0],description=r[1]) for i,r in enumerate(rows('Abilities'))]
    styles=[dict(id=i,name=r[0],description=r[1]) for i,r in enumerate(rows('Styles'))]
    stats=[dict(id=100+i,name=r[0],description=r[1],max_picks=5) for i,r in enumerate(rows('RunUpgrades'))]
    hearth=[]
    for i,r in enumerate(re.findall(r'\{([^{}]+)\}',block('Hearth'))):
        key,branch,name,desc=re.findall(r'TEXT\("([^"]*)"\)',r)
        cost,maximum=map(int,re.search(r'\),\s*(\d+),\s*(\d+)\s*$',r).groups())
        hearth.append(dict(id=i,key=key,branch=branch,name=name,description=desc,base_ash_cost=cost,max_rank=maximum))
    talents=[]
    for i,r in enumerate(re.findall(r'\{([^{}]+)\}',block('Talents'))):
        name,desc=re.findall(r'TEXT\("([^"]*)"\)',r)
        branch,cost,required=map(int,re.search(r'\),(\d+),(\d+),(\d+)\s*$',r).groups())
        talents.append(dict(id=i,name=name,description=desc,branch=branch,skill_point_cost=cost,branch_points_required=required,retired=i==9))
    traits=[]
    trait_rows=re.findall(r'\{"([^"]+)","([^"]+)",(\d+),(\d+),([^,]+),(\d+),(\d+),(true|false)\}',
                          (ROOT/'Source/AshenSpiral/Public/ASBuildRules.h').read_text())
    for i,(name,desc,maximum,kind,tags,rarity,mask,instant) in enumerate(trait_rows):
        traits.append(dict(id=i,offer_id=200+i,name=name,description=desc,max_picks=int(maximum),category=int(kind),
                           tags=tags,rarity=['Common','Rare','Legendary'][int(rarity)],weapon_mask=int(mask),instant=instant=='true',retired=i==55))
    achievements=[dict(id=i,name=n,description=d,metric=m,goals=list(map(int,(a,b,c))))
                  for i,(n,d,m,a,b,c) in enumerate(re.findall(r'\{"([^"]+)","([^"]+)",(\w+),\{(\d+),(\d+),(\d+)\}\}',career))]
    assert [len(x) for x in [weapons,relics,abilities,styles,stats,hearth,talents,traits,achievements]]==[9,24,6,10,6,20,12,66,12]
    tables=dict(version='0.22.0',protocol=2200,weapons=weapons,relics=relics,active_skills=abilities,spiral_styles=styles,
                core_upgrades=stats,hearth=hearth,talents=talents,traits=traits,achievements=achievements,
                materials=[dict(id=i,name=n,retired=i in (2,3)) for i,n in enumerate(['Scrap','Bone Dust','Retired Crystal Shard','Retired Vine Fiber','Spiral Core'])])
    (ROOT/'Docs/Source_Content_Tables.json').write_text(json.dumps(tables,indent=2)+'\n')
    lines=['ASHEN SPIRAL — EDITABLE GAME CATALOG', 'Version 0.22.0 / protocol 2200', '',
           'Generated from the runtime content tables. Edit this text to propose changes; editing this file does not change the game.',
           '0.22 follows the attached Grav Hammer rework prompt. Blade, ranged weapons, boss attacks, prices and Archives art retain 0.21 behavior.',
           'IDs stay stable. Retired entries are preserved for save compatibility and cannot be selected.', '']
    def heading(s): lines.extend(['',s.upper(),''])
    def entry(kind,r,extra=()):
        lines.extend([f'[{kind} {r["id"]:02d}] {r["name"]}',r['description'],*extra,'requested_changes =',''])
    heading('Weapons — three melee weapons without reloads; six ranged magazines')
    for r in weapons:
        entry('WEAPON',r,[f'ash_cost = {r["ash_cost"]}',f'base_damage_per_attack = {r["base_damage_per_attack"]:g}',
              f'attack_interval_s = {r["attack_interval_seconds"]:g}',f'capacity = {r["capacity"]}',f'reload_s = {r["reload_seconds"]:g}'])
    lines.extend(['Scattergun damage is 96 total at close range (6 pellets x 16), before modifiers and falloff.',
                  'Intervals are before attack-speed modifiers. Melee includes the full swing, impact and recovery; the third combo hit takes longer as listed below.',
                  'Ranged only: Deep Magazine adds ceil(20% x base capacity) per rank, max 5. Quick Hands: base reload / (1 + 0.15 x rank), max 5. Neither is offered to melee builds.',
                  'Tight Grouping: spread multiplier max(0.25, 1 - 0.15 x rank), max 5. Applies to primary and bonus/split shots.',
                  'Ranged only: the final round immediately starts reload, even when Attack is released. R / left-stick click reloads a partial magazine. Blade, Hammer and Scythe have no ammo, reload, or reload cues.'])
    heading('0.22 melee model and hammer tuning')
    lines.extend(['Hold Attack for automatic full swings. Releasing Attack lets the current swing finish. Hammer hits 1-2 can be canceled by dash/leap, resetting the combo. Hammer hit 3 commits through recovery: no dash, jump, leap, skill, Spiral or revive cancellation. Downing/room teardown/loadout changes can force cleanup.',
                  'Attack-speed multiplier = (1 + 0.08 x permanent rank + 0.12 x run rank) x Attuned haste (1.25 when active). Divide every swing phase by this multiplier.',
                  'Cinder Blade: 0.28 / 0.28 / 0.35 seconds; 40 / 40 / 52 base damage. Third hit lunges up to 160 cm along a clear grounded route. Full combo 0.91 seconds, 132 base damage (~145.05 DPS).',
                  'Grav Hammer: 0.72 / 0.72 / 1.188 seconds; 62 / 62 / 161.2 base damage. Third hit is a planted frontal smash with NO jump and NO radial primary hit. Both first swings also use the frontal hemisphere. Finisher collapses the nearest OWN well within 750 cm (well effects remain unchanged). Full combo 2.628 seconds, 285.2 base damage (~108.52 DPS).',
                  'Rift Scythe: 0.40-second full sweeping cycle, 41 base damage (~102.5 DPS); two-handed grip and no reload. No new three-hit finisher in this patch.',
                  'DPS figures are single-target primary damage before critical hits, traits, elements, well effects, missed hits and frame quantization; initial tuning for playtest.',
                  'Hammer combo grace is 1.2 seconds after recovery, divided by attack-speed multiplier. Whiffs advance the combo. Blade retains its 0.9-second grace and grounded lunge. Hammer hit 3 waits for ground contact when its preceding swings were airborne.',
                  'Air dash: available charges are spent normally. The airborne counter can reach total capacity (1-3), resets on landing, and survives saved-move replay and reconnect.',
                  'The brief mentions Bow third-shot double as design context, but it was absent from the supplied 0.20 source. Bow behavior is unchanged in this melee-only patch.'])
    heading('Hammer mobility and feel — 0.22')
    lines.extend(['RMB / controller X: hammer mobility leap. G / controller LT: former Spiral hold/release ability. Other weapon controls unchanged.',
                  'leap_cooldown_s = 7.5', 'leap_base_damage = 100', 'leap_radius_cm = 370', 'leap_ash_cost = 0',
                  'leap_startup_s = 0.16 (vulnerable)', 'leap_flight_s = 1.04 nominal; swept CharacterMovement collision and actual landing decide impact',
                  'leap_recovery_s = 0.22 (protected until back on feet)', 'leap_apex_cm = 208.35', 'leap_distance = 1.15 x normal dash distance (648.6 cm at base 0.24s dash)',
                  'Ground dash has no vertical apex; the 50% higher reference uses the normal 660 cm/s jump height at gravity 1568 cm/s^2.',
                  'Leap resets the combo; it does not collapse wells or spend ash/dash charges. Cooldown persists in the reconnect snapshot.',
                  'combo_hitstop_s = 0.06 / 0.06 / 0.14 at base speed; proportional visual contact holds inside each original cycle, not global time dilation',
                  'combo_knockback_cm_s = 230 / 230 / 950; leap = 700 (existing elite half-strength and boss/dummy immunity remain)',
                  'Settings > Motion sickness reduction disables every hammer camera shake. Preference is saved independently for each player.',
                  'Authoritative hammer defaults: Source/AshenSpiral/Public/ASHammerRules.h; combat/feedback: Private/ASMeleeCombat.cpp and ASHammerCombat.cpp.', 'requested_changes ='])
    for title,kind,items in [('Active skills — Focus / F','ACTIVE',abilities),('Spiral styles — right mouse','STYLE',styles),('Relics — unlockable reward pool','RELIC',relics)]:
        heading(title.replace('right mouse','G with hammer / right mouse otherwise'))
        for r in items:entry(kind,r)
    lines.extend(['Relics: first 8 initially available; spend 4 Relic Embers to add the next 4, up to 24.',
                  'Styles: first 6 initially available; spend 1 Tablet to discover the next, up to 10.',
                  'All six active skills are free to equip in the hub.',
                  '0.20 Rune Nova: Lance/Storm impacts and Winter Fan impact bloom use 300 cm base radius; War Cry 330 -> 630, Venom Bloom 320 -> 620, Cinder Mine 330 -> 630 cm. Area scaling is applied once; hit damage is unchanged. Storm does not damage the same enemy twice within one cast.',
                  'Ashen Kindling and Cinder Wake apply actual Burn status. Second Echo scales its damage area and ring once.',
                  'Ash Mantle / Linked Landing grant 10 / 12 shield toward 60 without lowering an existing higher shield. Guardian Link requires 720 cm proximity.'])
    heading('Core stat rewards — current run only')
    for r in stats:entry('STAT',r,['max_picks = 5; cooldown and critical effects may cap earlier'])
    heading('Run traits — 65 selectable, one retired ID')
    for r in traits:
        allowed=', '.join(w['name'] for w in weapons if r['weapon_mask']&(1<<w['id']))
        entry('TRAIT',r,[f'offer_id = {r["offer_id"]}',f'max_picks = {r["max_picks"]}',f'rarity = {r["rarity"]}',
                         f'category = {["Damage","Survival","Modifiers and economy"][r["category"]]}',
                         f'eligible_weapons = {"All nine" if r["weapon_mask"]==511 else allowed}',
                         f'retired = {str(r["retired"]).lower()}'])
    lines.extend(['Evolution can grant one effective rank above an eligible trait’s normal value without raising its selectable pick cap.',
                  'Deep Magazine, Quick Hands and Tight Grouping cannot evolve. Existing prerequisite and anti-recursion rules remain; see BUILDCRAFT_0.14_NOTES.md.'])
    heading('Ashen Hearth — permanent ranks')
    for r in hearth:entry('HEARTH',r,[f'branch = {r["branch"]}',f'max_rank = {r["max_rank"]}',f'next_rank_ash_cost = {r["base_ash_cost"]} x (current rank + 1)'])
    heading('Constellation talents — permanent skill points')
    for r in talents:entry('TALENT',r,[f'branch = {["Survivor","Tactician","Scavenger"][r["branch"]]}',
                                    f'skill_point_cost = {r["skill_point_cost"]}',f'branch_points_required = {r["branch_points_required"]}',f'retired = {str(r["retired"]).lower()}'])
    heading('Materials and workbench')
    lines.extend(['Active: Scrap (ID 0), Bone Dust (ID 1), Spiral Core (ID 4).',
                  'Crystal Shard (ID 2) and Vine Fiber (ID 3) are retired; legacy quantities convert 1:1 to Scrap.',
                  'Ward common drops: ward 2 gives Bone Dust; wards 1, 3 and 4 give Scrap.',
                  'Refinement: 15 Scrap + 8 Bone Dust -> 40 Ash, once per profile.',
                  'Memory: 3 Spiral Cores + 12 Bone Dust -> 1 Skill Point.',
                  'Treasure Sense (talent ID 9) is removed and refunds its 1 Skill Point once. Vault Sense (trait ID 55) remains unavailable.',
                  'Spiral Hoarder victory award: 30 Scrap + 10 Bone Dust.'])
    heading('Career — 12 achievements, 36 tiers, cosmetic titles')
    for r in achievements:entry('ACHIEVEMENT',r,[f'tier_goals = {", ".join(map(str,r["goals"]))}',
                                             'rewards = 1 Relic Ember per new tier; tier III also grants 1 Tablet; earned name/rank can be equipped as a title'])
    lines.extend(['Level milestones: 1 Relic Ember at every fifth level; 1 Tablet at every tenth level (both at multiples of ten).',
                  'Existing earned milestones receive a one-time catch-up. Paid cursors and balances save together; reopening a profile or retrying receipts does not pay again.',
                  'These milestones grant no direct damage, health, speed, skill points or XP.',
                  '0.21 retunes melee cadence and third-hit damage. Distant enemies gain at most 35% approach speed (bosses 15%), easing to their existing speed before attack range. Prices, ranged tuning and boss attacks are unchanged.',
                  '', 'Validation boundary: source/rule tests only. Unreal compilation, rendering, collision, audio, save I/O and co-op playtesting still require the installed editor.', ''])
    (ROOT/'Docs/ASHEN_SPIRAL_0.22_EDITABLE_CATALOG.txt').write_text('\n'.join(lines),encoding='utf-8')
    print('Generated full editable catalog and current JSON content tables.')

if __name__ == '__main__':
    generate()
    full_catalog()
