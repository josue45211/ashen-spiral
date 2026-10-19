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

// ---- Shockwave (the brute's ground pound) ----
AASShockwave::AASShockwave() {
    bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;SetReplicateMovement(true);
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Ring=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));Ring->SetupAttachment(RootComponent);
}
AASShockwave* AASShockwave::Spawn(UWorld* World,const FVector& Feet,AASEnemy* Source,float Damage,float MaxRadius) {
    if(!World||World->GetNetMode()==NM_Client) return nullptr;
    auto* S=AS::State(World);if(!S) return nullptr;
    const FTransform T(FRotator::ZeroRotator,Feet);
    auto* Wave=World->SpawnActorDeferred<AASShockwave>(StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!Wave) return nullptr;
    Wave->Born=S->SimTime;Wave->Damage=Damage;Wave->MaxRadius=MaxRadius;Wave->EnemySource=Source;
    UGameplayStatics::FinishSpawningActor(Wave,T);return Wave;
}
void AASShockwave::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASShockwave,Born);DOREPLIFETIME(AASShockwave,MaxRadius);DOREPLIFETIME(AASShockwave,Damage);
}
void AASShockwave::BeginPlay() {
    Super::BeginPlay();
    ASAssets::Style(Ring,TEXT("SM_Ring"),FLinearColor(1.f,.22f,.03f),0,8.f); // SM_Ring has an outer radius of 50 cm: the scale is radius / 50
    Ring->SetCastShadow(false);Ring->SetRelativeLocation(FVector(0,0,4));
    Ring->SetRelativeScale3D(FVector(ASEnemyRules::PoundStartRadius/50.f,ASEnemyRules::PoundStartRadius/50.f,1.f));
}
void AASShockwave::Tick(float Delta) {
    Super::Tick(Delta);
    namespace ER=ASEnemyRules;
    auto* S=AS::State(this);if(!S||S->Frozen()) return;
    const float Age=S->SimTime-Born;
    const float Radius=ER::ShockwaveRadius(Age,ER::PoundStartRadius,ER::PoundSpeed,MaxRadius);
    const float Life=(MaxRadius-ER::PoundStartRadius)/ER::PoundSpeed+.25f; // the ring lingers for a quarter of a second at its full size, fading
    if(GetNetMode()!=NM_DedicatedServer&&Ring) {
        const float Fade=FMath::Clamp((Life-Age)/.25f,0.f,1.f);
        Ring->SetRelativeScale3D(FVector(Radius/50.f,Radius/50.f,1.f));
        if(auto* M=Cast<UMaterialInstanceDynamic>(Ring->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),8.f*Fade);
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
