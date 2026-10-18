#include "ASLightning.h"
#include "ASGameState.h"
#include "ASCharacter.h"
#include "ASEnemy.h"
#include "ASWorld.h"
#include "ASExpeditionRules.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
AASLightningStrike::AASLightningStrike() {
    PrimaryActorTick.bCanEverTick=true;bReplicates=true;bAlwaysRelevant=true;SetReplicateMovement(false);SetNetUpdateFrequency(5.f);
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Outline=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Outline"));Outline->SetupAttachment(RootComponent);
    Fill=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fill"));Fill->SetupAttachment(RootComponent);
    Bolt=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bolt"));Bolt->SetupAttachment(RootComponent);
}
void AASLightningStrike::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASLightningStrike,StrikeAt);DOREPLIFETIME(AASLightningStrike,Radius);
}
void AASLightningStrike::BeginPlay() {
    Super::BeginPlay();
    if(GetNetMode()==NM_DedicatedServer) return;
    // The ring mesh is 53 cm in radius, so scale = radius / 53 makes the outline the exact damage radius.
    const float Scale=FMath::Max(1.f,Radius)/53.f;
    ASAssets::Style(Outline,TEXT("SM_Ring"),FLinearColor(1.f,.12f,.08f),0,4.f);Outline->SetCastShadow(false);Outline->SetRelativeLocation(FVector(0,0,6));Outline->SetRelativeScale3D(FVector(Scale,Scale,1.f));
    ASAssets::Style(Fill,TEXT("SM_Ring"),FLinearColor(1.f,.85f,.4f),0,6.f);Fill->SetCastShadow(false);Fill->SetRelativeLocation(FVector(0,0,8));Fill->SetRelativeScale3D(FVector(.02f,.02f,1.f));
    ASAssets::Style(Bolt,TEXT("Cube"),FLinearColor(.8f,.9f,1.f),0,12.f);Bolt->SetCastShadow(false);Bolt->SetVisibility(false);Bolt->SetRelativeLocation(FVector(0,0,600));Bolt->SetRelativeScale3D(FVector(.3f,.3f,12.f));
    // A faint crackle when the warning appears (placeholder: existing sound, pitched up).
    if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Hit.S_Hit"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.16f,2.4f);
}
void AASLightningStrike::Resolve() {
    bResolved=true;
    auto* S=AS::State(this);if(!S) return;
    const FVector At=GetActorLocation();const float R=Radius;
    for(auto W:S->Heroes) if(auto* H=W.Get()) if(!H->bDowned&&FVector::Dist2D(H->GetActorLocation(),At)<=R) H->Hurt(ASExpedition::LightningPlayerDamage(H->MaxHealth),nullptr,false);
    const auto Enemies=S->Enemies;
    for(auto W:Enemies) if(auto* E=W.Get()) if(E->Health>0&&!E->bTrainingDummy&&FVector::Dist2D(E->GetActorLocation(),At)<=R+E->GetCapsuleComponent()->GetScaledCapsuleRadius()) E->Hit(ASExpedition::LightningEnemyDamage(E->MaxHealth,E->Boss()),nullptr);
    S->Effect(8,At+FVector(0,0,20),FVector::UpVector,FLinearColor(.8f,.9f,1.f),R*1.4f);
}
void AASLightningStrike::Tick(float Delta) {
    Super::Tick(Delta);
    auto* S=AS::State(this);if(!S||S->Frozen()) return;
    const float Now=S->SimTime;
    if(HasAuthority()) {
        if(!bResolved&&Now>=StrikeAt) Resolve();
        if(Now>=StrikeAt+.6f) {Destroy();return;}
    }
    if(GetNetMode()==NM_DedicatedServer) return;
    const float Telegraph=ASExpedition::LightningTelegraph,Scale=FMath::Max(1.f,Radius)/53.f;
    if(Now<StrikeAt) {
        // Warning: the inner ring fills toward the outline; the outline pulses faster as the strike nears.
        const float Fraction=FMath::Clamp(1.f-(StrikeAt-Now)/Telegraph,0.f,1.f);
        const float Grow=FMath::Max(.02f,Fraction)*Scale;Fill->SetRelativeScale3D(FVector(Grow,Grow,1.f));
        if(auto* M=Cast<UMaterialInstanceDynamic>(Outline->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),4.f+4.f*FMath::Sin(Now*(14.f+24.f*Fraction)));
        return;
    }
    if(!bFlashed) {
        bFlashed=true;Bolt->SetVisibility(true);Fill->SetRelativeScale3D(FVector(Scale,Scale,1.f));
        // Placeholder thunder: existing sounds, pitched.
        if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_HammerSmash.S_HammerSmash"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.8f,.75f);
        if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Collapse.S_Collapse"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.45f,1.4f);
    }
    FlashAge+=Delta;const float Fade=FMath::Clamp(1.f-FlashAge/.35f,0.f,1.f);
    Bolt->SetRelativeScale3D(FVector(.3f*Fade+.02f,.3f*Fade+.02f,12.f));
    if(auto* M=Cast<UMaterialInstanceDynamic>(Bolt->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),12.f*Fade);
    if(auto* M=Cast<UMaterialInstanceDynamic>(Fill->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),6.f*Fade);
    if(Fade<=0.f) {Bolt->SetVisibility(false);Fill->SetVisibility(false);Outline->SetVisibility(false);}
}
