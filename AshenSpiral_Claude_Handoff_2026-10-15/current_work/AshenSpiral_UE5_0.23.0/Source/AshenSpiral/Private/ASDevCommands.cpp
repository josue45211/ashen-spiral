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
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Containers/Ticker.h"
#include "UnrealClient.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"

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
float WalkYaw=0.f;double WalkUntil=0.0;double AttackUntil=0.0,SpiralUntil=0.0; // as.dev.input: buttons held until then
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
void Weapon(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);if(!H) return;FASBuild B=H->Build;B.Weapon=FMath::Clamp(ArgInt(Args,0,0),0,ASRules::WeaponCount-1);H->ApplyBuild(B,false);H->RefreshLook();
    UE_LOG(LogTemp,Display,TEXT("AshenDev: weapon %d"),B.Weapon);
}
// as.dev.walk <yaw> <seconds>: the host hero walks that way (input, through its own movement and collision); the script keeps ticking meanwhile.
void Walk(const TArray<FString>& Args,UWorld* W) {
    WalkYaw=ArgFloat(Args,0,0.f);WalkUntil=FPlatformTime::Seconds()+ArgFloat(Args,1,1.f);
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
// as.dev.hurt <amount>: a blow to the host hero from the front through its own damage path (armour, shields, the flinch, going down); it lands even in god mode.
void HurtHero(const TArray<FString>& Args,UWorld* W) {
    auto* H=HostHero(W);if(!H) return;const float Before=H->Health;
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
// as.dev.follow <distance> <height> <angle> [fov] | off: the photo camera rides with the hero, <angle> degrees round from its facing (0 in front, 90 its right side, 180 behind),
// <height> cm above its middle, looking at its chest; for photographing the hero's own animation wherever it goes.
bool bFollow=false;float FollowDistance=360.f,FollowHeight=30.f,FollowAngle=90.f,FollowAhead=0.f;
void FollowTick(UWorld* W) {
    auto* H=HostHero(W);if(!bFollow||!H||!PhotoCamera.IsValid()) return;
    const FVector Chest=H->GetActorLocation()+FVector(0.f,0.f,25.f),Target=Chest+FRotator(0.f,H->GetActorRotation().Yaw,0.f).Vector()*FollowAhead; // looked at: the chest, or a point ahead of the hero
    const FVector From=Chest+FRotator(0.f,H->GetActorRotation().Yaw+FollowAngle,0.f).Vector()*FollowDistance+FVector(0.f,0.f,FollowHeight);
    PhotoCamera->SetActorLocationAndRotation(From,(Target-From).Rotation());
}
void Follow(const TArray<FString>& Args,UWorld* W) {
    if(Args.IsValidIndex(0)&&Args[0]==TEXT("off")) {bFollow=false;Cam({TEXT("off")},W);return;}
    FollowDistance=ArgFloat(Args,0,360.f);FollowHeight=ArgFloat(Args,1,30.f);FollowAngle=ArgFloat(Args,2,90.f);FollowAhead=ArgFloat(Args,4,0.f);
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
// as.dev.wave <n>: sets the Endless wave number (the sky, scenery stage and lightning follow it at the next wave's checks).
void Wave(const TArray<FString>& Args,UWorld* W) {if(auto* S=AS::State(W)) {S->EndlessWave=FMath::Clamp(ArgInt(Args,0,1),1,999);S->ForceNetUpdate();UE_LOG(LogTemp,Display,TEXT("AshenDev: endless wave set to %d"),S->EndlessWave);}}
// as.dev.state: one line with the phase, the wave, the hero and every enemy (kind, health, state, launched), for a run's log.
void State(const TArray<FString>&,UWorld* W) {
    auto* S=AS::State(W);auto* H=HostHero(W);if(!S) return;
    UE_LOG(LogTemp,Display,TEXT("AshenDev: phase %d room %d wave %d endless %d enemies %d hero %s hp %.0f"),int32(S->Phase),S->Room,S->Wave,S->bEndless?S->EndlessWave:-1,S->Enemies.Num(),H?*H->GetActorLocation().ToCompactString():TEXT("-"),H?H->Health:0.f);
    if(H) if(const auto* PS=H->GetPlayerState<AASPlayerState>()) UE_LOG(LogTemp,Display,TEXT("AshenDev:   run facts: kills %d, perfect dashes %d, impacts %d, echoes %d, metamorphosed %d"),PS->RunKills,PS->RunPerfectDashes,PS->RunImpacts,PS->RunEchoes,PS->bRunMetamorphosed?1:0);
    if(H) { // which way the hero faces: the actor, its controller, and (the Mannequin) its pelvis, whose forward is the bone's +Y... logged raw for comparison
        const USkeletalMeshComponent* Body=H->GetMesh();const bool Bones=Body&&Body->DoesSocketExist(TEXT("pelvis"));
        UE_LOG(LogTemp,Display,TEXT("AshenDev:   hero yaw %.0f control %.0f mesh %.0f pelvis %s velocity %s"),H->GetActorRotation().Yaw,H->GetControlRotation().Yaw,Body?Body->GetComponentRotation().Yaw:0.f,
            Bones?*Body->GetSocketRotation(TEXT("pelvis")).ToCompactString():TEXT("-"),*H->GetVelocity().ToCompactString());
    }
    for(auto Weak:S->Enemies) if(const AASEnemy* E=Weak.Get()) UE_LOG(LogTemp,Display,TEXT("AshenDev:   enemy kind %d hp %.0f/%.0f state %d at %s launched %d"),E->Kind,E->Health,E->MaxHealth,int32(E->AIState),*E->GetActorLocation().ToCompactString(),(E->Flags&ASEnemyRules::FlagLaunched)?1:0);
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
    if(Now<WalkUntil) if(auto* H=HostHero(W)) H->AddMovementInput(FRotator(0.f,WalkYaw,0.f).Vector(),1.f,true);
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
AS_DEV_COMMAND("as.dev.status",Status,"Applies <burn|poison|frost|bleed|shock> from the hero to every enemy near it [radius=3000].");
AS_DEV_COMMAND("as.dev.station",Station,"Puts the hero beside interaction point <id> (9 = the reward chest).");
AS_DEV_COMMAND("as.dev.knock",Knock,"Launches every enemy near the hero away from it: [strength=950] [radius=700].");
AS_DEV_COMMAND("as.dev.slamtest",SlamTest,"Launches a grunt at the nearest wall from 4.5 m: [strength=950]. Watch the log for AshenKnock: wall slam.");
AS_DEV_COMMAND("as.dev.piletest",PileTest,"Launches a grunt into another across open ground: [strength=950]. Watch the log for AshenKnock: pile-up.");
AS_DEV_COMMAND("as.dev.trait",Trait,"Sets trait <index> to <rank> on the host hero.");
AS_DEV_COMMAND("as.dev.pick",Pick,"The host hero takes trait <index> through the real reward path, [times=1] (stops at its maximum).");
AS_DEV_COMMAND("as.dev.weapon",Weapon,"Gives the host hero weapon <id> (0-8).");
AS_DEV_COMMAND("as.dev.walk",Walk,"Walks the host hero toward <yaw> for <seconds> (real input and collision).");
AS_DEV_COMMAND("as.dev.input",Input,"Presses the host's buttons: attack|spiral <hold seconds>, dash, jump, rune, reload.");
AS_DEV_COMMAND("as.dev.god",God,"1: the host hero cannot be hurt (and is healed back up); 0: off.");
AS_DEV_COMMAND("as.dev.hurt",HurtHero,"A blow of <amount> to the host hero from the front (lands even in god mode).");
AS_DEV_COMMAND("as.dev.place",Place,"Moves the host hero to <x> <y> [yaw].");
AS_DEV_COMMAND("as.dev.cam",Cam,"A fixed photo camera: <x> <y> <z> <pitch> <yaw> [fov=80], or off.");
AS_DEV_COMMAND("as.dev.face",Face,"Turns the camera to <yaw> [pitch].");
AS_DEV_COMMAND("as.dev.follow",Follow,"A photo camera riding with the hero: <distance> <height> <angle from its facing> [fov=55] [look-ahead cm=0], or off.");
AS_DEV_COMMAND("as.dev.clearing",Clearing,"Moves the hero to open ground near the arena's middle: [radius=600] clear all round.");
AS_DEV_COMMAND("as.dev.shot",Shot,"Screenshot to Saved/Screenshots/Dev/<name>.png [ui=1].");
AS_DEV_COMMAND("as.dev.lightning",Lightning,"Calls [count=1] sky strikes around the host hero [distance=520].");
AS_DEV_COMMAND("as.dev.decortest",DecorTest,"Walks the host hero at the nearest standing ruin from 5 m.");
AS_DEV_COMMAND("as.dev.decorcheck",DecorCheck,"Logs whether the hero stopped at the ruin's surface.");
AS_DEV_COMMAND("as.dev.wave",Wave,"Sets the Endless wave number <n>.");
AS_DEV_COMMAND("as.dev.state",State,"Logs the phase, wave, hero and every enemy.");
AS_DEV_COMMAND("as.dev.script",Script,"Plays a script file: one console command per line, plus wait <s> and waitfor ready|combat|hub [timeout].");
#undef AS_DEV_COMMAND
} // namespace ASDev
#endif
