#include "ASSpell.h"
#include "ASCharacter.h"
#include "ASEnemy.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
// 0.26 Twin Claws: the claws' and the pounce's cosmetic effects. Everything here is local to each machine (spawned from replicated cues), costs nothing on the network, and is capped
// so a swarm of hits cannot flood the scene.
namespace ASClawFx {
void Sound(const UObject* Context,const TCHAR* Name,int32 Variants,const FVector& At,float Volume,float PitchMin,float PitchMax) {
    if(!Context||!Context->GetWorld()) return;
    const int32 Pick=Variants>1?FMath::RandRange(1,Variants):0;
    const FString Path=Pick>0?FString::Printf(TEXT("/Game/Audio/%s%d.%s%d"),Name,Pick,Name,Pick):FString::Printf(TEXT("/Game/Audio/%s.%s"),Name,Name);
    USoundBase* Cue=LoadObject<USoundBase>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet);if(!Cue) return;
    const APawn* Local=UGameplayStatics::GetPlayerPawn(Context,0);
    const float Gain=Local?ASEnemyRules::CueDistanceGain(float(FVector::Dist(Local->GetActorLocation(),At))):1.f; // a partner's blows across the arena are quieter than your own
    if(Gain*Volume<.015f) return;
    UGameplayStatics::PlaySoundAtLocation(Context,Cue,At,Volume*Gain,FMath::FRandRange(PitchMin,FMath::Max(PitchMin,PitchMax)));
}
}

int32 AASClawFx::Active=0;
AASClawFx::AASClawFx() {PrimaryActorTick.bCanEverTick=true;SetReplicates(false);RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));}
void AASClawFx::EndPlay(const EEndPlayReason::Type Reason) {Super::EndPlay(Reason);if(bCounted) {Active=FMath::Max(0,Active-1);bCounted=false;}}
UStaticMeshComponent* AASClawFx::AddBit(const TCHAR* Mesh,UMaterialInterface* Material) {
    auto* C=NewObject<UStaticMeshComponent>(this);C->SetStaticMesh(ASAssets::Mesh(Mesh));C->SetupAttachment(RootComponent);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetGenerateOverlapEvents(false);C->SetCastShadow(false);
    ASAssets::Paint(C,Material);C->RegisterComponent();return C;
}
// The rake: three parallel streaks across the body at At, on the face the attacker sees (From is where the attacker stands), crossing along a diagonal that alternates with Step.
void AASClawFx::SetupRake(const FVector& At,const FVector& From,FLinearColor Color,int32 Step,float Power) {
    if(Active>=30) {Destroy();return;}
    ++Active;bCounted=true;Kind=Look::Rake;Life=.16f+.04f*Power;Emission=11.f+8.f*Power; // bright enough to bloom, not so bright it washes to white (the bleed colour should still read)
    FVector Toward=(At-From).GetSafeNormal2D();if(Toward.IsNearlyZero()) Toward=FVector::ForwardVector;
    const FVector Right=FVector::CrossProduct(FVector::UpVector,Toward).GetSafeNormal(); // across the view of the attacker
    const float Tilt=(Step&1?-1.f:1.f)*FMath::DegreesToRadians(32.f);
    const FVector Axis=(FVector::UpVector*FMath::Cos(Tilt)+Right*FMath::Sin(Tilt)).GetSafeNormal(),Side=FVector::CrossProduct(Toward,Axis).GetSafeNormal();
    SetActorLocation(At-Toward*30.f);
    Glow=ASAssets::Glow(this,FMath::Lerp(Color,FLinearColor::White,.22f),Emission);
    const float Lengths[3]={110.f+30.f*Power,150.f+40.f*Power,110.f+30.f*Power},Offsets[3]={-17.f,0.f,17.f};
    for(int32 I=0;I<3;++I) {
        FBit B;B.Mesh=AddBit(TEXT("Cube"),Glow);B.Axis=Axis;B.Length=Lengths[I];B.Start=Side*Offsets[I]-Axis*Lengths[I]*.5f; // from the head of the streak toward its foot
        B.Mesh->SetRelativeRotation(FRotationMatrix::MakeFromXZ(Axis,Toward).Rotator());B.Mesh->SetRelativeScale3D(FVector(.01f,.02f,.02f));Bits.Add(B);
    }
}
// Sparks thrown out of the impact: toward the attacker's side and up, in the weapon's element colour burning white at the core. Power 0..1 scales how many and how fast.
void AASClawFx::SetupSparks(const FVector& At,const FVector& From,FLinearColor Color,float Power) {
    if(Active>=30) {Destroy();return;}
    ++Active;bCounted=true;Kind=Look::Sparks;Life=.30f+.12f*Power;SetActorLocation(At);
    Glow=ASAssets::Glow(this,FMath::Lerp(Color,FLinearColor::White,.45f),18.f);Emission=18.f;
    const FVector Back=(From-At).GetSafeNormal();const int32 Count=FMath::RoundToInt(6.f+10.f*Power);
    for(int32 I=0;I<Count;++I) {
        // a spark is a short bright streak that points the way it is going (re-aimed along its flight every frame: Tick), not a tumbling chip
        const FVector Dir=(Back*.55f+FMath::VRand()*.95f+FVector(0,0,.35f)).GetSafeNormal();
        FBit B;B.Mesh=AddBit(TEXT("Cube"),Glow);B.Velocity=Dir*FMath::FRandRange(300.f,700.f+420.f*Power);B.Gravity=900.f;
        const float Len=FMath::FRandRange(.06f,.12f)*(1.f+.5f*Power),Thick=.008f*(1.f+.4f*Power);B.Scale0=FVector(Len,Thick,Thick);B.Scale1=FVector(.004f,.002f,.002f);
        B.Mesh->SetRelativeRotation(Dir.Rotation());B.Mesh->SetRelativeScale3D(B.Scale0);Bits.Add(B);
    }
}
// Dust and grit flung up round a point on the floor: flat-brown chunks that tumble and settle, and a quick low burst outward.
void AASClawFx::SetupDust(const FVector& At,float R,float Power) {
    if(Active>=30) {Destroy();return;}
    ++Active;bCounted=true;Kind=Look::Dust;Life=.55f;SetActorLocation(At);
    Matte=ASAssets::Material(this,FLinearColor(.78f,.68f,.52f),0.f,.7f,.95f); // sandstone dust (the floor's own tone; a faint glow so a chip in shade does not go black)
    const int32 Count=FMath::RoundToInt(8.f+10.f*Power);
    for(int32 I=0;I<Count;++I) {
        const float A=FMath::FRandRange(0.f,UE_TWO_PI);const FVector Out(FMath::Cos(A),FMath::Sin(A),0.f);
        FBit B;B.Mesh=AddBit(TEXT("Cube"),Matte);B.Start=Out*R*.25f+FVector(0,0,4);B.Velocity=Out*FMath::FRandRange(.5f,1.f)*R*2.2f+FVector(0,0,FMath::FRandRange(140.f,320.f)*(.6f+.6f*Power));B.Gravity=820.f;
        const float S=FMath::FRandRange(.03f,.075f)*(.8f+.6f*Power);B.Scale0=FVector(S,S*FMath::FRandRange(.6f,1.f),S*FMath::FRandRange(.5f,1.f));B.Scale1=FVector(.004f);
        B.Spin=FRotator(FMath::FRandRange(-420.f,420.f),FMath::FRandRange(-420.f,420.f),0.f);B.Mesh->SetRelativeScale3D(B.Scale0);Bits.Add(B);
    }
}
void AASClawFx::SetupRing(const FVector& At,FLinearColor Color,float R,float Seconds) {
    if(Active>=30) {Destroy();return;}
    ++Active;bCounted=true;Kind=Look::Ring;Life=FMath::Max(.1f,Seconds);Radius=R;Emission=7.f;
    FHitResult Floor;FVector Place=At;
    if(GetWorld()&&GetWorld()->LineTraceSingleByObjectType(Floor,At+FVector(0,0,40),At-FVector(0,0,320),FCollisionObjectQueryParams(ECC_WorldStatic))) Place=Floor.ImpactPoint+FVector(0,0,3);
    SetActorLocation(Place);
    FBit B;B.Mesh=AddBit(TEXT("SM_Ring"),nullptr);ASAssets::StyleGlowBand(B.Mesh,TEXT("SM_Ring"),Color,Emission,.45f,.52f,0.f);B.Mesh->SetRelativeScale3D(FVector(.2f,.2f,1.f));Bits.Add(B);
}
// The bond: a thin ring on the floor under the target; while the wielder holds on it breathes, and it goes the moment the latch ends.
void AASClawFx::SetupBond(AASCharacter* Hero,FLinearColor Color) {
    if(!Hero||Active>=30) {Destroy();return;}
    ++Active;bCounted=true;Kind=Look::Bond;Holder=Hero;Life=ASClaws::GlueSeconds+1.f;Emission=6.f;
    FBit B;B.Mesh=AddBit(TEXT("SM_Ring"),nullptr);ASAssets::StyleGlowBand(B.Mesh,TEXT("SM_Ring"),Color,Emission,.44f,.52f,0.f);B.Mesh->SetRelativeScale3D(FVector(.1f,.1f,1.f));Bits.Add(B);
}
void AASClawFx::Tick(float Delta) {
    Super::Tick(Delta);if(auto* S=AS::State(this)) if(S->Frozen()) return;
    Age+=Delta;
    if(Kind==Look::Bond) {
        const AASCharacter* H=Holder.Get();const AASEnemy* T=H?H->PounceTarget.Get():nullptr;
        if(!H||!T||!H->PounceLatched()||Age>Life) {Destroy();return;}
        const float R=H->GetCapsuleComponent()->GetScaledCapsuleRadius()+T->GetCapsuleComponent()->GetScaledCapsuleRadius()+ASClaws::RingGap+22.f;
        const FVector Feet=T->GetActorLocation()-FVector(0,0,T->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-4.f);
        SetActorLocation(Feet);
        const float Grow=FMath::Min(1.f,Age/.12f),Pulse=1.f+.06f*FMath::Sin(Age*14.f);
        Bits[0].Mesh->SetRelativeScale3D(FVector(R/50.f*Pulse*(.5f+.5f*Grow),R/50.f*Pulse*(.5f+.5f*Grow),1.f));
        if(auto* M=Cast<UMaterialInstanceDynamic>(Bits[0].Mesh->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),Emission*(.7f+.5f*FMath::Sin(Age*9.f))*Grow);
        return;
    }
    if(Age>=Life) {Destroy();return;}
    const float T=Age/Life;
    if(Kind==Look::Rake) {
        const float Draw=FMath::InterpEaseOut(0.f,1.f,FMath::Min(1.f,T/.38f),2.f),Thin=1.f-FMath::Clamp((T-.38f)/.62f,0.f,1.f);
        for(auto& B:Bits) {
            const float Now=B.Length*Draw;B.Mesh->SetRelativeLocation(B.Start+B.Axis*Now*.5f);B.Mesh->SetRelativeScale3D(FVector(Now/100.f,.022f*(.25f+.75f*Thin),.022f*(.25f+.75f*Thin)));
        }
        if(Glow) Glow->SetScalarParameterValue(TEXT("Emission"),Emission*FMath::Square(1.f-T*.9f));
        return;
    }
    if(Kind==Look::Ring) {
        const float Grow=FMath::InterpEaseOut(.2f,1.f,T,2.2f),R=Radius/50.f*Grow;
        Bits[0].Mesh->SetRelativeScale3D(FVector(R,R,1.f));
        if(auto* M=Cast<UMaterialInstanceDynamic>(Bits[0].Mesh->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),Emission*FMath::Square(1.f-T));
        return;
    }
    // sparks and dust: ballistic, shrinking; dust tumbles and settles on the floor, a spark points along its flight
    for(auto& B:Bits) if(B.Mesh) {
        FVector P=B.Start+B.Velocity*Age-FVector(0,0,.5f*B.Gravity*Age*Age);if(Kind==Look::Dust) P.Z=FMath::Max(P.Z,1.f);
        B.Mesh->SetRelativeLocation(P);
        if(Kind==Look::Sparks) B.Mesh->SetRelativeRotation((B.Velocity-FVector(0,0,B.Gravity*Age)).Rotation());else B.Mesh->AddLocalRotation(B.Spin*Delta);
        B.Mesh->SetRelativeScale3D(FMath::Lerp(B.Scale0,B.Scale1,FMath::Square(T)));
    }
    if(Kind==Look::Sparks&&Glow) Glow->SetScalarParameterValue(TEXT("Emission"),Emission*(1.f-T));
}
