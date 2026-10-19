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
    Disc=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));Disc->SetupAttachment(RootComponent);
    Outline=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Outline"));Outline->SetupAttachment(RootComponent);
    Fill=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fill"));Fill->SetupAttachment(RootComponent);
    SpokePivot=CreateDefaultSubobject<USceneComponent>(TEXT("SpokePivot"));SpokePivot->SetupAttachment(RootComponent);
    for(int32 I=0;I<4;++I) {auto* Spoke=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Spoke%d"),I));Spoke->SetupAttachment(SpokePivot);Spokes.Add(Spoke);}
    Bolt=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bolt"));Bolt->SetupAttachment(RootComponent);
}
void AASLightningStrike::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASLightningStrike,StrikeAt);DOREPLIFETIME(AASLightningStrike,Radius);
}
void AASLightningStrike::BeginPlay() {
    Super::BeginPlay();
    if(GetNetMode()==NM_DedicatedServer) return;
    // Palette: violet and pale white only. Enemy telegraphs are red/orange rings and frost is blue, so a strike can never be mistaken for either.
    const FLinearColor Violet(.58f,.32f,1.f),Pale(.9f,.84f,1.f),Deep(.16f,.06f,.34f);
    const float R=FMath::Max(1.f,Radius);
    const float RingScale=R/53.f; // the ring mesh is 53 cm in radius, so scale = radius / 53 makes the outline the exact damage radius
    ASAssets::Style(Disc,TEXT("Cylinder"),Deep,0,1.2f);Disc->SetCastShadow(false);Disc->SetRelativeLocation(FVector(0,0,3));Disc->SetRelativeScale3D(FVector(R/50.f,R/50.f,.02f)); // 100 cm cylinder: R/50 is the exact radius
    ASAssets::Style(Outline,TEXT("SM_Ring"),Violet,0,5.f);Outline->SetCastShadow(false);Outline->SetRelativeLocation(FVector(0,0,6));Outline->SetRelativeScale3D(FVector(RingScale,RingScale,1.f));
    ASAssets::Style(Fill,TEXT("SM_Ring"),Pale,0,6.f);Fill->SetCastShadow(false);Fill->SetRelativeLocation(FVector(0,0,8));Fill->SetRelativeScale3D(FVector(.02f,.02f,1.f));
    SpokePivot->SetRelativeLocation(FVector(0,0,10));
    for(int32 I=0;I<Spokes.Num();++I) {
        auto* Spoke=Spokes[I].Get();if(!Spoke) continue;
        ASAssets::Style(Spoke,TEXT("Cube"),Violet,0,4.f);Spoke->SetCastShadow(false);
        Spoke->SetRelativeRotation(FRotator(0.f,45.f*float(I),0.f));Spoke->SetRelativeScale3D(FVector(R*1.9f/100.f,.035f,.025f)); // four diameters = eight spokes
    }
    ASAssets::Style(Bolt,TEXT("Cylinder"),Pale,0,12.f);Bolt->SetCastShadow(false);Bolt->SetRelativeLocation(FVector(0,0,25));Bolt->SetRelativeScale3D(FVector(.12f,.12f,.5f));
    // A faint crackle when the warning appears (placeholder: existing sound, pitched up); two more, higher, follow as the strike nears.
    if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Hit.S_Hit"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.16f,1.6f);
}
void AASLightningStrike::Resolve() {
    bResolved=true;
    auto* S=AS::State(this);if(!S) return;
    const FVector At=GetActorLocation();const float R=Radius;
    for(auto W:S->Heroes) if(auto* H=W.Get()) if(!H->bDowned&&FVector::Dist2D(H->GetActorLocation(),At)<=R) H->Hurt(ASExpedition::LightningPlayerDamage(H->MaxHealth),nullptr,false);
    const auto Enemies=S->Enemies;
    for(auto W:Enemies) if(auto* E=W.Get()) if(E->Health>0&&!E->bTrainingDummy&&FVector::Dist2D(E->GetActorLocation(),At)<=R+E->GetCapsuleComponent()->GetScaledCapsuleRadius()) E->Hit(ASExpedition::LightningEnemyDamage(E->MaxHealth,E->Boss()),nullptr);
    S->Effect(8,At+FVector(0,0,20),FVector::UpVector,FLinearColor(.8f,.7f,1.f),R*1.4f);
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
    const auto Glow=[](UStaticMeshComponent* C,float Emission) {if(C) if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),Emission);};
    if(Now<StrikeAt) {
        // Warning: the disc brightens, the spokes spin faster, the pale ring fills toward the outline, and the sky column grows toward the ground.
        const float Fraction=FMath::Clamp(1.f-(StrikeAt-Now)/Telegraph,0.f,1.f);
        const float Grow=FMath::Max(.02f,Fraction)*Scale;Fill->SetRelativeScale3D(FVector(Grow,Grow,1.f));
        Glow(Outline,4.f+4.f*FMath::Sin(Now*(14.f+24.f*Fraction)));
        Glow(Disc,1.2f+2.6f*Fraction);
        SpokePivot->SetRelativeRotation(FRotator(0.f,Now*(40.f+420.f*Fraction),0.f));
        for(auto& Spoke:Spokes) Glow(Spoke.Get(),3.f+5.f*Fraction);
        const float Height=FMath::Lerp(.5f,13.f,Fraction*Fraction); // cylinder is 100 cm tall: scale 13 is a 1300 cm column
        Bolt->SetRelativeScale3D(FVector(.12f,.12f,Height));Bolt->SetRelativeLocation(FVector(0,0,Height*50.f));Glow(Bolt,3.f+7.f*Fraction);
        if(WarnTicks<1&&Fraction>=.45f) {WarnTicks=1;if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Hit.S_Hit"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.18f,2.1f);}
        if(WarnTicks<2&&Fraction>=.75f) {WarnTicks=2;if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Hit.S_Hit"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.22f,2.7f);}
        return;
    }
    if(!bFlashed) {
        bFlashed=true;Fill->SetRelativeScale3D(FVector(Scale,Scale,1.f));
        // Placeholder thunder: existing sounds, pitched.
        if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_HammerSmash.S_HammerSmash"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.8f,.75f);
        if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Collapse.S_Collapse"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.45f,1.4f);
    }
    FlashAge+=Delta;const float Fade=FMath::Clamp(1.f-FlashAge/.35f,0.f,1.f);
    SpokePivot->SetRelativeRotation(FRotator(0.f,Now*460.f,0.f));
    Bolt->SetRelativeScale3D(FVector(.9f*Fade+.04f,.9f*Fade+.04f,14.f));Bolt->SetRelativeLocation(FVector(0,0,700.f));
    Glow(Bolt,14.f*Fade);Glow(Fill,6.f*Fade);Glow(Disc,7.f*Fade);Glow(Outline,8.f*Fade);
    for(auto& Spoke:Spokes) Glow(Spoke.Get(),9.f*Fade);
    if(Fade<=0.f) {
        Bolt->SetVisibility(false);Fill->SetVisibility(false);Outline->SetVisibility(false);Disc->SetVisibility(false);
        for(auto& Spoke:Spokes) if(Spoke) Spoke->SetVisibility(false);
    }
}
