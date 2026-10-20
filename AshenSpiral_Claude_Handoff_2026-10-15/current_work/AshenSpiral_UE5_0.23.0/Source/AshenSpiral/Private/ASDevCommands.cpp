// 0.24.2: developer console commands for in-engine verification runs (Josue allowed in-engine runs for visual checks on 2026-10-19). They start a run, spawn and launch enemies,
// grant traits, take screenshots, and play a script of commands with waits, so a standalone game can stage a scene, photograph it and quit on its own:
//   UnrealEditor.exe AshenSpiral.uproject /Game/Maps/AshenSpiral -game -windowed -ProfileSlot=95 -ExecCmds="as.dev.script Saved/DevScripts/knock.txt"
// Everything goes through the authority paths the game itself uses (the host's game mode, the enemies' Control), so a run exercises the real code. Not in shipping builds.
// Every action logs an "AshenDev:" line; the knockback code logs "AshenKnock:" lines, so a run's log is its result.
#include "CoreMinimal.h"
#if !UE_BUILD_SHIPPING
#include "ASGameMode.h"
#include "ASGameState.h"
#include "ASPlayerController.h"
#include "ASCharacter.h"
#include "ASEnemy.h"
#include "ASWorld.h"
#include "ASBuildRules.h"
#include "ASExpeditionRules.h"
#include "ASLightning.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Containers/Ticker.h"
#include "UnrealClient.h"

namespace ASDev {
UWorld* GameWorld() {
    if(!GEngine) return nullptr;
    for(const FWorldContext& Context:GEngine->GetWorldContexts()) if(UWorld* W=Context.World()) if(Context.WorldType==EWorldType::Game||Context.WorldType==EWorldType::PIE) return W;
    return nullptr;
}
AASPlayerController* HostController(UWorld* W) {return W?Cast<AASPlayerController>(W->GetFirstPlayerController()):nullptr;}
AASCharacter* HostHero(UWorld* W) {auto* PC=HostController(W);return PC?Cast<AASCharacter>(PC->GetPawn()):nullptr;}
AASGameMode* Mode(UWorld* W) {return W?W->GetAuthGameMode<AASGameMode>():nullptr;}
float ArgFloat(const TArray<FString>& Args,int32 I,float Default) {return Args.IsValidIndex(I)?FCString::Atof(*Args[I]):Default;}
int32 ArgInt(const TArray<FString>& Args,int32 I,int32 Default) {return Args.IsValidIndex(I)?FCString::Atoi(*Args[I]):Default;}
bool bGod=false;

// The direction from a point to the nearest wall (world-static, side-on) between MinDistance and MaxDistance, from 24 probes; false when none is in that band.
bool NearestWall(UWorld* W,const FVector& From,float MinDistance,float MaxDistance,FVector& Direction,float& Distance) {
    Distance=MaxDistance;bool Found=false;FHitResult Hit;
    for(int32 I=0;I<24;++I) {
        const FVector Dir=FRotator(0.f,I*15.f,0.f).Vector();
        if(W->LineTraceSingleByObjectType(Hit,From,From+Dir*MaxDistance,FCollisionObjectQueryParams(ECC_WorldStatic))&&FMath::Abs(Hit.ImpactNormal.Z)<.5f&&Hit.Distance>=MinDistance&&Hit.Distance<Distance) {
            Distance=Hit.Distance;Direction=Dir;Found=true;
        }
    }
    return Found;
}

void Endless(const TArray<FString>& Args,UWorld* W) {
    auto* GM=Mode(W);auto* PC=HostController(W);if(!GM||!PC) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: endless - no host game mode or controller"));return;}
    if(auto* PS=PC->GetPlayerState<AASPlayerState>()) PS->bEndlessUnlocked=true; // dev only: the test profile has not beaten Normal
    GM->BeginEndless(PC,ArgInt(Args,0,4242));
    UE_LOG(LogTemp,Display,TEXT("AshenDev: endless begun (seed %d, ready=%d)"),ArgInt(Args,0,4242),GM->AllReady()?1:0);
}
void Run(const TArray<FString>& Args,UWorld* W) {
    auto* GM=Mode(W);auto* PC=HostController(W);if(!GM||!PC) return;
    GM->BeginRun(PC,ArgInt(Args,0,4242));
    UE_LOG(LogTemp,Display,TEXT("AshenDev: story run begun (seed %d, ready=%d)"),ArgInt(Args,0,4242),GM->AllReady()?1:0);
}
void Room(const TArray<FString>& Args,UWorld* W) {if(auto* GM=Mode(W)) {GM->EnterRoom(FMath::Clamp(ArgInt(Args,0,0),0,31));UE_LOG(LogTemp,Display,TEXT("AshenDev: room %d entered"),ArgInt(Args,0,0));}}
// as.dev.spawn <kind> [count] [distance cm] [health scale]: enemies in a row in front of the hero (combat phase only, like every minion).
void Spawn(const TArray<FString>& Args,UWorld* W) {
    auto* GM=Mode(W);auto* H=HostHero(W);if(!GM||!H) return;
    const int32 Kind=ArgInt(Args,0,0),Count=FMath::Clamp(ArgInt(Args,1,1),1,20);const float Distance=ArgFloat(Args,2,500.f),Health=ArgFloat(Args,3,1.f);
    const FVector Forward=H->GetActorForwardVector().GetSafeNormal2D(),Right(-Forward.Y,Forward.X,0.f);int32 Made=0;
    for(int32 I=0;I<Count;++I) if(AASEnemy* E=GM->SpawnMinion(H->GetActorLocation()+Forward*Distance+Right*(float(I)-(Count-1)*.5f)*160.f,nullptr,Kind,Health,false)) {++Made;E->RefreshLook();}
    UE_LOG(LogTemp,Display,TEXT("AshenDev: spawned %d of %d kind %d at %.0f cm"),Made,Count,Kind,Distance);
}
// as.dev.knock <strength> [radius]: every enemy near the hero is launched away from it, credited to the hero (the same call the maul's slam makes).
void Knock(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);auto* S=AS::State(W);if(!H||!S) return;
    const float Strength=ArgFloat(Args,0,950.f),Radius=ArgFloat(Args,1,700.f);int32 Count=0;
    for(auto Weak:TArray<TWeakObjectPtr<AASEnemy>>(S->Enemies)) if(AASEnemy* E=Weak.Get()) if(E->Health>0&&FVector::Dist2D(E->GetActorLocation(),H->GetActorLocation())<Radius) {E->Control(0,E->GetActorLocation()-H->GetActorLocation(),Strength,0,H);++Count;}
    UE_LOG(LogTemp,Display,TEXT("AshenDev: launched %d enemies at strength %.0f"),Count,Strength);
}
// as.dev.slamtest [strength]: a grunt is put about 4.5 m from the nearest wall and launched straight at it. Pass: an "AshenKnock: wall slam" line follows.
void SlamTest(const TArray<FString>& Args,UWorld* W) {
    auto* GM=Mode(W);auto* H=HostHero(W);if(!GM||!H) return;
    FVector Dir;float Distance=0.f;
    if(!NearestWall(W,H->GetActorLocation(),650.f,1600.f,Dir,Distance)) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: slamtest - no wall 6.5-16 m from the hero"));return;}
    AASEnemy* E=GM->SpawnMinion(H->GetActorLocation()+Dir*(Distance-450.f),nullptr,0,3.f,true);
    if(!E) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: slamtest - spawn failed (combat phase only)"));return;}
    E->Control(0,Dir,ArgFloat(Args,0,950.f),0,H);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: slamtest launched a grunt at a wall %.0f cm away from %.0f cm out (strength %.0f)"),450.f,Distance,ArgFloat(Args,0,950.f));
}
// as.dev.piletest [strength]: two grunts in a line across open ground; the near one is launched into the far one. Pass: an "AshenKnock: pile-up" line follows.
void PileTest(const TArray<FString>& Args,UWorld* W) {
    auto* GM=Mode(W);auto* H=HostHero(W);if(!GM||!H) return;
    FVector Dir=H->GetActorForwardVector().GetSafeNormal2D();FHitResult Hit;float Best=0.f;
    for(int32 I=0;I<24;++I) { // the most open direction
        const FVector D=FRotator(0.f,I*15.f,0.f).Vector();const float Reach=W->LineTraceSingleByObjectType(Hit,H->GetActorLocation(),H->GetActorLocation()+D*2000.f,FCollisionObjectQueryParams(ECC_WorldStatic))?Hit.Distance:2000.f;
        if(Reach>Best) {Best=Reach;Dir=D;}
    }
    AASEnemy* Near=GM->SpawnMinion(H->GetActorLocation()+Dir*260.f,nullptr,0,3.f,true);AASEnemy* Far=GM->SpawnMinion(H->GetActorLocation()+Dir*560.f,nullptr,0,3.f,true);
    if(!Near||!Far) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: piletest - spawn failed"));return;}
    Near->Control(0,Dir,ArgFloat(Args,0,950.f),0,H);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: piletest launched one grunt into another 3 m away (open ground %.0f cm)"),Best);
}
void Trait(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);if(!H) return;const int32 I=ArgInt(Args,0,-1),Rank=ArgInt(Args,1,1);
    if(I<0||I>=ASBuild::Count) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: trait index %d out of range"),I);return;}
    H->TraitRanks.SetNumZeroed(ASBuild::Count);H->TraitRanks[I]=FMath::Clamp(Rank,0,9);H->ForceNetUpdate();
    UE_LOG(LogTemp,Display,TEXT("AshenDev: trait %d (%s) set to rank %d"),I,UTF8_TO_TCHAR(ASBuild::Catalog[I].Name),H->TraitRanks[I]);
}
void Weapon(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);if(!H) return;FASBuild B=H->Build;B.Weapon=FMath::Clamp(ArgInt(Args,0,0),0,ASRules::WeaponCount-1);H->ApplyBuild(B,false);H->RefreshLook();
    UE_LOG(LogTemp,Display,TEXT("AshenDev: weapon %d"),B.Weapon);
}
void God(const TArray<FString>& Args,UWorld*) {bGod=ArgInt(Args,0,1)!=0;UE_LOG(LogTemp,Display,TEXT("AshenDev: god %d"),bGod?1:0);}
// as.dev.place <x> <y> [yaw] / as.dev.face <yaw>: move or turn the hero (and its controller, so the camera follows).
void Place(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);if(!H) return;FVector P=H->GetActorLocation();P.X=ArgFloat(Args,0,P.X);P.Y=ArgFloat(Args,1,P.Y);
    H->SetActorLocation(P,false,nullptr,ETeleportType::TeleportPhysics);if(Args.IsValidIndex(2)) if(auto* PC=HostController(W)) PC->SetControlRotation(FRotator(-15.f,ArgFloat(Args,2,0.f),0.f));
}
void Face(const TArray<FString>& Args,UWorld* W) {if(auto* PC=HostController(W)) PC->SetControlRotation(FRotator(ArgFloat(Args,1,-15.f),ArgFloat(Args,0,0.f),0.f));}
// as.dev.shot <name> [ui]: a screenshot of the game view to Saved/Screenshots/Dev/<name>.png (ui 1: with the HUD).
void Shot(const TArray<FString>& Args,UWorld*) {
    const FString Name=Args.IsValidIndex(0)?Args[0]:FString::Printf(TEXT("shot_%.0f"),FPlatformTime::Seconds());
    const FString File=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Screenshots/Dev")/(Name+TEXT(".png")));
    FScreenshotRequest::RequestScreenshot(File,ArgInt(Args,1,1)!=0,false);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: screenshot requested %s"),*File);
}
// as.dev.lightning [count] [distance]: Endless sky strikes around the hero (the same actor the wave 50+ timer spawns), the first right in front of the camera.
void Lightning(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);auto* S=AS::State(W);if(!H||!S) return;
    const int32 Count=FMath::Clamp(ArgInt(Args,0,1),1,12);const float Distance=ArgFloat(Args,1,520.f);
    const FVector Forward=H->GetActorForwardVector().GetSafeNormal2D();
    for(int32 I=0;I<Count;++I) {
        FVector Aim=H->GetActorLocation()+FRotator(0.f,I*137.5f,0.f).RotateVector(Forward)*(Distance+I*140.f);
        FHitResult Floor;Aim=W->LineTraceSingleByObjectType(Floor,Aim+FVector(0,0,300),Aim-FVector(0,0,1500),FCollisionObjectQueryParams(ECC_WorldStatic))?Floor.ImpactPoint+FVector(0,0,4):Aim-FVector(0,0,85);
        const FTransform Where(Aim);
        if(auto* Strike=W->SpawnActorDeferred<AASLightningStrike>(AASLightningStrike::StaticClass(),Where,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn)) {
            Strike->StrikeAt=S->SimTime+ASExpedition::LightningTelegraph+I*.12f;Strike->Radius=ASExpedition::LightningRadius;UGameplayStatics::FinishSpawningActor(Strike,Where);
        }
    }
    UE_LOG(LogTemp,Display,TEXT("AshenDev: %d lightning strikes called"),Count);
}
// as.dev.wave <n>: sets the Endless wave number (the sky, scenery stage and lightning follow it at the next wave's checks).
void Wave(const TArray<FString>& Args,UWorld* W) {if(auto* S=AS::State(W)) {S->EndlessWave=FMath::Clamp(ArgInt(Args,0,1),1,999);S->ForceNetUpdate();UE_LOG(LogTemp,Display,TEXT("AshenDev: endless wave set to %d"),S->EndlessWave);}}
// as.dev.state: one line with the phase, the wave, the hero and every enemy (kind, health, state, launched), for a run's log.
void State(const TArray<FString>&,UWorld* W) {
    auto* S=AS::State(W);auto* H=HostHero(W);if(!S) return;
    UE_LOG(LogTemp,Display,TEXT("AshenDev: phase %d room %d wave %d endless %d enemies %d hero %s hp %.0f"),int32(S->Phase),S->Room,S->Wave,S->bEndless?S->EndlessWave:-1,S->Enemies.Num(),H?*H->GetActorLocation().ToCompactString():TEXT("-"),H?H->Health:0.f);
    for(auto Weak:S->Enemies) if(const AASEnemy* E=Weak.Get()) UE_LOG(LogTemp,Display,TEXT("AshenDev:   enemy kind %d hp %.0f/%.0f state %d at %s launched %d"),E->Kind,E->Health,E->MaxHealth,int32(E->AIState),*E->GetActorLocation().ToCompactString(),(E->Flags&ASEnemyRules::FlagLaunched)?1:0);
}

// ---- the script player: one command per line; "wait <seconds>", "waitfor ready|combat|hub [timeout]", "#" comments; anything else is a console command ----
TArray<FString> Lines;int32 Next=0;double ResumeAt=0.0,WaitDeadline=0.0;FString WaitingFor;FTSTicker::FDelegateHandle Handle;
bool Ready(const FString& What,UWorld* W) {
    auto* S=AS::State(W);auto* GM=Mode(W);if(!S||!GM) return false;
    if(What==TEXT("ready")) return GM->AllReady()&&HostHero(W)!=nullptr;
    if(What==TEXT("combat")) return S->Phase==EASPhase::Combat;
    if(What==TEXT("hub")) return S->Phase==EASPhase::Hub;
    return true;
}
bool Tick(float) {
    UWorld* W=GameWorld();
    if(bGod) if(auto* H=HostHero(W)) {H->InvulnerableUntil=AS::Time(W)+1.f;if(H->Health<H->MaxHealth*.5f) H->Health=H->MaxHealth;}
    const double Now=FPlatformTime::Seconds();
    if(Next>=Lines.Num()) return true; // keep ticking for god mode; a new script restarts the list
    if(Now<ResumeAt) return true;
    if(!WaitingFor.IsEmpty()) {
        if(!Ready(WaitingFor,W)&&Now<WaitDeadline) return true;
        UE_LOG(LogTemp,Display,TEXT("AshenDev: waitfor %s %s"),*WaitingFor,Now<WaitDeadline?TEXT("met"):TEXT("TIMED OUT"));WaitingFor.Empty();
    }
    while(Next<Lines.Num()) {
        const FString Line=Lines[Next++].TrimStartAndEnd();
        if(Line.IsEmpty()||Line.StartsWith(TEXT("#"))) continue;
        TArray<FString> Parts;Line.ParseIntoArrayWS(Parts);
        if(Parts[0]==TEXT("wait")) {ResumeAt=Now+FMath::Max(0.f,FCString::Atof(Parts.IsValidIndex(1)?*Parts[1]:TEXT("1")));return true;}
        if(Parts[0]==TEXT("waitfor")) {WaitingFor=Parts.IsValidIndex(1)?Parts[1]:TEXT("ready");WaitDeadline=Now+(Parts.IsValidIndex(2)?FCString::Atof(*Parts[2]):30.0);return true;}
        UE_LOG(LogTemp,Display,TEXT("AshenDev: > %s"),*Line);
        if(GEngine&&W) GEngine->Exec(W,*Line);
        if(Parts[0]==TEXT("quit")||Parts[0]==TEXT("exit")) return false;
    }
    return true;
}
void Script(const TArray<FString>& Args,UWorld*) {
    if(!Args.IsValidIndex(0)) return;
    FString Path=Args[0];if(FPaths::IsRelative(Path)) Path=FPaths::ProjectDir()/Path;
    FString Text;if(!FFileHelper::LoadFileToString(Text,*Path)) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: cannot read script %s"),*Path);return;}
    Text.ParseIntoArrayLines(Lines,false);Next=0;ResumeAt=0.0;WaitingFor.Empty();
    if(!Handle.IsValid()) Handle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick),0.f);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: script %s (%d lines)"),*Path,Lines.Num());
}

#define AS_DEV_COMMAND(Name,Function,Help) static FAutoConsoleCommandWithWorldAndArgs ASDev##Function##Command(TEXT(Name),TEXT(Help),FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Function))
AS_DEV_COMMAND("as.dev.endless",Endless,"Starts an Endless run as the host (unlocks it on this profile first). Optional seed.");
AS_DEV_COMMAND("as.dev.run",Run,"Starts a story run as the host. Optional seed.");
AS_DEV_COMMAND("as.dev.room",Room,"Enters story room N.");
AS_DEV_COMMAND("as.dev.spawn",Spawn,"Spawns <kind> [count] [distance] [health scale] enemies in front of the hero (combat only).");
AS_DEV_COMMAND("as.dev.knock",Knock,"Launches every enemy near the hero away from it: [strength=950] [radius=700].");
AS_DEV_COMMAND("as.dev.slamtest",SlamTest,"Launches a grunt at the nearest wall from 4.5 m: [strength=950]. Watch the log for AshenKnock: wall slam.");
AS_DEV_COMMAND("as.dev.piletest",PileTest,"Launches a grunt into another across open ground: [strength=950]. Watch the log for AshenKnock: pile-up.");
AS_DEV_COMMAND("as.dev.trait",Trait,"Sets trait <index> to <rank> on the host hero.");
AS_DEV_COMMAND("as.dev.weapon",Weapon,"Gives the host hero weapon <id> (0-8).");
AS_DEV_COMMAND("as.dev.god",God,"1: the host hero cannot be hurt (and is healed back up); 0: off.");
AS_DEV_COMMAND("as.dev.place",Place,"Moves the host hero to <x> <y> [yaw].");
AS_DEV_COMMAND("as.dev.face",Face,"Turns the camera to <yaw> [pitch].");
AS_DEV_COMMAND("as.dev.shot",Shot,"Screenshot to Saved/Screenshots/Dev/<name>.png [ui=1].");
AS_DEV_COMMAND("as.dev.lightning",Lightning,"Calls [count=1] sky strikes around the host hero [distance=520].");
AS_DEV_COMMAND("as.dev.wave",Wave,"Sets the Endless wave number <n>.");
AS_DEV_COMMAND("as.dev.state",State,"Logs the phase, wave, hero and every enemy.");
AS_DEV_COMMAND("as.dev.script",Script,"Plays a script file: one console command per line, plus wait <s> and waitfor ready|combat|hub [timeout].");
#undef AS_DEV_COMMAND
} // namespace ASDev
#endif
