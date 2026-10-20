#include "ASLightning.h"
#include "ASGameState.h"
#include "ASCharacter.h"
#include "ASEnemy.h"
#include "ASWorld.h"
#include "ASExpeditionRules.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
// The strike's timeline after impact (seconds): the bolt flickers through three shapes and is gone at BoltLife; the flash of light fades by FlashLife; the scorch shrinks away
// by ScorchLife, when the server removes the actor. TUNABLE, cosmetic.
namespace ASLightningLook {
constexpr float BoltLife=.34f,FlashLife=.26f,ScorchLife=2.4f,FlashCandela=5500.f,FlashReach=1700.f,ChannelTop=1700.f; // in-engine check: 60000 cd washed the whole screen white
constexpr float Reshape[]={.075f,.16f}; // the bolt jumps to a new shape at these ages
}
AASLightningStrike::AASLightningStrike() {
    PrimaryActorTick.bCanEverTick=true;bReplicates=true;bAlwaysRelevant=true;SetReplicateMovement(false);SetNetUpdateFrequency(5.f);
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Disc=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));Disc->SetupAttachment(RootComponent);
    Outline=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Outline"));Outline->SetupAttachment(RootComponent);
    Fill=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fill"));Fill->SetupAttachment(RootComponent);
    Scorch=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Scorch"));Scorch->SetupAttachment(RootComponent);
    SpokePivot=CreateDefaultSubobject<USceneComponent>(TEXT("SpokePivot"));SpokePivot->SetupAttachment(RootComponent);
    for(int32 I=0;I<4;++I) {auto* Spoke=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Spoke%d"),I));Spoke->SetupAttachment(SpokePivot);Spokes.Add(Spoke);}
    Bolt=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bolt"));Bolt->SetupAttachment(RootComponent);
    for(int32 I=0;I<MainSegments+2*BranchSegments;++I) {auto* Segment=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Channel%02d"),I));Segment->SetupAttachment(RootComponent);Channel.Add(Segment);}
    Flash=CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));Flash->SetupAttachment(RootComponent);
    Flash->SetCastShadows(false);Flash->SetIntensityUnits(ELightUnits::Candelas);Flash->SetIntensity(0.f);Flash->SetAttenuationRadius(ASLightningLook::FlashReach);Flash->SetVisibility(false);
    for(UStaticMeshComponent* C:{Disc.Get(),Outline.Get(),Fill.Get(),Scorch.Get(),Bolt.Get()}) C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AASLightningStrike::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASLightningStrike,StrikeAt);DOREPLIFETIME(AASLightningStrike,Radius);
}
void AASLightningStrike::BeginPlay() {
    Super::BeginPlay();
    if(GetNetMode()==NM_DedicatedServer) return;
    // Palette: violet and pale white only. Enemy telegraphs are red/orange rings and frost is blue, so a strike can never be mistaken for either.
    const FLinearColor Violet(.58f,.32f,1.f),Pale(.9f,.84f,1.f),Deep(.16f,.06f,.34f),Core(.96f,.93f,1.f);
    const float R=FMath::Max(1.f,Radius);
    const float RingScale=R/53.f; // the ring mesh is 53 cm in radius, so scale = radius / 53 makes the outline the exact damage radius
    ASAssets::Style(Disc,TEXT("Cylinder"),Deep,0,1.2f);Disc->SetCastShadow(false);Disc->SetRelativeLocation(FVector(0,0,3));Disc->SetRelativeScale3D(FVector(R/50.f,R/50.f,.02f)); // 100 cm cylinder: R/50 is the exact radius
    ASAssets::Style(Outline,TEXT("SM_Ring"),Violet,0,5.f);Outline->SetCastShadow(false);Outline->SetRelativeLocation(FVector(0,0,6));Outline->SetRelativeScale3D(FVector(RingScale,RingScale,1.f));
    ASAssets::Style(Fill,TEXT("SM_Ring"),Pale,0,6.f);Fill->SetCastShadow(false);Fill->SetRelativeLocation(FVector(0,0,8));Fill->SetRelativeScale3D(FVector(.02f,.02f,1.f));
    ASAssets::Style(Scorch,TEXT("Cylinder"),FLinearColor(.025f,.018f,.03f),0,0.f);Scorch->SetCastShadow(false);Scorch->SetRelativeLocation(FVector(0,0,2));Scorch->SetVisibility(false);
    SpokePivot->SetRelativeLocation(FVector(0,0,10));
    for(int32 I=0;I<Spokes.Num();++I) {
        auto* Spoke=Spokes[I].Get();if(!Spoke) continue;
        ASAssets::Style(Spoke,TEXT("Cube"),Violet,0,4.f);Spoke->SetCastShadow(false);Spoke->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Spoke->SetRelativeRotation(FRotator(0.f,45.f*float(I),0.f));Spoke->SetRelativeScale3D(FVector(R*1.9f/100.f,.035f,.025f)); // four diameters = eight spokes
    }
    ASAssets::Style(Bolt,TEXT("Cylinder"),Pale,0,12.f);Bolt->SetCastShadow(false);Bolt->SetRelativeLocation(FVector(0,0,25));Bolt->SetRelativeScale3D(FVector(.05f,.05f,.5f));
    for(int32 I=0;I<Channel.Num();++I) if(auto* Segment=Channel[I].Get()) {ASAssets::Style(Segment,TEXT("Cube"),Core,0,40.f);Segment->SetCastShadow(false);Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);Segment->SetVisibility(false);}
    Flash->SetLightColor(FLinearColor(.82f,.74f,1.f));Flash->SetRelativeLocation(FVector(0,0,260));
    // A faint crackle when the warning appears (placeholder: existing sound, pitched up); two more, higher, follow as the strike nears.
    if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Hit.S_Hit"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.16f,1.6f);
}
void AASLightningStrike::ShapeBolt(int32 Seed) {
    // A main channel from high in the sky to the floor: every joint wanders sideways, most in the middle of the fall and least at the ends, so the bolt meets the floor at the
    // centre of the warning. Two forks split off the upper half and reach down and out, thinner. Segments are unit cubes stretched between joints.
    FRandomStream Rand(Seed);
    auto Lay=[](UStaticMeshComponent* Segment,const FVector& A,const FVector& B,float Thickness) {
        if(!Segment) return;const FVector Span=B-A;const float Length=Span.Size();
        if(Length<1.f) {Segment->SetVisibility(false);return;}
        Segment->SetRelativeLocationAndRotation((A+B)*.5f,Span.Rotation());Segment->SetRelativeScale3D(FVector(Length/100.f,Thickness,Thickness));Segment->SetVisibility(true);
    };
    TArray<FVector> Joints;Joints.Reserve(MainSegments+1);
    for(int32 I=0;I<=MainSegments;++I) {
        const float T=float(I)/MainSegments,Wander=FMath::Sin(T*UE_PI)*95.f+12.f;
        const FVector Side=I==0||I==MainSegments?FVector::ZeroVector:FVector(Rand.FRandRange(-1.f,1.f),Rand.FRandRange(-1.f,1.f),0.f)*Wander;
        Joints.Add(FVector(Side.X,Side.Y,ASLightningLook::ChannelTop*(1.f-T)+6.f));
    }
    for(int32 I=0;I<MainSegments;++I) Lay(Channel[I].Get(),Joints[I],Joints[I+1],I>MainSegments-3?.11f:.075f); // a little thicker as it nears the floor
    for(int32 Fork=0;Fork<2;++Fork) {
        FVector At=Joints[2+Fork*2+Rand.RandRange(0,1)];const float Heading=Rand.FRandRange(0.f,360.f);
        for(int32 I=0;I<BranchSegments;++I) {
            const FVector Out=FRotator(0.f,Heading+Rand.FRandRange(-35.f,35.f),0.f).Vector()*Rand.FRandRange(45.f,90.f);
            const FVector Next=At+Out+FVector(0.f,0.f,-Rand.FRandRange(70.f,140.f));
            Lay(Channel[MainSegments+Fork*BranchSegments+I].Get(),At,FVector(Next.X,Next.Y,FMath::Max(20.f,Next.Z)),.045f*(1.f-.1f*I));At=Next;
            if(At.Z<=20.f) {for(int32 J=I+1;J<BranchSegments;++J) if(auto* Rest=Channel[MainSegments+Fork*BranchSegments+J].Get()) Rest->SetVisibility(false);break;}
        }
    }
}
void AASLightningStrike::Resolve() {
    bResolved=true;
    auto* S=AS::State(this);if(!S) return;
    const FVector At=GetActorLocation();const float R=Radius;
    for(auto W:S->Heroes) if(auto* H=W.Get()) if(!H->bDowned&&FVector::Dist2D(H->GetActorLocation(),At)<=R) H->Hurt(ASExpedition::LightningPlayerDamage(H->MaxHealth),nullptr,false);
    const auto Enemies=S->Enemies;
    for(auto W:Enemies) if(auto* E=W.Get()) if(E->Health>0&&!E->bTrainingDummy&&FVector::Dist2D(E->GetActorLocation(),At)<=R+E->GetCapsuleComponent()->GetScaledCapsuleRadius()) E->Hit(ASExpedition::LightningEnemyDamage(E->MaxHealth,E->Boss()),nullptr);
    S->Effect(8,At+FVector(0,0,20),FVector::UpVector,FLinearColor(.8f,.7f,1.f),R*1.4f);
    S->Effect(11,At+FVector(0,0,15),FVector::UpVector,FLinearColor(.9f,.82f,1.f),1.6f); // 0.24.2: a burst of white-violet sparks off the floor
}
void AASLightningStrike::Tick(float Delta) {
    Super::Tick(Delta);
    auto* S=AS::State(this);if(!S||S->Frozen()) return;
    const float Now=S->SimTime;
    if(HasAuthority()) {
        if(!bResolved&&Now>=StrikeAt) Resolve();
        if(Now>=StrikeAt+ASLightningLook::ScorchLife) {Destroy();return;}
    }
    if(GetNetMode()==NM_DedicatedServer) return;
    const float Telegraph=ASExpedition::LightningTelegraph,Scale=FMath::Max(1.f,Radius)/53.f;
    const auto Glow=[](UStaticMeshComponent* C,float Emission) {if(C) if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),Emission);};
    if(Now<StrikeAt) {
        // Warning: the disc brightens, the spokes spin faster, the pale ring fills toward the outline, and a thin flickering column reaches down from the sky.
        const float Fraction=FMath::Clamp(1.f-(StrikeAt-Now)/Telegraph,0.f,1.f);
        const float Grow=FMath::Max(.02f,Fraction)*Scale;Fill->SetRelativeScale3D(FVector(Grow,Grow,1.f));
        Glow(Outline,4.f+4.f*FMath::Sin(Now*(14.f+24.f*Fraction)));
        Glow(Disc,1.2f+2.6f*Fraction);
        SpokePivot->SetRelativeRotation(FRotator(0.f,Now*(40.f+420.f*Fraction),0.f));
        for(auto& Spoke:Spokes) Glow(Spoke.Get(),3.f+5.f*Fraction);
        const float Height=FMath::Lerp(.5f,17.f,Fraction*Fraction),Flicker=.6f+.4f*FMath::Abs(FMath::Sin(Now*61.f)); // cylinder is 100 cm tall: scale 17 is a 1700 cm column
        Bolt->SetRelativeScale3D(FVector(.05f*Flicker,.05f*Flicker,Height));Bolt->SetRelativeLocation(FVector(0,0,ASLightningLook::ChannelTop-Height*50.f));Glow(Bolt,(3.f+9.f*Fraction)*Flicker);
        if(WarnTicks<1&&Fraction>=.45f) {WarnTicks=1;if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Hit.S_Hit"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.18f,2.1f);}
        if(WarnTicks<2&&Fraction>=.75f) {WarnTicks=2;if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Hit.S_Hit"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.22f,2.7f);}
        return;
    }
    if(!bFlashed) {
        bFlashed=true;Fill->SetRelativeScale3D(FVector(Scale,Scale,1.f));Bolt->SetVisibility(false);
        Shape=0;ShapeBolt(int32(GetUniqueID())*7919+1);Flash->SetVisibility(true);
        Scorch->SetVisibility(true);Scorch->SetRelativeScale3D(FVector(Radius*.62f/50.f,Radius*.62f/50.f,.012f));
        // Placeholder thunder: existing sounds, pitched - a crack at once, and the roll a moment later.
        if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_HammerSmash.S_HammerSmash"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.85f,.75f);
        if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Collapse.S_Collapse"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.45f,1.4f);
    }
    FlashAge+=Delta;
    // The bolt: bright, jumping to a new shape twice, dimming between, gone at BoltLife.
    while(Shape<2&&FlashAge>=ASLightningLook::Reshape[Shape]) {++Shape;ShapeBolt(int32(GetUniqueID())*7919+1+Shape*104729);}
    const float BoltFade=FMath::Clamp(1.f-FlashAge/ASLightningLook::BoltLife,0.f,1.f),Pulse=.55f+.45f*FMath::Abs(FMath::Cos(FlashAge*72.f));
    for(auto& Segment:Channel) if(Segment) {if(BoltFade<=0.f) Segment->SetVisibility(false);else Glow(Segment.Get(),40.f*BoltFade*Pulse);}
    // The flash lights the arena for an instant, flickering with the bolt.
    const float Light=FMath::Clamp(1.f-FlashAge/ASLightningLook::FlashLife,0.f,1.f);
    Flash->SetIntensity(ASLightningLook::FlashCandela*Light*Light*Pulse);if(Light<=0.f) Flash->SetVisibility(false);
    // The floor warning flares and goes out; the scorch shrinks away.
    const float Fade=FMath::Clamp(1.f-FlashAge/.35f,0.f,1.f);
    SpokePivot->SetRelativeRotation(FRotator(0.f,Now*460.f,0.f));
    Glow(Fill,6.f*Fade);Glow(Disc,7.f*Fade);Glow(Outline,8.f*Fade);
    for(auto& Spoke:Spokes) Glow(Spoke.Get(),9.f*Fade);
    if(Fade<=0.f) {
        Fill->SetVisibility(false);Outline->SetVisibility(false);Disc->SetVisibility(false);
        for(auto& Spoke:Spokes) if(Spoke) Spoke->SetVisibility(false);
    }
    const float Burn=FMath::Clamp((ASLightningLook::ScorchLife-FlashAge)/(ASLightningLook::ScorchLife-.4f),0.f,1.f);
    Scorch->SetRelativeScale3D(FVector(Radius*.62f/50.f*Burn,Radius*.62f/50.f*Burn,.012f));Glow(Scorch,FlashAge<.5f?2.5f*(1.f-FlashAge/.5f):0.f); // the burn glows hot for a moment, then cools
    if(Burn<=0.f) Scorch->SetVisibility(false);
}
