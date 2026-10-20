#include "ASEnemyHazards.h"
#include "ASEnemy.h"
#include "ASData.h"
#include "ASEnemyAbilities.h"
#include "ASCharacter.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Pawn.h"

// ---- Shockwave (the brute's ground pound) ----
AASShockwave::AASShockwave() {
    bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;SetReplicateMovement(true);
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Ring=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));Ring->SetupAttachment(RootComponent);
}
AASShockwave* AASShockwave::Spawn(UWorld* World,const FVector& Feet,AASEnemy* Source,float Damage,float MaxRadius,float Delay) {
    if(!World||World->GetNetMode()==NM_Client) return nullptr;
    auto* S=AS::State(World);if(!S) return nullptr;
    const FTransform T(FRotator::ZeroRotator,Feet);
    auto* Wave=World->SpawnActorDeferred<AASShockwave>(StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!Wave) return nullptr;
    Wave->Born=S->SimTime+FMath::Max(0.f,Delay);Wave->Damage=Damage;Wave->MaxRadius=MaxRadius;Wave->EnemySource=Source;
    UGameplayStatics::FinishSpawningActor(Wave,T);return Wave;
}
void AASShockwave::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASShockwave,Born);DOREPLIFETIME(AASShockwave,MaxRadius);DOREPLIFETIME(AASShockwave,Damage);
}
void AASShockwave::BeginPlay() {
    Super::BeginPlay();
    // 0.24.2 (F6): the wave is a short glowing wall (about 40 cm, just under the 45 cm a jump clears) in the ripples' hot yellow, not a flat line on the floor, so it reads as
    // "jump this". SM_Ring has an outer radius of 50 cm and a height of 3 cm: the scale is radius / 50 across and WaveWall up.
    ASAssets::Style(Ring,TEXT("SM_Ring"),FLinearColor(1.f,.62f,.16f),0,10.f);
    Ring->SetCastShadow(false);Ring->SetRelativeLocation(FVector(0,0,2));
    Ring->SetRelativeScale3D(FVector(ASEnemyRules::PoundStartRadius/50.f,ASEnemyRules::PoundStartRadius/50.f,ASEnemyRules::WaveWallScale));
}
void AASShockwave::Tick(float Delta) {
    Super::Tick(Delta);
    namespace ER=ASEnemyRules;
    auto* S=AS::State(this);if(!S||S->Frozen()) return;
    const float Age=S->SimTime-Born;
    if(Age<0.f) {if(Ring) Ring->SetVisibility(false);return;} // a ring that has not started yet (the second of a toll): nothing to draw, nobody to hurt
    const float Radius=ER::ShockwaveRadius(Age,ER::PoundStartRadius,ER::PoundSpeed,MaxRadius);
    const float Life=(MaxRadius-ER::PoundStartRadius)/ER::PoundSpeed+.25f; // the ring lingers for a quarter of a second at its full size, fading
    if(GetNetMode()!=NM_DedicatedServer&&Ring) {
        const float Fade=FMath::Clamp((Life-Age)/.25f,0.f,1.f);
        Ring->SetRelativeScale3D(FVector(Radius/50.f,Radius/50.f,ASEnemyRules::WaveWallScale*FMath::Max(.15f,Fade)));
        if(auto* M=Cast<UMaterialInstanceDynamic>(Ring->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),10.f*Fade);
        Ring->SetVisibility(Fade>.02f);
    }
    if(!HasAuthority()) return;
    if(Age>=Life||S->Phase!=EASPhase::Combat) {Destroy();return;}
    // Sweep the heroes against the band the front covered this tick (the first tick sweeps from the centre, so a hero standing right under the enemy is caught too).
    const float Prev=bSwept?PrevRadius:0.f;const FVector Me=GetActorLocation();
    for(auto W:S->Heroes) if(auto* Hero=W.Get()) {
        if(Hero->bDowned||Struck.Contains(Hero)) continue;
        const FVector At=Hero->GetActorLocation();
        const auto* Move=Hero->GetCharacterMovement();const bool Grounded=!Move||!Move->IsFalling();
        const float Height=At.Z-Hero->GetSimpleCollisionHalfHeight()-Me.Z; // the hero's feet above the floor the wave started on
        if(ER::ShockwaveHits(Prev,Radius,FVector::Dist2D(At,Me),Grounded,Height,ER::PoundBand,ER::PoundJumpClearance)) {
            Struck.Add(Hero);Hero->Hurt(Damage,EnemySource.Get(),true); // a hero in a dash's invulnerability is "struck" and unhurt: it went through the wave
        }
    }
    PrevRadius=Radius;bSwept=true;
}

// ---- Lingering hazard ----
int32 AASHazard::Alive=0;
AASHazard::AASHazard() {
    bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;SetReplicateMovement(true);
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Disc=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));Disc->SetupAttachment(RootComponent);
    Fill=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fill"));Fill->SetupAttachment(RootComponent);
    Bolt=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bolt"));Bolt->SetupAttachment(RootComponent);
}
AASHazard* AASHazard::Spawn(UWorld* World,const FVector& Feet,int32 Style,AASEnemy* Source,float WarnSeconds,float ImpactDamage) {
    if(!World||World->GetNetMode()==NM_Client||Alive>=24||Feet.ContainsNaN()) return nullptr; // 24 zones at once is already a lot of floor to read
    auto* S=AS::State(World);if(!S) return nullptr;
    const FTransform T(FRotator::ZeroRotator,Feet);
    auto* Zone=World->SpawnActorDeferred<AASHazard>(StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!Zone) return nullptr;
    Zone->Style=uint8(FMath::Clamp(Style,0,ASEnemyRules::HazardStyles-1));Zone->Born=S->SimTime;Zone->ArmAt=S->SimTime+FMath::Max(0.f,WarnSeconds);
    Zone->ImpactDamage=ImpactDamage;Zone->EnemySource=Source;
    UGameplayStatics::FinishSpawningActor(Zone,T);return Zone;
}
void AASHazard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASHazard,Born);DOREPLIFETIME(AASHazard,ArmAt);DOREPLIFETIME(AASHazard,Style);
}
void AASHazard::BeginPlay() {
    Super::BeginPlay();
    if(!bCounted) {bCounted=true;++Alive;}
    if(GetNetMode()==NM_DedicatedServer) return;
    namespace ER=ASEnemyRules;
    const ER::HazardSpec& Spec=ER::HazardFor(Style);const bool Storm=Style==ER::HazardStorm;
    const float Scale=Spec.Radius/50.f; // the basic cylinder is 100 cm across: flattened it is a disc of exactly this radius
    ASAssets::Style(Disc,TEXT("Cylinder"),Storm?FLinearColor(.30f,.16f,.55f):FLinearColor(.55f,.05f,.02f),0,1.5f);Disc->SetCastShadow(false);Disc->SetRelativeLocation(FVector(0,0,3));Disc->SetRelativeScale3D(FVector(Scale,Scale,.02f));
    ASAssets::Style(Fill,TEXT("Cylinder"),Storm?FLinearColor(.90f,.84f,1.f):FLinearColor(1.f,.16f,.03f),0,6.f);Fill->SetCastShadow(false);Fill->SetRelativeLocation(FVector(0,0,4.5f));Fill->SetRelativeScale3D(FVector(.02f,.02f,.03f));
    ASAssets::Style(Bolt,TEXT("Cylinder"),FLinearColor(.9f,.84f,1.f),0,12.f);Bolt->SetCastShadow(false);Bolt->SetVisibility(Storm);Bolt->SetRelativeScale3D(FVector(.1f,.1f,.5f));
}
void AASHazard::EndPlay(const EEndPlayReason::Type Reason) {
    if(bCounted) {bCounted=false;Alive=FMath::Max(0,Alive-1);}
    Super::EndPlay(Reason);
}
void AASHazard::Tick(float Delta) {
    Super::Tick(Delta);
    namespace ER=ASEnemyRules;
    auto* S=AS::State(this);if(!S||S->Frozen()) return;
    const ER::HazardSpec& Spec=ER::HazardFor(Style);const float Now=S->SimTime;const bool Armed=Now>=ArmAt;
    const FVector Me=GetActorLocation();
    if(GetNetMode()!=NM_DedicatedServer&&Disc&&Fill&&Bolt) {
        const float Full=Spec.Radius/50.f;const bool Storm=Style==ER::HazardStorm;
        const auto Glow=[](UStaticMeshComponent* C,float Emission) {if(C) if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),Emission);};
        if(!Armed) { // the warning: the bright disc grows over the dim one; a storm also grows a sky column toward the ground
            const float Span=ArmAt-Born,Progress=Span>.01f?FMath::Clamp((Now-Born)/Span,0.f,1.f):1.f,Grow=FMath::Max(.02f,Progress)*Full;
            Fill->SetRelativeScale3D(FVector(Grow,Grow,.03f));Glow(Fill,3.f+6.f*Progress);
            if(Storm) {const float Height=FMath::Lerp(.5f,13.f,Progress*Progress);Bolt->SetRelativeScale3D(FVector(.1f,.1f,Height));Bolt->SetRelativeLocation(FVector(0,0,Height*50.f));Glow(Bolt,3.f+7.f*Progress);}
        } else {
            if(!bDrawnArmed) { // armed: the disc takes its own colour, the warning fill goes
                bDrawnArmed=true;Fill->SetVisibility(false);
                ASAssets::Style(Disc,TEXT("Cylinder"),FLinearColor(Spec.R,Spec.G,Spec.B)*.75f,0,2.5f);Disc->SetCastShadow(false);Disc->SetRelativeLocation(FVector(0,0,3));
            }
            const float Age=Now-ArmAt,Scale=Full*ER::HazardScale(Age,Spec.Seconds);
            Disc->SetRelativeScale3D(FVector(Scale,Scale,.02f));Glow(Disc,2.5f+1.2f*FMath::Sin(Now*6.f+float(GetUniqueID()%5)));
            if(Storm) {const float Fade=FMath::Clamp(1.f-Age/.35f,0.f,1.f);Bolt->SetVisibility(Fade>0.f);Bolt->SetRelativeScale3D(FVector(.7f*Fade+.04f,.7f*Fade+.04f,14.f));Bolt->SetRelativeLocation(FVector(0,0,700.f));Glow(Bolt,14.f*Fade);}
            if(!bHeardArm&&Age<.5f) { // where it bites, a sound at the distance-scaled volume (a late joiner does not replay it)
                bHeardArm=true;float Volume=.3f,Pitch=1.f;USoundBase* Cue=AASEnemy::ResolveCue(ER::HazardArmCue(Style),Volume,Pitch);
                const APawn* Local=UGameplayStatics::GetPlayerPawn(this,0);Volume*=ER::CueDistanceGain(Local?float(FVector::Dist(Local->GetActorLocation(),Me)):0.f);
                if(Cue&&Volume>=.02f) UGameplayStatics::PlaySoundAtLocation(this,Cue,Me,Volume,Pitch);
            }
            if(Age>=0.f) bHeardArm=true;
        }
    }
    if(!HasAuthority()) return;
    if(S->Phase!=EASPhase::Combat||Now>=ArmAt+Spec.Seconds) {Destroy();return;}
    if(!Armed) return;
    auto Inside=[&](const AASCharacter* Hero,float Radius) {const FVector At=Hero->GetActorLocation();return !Hero->bDowned&&ER::HazardContains(FVector::Dist2D(At,Me),At.Z-Me.Z,Radius);};
    if(!bArmed) { // the moment it arms: an impact for everyone inside (when it has one) and a flash in its own colour
        bArmed=true;NextTick=Now+Spec.Interval;
        if(ImpactDamage>0.f) {
            for(auto W:S->Heroes) if(auto* Hero=W.Get()) if(Inside(Hero,Spec.Radius)) Hero->Hurt(ImpactDamage,EnemySource.Get(),true);
            S->Effect(6,Me+FVector(0,0,10),FVector::UpVector,FLinearColor(Spec.R,Spec.G,Spec.B),Spec.Radius);
        }
    }
    if(Now>=NextTick) {
        NextTick=Now+Spec.Interval;const float Radius=Spec.Radius*ER::HazardScale(Now-ArmAt,Spec.Seconds);
        for(auto W:S->Heroes) if(auto* Hero=W.Get()) if(Inside(Hero,Radius)) Hero->Hurt(Spec.Damage,EnemySource.Get(),true);
    }
}
