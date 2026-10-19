"""Repeatable source checks + the same ASRules.h used by the game.
Python 3.9+. No external packages. Needs g++, clang++, or cl on PATH for native tests.
These checks DO NOT compile Unreal modules or test gameplay in Unreal.
"""
from pathlib import Path
import ast
import hashlib
import json
import math
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import wave

ROOT = Path(__file__).resolve().parents[1]
checks = []

def check(condition, description):
    if not condition:
        raise AssertionError(description)
    checks.append(description)

def validate():
    project=json.loads((ROOT/'AshenSpiral.uproject').read_text())
    check(project['EngineAssociation']=='5.8', 'Engine association is UE 5.8 (minimum hotfix 5.8.3 in setup script)')
    generated_dirs = {'saved', 'intermediate', 'binaries', 'deriveddatacache', '.vs', 'build'}
    for p in ROOT.rglob('*.json'):
        if any(part.lower() in generated_dirs for part in p.relative_to(ROOT).parts[:-1]):
            continue
        json.loads(p.read_text())
    check(True, 'All JSON files parse')
    pyfiles=list((ROOT/'Tools').glob('*.py'))+[Path(__file__)]
    for p in pyfiles: ast.parse(p.read_text(), filename=str(p))
    check(True,'Python files pass Python AST parsing (not Unreal API execution)')
    source=list((ROOT/'Source').rglob('*.h'))+list((ROOT/'Source').rglob('*.cpp'))
    class_sources='\n'.join(p.read_text() for p in source if p.suffix=='.cpp')
    # UE replication macros expand against a variable literally named OutLifetimeProps.
    for p in source:
        s=p.read_text()
        if 'DOREPLIFETIME' in s:
            check('GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)' in s,
                  f'{p.name}: replication override uses OutLifetimeProps required by DOREPLIFETIME')
    check('Misc/LexFromString.h' not in class_sources, 'Removed UE 5.8-incompatible Misc/LexFromString.h include')
    check('Containers/UnrealString.h.inl' not in class_sources, 'Do not include UnrealString.h.inl directly')
    check('LexTryParseString' not in class_sources, 'Seed parsing no longer depends on LexTryParseString header internals')
    check('TryParseInt32Strict' in class_sources, 'Strict local int32 seed parser is present')
    check('Engine/SkyAtmosphere.h' not in class_sources, 'Removed UE 5.8-incompatible Engine/SkyAtmosphere.h include')
    check('#include \"Components/SkyAtmosphereComponent.h\"' in class_sources, 'UE 5.8 ASkyAtmosphere include is present')
    check('const int32 Owner=' not in class_sources, 'No local Owner variable shadows AActor::Owner')
    rpcs=0
    for p in source:
        s=p.read_text()
        # Deliberately a lexical structural check, not a C++ compiler substitute.
        stripped=re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', '', s, flags=re.S)
        stack=[]
        for c in stripped:
            if c in '({[': stack.append(c)
            elif c in ')}]':
                check(bool(stack) and stack.pop()==dict(zip(')}]','({['))[c], f'{p.name}: matching delimiter')
        check(not stack, f'{p.name}: all delimiters closed')
        if p.suffix=='.h' and '.generated.h"' in s:
            inc=re.findall(r'^#include\s+"([^"]+)"', s, re.M)
            check(inc[-1].endswith('.generated.h'),f'{p.name}: generated header is last include')
            for macro, name in re.findall(r'UFUNCTION\(([^)]*)\)\s+void\s+(\w+)\(',s):
                if any(flag in macro for flag in ('Server','Client','NetMulticast')):
                    check(name+'_Implementation(' in class_sources,f'{name}: RPC implementation exists')
                    rpcs+=1
    check(rpcs>10,'RPC presence checks cover more than 10 network functions')
    mappings=(ROOT/'Config/DefaultInput.ini').read_text()
    controller=(ROOT/'Source/AshenSpiral/Private/ASPlayerController.cpp').read_text()
    for kind,name in re.findall(r'Bind(Axis|Action)\("([^"]+)"', controller):
        check(f'{kind}Name="{name}"' in mappings, f'{name}: binding has an input mapping')
    actions={}
    for name,key in re.findall(r'ActionName="([^"]+)",Key=([^,)]+)',mappings):
        actions.setdefault(name,set()).add(key)
    check(actions.get('Jump')=={'SpaceBar','Gamepad_FaceButton_Bottom'}, 'Jump defaults are Space / controller A')
    check(actions.get('Dash')=={'LeftShift','RightShift','Gamepad_FaceButton_Right'}, 'Dash defaults are Shift / controller B without a jump conflict')
    protocol=re.search(r'constexpr int32 Protocol=(\d+)',(ROOT/'Source/AshenSpiral/Public/ASData.h').read_text())
    launcher_protocol=re.search(r'Protocol=(\d+)',(ROOT/'Tools/TestTwoPlayers.ps1').read_text())
    check(protocol is not None and launcher_protocol is not None and protocol[1]==launcher_protocol[1],
          'Local two-player launcher uses the runtime network protocol')
    version=re.search(r'constexpr const TCHAR\* Version=TEXT\("([^"]+)"\)',(ROOT/'Source/AshenSpiral/Public/ASData.h').read_text())
    project_version=re.search(r'^ProjectVersion=(\S+)',(ROOT/'Config/DefaultGame.ini').read_text(),re.M)
    check(version is not None and project_version is not None and version[1]==project_version[1],
          'Version constant (menu label, join error) matches ProjectVersion in DefaultGame.ini')
    check('ASHEN SPIRAL  0.' not in (ROOT/'Source/AshenSpiral/Private/SASMenu.cpp').read_text(),'Menu version label comes from ASData::Version, not a hard-coded string')
    cue_stems=re.findall(r'\{"(E_[A-Za-z0-9]+)"',(ROOT/'Source/AshenSpiral/Public/ASEnemyRules.h').read_text())
    audio_manifest=(ROOT/'Docs/AUDIO_MANIFEST_0.24.md').read_text()
    check(len(cue_stems)>=44 and all('`'+stem+'.wav`' in audio_manifest for stem in cue_stems),'Every enemy sound cue in ASEnemyRules.h is listed in Docs/AUDIO_MANIFEST_0.24.md (regenerate it with Tools/export_audio_manifest.cpp)')
    check(actions.get('HammerAlt')=={'G','Gamepad_LeftTrigger'}, 'Hammer former alt maps to G / controller LT')
    check(actions.get('Rune')=={'F','Gamepad_RightTrigger'}, 'Active ability defaults are F / controller RT')
    build_rules=(ROOT/'Source/AshenSpiral/Public/ASBuildRules.h').read_text()
    traits=build_rules.split('enum class Trait : int {')[1].split('};')[0].replace('\n','').split(',')
    runtime='\n'.join(p.read_text() for p in source if p.name!='ASBuildRules.h')
    for trait in traits:
        trait=trait.strip()
        if trait in ('Count','Sense') or '=' in trait: continue  # '=' marks a compatibility alias (FullHeal=Vengeance)
        check('Trait::'+trait in runtime, f'{trait}: trait referenced by runtime integration (not execution proof)')
    gm_src=(ROOT/'Source/AshenSpiral/Private/ASGameMode.cpp').read_text()
    gi_src=(ROOT/'Source/AshenSpiral/Private/ASGameInstance.cpp').read_text()
    check('R.ProfileId=PS->ProfileId' in gm_src,'SendReceipt stamps the receipt profile id (lexical check)')
    check('PC->ProfileRejected(PS->ProfileId)' in gm_src and 'ProfileRejected_Implementation' in controller,'Refused hub character switches revert the client slot (lexical check)')
    check('else if(!Commit(Profile))' in gi_src,'Fresh character slot creation failure blocks progress instead of being ignored (lexical check)')
    check('ReceiptQueue.RemoveAt(0)' in controller and 'ReceiveReceipt(PS->PendingReceipt)' in controller,
          'Acknowledgement drains queued receipts rather than dropping later rewards')
    check('GetWorld()->WorldType==EWorldType::PIE' in (ROOT/'Source/AshenSpiral/Private/ASGameInstance.cpp').read_text(),
          'Display changes include an explicit PIE guard')
    for p in source:
        for flags, field in re.findall(r'UPROPERTY\(([^)]*)\)\s+[^;=\n]+?\s+(\w+)\s*(?:=|;)',p.read_text()):
            if 'Replicated' in flags:
                check(re.search(r'DOREPLIFETIME(?:_CONDITION)?\(\w+,'+field+r'[,)]',class_sources) is not None,
                      f'{field}: declared replicated property has a registration macro')
    bounds=json.loads((ROOT/'SourceArt/mesh_bounds.json').read_text())
    meshes=sorted((ROOT/'SourceArt/Meshes').glob('*.obj'))
    check(len(meshes)==82,'82 meshes: the 78 originals (including the 7-mesh Archives kit) plus the 0.24 swing arc, strike sector and boss wing membrane + bones')
    vertices=triangles=degenerates=0
    for p in meshes:
        v=[];uv=[];norm=[];faces=[]
        for line in p.read_text().splitlines():
            a=line.split()
            if not a: continue
            if a[0] in ('v','vt','vn'):
                nums=[float(x) for x in a[1:]]
                check(all(math.isfinite(x) for x in nums),p.name+': finite geometry')
                {'v':v,'vt':uv,'vn':norm}[a[0]].append(nums)
            elif a[0]=='f': faces.append(a[1:])
        check(bool(v) and bool(faces),p.name+': nonempty geometry')
        volume=0.
        for face in faces:
            check(len(face)==3,p.name+': triangulated')
            ids=[]
            for item in face:
                parts=item.split('/')
                check(len(parts)==3 and all(parts),p.name+': UV and normal indices provided')
                vi,ti,ni=map(int,parts)
                check(1<=vi<=len(v) and 1<=ti<=len(uv) and 1<=ni<=len(norm),p.name+': indices in range')
                ids.append(vi-1)
            a,b,c=[v[i] for i in ids]
            u=[b[i]-a[i] for i in range(3)];w=[c[i]-a[i] for i in range(3)]
            cross=[u[1]*w[2]-u[2]*w[1],u[2]*w[0]-u[0]*w[2],u[0]*w[1]-u[1]*w[0]]
            degenerates+=sum(x*x for x in cross)<1e-12
            volume+=sum(a[i]*cross[i] for i in range(3))/6
        if p.stem in {'SM_Stalagmite','SM_Stalactite'}:
            check(volume>0,p.name+': reflected formations retain outward face winding')
        ext=[max(x[i] for x in v)-min(x[i] for x in v) for i in range(3)]
        check(all(abs(a-b)<.002 for a,b in zip(ext,bounds[p.stem]['extent'])),p.name+': expected centimeter extents')
        vertices+=len(v);triangles+=len(faces)
    check(degenerates==0,'All source mesh triangles have nonzero area')
    names={p.stem for p in meshes}
    used=set(re.findall(r'"(SM_[A-Za-z0-9_]+)"', class_sources))
    check(used.issubset(names), 'Every literal runtime custom-mesh reference has source geometry')
    textures=list((ROOT/'SourceArt/Textures').glob('*.png'))
    check(len(textures)==12,'12 source material maps including three Archives PBR sets')
    for p in textures:
        raw=p.read_bytes();check(raw[:8]==b'\x89PNG\r\n\x1a\n',p.name+': PNG signature')
        check(struct.unpack('>II',raw[16:24])==(512,512),p.name+': 512x512 resolution')
    sounds=list((ROOT/'SourceArt/Audio').glob('*.wav'))
    check(len(sounds)>=12,'At least the twelve original source audio effects, including four hammer cues (delivered enemy cue files add more)')
    for p in sounds:
        with wave.open(str(p)) as w:
            check(w.getnchannels()==1 and w.getsampwidth()==2 and w.getframerate()==44100 and w.getnframes()>0,p.name+': valid PCM audio')
    compiler=next((shutil.which(x) for x in ('g++','clang++','cl') if shutil.which(x)),None)
    native='NOT RUN - install a C++ compiler and run this script again.'
    gameplay='NOT RUN - install a C++ compiler and run this script again.'
    buildcraft='NOT RUN - install a C++ compiler and run this script again.'
    adventure='NOT RUN - install a C++ compiler and run this script again.'
    career='NOT RUN - install a C++ compiler and run this script again.'
    trig='NOT RUN - install a C++ compiler and run this script again.'
    interface_arsenal='NOT RUN - install a C++ compiler and run this script again.'
    reload_navigation='NOT RUN - install a C++ compiler and run this script again.'
    bugfix_020=reload_navigation
    melee_021=reload_navigation
    hammer_022=reload_navigation
    enemy_024=reload_navigation
    arena_024=reload_navigation
    arsenal_024=reload_navigation
    ai_024=reload_navigation
    pose_024=reload_navigation
    ability_024=reload_navigation
    if compiler:
        with tempfile.TemporaryDirectory(prefix='ashen_native_') as d:
            exe=Path(d)/('rules_tests.exe' if sys.platform=='win32' else 'rules_tests')
            src=ROOT/'Tests/rules_tests.cpp'
            if Path(compiler).stem.lower()=='cl':
                cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(src),'/Fe:'+str(exe)]
            else: cmd=[compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(src),'-o',str(exe)]
            subprocess.run(cmd,cwd=d,check=True)
            native=subprocess.run([str(exe)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/native_results.txt').write_text(native+'\n')
            check(native.startswith('PASS:'),'Standalone shared-rule native test executable passed')
            gameplay_exe=Path(d)/('gameplay_tests.exe' if sys.platform=='win32' else 'gameplay_tests')
            gameplay_src=ROOT/'Tests/gameplay_tests.cpp'
            if Path(compiler).stem.lower()=='cl':
                gameplay_cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(gameplay_src),'/Fe:'+str(gameplay_exe)]
            else: gameplay_cmd=[compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(gameplay_src),'-o',str(gameplay_exe)]
            subprocess.run(gameplay_cmd,cwd=d,check=True)
            gameplay=subprocess.run([str(gameplay_exe),str(ROOT)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/gameplay_results.txt').write_text(gameplay+'\n')
            check(gameplay.startswith('PASS:'),'Gameplay upgrades, revival, navigation, phase gates, elements, fade and weapon geometry tests passed')
            buildcraft_exe=Path(d)/('buildcraft_tests.exe' if sys.platform=='win32' else 'buildcraft_tests')
            buildcraft_src=ROOT/'Tests/buildcraft_tests.cpp'
            if Path(compiler).stem.lower()=='cl':
                buildcraft_cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(buildcraft_src),'/Fe:'+str(buildcraft_exe)]
            else: buildcraft_cmd=[compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(buildcraft_src),'-o',str(buildcraft_exe)]
            subprocess.run(buildcraft_cmd,cwd=d,check=True)
            buildcraft=subprocess.run([str(buildcraft_exe)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/buildcraft_results.txt').write_text(buildcraft+'\n')
            check(buildcraft.startswith('PASS:'),'Weighted choices, trait caps, resources, bleed, wagers and trap geometry tests passed')
            adventure_exe=Path(d)/('adventure_tests.exe' if sys.platform=='win32' else 'adventure_tests')
            adventure_src=ROOT/'Tests/adventure_tests.cpp'
            if Path(compiler).stem.lower()=='cl':
                adventure_cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(adventure_src),'/Fe:'+str(adventure_exe)]
            else: adventure_cmd=[compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(adventure_src),'-o',str(adventure_exe)]
            subprocess.run(adventure_cmd,cwd=d,check=True)
            adventure=subprocess.run([str(adventure_exe)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/adventure_results.txt').write_text(adventure+'\n')
            check(adventure.startswith('PASS:'),'Adventure camera, reach, ability, offering and biome/trap shared rules passed')
            expedition_exe=Path(d)/('expedition_023_tests.exe' if sys.platform=='win32' else 'expedition_023_tests')
            expedition_src=ROOT/'Tests/expedition_023_tests.cpp'
            if Path(compiler).stem.lower()=='cl':
                expedition_cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(expedition_src),'/Fe:'+str(expedition_exe)]
            else: expedition_cmd=[compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(expedition_src),'-o',str(expedition_exe)]
            subprocess.run(expedition_cmd,cwd=d,check=True)
            expedition=subprocess.run([str(expedition_exe)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/expedition_023_results.txt').write_text(expedition+'\n')
            check(expedition.startswith('PASS:'),'0.23 expedition rules (difficulty, Endless cadence, gambling odds, synergies) passed')
            arena_exe=Path(d)/('boss_arena_023_tests.exe' if sys.platform=='win32' else 'boss_arena_023_tests')
            arena_src=ROOT/'Tests/boss_arena_023_tests.cpp'
            if Path(compiler).stem.lower()=='cl':
                arena_cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(arena_src),'/Fe:'+str(arena_exe)]
            else: arena_cmd=[compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(arena_src),'-o',str(arena_exe)]
            subprocess.run(arena_cmd,cwd=d,check=True)
            arena=subprocess.run([str(arena_exe)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/boss_arena_023_results.txt').write_text(arena+'\n')
            check(arena.startswith('PASS:'),'0.23 boss arena layouts: structure, clearances and attack-aligned cover passed')
            capechain_exe=Path(d)/('cape_0231_tests.exe' if sys.platform=='win32' else 'cape_0231_tests')
            capechain_src=ROOT/'Tests/cape_0231_tests.cpp'
            if Path(compiler).stem.lower()=='cl':
                capechain_cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(capechain_src),'/Fe:'+str(capechain_exe)]
            else: capechain_cmd=[compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(capechain_src),'-o',str(capechain_exe)]
            subprocess.run(capechain_cmd,cwd=d,check=True)
            capechain=subprocess.run([str(capechain_exe)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/cape_0231_results.txt').write_text(capechain+chr(10))
            check(capechain.startswith('PASS:'),'0.23.1 heavy cape chain: stability, clearance, length, lag and response passed')
            career_exe=Path(d)/('career_tests.exe' if sys.platform=='win32' else 'career_tests')
            career_src=ROOT/'Tests/career_tests.cpp'
            if Path(compiler).stem.lower()=='cl':
                career_cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(career_src),'/Fe:'+str(career_exe)]
            else: career_cmd=[compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(career_src),'-o',str(career_exe)]
            subprocess.run(career_cmd,cwd=d,check=True)
            career=subprocess.run([str(career_exe)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/career_results.txt').write_text(career+'\n')
            check(career.startswith('PASS:'),'Career level, achievement, cashout and combined-status shared rules passed')
            feature_exe=Path(d)/('interface_arsenal_tests.exe' if sys.platform=='win32' else 'interface_arsenal_tests')
            feature_src=ROOT/'Tests/interface_arsenal_tests.cpp'
            if Path(compiler).stem.lower()=='cl':
                feature_cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(feature_src),'/Fe:'+str(feature_exe)]
            else: feature_cmd=[compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(feature_src),'-o',str(feature_exe)]
            subprocess.run(feature_cmd,cwd=d,check=True)
            interface_arsenal=subprocess.run([str(feature_exe)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/interface_arsenal_results.txt').write_text(interface_arsenal+'\n')
            check(interface_arsenal.startswith('PASS:'),'HUD scale, slower notification fade, all nine unlock bits and new weapon tuning passed')
            reload_exe=Path(d)/('reload_navigation_tests.exe' if sys.platform=='win32' else 'reload_navigation_tests')
            reload_src=ROOT/'Tests/reload_navigation_tests.cpp'
            if Path(compiler).stem.lower()=='cl':
                reload_cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(reload_src),'/Fe:'+str(reload_exe)]
            else: reload_cmd=[compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(reload_src),'-o',str(reload_exe)]
            subprocess.run(reload_cmd,cwd=d,check=True)
            reload_navigation=subprocess.run([str(reload_exe)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/reload_navigation_results.txt').write_text(reload_navigation+'\n')
            check(reload_navigation.startswith('PASS:'),'Reload gating, upgrade limits, rarity distribution, and weighted routes match independent shortest-distance oracle')
            fix_exe=Path(d)/('bugfix_020_tests.exe' if sys.platform=='win32' else 'bugfix_020_tests')
            fix_src=ROOT/'Tests/bugfix_020_tests.cpp'
            fix_cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(fix_src),'/Fe:'+str(fix_exe)] if Path(compiler).stem.lower()=='cl' else [compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(fix_src),'-o',str(fix_exe)]
            subprocess.run(fix_cmd,cwd=d,check=True)
            bugfix_020=subprocess.run([str(fix_exe)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/bugfix_020_results.txt').write_text(bugfix_020+'\n')
            check(bugfix_020.startswith('PASS:'),'0.20 migration, reward, routing, seed and animation geometry regressions passed')
            melee_exe=Path(d)/('melee_021_tests.exe' if sys.platform=='win32' else 'melee_021_tests')
            melee_src=ROOT/'Tests/melee_021_tests.cpp'
            melee_cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(melee_src),'/Fe:'+str(melee_exe)] if Path(compiler).stem.lower()=='cl' else [compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(melee_src),'-o',str(melee_exe)]
            subprocess.run(melee_cmd,cwd=d,check=True)
            melee_021=subprocess.run([str(melee_exe),str(ROOT)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/melee_021_results.txt').write_text(melee_021+'\n')
            check(melee_021.startswith('PASS:'),'0.21 multi-air-dash, melee cadence/grips, bounded approach and Archives clearance regressions passed')
            hammer_exe=Path(d)/('hammer_022_tests.exe' if sys.platform=='win32' else 'hammer_022_tests')
            hammer_src=ROOT/'Tests/hammer_022_tests.cpp'
            hammer_cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(hammer_src),'/Fe:'+str(hammer_exe)] if Path(compiler).stem.lower()=='cl' else [compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(hammer_src),'-o',str(hammer_exe)]
            subprocess.run(hammer_cmd,cwd=d,check=True)
            hammer_022=subprocess.run([str(hammer_exe),str(ROOT)],check=True,text=True,capture_output=True).stdout.strip()
            (ROOT/'Tests/hammer_022_results.txt').write_text(hammer_022+'\n')
            check(hammer_022.startswith('PASS:'),'0.22 hammer orientation, two-hand reach, hitstop, damage cadence and leap boundaries passed')
            def native_suite(name,description,*args):
                """0.24 suites: compile one Tests/<name>.cpp, fail on any compiler warning, run it, store <prefix>_results.txt."""
                src=ROOT/'Tests'/(name+'.cpp');exe=Path(d)/(name+('.exe' if sys.platform=='win32' else ''))
                cmd=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(src),'/Fe:'+str(exe)] if Path(compiler).stem.lower()=='cl' else [compiler,'-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(src),'-o',str(exe)]
                built=subprocess.run(cmd,cwd=d,check=True,text=True,capture_output=True)
                check('warning' not in (built.stdout+built.stderr).lower(),f'{name}: compiles without warnings')
                out=subprocess.run([str(exe),*args],check=True,text=True,capture_output=True).stdout.strip()
                (ROOT/'Tests'/(name.replace('_tests','')+'_results.txt')).write_text(out+'\n')
                check(out.startswith('PASS:'),description)
                return out
            enemy_024=native_suite('enemy_024_tests','0.24 enemy rules (broodcaller cap/cooldown and the rules added after it) passed',str(ROOT))
            arena_024=native_suite('arena_024_tests','0.24 Endless cover-collision rule (server-decided, hold-off cap, no flicker) passed')
            arsenal_024=native_suite('arsenal_024_tests','0.24 on-hit reward scaling (per-weapon base rate, equal reward rate, exact expected value) passed')
            ai_024=native_suite('enemy_ai_024_tests','0.24 enemy foundation (kits equal the old numbers, fluid movement planning, retreat bounds, pack spreading, unsticking, sound cues) passed',str(ROOT))
            pose_024=native_suite('enemy_pose_024_tests','0.24 enemy pose system (Unreal-faithful rotation math, track structure, timeline, neutral pose equals the old gait, sweeps over every ability) passed')
            ability_024=native_suite('enemy_ability_024_tests','0.24 enemy abilities (shove, back-hop, parry, shockwave swept against heroes, enrage, hazards, guard break, meteor volleys, blink, sniper tracking) passed')
            trig=subprocess.run([sys.executable,str(ROOT/'Tests/check_trig_overloads.py')],check=True,text=True,capture_output=True).stdout.strip()
            check(trig.startswith('PASS:'),'Actual fern expressions compile against the float/double overloads from the reported C2668 diagnostic')
    report={
        'source_checks_passed':len(checks), 'source_files_checked':len(source),
        'mesh_count':len(meshes),'mesh_vertex_count':vertices,'mesh_triangle_count':triangles,
        'degenerate_triangles':degenerates,'texture_count':len(textures),'audio_count':len(sounds),
        'rpc_implementation_presence_checks':rpcs,'native_tests':native,'gameplay_tests':gameplay,'buildcraft_tests':buildcraft,'adventure_tests':adventure,'career_tests':career,
        'hammer_022_tests':hammer_022,'enemy_024_tests':enemy_024,'arena_024_tests':arena_024,'arsenal_024_tests':arsenal_024,'enemy_ai_024_tests':ai_024,'enemy_pose_024_tests':pose_024,'enemy_ability_024_tests':ability_024,'melee_021_tests':melee_021,'bugfix_020_tests':bugfix_020,'reload_navigation_tests':reload_navigation,'trig_overload_regression':trig,'interface_arsenal_tests':interface_arsenal,
        'unreal_module_compile':'NOT RUN - Unreal Engine unavailable',
        'unreal_import_and_material_compile':'NOT RUN',
        'unreal_visual_physics_network_performance_tests':'NOT RUN',
        'windows_powershell_and_packaging':'NOT RUN',
        'limitations':'Lexical checks and shared-rule tests do not establish Unreal compilation or gameplay correctness.'}
    (ROOT/'Tests/validation_results.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    if not compiler: return 2
    return 0

if __name__=='__main__':
    raise SystemExit(validate())
