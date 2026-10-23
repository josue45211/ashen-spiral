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
#include "ASDecorRules.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "RenderTimer.h"
#include "DynamicRHI.h"
#include "Misc/App.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Containers/Ticker.h"
#include "UnrealClient.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/SpringArmComponent.h"

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
float WalkYaw=0.f,WalkScale=1.f;double WalkUntil=0.0;double AttackUntil=0.0,SpiralUntil=0.0; // as.dev.input: buttons held until then
double SlayUntil=0.0,NextSlay=0.0; // as.dev.slay: every enemy that appears is struck down until then (clears a story room's waves)
bool Tick(float);extern FTSTicker::FDelegateHandle Handle; // as.dev.walk: the host hero walks this way until then
void Cam(const TArray<FString>& Args,UWorld* W);

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
// as.dev.elite <mutation> [count] [distance] [side]: elite brutes in a row in front of the hero with that mutation (1 frostbound, 2 burning, 3 shocking, 4 venomous, 0 none).
void Elite(const TArray<FString>& Args,UWorld* W) {
    auto* GM=Mode(W);auto* H=HostHero(W);if(!GM||!H) return;
    const int32 Mutation=FMath::Clamp(ArgInt(Args,0,1),0,4),Count=FMath::Clamp(ArgInt(Args,1,1),1,8);const float Distance=ArgFloat(Args,2,900.f),Side=ArgFloat(Args,3,0.f);
    const FVector Forward=H->GetActorForwardVector().GetSafeNormal2D(),Right(-Forward.Y,Forward.X,0.f);int32 Made=0;
    for(int32 I=0;I<Count;++I) if(AASEnemy* E=GM->SpawnMinion(H->GetActorLocation()+Forward*Distance+Right*((float(I)-(Count-1)*.5f)*280.f+Side),nullptr,3,1.f,false)) {
        E->Mutation=uint8(Mutation);E->MaxHealth=FMath::Max(E->MaxHealth,300.f);E->Health=E->MaxHealth;E->RefreshLook();E->ForceNetUpdate();++Made;
    }
    UE_LOG(LogTemp,Display,TEXT("AshenDev: %d elite brutes with mutation %d at %.0f cm"),Made,Mutation,Distance);
}
// as.dev.boss <ward 0-3> [airborne] [distance]: a ward boss in front of the hero with its ward's kit (airborne 1: the Endless flying boss's dive loop instead), at its real health.
void Boss(const TArray<FString>& Args,UWorld* W) {
    auto* GM=Mode(W);auto* H=HostHero(W);if(!GM||!H) return;
    const int32 Ward=FMath::Clamp(ArgInt(Args,0,0),0,ASRules::WardCount-1);const bool Air=ArgInt(Args,1,0)!=0;
    AASEnemy* E=GM->SpawnMinion(H->GetActorLocation()+H->GetActorForwardVector().GetSafeNormal2D()*ArgFloat(Args,2,1000.f),nullptr,4,1.f,false);
    if(!E) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: boss - spawn failed (combat phase only)"));return;}
    E->Ward=Ward;E->Theme=Ward;E->bAirborne=Air;E->MaxHealth=1100.f+450.f*Ward;E->Health=E->MaxHealth;E->RefreshLook();E->ForceNetUpdate();
    UE_LOG(LogTemp,Display,TEXT("AshenDev: boss of ward %d%s, %.0f hp"),Ward,Air?TEXT(" (flying)"):TEXT(""),E->MaxHealth);
}
// as.dev.wound <fraction> [radius]: every enemy near the hero is hit (credited to the hero, through its own damage path) down to that share of its health; killall: to nothing.
int32 HitAll(UWorld* W,float Leave,float Radius) {
    auto* H=HostHero(W);auto* S=AS::State(W);if(!H||!S) return 0;int32 Count=0;
    for(auto Weak:TArray<TWeakObjectPtr<AASEnemy>>(S->Enemies)) if(AASEnemy* E=Weak.Get()) if(E->Health>0&&E->CustomTimeDilation>0.f&&FVector::Dist2D(E->GetActorLocation(),H->GetActorLocation())<Radius) {
        const float Blow=E->Health-E->MaxHealth*Leave;if(Blow<=0.f) continue;
        E->Hit(Leave<=0.f?E->Health+E->MaxHealth:Blow,H,true);++Count;
    }
    return Count;
}
void Wound(const TArray<FString>& Args,UWorld* W) {const int32 N=HitAll(W,FMath::Clamp(ArgFloat(Args,0,.5f),.01f,1.f),ArgFloat(Args,1,3000.f));UE_LOG(LogTemp,Display,TEXT("AshenDev: wounded %d enemies to %.0f%%"),N,ArgFloat(Args,0,.5f)*100.f);}
void KillAll(const TArray<FString>& Args,UWorld* W) {const int32 N=HitAll(W,0.f,ArgFloat(Args,0,6000.f));UE_LOG(LogTemp,Display,TEXT("AshenDev: struck down %d enemies"),N);}
void Slay(const TArray<FString>& Args,UWorld*) {
    SlayUntil=FPlatformTime::Seconds()+ArgFloat(Args,0,20.f);NextSlay=0.0;
    if(!Handle.IsValid()) Handle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick),0.f);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: slaying every enemy for %.0f s"),ArgFloat(Args,0,20.f));
}
// as.dev.keeper: a grunt frozen in time 25 m behind the hero (it never moves, attacks or dies to killall), so a wave stays in combat while scenes are staged and cleared.
void Keeper(const TArray<FString>&,UWorld* W) {
    auto* GM=Mode(W);auto* H=HostHero(W);if(!GM||!H) return;
    if(AASEnemy* E=GM->SpawnMinion(H->GetActorLocation()-H->GetActorForwardVector().GetSafeNormal2D()*2500.f,nullptr,0,1.f,false)) {E->CustomTimeDilation=0.f;UE_LOG(LogTemp,Display,TEXT("AshenDev: keeper frozen at %s"),*E->GetActorLocation().ToCompactString());}
}
// as.dev.freeze [1|0] [radius=2000]: every enemy within the radius of the hero stops in time (CustomTimeDilation 0) or runs again; for photographing them where they stand. The keeper
// (25 m behind the hero) is out of its reach, so it keeps the wave going.
void Freeze(const TArray<FString>& Args,UWorld* W) {
    auto* S=AS::State(W);auto* H=HostHero(W);if(!S) return;const bool Stop=ArgInt(Args,0,1)!=0;const float Radius=ArgFloat(Args,1,2000.f);int32 Count=0;
    for(auto Weak:S->Enemies) if(AASEnemy* E=Weak.Get()) if(!H||FVector::Dist2D(E->GetActorLocation(),H->GetActorLocation())<=Radius) {E->CustomTimeDilation=Stop?0.f:1.f;++Count;}
    UE_LOG(LogTemp,Display,TEXT("AshenDev: %d enemies %s"),Count,Stop?TEXT("frozen"):TEXT("running"));
}
// as.dev.lineup [distance=500] [spacing=260]: one enemy of every ground kind (0-9, the boss and the elite left out) in a row in front of the hero, at full health, turned to face it.
TArray<TWeakObjectPtr<AASEnemy>> LineupEnemies; // the last lineup, from the hero's left (as.dev.frame)
void Lineup(const TArray<FString>& Args,UWorld* W) {
    auto* GM=Mode(W);auto* H=HostHero(W);if(!GM||!H) return;
    LineupEnemies.Reset();
    const FVector Forward=H->GetActorForwardVector().GetSafeNormal2D(),Right(-Forward.Y,Forward.X,0.f);const float Distance=ArgFloat(Args,0,500.f),Spacing=ArgFloat(Args,1,260.f);
    const int32 Kinds[]={0,1,2,5,6,7,8,9};int32 Made=0;
    for(int32 I=0;I<UE_ARRAY_COUNT(Kinds);++I) if(AASEnemy* E=GM->SpawnMinion(H->GetActorLocation()+Forward*Distance+Right*(float(I)-(UE_ARRAY_COUNT(Kinds)-1)*.5f)*Spacing,nullptr,Kinds[I],1.f,true)) {
        E->SetActorRotation((-Forward).Rotation());E->RefreshLook();LineupEnemies.Add(E);++Made;
    }
    UE_LOG(LogTemp,Display,TEXT("AshenDev: lineup of %d enemy kinds"),Made);
}
// as.dev.status <burn|poison|frost|bleed|shock> [radius]: the element applied by the hero (a full proc with its pool) to every enemy near it.
void Status(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);auto* S=AS::State(W);if(!H||!S||!Args.IsValidIndex(0)) return;
    const FString N=Args[0];const EASHitKind Kind=N==TEXT("burn")?EASHitKind::Burn:N==TEXT("poison")?EASHitKind::Poison:N==TEXT("frost")?EASHitKind::Frost:N==TEXT("bleed")?EASHitKind::Bleed:EASHitKind::Shock;
    int32 Count=0;
    for(auto Weak:TArray<TWeakObjectPtr<AASEnemy>>(S->Enemies)) if(AASEnemy* E=Weak.Get()) if(E->Health>0&&FVector::Dist2D(E->GetActorLocation(),H->GetActorLocation())<ArgFloat(Args,1,3000.f)) {E->ApplyElement(Kind,H,1.f,3.f,40.f);++Count;}
    UE_LOG(LogTemp,Display,TEXT("AshenDev: %s applied to %d enemies"),*N,Count);
}
// as.dev.station <id>: the hero is put beside an interaction point (9 = the reward chest), facing it.
void Station(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);auto* S=AS::State(W);if(!H||!S||!S->Arena) return;
    const FVector Point=S->Arena->InteractionPoint(ArgInt(Args,0,9));FVector Away=H->GetActorLocation()-Point;Away.Z=0.f;Away=Away.IsNearlyZero()?FVector(-1.f,0.f,0.f):Away.GetSafeNormal();
    const FVector At=FVector(Point.X,Point.Y,H->GetActorLocation().Z)+Away*120.f;
    H->SetActorLocation(At,false,nullptr,ETeleportType::TeleportPhysics);if(auto* PC=HostController(W)) PC->SetControlRotation(FRotator(-15.f,(-Away).Rotation().Yaw,0.f));
    UE_LOG(LogTemp,Display,TEXT("AshenDev: by station %d at %s (station page here: %d)"),ArgInt(Args,0,9),*At.ToCompactString(),S->Arena->StationAt(At));
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
// as.dev.pick <trait> [times]: the host hero takes that trait from a reward, through the real pick path (ranks, evolutions, the metamorphosis notice), up to <times> or its maximum.
void Pick(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);if(!H) return;const int32 I=ArgInt(Args,0,-1);
    if(I<0||I>=ASBuild::Count) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: pick - trait %d out of range"),I);return;}
    int32 Taken=0;for(int32 N=0;N<FMath::Clamp(ArgInt(Args,1,1),1,12);++N) if(H->CanApplyChoice(ASBuild::OfferBase+I)&&H->ApplyChoice(ASBuild::OfferBase+I)) ++Taken;
    UE_LOG(LogTemp,Display,TEXT("AshenDev: picked %s %d time(s) (catalogue max %d)"),UTF8_TO_TCHAR(ASBuild::Catalog[I].Name),Taken,ASBuild::Catalog[I].Max);
}
// as.dev.weaponlook <metallic> <roughness>: retunes the host hero's weapon material live (for judging finishes by screenshot; the next look refresh restores ASData::WeaponFinish).
void WeaponLook(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);if(!H) return;
    TArray<UStaticMeshComponent*> Meshes;H->CollectTrailMeshes(Meshes);const FString Held=FString::Printf(TEXT("SM_Weapon%d"),H->Build.Weapon);
    for(UStaticMeshComponent* Mesh:Meshes) if(Mesh&&Mesh->GetStaticMesh()&&Mesh->GetStaticMesh()->GetName()==Held)
        if(auto* M=Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0))) {M->SetScalarParameterValue(TEXT("Metallic"),ArgFloat(Args,0,.9f));M->SetScalarParameterValue(TEXT("Roughness"),ArgFloat(Args,1,.4f));}
    UE_LOG(LogTemp,Display,TEXT("AshenDev: weapon look metallic %.2f roughness %.2f"),ArgFloat(Args,0,.9f),ArgFloat(Args,1,.4f));
}
// as.dev.weapon <id> [other=1]: the host hero's weapon, or (other 1, run on the host) the guest's: the build replicates, so both machines draw it.
void Weapon(const TArray<FString>& Args,UWorld* W) {
    AASCharacter* H=HostHero(W);
    if(ArgInt(Args,1,0)!=0) {H=nullptr;if(auto* S=AS::State(W)) for(auto Weak:S->Heroes) if(AASCharacter* Other=Weak.Get()) if(!Other->IsLocallyControlled()) {H=Other;break;}}
    if(!H) return;FASBuild B=H->Build;B.Weapon=FMath::Clamp(ArgInt(Args,0,0),0,ASRules::WeaponCount-1);H->ApplyBuild(B,false);H->RefreshLook();
    UE_LOG(LogTemp,Display,TEXT("AshenDev: weapon %d%s"),B.Weapon,ArgInt(Args,1,0)!=0?TEXT(" (the other player's hero)"):TEXT(""));
}
// as.dev.walk <yaw> <seconds> [scale=1]: the host hero walks that way (input, through its own movement and collision); the script keeps ticking meanwhile. Scale is the stick's push:
// 1 runs at full speed, .5 jogs at half of it, .25 walks.
void Walk(const TArray<FString>& Args,UWorld* W) {
    WalkYaw=ArgFloat(Args,0,0.f);WalkScale=FMath::Clamp(ArgFloat(Args,2,1.f),.05f,1.f);WalkUntil=FPlatformTime::Seconds()+ArgFloat(Args,1,1.f);
    if(!Handle.IsValid()) Handle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick),0.f);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: walking yaw %.0f for %.1f s from %s"),WalkYaw,ArgFloat(Args,1,1.f),HostHero(W)?*HostHero(W)->GetActorLocation().ToCompactString():TEXT("-"));
}
// as.dev.input attack|spiral <seconds> | dash | jump | rune | reload: the host's own buttons, through its controller (the same calls the key bindings make).
void Input(const TArray<FString>& Args,UWorld* W) {
    auto* PC=HostController(W);if(!PC||!Args.IsValidIndex(0)) return;
    const FString What=Args[0];const double Hold=FPlatformTime::Seconds()+ArgFloat(Args,1,.5f);
    if(!Handle.IsValid()) Handle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick),0.f);
    PC->DevButton(What,true);
    if(What==TEXT("attack")) AttackUntil=Hold;else if(What==TEXT("spiral")) SpiralUntil=Hold;else if(What==TEXT("jump")) PC->DevButton(What,false);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: input %s"),*What);
}
void God(const TArray<FString>& Args,UWorld*) {bGod=ArgInt(Args,0,1)!=0;UE_LOG(LogTemp,Display,TEXT("AshenDev: god %d"),bGod?1:0);}
// as.dev.hurt <amount> [other=1]: a blow to the host hero from the front through its own damage path (armour, shields, the flinch, going down); it lands even in god mode. other 1: the
// first hero that is not the local player's (run on the host, that is the guest's: only the authority can hurt a hero, so a guest's own script cannot down itself).
void HurtHero(const TArray<FString>& Args,UWorld* W) {
    AASCharacter* H=HostHero(W);
    if(ArgInt(Args,1,0)!=0) {H=nullptr;if(auto* S=AS::State(W)) for(auto Weak:S->Heroes) if(AASCharacter* Other=Weak.Get()) if(!Other->IsLocallyControlled()) {H=Other;break;}}
    if(!H) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: hurt - no such hero"));return;}const float Before=H->Health;
    H->InvulnerableUntil=0.f;H->Hurt(ArgFloat(Args,0,20.f));
    UE_LOG(LogTemp,Display,TEXT("AshenDev: hurt %.0f: health %.0f -> %.0f, downed %d"),ArgFloat(Args,0,20.f),Before,H->Health,H->bDowned?1:0);
}
// as.dev.place <x> <y> [yaw] / as.dev.face <yaw>: move or turn the hero (and its controller, so the camera follows).
void Place(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);if(!H) return;FVector P=H->GetActorLocation();P.X=ArgFloat(Args,0,P.X);P.Y=ArgFloat(Args,1,P.Y);
    H->SetActorLocation(P,false,nullptr,ETeleportType::TeleportPhysics);if(Args.IsValidIndex(2)) if(auto* PC=HostController(W)) PC->SetControlRotation(FRotator(-15.f,ArgFloat(Args,2,0.f),0.f));
}
// as.dev.cam <x> <y> <z> <pitch> <yaw> [fov] | as.dev.cam off: a fixed photo camera for screenshots (the game's own camera follows the aim), or back to the hero's.
TWeakObjectPtr<ACameraActor> PhotoCamera;
void Cam(const TArray<FString>& Args,UWorld* W) {
    auto* PC=HostController(W);if(!PC||!W) return;
    if(Args.IsValidIndex(0)&&Args[0]==TEXT("off")) {if(APawn* P=PC->GetPawn()) PC->SetViewTarget(P);if(PhotoCamera.IsValid()) PhotoCamera->Destroy();PhotoCamera.Reset();return;}
    if(!PhotoCamera.IsValid()) PhotoCamera=W->SpawnActor<ACameraActor>();
    if(!PhotoCamera.IsValid()) return;
    PhotoCamera->SetActorLocationAndRotation(FVector(ArgFloat(Args,0,0.f),ArgFloat(Args,1,0.f),ArgFloat(Args,2,800.f)),FRotator(ArgFloat(Args,3,-30.f),ArgFloat(Args,4,0.f),0.f));
    if(UCameraComponent* C=PhotoCamera->GetCameraComponent()) {C->SetFieldOfView(ArgFloat(Args,5,80.f));C->SetConstraintAspectRatio(false);}
    PC->SetViewTarget(PhotoCamera.Get());
}
void Face(const TArray<FString>& Args,UWorld* W) {if(auto* PC=HostController(W)) PC->SetControlRotation(FRotator(ArgFloat(Args,1,-15.f),ArgFloat(Args,0,0.f),0.f));}
// as.dev.aimat [nth=0]: turns the camera so the crosshair's ray (the one the claws' pounce reads, ASPlayerController::UpdateAim) runs through the nth nearest living enemy's body. The camera sits
// on a boom to the hero's right, so a plain "face the enemy" misses it by the boom's offset; this solves for the camera's own position (a few rounds of the same boom arithmetic).
void AimAt(const TArray<FString>& Args,UWorld* W) {
    auto* PC=HostController(W);auto* H=HostHero(W);auto* S=AS::State(W);if(!PC||!H||!S) return;
    TArray<AASEnemy*> Near;for(auto Weak:S->Enemies) if(AASEnemy* E=Weak.Get()) if(E->Health>0) Near.Add(E);
    const FVector Me=H->GetActorLocation();Near.Sort([&Me](const AASEnemy& A,const AASEnemy& B){return FVector::DistSquared(Me,A.GetActorLocation())<FVector::DistSquared(Me,B.GetActorLocation());});
    const int32 Nth=ArgInt(Args,0,0);if(!Near.IsValidIndex(Nth)) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: aimat - no enemy number %d"),Nth);return;}
    const FVector Chest=Near[Nth]->GetActorLocation();FRotator R=(Chest-Me).Rotation();
    for(int32 Round=0;Round<6;++Round) {
        const FVector Camera=Me+H->Boom->TargetOffset+FRotationMatrix(R).TransformVector(FVector(-H->Boom->TargetArmLength,0.f,0.f)+H->Boom->SocketOffset);
        const FRotator Want=(Chest-Camera).Rotation();R=FRotator(Want.Pitch-H->Camera->GetRelativeRotation().Pitch,Want.Yaw,0.f);
    }
    PC->SetControlRotation(R);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: aimat - the camera ray runs through %s at %.0f cm (control rotation %s)"),*Near[Nth]->GetName(),FVector::Dist(Me,Chest),*R.ToCompactString());
}
// as.dev.follow <distance> <height> <angle> [fov] | off: the photo camera rides with the hero, <angle> degrees round from its facing (0 in front, 90 its right side, 180 behind),
// <height> cm above its middle, looking at its chest; for photographing the hero's own animation wherever it goes. [look-ahead] [other=1]: it rides with the OTHER player's hero instead,
// as this machine draws it (a simulated proxy, or on the host the guest's hero).
bool bFollow=false,bFollowOther=false;float FollowDistance=360.f,FollowHeight=30.f,FollowAngle=90.f,FollowAhead=0.f;
AASCharacter* FollowedHero(UWorld* W) {
    if(!bFollowOther) return HostHero(W);
    if(auto* S=AS::State(W)) for(auto Weak:S->Heroes) if(AASCharacter* H=Weak.Get()) if(!H->IsLocallyControlled()) return H;
    return nullptr;
}
void FollowTick(UWorld* W) {
    auto* H=FollowedHero(W);if(!bFollow||!H||!PhotoCamera.IsValid()) return;
    const FVector Chest=H->GetActorLocation()+FVector(0.f,0.f,25.f),Target=Chest+FRotator(0.f,H->GetActorRotation().Yaw,0.f).Vector()*FollowAhead; // looked at: the chest, or a point ahead of the hero
    const FVector From=Chest+FRotator(0.f,H->GetActorRotation().Yaw+FollowAngle,0.f).Vector()*FollowDistance+FVector(0.f,0.f,FollowHeight);
    PhotoCamera->SetActorLocationAndRotation(From,(Target-From).Rotation());
}
void Follow(const TArray<FString>& Args,UWorld* W) {
    if(Args.IsValidIndex(0)&&Args[0]==TEXT("off")) {bFollow=false;bFollowOther=false;Cam({TEXT("off")},W);return;}
    FollowDistance=ArgFloat(Args,0,360.f);FollowHeight=ArgFloat(Args,1,30.f);FollowAngle=ArgFloat(Args,2,90.f);FollowAhead=ArgFloat(Args,4,0.f);bFollowOther=ArgInt(Args,5,0)!=0;
    // the camera is placed beside the hero at once (re-aiming a camera that is already the view never sends it through the origin)
    if(!PhotoCamera.IsValid()) Cam({TEXT("0"),TEXT("0"),TEXT("0"),TEXT("0"),TEXT("0"),TEXT("55")},W);
    else if(auto* PC=HostController(W)) if(PC->GetViewTarget()!=PhotoCamera.Get()) PC->SetViewTarget(PhotoCamera.Get());
    if(PhotoCamera.IsValid()) if(UCameraComponent* C=PhotoCamera->GetCameraComponent()) C->SetFieldOfView(ArgFloat(Args,3,55.f));
    bFollow=PhotoCamera.IsValid();FollowTick(W);
    if(!Handle.IsValid()) Handle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick),0.f);
}
// as.dev.clearing [radius=600] [away=1600]: the hero is moved to the open ground nearest the arena's middle, at least <away> cm from it (the chest and its loot stand there,
// without collision to find), with no wall, cover or ruin within <radius>, for clean photographs.
void Clearing(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);auto* S=AS::State(W);if(!H||!W) return;
    const float Radius=ArgFloat(Args,0,600.f),Half=H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector Middle=S&&S->Arena?S->Arena->CellPosition(0):H->GetActorLocation();const FCollisionObjectQueryParams Static(ECC_WorldStatic);
    FVector Best=FVector::ZeroVector;float BestDistance=FLT_MAX;const float Ground=H->GetActorLocation().Z-Half; // the hero stands on the arena floor: only that level counts (not a block's top)
    for(int32 X=-14;X<=14;++X) for(int32 Y=-14;Y<=14;++Y) {
        const FVector P=Middle+FVector(X*200.f,Y*200.f,0.f);FHitResult Floor;
        if(FVector::Dist2D(P,Middle)<ArgFloat(Args,1,1600.f)||FVector::Dist2D(P,Middle)>=BestDistance||!W->LineTraceSingleByObjectType(Floor,P+FVector(0,0,800),P-FVector(0,0,800),Static)||FMath::Abs(Floor.ImpactPoint.Z-Ground)>40.f) continue;
        if(W->OverlapAnyTestByObjectType(Floor.ImpactPoint+FVector(0,0,110),FQuat::Identity,Static,FCollisionShape::MakeBox(FVector(Radius,Radius,80.f)))) continue; // 30-190 cm above the ground: walls, cover, ruins; not the floor
        Best=Floor.ImpactPoint+FVector(0,0,Half+2.f);BestDistance=FVector::Dist2D(P,Middle);
    }
    if(BestDistance==FLT_MAX) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: clearing - no open ground %.0f cm wide near the middle"),Radius);return;}
    H->SetActorLocation(Best,false,nullptr,ETeleportType::TeleportPhysics);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: clearing at %s (%.0f cm from the middle)"),*Best.ToCompactString(),BestDistance);
}
// as.dev.shot <name> [ui]: a screenshot of the game view to Saved/Screenshots/Dev/<name>.png (ui 1: with the HUD).
void Shot(const TArray<FString>& Args,UWorld* W) {
    const FString Name=Args.IsValidIndex(0)?Args[0]:FString::Printf(TEXT("shot_%.0f"),FPlatformTime::Seconds());
    const FString File=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Screenshots/Dev")/(Name+TEXT(".png")));
    FScreenshotRequest::RequestScreenshot(File,ArgInt(Args,1,1)!=0,false);
    const AASPlayerController* PC=HostController(W);const AASCharacter* H=HostHero(W);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: screenshot requested %s (camera %s, hero %s)"),*File,PC&&PC->PlayerCameraManager?*PC->PlayerCameraManager->GetCameraLocation().ToCompactString():TEXT("-"),H?*H->GetActorLocation().ToCompactString():TEXT("-"));
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
// as.dev.decortest: the hero is put 5 m from the nearest standing ruin (stage 0) and walks straight at its middle for 2.5 s; a later as.dev.decorcheck reports how far from
// the middle it stopped against the footprint's radius (it should stop at radius + its capsule, never inside: the "correct hitbox" check for the explorable ring).
FVector DecorCore=FVector::ZeroVector;float DecorRadius=0.f;
void DecorTest(const TArray<FString>&,UWorld* W) {
    auto* H=HostHero(W);auto* S=AS::State(W);if(!H||!S||!S->Arena) return;
    float Best=FLT_MAX;
    for(const auto& P:ASDecor::Ring(uint32(S->Arena->Config.Seed))) {
        if(P.Kit<0||P.Kit>=int(ASDecor::Kits().size())) continue;const auto& K=ASDecor::Kits()[size_t(P.Kit)];if(K.Stage!=0||K.Legs) continue;
        const FVector Core(S->Arena->CellPosition(0).X+P.X,S->Arena->CellPosition(0).Y+P.Y,H->GetActorLocation().Z);const float D=FVector::Dist2D(Core,H->GetActorLocation());
        if(D<Best) {Best=D;DecorCore=Core;DecorRadius=ASDecor::SolidFor(K,P.Scale).Radius;}
    }
    if(Best==FLT_MAX) return;
    const FVector Toward=(DecorCore-S->Arena->CellPosition(0)).GetSafeNormal2D(); // approach from the arena side
    H->SetActorLocation(DecorCore-Toward*(DecorRadius+500.f),false,nullptr,ETeleportType::TeleportPhysics);
    WalkYaw=Toward.Rotation().Yaw;WalkUntil=FPlatformTime::Seconds()+2.5;
    UE_LOG(LogTemp,Display,TEXT("AshenDev: decortest walking at a ruin of solid radius %.0f from 5 m"),DecorRadius);
}
void DecorCheck(const TArray<FString>&,UWorld* W) {
    auto* H=HostHero(W);if(!H) return;
    const float D=FVector::Dist2D(H->GetActorLocation(),DecorCore),Capsule=H->GetCapsuleComponent()->GetScaledCapsuleRadius();
    UE_LOG(LogTemp,Display,TEXT("AshenDev: decorcheck stopped %.0f cm from the ruin's middle; footprint %.0f + capsule %.0f = %.0f: %s"),D,DecorRadius,Capsule,DecorRadius+Capsule,
        D>=DecorRadius+Capsule-8.f&&D<=DecorRadius+Capsule+60.f?TEXT("PASS (stopped at the surface)"):(D<DecorRadius?TEXT("FAIL (inside the ruin)"):TEXT("FAIL (stopped short or slid past)")));
}
// as.dev.lookat <mesh name part> [distance=1400] [height=300]: the photo camera looks, from the arena side, at the nearest ruin piece whose mesh name contains the text (a later stage's
// pieces show only once the sky reaches their stage: set the wave first).
void LookAt(const TArray<FString>& Args,UWorld* W) {
    auto* S=AS::State(W);auto* PC=HostController(W);if(!S||!S->Arena||!PC||!W||!Args.IsValidIndex(0)) return;
    const FVector Middle=S->Arena->CellPosition(0);float Best=FLT_MAX,Height=0.f;FVector Target=FVector::ZeroVector;FString Found;
    for(const auto& P:ASDecor::Ring(uint32(S->Arena->Config.Seed))) {
        if(P.Kit<0||P.Kit>=int(ASDecor::Kits().size())) continue;const auto& K=ASDecor::Kits()[size_t(P.Kit)];
        const FString Mesh=UTF8_TO_TCHAR(K.Mesh);if(!Mesh.Contains(Args[0])) continue;
        const FVector At(Middle.X+P.X,Middle.Y+P.Y,Middle.Z);const float D=FVector::Dist2D(At,Middle);
        if(D<Best) {Best=D;Target=At;Found=Mesh;Height=K.Height*P.Scale;}
    }
    if(Best==FLT_MAX) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: lookat - no ruin piece named like %s"),*Args[0]);return;}
    const FVector Toward=(Target-Middle).GetSafeNormal2D(),From=Target-Toward*ArgFloat(Args,1,1400.f)+FVector(0.f,0.f,ArgFloat(Args,2,300.f)),Aim=Target+FVector(0.f,0.f,Height*.45f);
    bFollow=false;
    if(!PhotoCamera.IsValid()) PhotoCamera=W->SpawnActor<ACameraActor>();
    if(!PhotoCamera.IsValid()) return;
    PhotoCamera->SetActorLocationAndRotation(From,(Aim-From).Rotation());
    if(UCameraComponent* C=PhotoCamera->GetCameraComponent()) {C->SetFieldOfView(60.f);C->SetConstraintAspectRatio(false);}
    PC->SetViewTarget(PhotoCamera.Get());
    UE_LOG(LogTemp,Display,TEXT("AshenDev: looking at %s at %s (%.0f cm tall)"),*Found,*Target.ToCompactString(),Height);
}
// as.dev.frame <n> [distance=420] [height=10] [fov=50]: the photo camera stands in front of the n-th enemy of the last lineup (0 = the hero's left), <height> cm above its middle, looking at it.
void Frame(const TArray<FString>& Args,UWorld* W) {
    auto* PC=HostController(W);const int32 N=ArgInt(Args,0,0);const AASEnemy* E=LineupEnemies.IsValidIndex(N)?LineupEnemies[N].Get():nullptr;
    if(!PC||!W||!E) {UE_LOG(LogTemp,Warning,TEXT("AshenDev: frame - no lineup enemy %d"),N);return;}
    const FVector Middle=E->GetActorLocation(),From=Middle+E->GetActorForwardVector().GetSafeNormal2D()*ArgFloat(Args,1,420.f)+FVector(0.f,0.f,ArgFloat(Args,2,10.f));
    bFollow=false;
    if(!PhotoCamera.IsValid()) PhotoCamera=W->SpawnActor<ACameraActor>();
    if(!PhotoCamera.IsValid()) return;
    PhotoCamera->SetActorLocationAndRotation(From,(Middle-From).Rotation());
    if(UCameraComponent* C=PhotoCamera->GetCameraComponent()) {C->SetFieldOfView(ArgFloat(Args,3,50.f));C->SetConstraintAspectRatio(false);}
    PC->SetViewTarget(PhotoCamera.Get());
    UE_LOG(LogTemp,Display,TEXT("AshenDev: framing lineup enemy %d (kind %d) at %s"),N,E->Kind,*Middle.ToCompactString());
}
// as.dev.record start [seconds] | stop <name>: records what the game plays (the main submix) to Saved/AudioRecordings/Dev/<name>.wav, to measure sounds in the mix (the roars).
void Record(const TArray<FString>& Args,UWorld* W) {
    if(!W||!Args.IsValidIndex(0)) return;
    if(Args[0]==TEXT("start")) {UAudioMixerBlueprintLibrary::StartRecordingOutput(W,ArgFloat(Args,1,30.f));UE_LOG(LogTemp,Display,TEXT("AshenDev: recording the game's audio"));return;}
    const FString Name=Args.IsValidIndex(1)?Args[1]:TEXT("recording");
    const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("AudioRecordings/Dev"));
    UAudioMixerBlueprintLibrary::StopRecordingOutput(W,EAudioRecordingExportType::WavFile,Name,Folder);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: audio recording written to %s/%s.wav"),*Folder,*Name);
}
// as.dev.perf <seconds> [label]: samples the frame time for that long and logs it - average, 95th and 99th percentile, worst, the frame rate, and the game, render and GPU
// thread averages in ms - for checking what a look costs in a fight ("AshenPerf:" lines).
void Perf(const TArray<FString>& Args,UWorld*) {
    struct FSamples {TArray<float> Frame;double GameSum=0.0,RenderSum=0.0,GpuSum=0.0;double Until=0.0;FString Label;};
    auto State=MakeShared<FSamples>();State->Label=Args.IsValidIndex(1)?Args[1]:TEXT("run");State->Until=FPlatformTime::Seconds()+FMath::Max(1.f,ArgFloat(Args,0,10.f));
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([State](float) {
        State->Frame.Add(float(FApp::GetDeltaTime()*1000.0));
        State->GameSum+=FPlatformTime::ToMilliseconds(GGameThreadTime);State->RenderSum+=FPlatformTime::ToMilliseconds(GRenderThreadTime);State->GpuSum+=FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
        if(FPlatformTime::Seconds()<State->Until) return true;
        TArray<float> Sorted=State->Frame;Sorted.Sort();const int32 N=Sorted.Num();double Sum=0.0;for(float F:Sorted) Sum+=F;
        const auto Percentile=[&](float P) {return N?Sorted[FMath::Clamp(int32(P*float(N)),0,N-1)]:0.f;};
        UE_LOG(LogTemp,Display,TEXT("AshenPerf: %s - %d frames, avg %.2f ms (%.0f fps), p95 %.2f, p99 %.2f, worst %.2f; game thread %.2f ms, render thread %.2f ms, GPU %.2f ms"),*State->Label,N,N?Sum/N:0.0,Sum>0.0?1000.0*N/Sum:0.0,
            Percentile(.95f),Percentile(.99f),N?Sorted.Last():0.f,N?State->GameSum/N:0.0,N?State->RenderSum/N:0.0,N?State->GpuSum/N:0.0);
        return false;
    }),0.f);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: measuring the frame time for %.0f s (%s)"),FMath::Max(1.f,ArgFloat(Args,0,10.f)),*State->Label);
}
// as.dev.motion <seconds> [label]: for that long, counts each frame in which an enemy that is moving (its velocity over 150 cm/s) does NOT move on screen - a client that skips a
// proxy's movement simulation (an unresolved movement base) shows enemies that hold still between net updates; the host never does ("AshenMotion:" line). Also the other heroes.
void Motion(const TArray<FString>& Args,UWorld* W) {
    struct FTrack {FVector Last=FVector::ZeroVector;bool bHave=false;};
    struct FCount {TMap<TWeakObjectPtr<AActor>,FTrack> Seen;int64 Moving=0,Held=0,Frames=0,Watched=0,Unresolved=0;double Moved=0.0,Expected=0.0;double Until=0.0,Began=0.0;FString Label;};
    auto State=MakeShared<FCount>();State->Label=Args.IsValidIndex(1)?Args[1]:TEXT("run");State->Began=FPlatformTime::Seconds();State->Until=State->Began+FMath::Max(1.f,ArgFloat(Args,0,10.f));
    TWeakObjectPtr<UWorld> World=W;
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([State,World](float Delta) {
        UWorld* Now=World.Get();auto* S=Now?AS::State(Now):nullptr;
        if(Delta>0.f) ++State->Frames;
        if(S&&Delta>0.f) {
            TArray<AActor*> Watched;for(auto Weak:S->Enemies) if(AASEnemy* E=Weak.Get()) Watched.Add(E);
            for(auto Weak:S->Heroes) if(AASCharacter* H=Weak.Get()) if(!H->IsLocallyControlled()) Watched.Add(H);
            for(AActor* A:Watched) {
                FTrack& T=State->Seen.FindOrAdd(A);const FVector Here=A->GetActorLocation(),Velocity=A->GetVelocity();
                ++State->Watched;if(const ACharacter* Body=Cast<ACharacter>(A)) if(Body->GetReplicatedBasedMovement().IsBaseUnresolved()) ++State->Unresolved; // what makes a client skip simulating a proxy
                if(T.bHave&&Velocity.Size2D()>150.f&&FVector::Dist2D(Here,T.Last)<300.f) {
                    const double Step=FVector::Dist2D(Here,T.Last);++State->Moving;State->Moved+=Step;State->Expected+=Velocity.Size2D()*Delta;
                    if(Step<.5) ++State->Held;
                }
                T.Last=Here;T.bHave=true;
            }
        }
        if(FPlatformTime::Seconds()<State->Until) return true;
        UE_LOG(LogTemp,Display,TEXT("AshenMotion: %s - %lld frames at %.0f fps; %lld moving-actor frames, %lld of them held still (%.1f%%), moved %.0f%% of what their velocity says; %lld of %lld actor-frames had an UNRESOLVED movement base (%.1f%%: the client skips simulating those) (%s)"),*State->Label,State->Frames,double(State->Frames)/FMath::Max(.001,FPlatformTime::Seconds()-State->Began),State->Moving,State->Held,
            State->Moving?100.0*double(State->Held)/double(State->Moving):0.0,State->Expected>0.0?100.0*State->Moved/State->Expected:0.0,State->Unresolved,State->Watched,State->Watched?100.0*double(State->Unresolved)/double(State->Watched):0.0,Now&&Now->GetNetMode()==NM_Client?TEXT("a client"):TEXT("the host or a single player"));
        return false;
    }),0.f);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: measuring on-screen motion for %.0f s (%s)"),FMath::Max(1.f,ArgFloat(Args,0,10.f)),*State->Label);
}
// as.dev.heroes [label]: one line per hero as THIS machine draws it ("AshenHeroes:"): its role (authority, autonomous or simulated proxy), whether it is the local player, where the mesh
// sits against the capsule (a Mannequin stands at -half height, turned -90 degrees), the foot bone's height above the capsule's bottom (about 0 when standing on the floor), and the
// mesh offset the engine's network smoothing rebuilds a proxy's mesh from (ACharacter::CacheInitialMeshOffset). A proxy drawn at the engine's cached offset instead of where the look
// code put the mesh floats on every OTHER machine (0.26 G1).
void Heroes(const TArray<FString>& Args,UWorld* W) {
    auto* S=AS::State(W);if(!S) return;const FString Label=Args.IsValidIndex(0)?Args[0]:TEXT("run");
    for(auto Weak:S->Heroes) if(const AASCharacter* H=Weak.Get()) {
        const USkeletalMeshComponent* Body=H->GetMesh();const float Half=H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),Bottom=H->GetActorLocation().Z-Half;const ENetRole Role=H->GetLocalRole();
        const bool Foot=Body&&Body->DoesSocketExist(TEXT("foot_l"));
        const FString Mesh=Body?FString::Printf(TEXT("mesh rel z %.0f yaw %.0f, mesh world z %+.0f from the actor, foot_l %+.0f above the capsule bottom"),Body->GetRelativeLocation().Z,Body->GetRelativeRotation().Yaw,Body->GetComponentLocation().Z-H->GetActorLocation().Z,Foot?Body->GetSocketLocation(TEXT("foot_l")).Z-Bottom:0.f):FString(TEXT("no mesh"));
        UE_LOG(LogTemp,Display,TEXT("AshenHeroes: %s - hero %d (%s%s, skeletal %d) at %s; %s; engine cached mesh offset z %.0f yaw %.0f, smoothing %d"),*Label,H->ColorIndex,Role==ROLE_Authority?TEXT("authority"):(Role==ROLE_AutonomousProxy?TEXT("autonomous"):TEXT("simulated")),
            H->IsLocallyControlled()?TEXT(", LOCAL"):TEXT(", other player"),H->IsSkeletalHero()?1:0,*H->GetActorLocation().ToCompactString(),*Mesh,H->GetBaseTranslationOffset().Z,H->GetBaseRotationOffset().Rotator().Yaw,
            H->GetCharacterMovement()?int32(H->GetCharacterMovement()->NetworkSmoothingMode):-1);
    }
}
// as.dev.trace <seconds> [label] [interval=.1]: for that long, a line every <interval> s for the host hero ("AshenTrace:") - its speed over the ground, its melee combo step, whether the hammer
// is committed to its third swing, the leap state, how far each foot moves against the mesh (a planted foot reads 0 while the body travels), and the montage playing. For judging how the
// body travels against what the animation shows (0.26 G3: the third hammer swing's moonwalk).
void Trace(const TArray<FString>& Args,UWorld* W) {
    struct FRun {double Began=0.0,Until=0.0,Next=0.0;float Interval=.1f;FString Label;FVector FootL=FVector::ZeroVector,FootR=FVector::ZeroVector;double LastAt=0.0;bool bHave=false;};
    auto State=MakeShared<FRun>();State->Label=Args.IsValidIndex(1)?Args[1]:TEXT("run");State->Interval=FMath::Max(.02f,ArgFloat(Args,2,.1f));State->Began=FPlatformTime::Seconds();State->Until=State->Began+FMath::Max(.5f,ArgFloat(Args,0,5.f));
    TWeakObjectPtr<UWorld> World=W;
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([State,World](float) {
        const double Now=FPlatformTime::Seconds();const AASCharacter* H=ASDev::HostHero(World.Get());
        if(H&&Now>=State->Next) {
            State->Next=Now+State->Interval;
            const USkeletalMeshComponent* Body=H->GetMesh();const UAnimInstance* Anim=Body?Body->GetAnimInstance():nullptr;const UAnimMontage* Playing=Anim?Anim->GetCurrentActiveMontage():nullptr;
            // each foot's travel in the MESH's own frame since the last line: a foot the clip keeps planted does not move there however fast the body goes; and each foot bone's height above the
            // capsule's bottom (what the footstep rule reads: ASFoley::FootStruck)
            FString Feet;
            if(Body&&Body->DoesSocketExist(TEXT("foot_l"))&&Body->DoesSocketExist(TEXT("foot_r"))) {
                const FTransform Frame=Body->GetComponentTransform();
                const FVector FootLeft=Body->GetSocketLocation(TEXT("foot_l")),FootRight=Body->GetSocketLocation(TEXT("foot_r"));
                const FVector L=Frame.InverseTransformPosition(FootLeft),R=Frame.InverseTransformPosition(FootRight);
                const double Dt=FMath::Max(.001,Now-State->LastAt);const float Bottom=H->GetActorLocation().Z-H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
                if(State->bHave) Feet=FString::Printf(TEXT("feet %.0f / %.0f cm/s in the mesh frame, %.1f / %.1f cm above the capsule bottom"),FVector::Dist(L,State->FootL)/Dt,FVector::Dist(R,State->FootR)/Dt,FootLeft.Z-Bottom,FootRight.Z-Bottom);
                State->FootL=L;State->FootR=R;State->LastAt=Now;State->bHave=true;
            }
            // 0.26 Twin Claws: the pounce (state 0 none, 1 crouch, 2 leap, 3 latched), how far the wielder stands from its target's surface against the ring gap (the latch holds it at ASClaws::RingGap),
            // and how far the capsule's bottom is above the floor under it (0 on the ground: a float or a sink shows here, whatever the animation does)
            FString Pounce;
            {
                float Gap=0.f;const AASEnemy* T=H->PounceTarget;
                if(IsValid(T)) Gap=float(FVector::Dist2D(H->GetActorLocation(),T->GetActorLocation()))-H->GetCapsuleComponent()->GetScaledCapsuleRadius()-T->GetCapsuleComponent()->GetScaledCapsuleRadius();
                FHitResult Floor;const FVector Me=H->GetActorLocation();const float Bottom=Me.Z-H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
                const bool Found=World.IsValid()&&World->LineTraceSingleByObjectType(Floor,Me,Me-FVector(0,0,600),FCollisionObjectQueryParams(ECC_WorldStatic));
                Pounce=FString::Printf(TEXT("pounce %d target %s gap %.0f cm, floor clearance %.1f cm"),int32(H->PounceState),IsValid(T)?*T->GetName():TEXT("-"),Gap,Found?Bottom-Floor.ImpactPoint.Z:-999.f);
            }
            UE_LOG(LogTemp,Display,TEXT("AshenTrace: %s +%.2f speed %.0f cm/s at %s combo step %d, hammer committed %d, leap %d, %s, %s, montage %s"),*State->Label,Now-State->Began,H->GetVelocity().Size2D(),*H->GetActorLocation().ToCompactString(),H->MeleeStep,
                H->HammerCommitted()?1:0,int32(H->HammerLeapState),*Pounce,Feet.IsEmpty()?TEXT("no feet yet"):*Feet,Playing?*Playing->GetName():TEXT("-"));
        }
        return Now<State->Until;
    }),0.f);
    UE_LOG(LogTemp,Display,TEXT("AshenDev: tracing the hero for %.0f s (%s)"),FMath::Max(.5f,ArgFloat(Args,0,5.f)),*State->Label);
}
// as.dev.wave <n>: sets the Endless wave number (the sky, scenery stage and lightning follow it at the next wave's checks).
void Wave(const TArray<FString>& Args,UWorld* W) {if(auto* S=AS::State(W)) {S->EndlessWave=FMath::Clamp(ArgInt(Args,0,1),1,999);S->ForceNetUpdate();UE_LOG(LogTemp,Display,TEXT("AshenDev: endless wave set to %d"),S->EndlessWave);}}
// what a character stands on as its movement base: the component, its mobility, whether a client can name it, and (on a client) whether the server's base could be resolved
FString BaseText(const ACharacter* C) {
    if(!C) return TEXT("-");
    const UObject* Base=C->GetBasedMovement().MovementBaseInterfaceData.GetMovementBaseObject();
    const UPrimitiveComponent* Part=Cast<UPrimitiveComponent>(Base);
    return FString::Printf(TEXT("base %s%s%s%s (replicated: server has base %d, unresolved %d)"),Base?*Base->GetName():TEXT("none"),Part?(Part->Mobility==EComponentMobility::Movable?TEXT(" MOVABLE"):TEXT(" static")):TEXT(""),
        Base&&Base->IsNameStableForNetworking()?TEXT(" net-stable"):TEXT(""),Part&&Part->GetOwner()?*FString::Printf(TEXT(" of %s"),*Part->GetOwner()->GetName()):TEXT(""),
        C->GetReplicatedBasedMovement().bServerHasBaseComponent?1:0,C->GetReplicatedBasedMovement().IsBaseUnresolved()?1:0);
}
// as.dev.state: one line with the phase, the wave, the hero and every enemy (kind, health, state, launched), for a run's log.
void State(const TArray<FString>&,UWorld* W) {
    auto* S=AS::State(W);auto* H=HostHero(W);if(!S) return;
    UE_LOG(LogTemp,Display,TEXT("AshenDev: phase %d room %d wave %d endless %d enemies %d hero %s hp %.0f"),int32(S->Phase),S->Room,S->Wave,S->bEndless?S->EndlessWave:-1,S->Enemies.Num(),H?*H->GetActorLocation().ToCompactString():TEXT("-"),H?H->Health:0.f);
    if(H) if(const auto* PS=H->GetPlayerState<AASPlayerState>()) UE_LOG(LogTemp,Display,TEXT("AshenDev:   run facts: kills %d, perfect dashes %d, impacts %d, echoes %d, metamorphosed %d"),PS->RunKills,PS->RunPerfectDashes,PS->RunImpacts,PS->RunEchoes,PS->bRunMetamorphosed?1:0);
    if(H) { // which way the hero faces: the actor, its controller, and (the Mannequin) its pelvis, whose forward is the bone's +Y... logged raw for comparison
        const USkeletalMeshComponent* Body=H->GetMesh();const bool Bones=Body&&Body->DoesSocketExist(TEXT("pelvis"));
        UE_LOG(LogTemp,Display,TEXT("AshenDev:   hero yaw %.0f control %.0f mesh %.0f pelvis %s velocity %s; %s"),H->GetActorRotation().Yaw,H->GetControlRotation().Yaw,Body?Body->GetComponentRotation().Yaw:0.f,
            Bones?*Body->GetSocketRotation(TEXT("pelvis")).ToCompactString():TEXT("-"),*H->GetVelocity().ToCompactString(),*BaseText(H));
    }
    for(auto Weak:S->Enemies) if(const AASEnemy* E=Weak.Get()) {
        UE_LOG(LogTemp,Display,TEXT("AshenDev:   enemy kind %d hp %.0f/%.0f state %d at %s launched %d; %s"),E->Kind,E->Health,E->MaxHealth,int32(E->AIState),*E->GetActorLocation().ToCompactString(),(E->Flags&ASEnemyRules::FlagLaunched)?1:0,*BaseText(E));
        if(E->IsSkeletalBoss()) if(const USkeletalMeshComponent* M=E->GetMesh()) {
            const UAnimInstance* A=M->GetAnimInstance();const UAnimMontage* Playing=A?A->GetCurrentActiveMontage():nullptr;
            const float HeadZ=M->DoesSocketExist(TEXT("Head"))?float(M->GetSocketLocation(TEXT("Head")).Z-E->GetActorLocation().Z):0.f,FootZ=M->DoesSocketExist(TEXT("LeftFoot"))?float(M->GetSocketLocation(TEXT("LeftFoot")).Z-E->GetActorLocation().Z):0.f;
            UE_LOG(LogTemp,Display,TEXT("AshenDev:     titan mesh rot %s scale %.2f head %+.0f foot %+.0f (from the actor) move mode %d montage %s actor rot %s"),*M->GetComponentRotation().ToCompactString(),M->GetComponentScale().Z,HeadZ,FootZ,
                E->GetCharacterMovement()?int32(E->GetCharacterMovement()->MovementMode.GetValue()):-1,Playing?*Playing->GetName():TEXT("-"),*E->GetActorRotation().ToCompactString());
        }
    }
}

// ---- the script player: one command per line; "wait <seconds>", "waitfor ready|players|combat|hub|reward [timeout]", "#" comments; anything else is a console command ----
TArray<FString> Lines;int32 Next=0;double ResumeAt=0.0,WaitDeadline=0.0;FString WaitingFor;FTSTicker::FDelegateHandle Handle;
bool Ready(const FString& What,UWorld* W) {
    auto* S=AS::State(W);auto* GM=Mode(W);if(!S) return false; // the phases replicate, so a client's script can wait on them too
    if(What==TEXT("ready")) return GM&&GM->AllReady()&&HostHero(W)!=nullptr;
    if(What==TEXT("players")) return GM&&S->PlayerArray.Num()>=2&&GM->AllReady()&&S->Heroes.Num()>=2; // the host waits for its guest
    if(What==TEXT("combat")) return S->Phase==EASPhase::Combat;
    if(What==TEXT("hub")) return S->Phase==EASPhase::Hub;
    if(What==TEXT("reward")) return S->Phase==EASPhase::Reward;
    return true;
}
bool Tick(float) {
    UWorld* W=GameWorld();
    if(bGod) if(auto* S=AS::State(W)) if(Mode(W)) for(auto Weak:S->Heroes) if(AASCharacter* H=Weak.Get()) {H->InvulnerableUntil=AS::Time(W)+1.f;if(H->Health<H->MaxHealth*.5f) H->Health=H->MaxHealth;} // every hero, on the host
    const double Now=FPlatformTime::Seconds();
    if(Now<WalkUntil) if(auto* H=HostHero(W)) H->AddMovementInput(FRotator(0.f,WalkYaw,0.f).Vector(),WalkScale,true);
    FollowTick(W);
    if(Now<SlayUntil&&Now>=NextSlay) {NextSlay=Now+.3;HitAll(W,0.f,1.e6f);}
    if(AttackUntil>0.0&&Now>=AttackUntil) {AttackUntil=0.0;if(auto* PC=HostController(W)) PC->DevButton(TEXT("attack"),false);}
    if(SpiralUntil>0.0&&Now>=SpiralUntil) {SpiralUntil=0.0;if(auto* PC=HostController(W)) PC->DevButton(TEXT("spiral"),false);}
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
        if(Parts[0]==TEXT("as.dev.shot")) return true; // the picture is taken when this frame draws: the next line waits for the next frame, so it cannot leak into the shot
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
AS_DEV_COMMAND("as.dev.elite",Elite,"Elite brutes in front of the hero: <mutation 0-4> [count=1] [distance=900] [side offset=0].");
AS_DEV_COMMAND("as.dev.boss",Boss,"A ward boss in front of the hero: <ward 0-3> [airborne=0] [distance=1000].");
AS_DEV_COMMAND("as.dev.wound",Wound,"Hits every enemy near the hero down to <fraction> of its health [radius=3000].");
AS_DEV_COMMAND("as.dev.killall",KillAll,"Strikes down every enemy near the hero, credited to it [radius=6000].");
AS_DEV_COMMAND("as.dev.slay",Slay,"Strikes down every enemy as it appears for [seconds=20] (clears a story room's waves).");
AS_DEV_COMMAND("as.dev.keeper",Keeper,"A grunt frozen 25 m behind the hero that keeps the wave in combat (killall spares it).");
AS_DEV_COMMAND("as.dev.freeze",Freeze,"Stops every enemy near the hero in time [1] or lets them run again [0]: [radius=2000].");
AS_DEV_COMMAND("as.dev.lineup",Lineup,"One enemy of every ground kind in a row in front of the hero: [distance=500] [spacing=260].");
AS_DEV_COMMAND("as.dev.frame",Frame,"The photo camera in front of lineup enemy <n>: [distance=420] [height=10] [fov=50].");
AS_DEV_COMMAND("as.dev.status",Status,"Applies <burn|poison|frost|bleed|shock> from the hero to every enemy near it [radius=3000].");
AS_DEV_COMMAND("as.dev.station",Station,"Puts the hero beside interaction point <id> (9 = the reward chest).");
AS_DEV_COMMAND("as.dev.knock",Knock,"Launches every enemy near the hero away from it: [strength=950] [radius=700].");
AS_DEV_COMMAND("as.dev.slamtest",SlamTest,"Launches a grunt at the nearest wall from 4.5 m: [strength=950]. Watch the log for AshenKnock: wall slam.");
AS_DEV_COMMAND("as.dev.piletest",PileTest,"Launches a grunt into another across open ground: [strength=950]. Watch the log for AshenKnock: pile-up.");
AS_DEV_COMMAND("as.dev.trait",Trait,"Sets trait <index> to <rank> on the host hero.");
AS_DEV_COMMAND("as.dev.pick",Pick,"The host hero takes trait <index> through the real reward path, [times=1] (stops at its maximum).");
AS_DEV_COMMAND("as.dev.weaponlook",WeaponLook,"Retunes the host hero's weapon material: <metallic> <roughness> (until the next look refresh).");
AS_DEV_COMMAND("as.dev.weapon",Weapon,"Gives the host hero weapon <id> (0-9; 9 = Twin Claws); [other=1: the guest's hero, from the host].");
AS_DEV_COMMAND("as.dev.walk",Walk,"Walks the host hero toward <yaw> for <seconds> (real input and collision).");
AS_DEV_COMMAND("as.dev.input",Input,"Presses the host's buttons: attack|spiral <hold seconds>, dash, jump, rune, reload.");
AS_DEV_COMMAND("as.dev.god",God,"1: the host hero cannot be hurt (and is healed back up); 0: off.");
AS_DEV_COMMAND("as.dev.hurt",HurtHero,"A blow of <amount> to the host hero from the front (lands even in god mode); [other=1: to the other player's hero, from the host].");
AS_DEV_COMMAND("as.dev.place",Place,"Moves the host hero to <x> <y> [yaw].");
AS_DEV_COMMAND("as.dev.cam",Cam,"A fixed photo camera: <x> <y> <z> <pitch> <yaw> [fov=80], or off.");
AS_DEV_COMMAND("as.dev.face",Face,"Turns the camera to <yaw> [pitch].");
AS_DEV_COMMAND("as.dev.aimat",AimAt,"Turns the camera so the crosshair's ray runs through the [nth=0] nearest living enemy (what a pounce picks).");
AS_DEV_COMMAND("as.dev.follow",Follow,"A photo camera riding with the hero: <distance> <height> <angle from its facing> [fov=55] [look-ahead cm=0] [other=1: the other player's hero], or off.");
AS_DEV_COMMAND("as.dev.heroes",Heroes,"Logs per hero how THIS machine draws it: role, mesh offset against the capsule, the foot bone's height above the capsule bottom, the engine's cached mesh offset ('AshenHeroes:' lines); [label].");
AS_DEV_COMMAND("as.dev.clearing",Clearing,"Moves the hero to open ground near the arena's middle: [radius=600] clear all round.");
AS_DEV_COMMAND("as.dev.shot",Shot,"Screenshot to Saved/Screenshots/Dev/<name>.png [ui=1].");
AS_DEV_COMMAND("as.dev.lightning",Lightning,"Calls [count=1] sky strikes around the host hero [distance=520].");
AS_DEV_COMMAND("as.dev.decortest",DecorTest,"Walks the host hero at the nearest standing ruin from 5 m.");
AS_DEV_COMMAND("as.dev.decorcheck",DecorCheck,"Logs whether the hero stopped at the ruin's surface.");
AS_DEV_COMMAND("as.dev.lookat",LookAt,"Photo camera on the nearest ruin piece whose mesh name contains <text> [distance=1400] [height=300].");
AS_DEV_COMMAND("as.dev.record",Record,"Records the game's audio: start [seconds=30] | stop <name> (Saved/AudioRecordings/Dev/<name>.wav).");
AS_DEV_COMMAND("as.dev.perf",Perf,"Logs the frame time over [seconds=10]: avg, p95, p99, worst, fps and the game, render and GPU thread times; [label].");
AS_DEV_COMMAND("as.dev.motion",Motion,"Counts frames in which moving enemies and other heroes hold still on screen over [seconds=10] ('AshenMotion:' line): a client skipping proxy simulation shows many; [label].");
AS_DEV_COMMAND("as.dev.trace",Trace,"Logs the host hero every [interval=.1] s for <seconds>: ground speed, combo step, hammer committed, leap, foot travel in the mesh frame, montage ('AshenTrace:' lines); [label].");
AS_DEV_COMMAND("as.dev.wave",Wave,"Sets the Endless wave number <n>.");
AS_DEV_COMMAND("as.dev.state",State,"Logs the phase, wave, hero and every enemy.");
AS_DEV_COMMAND("as.dev.script",Script,"Plays a script file: one console command per line, plus wait <s> and waitfor ready|combat|hub [timeout].");
#undef AS_DEV_COMMAND
} // namespace ASDev
#endif
