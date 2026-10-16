#include "ASSpell.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
AASHammerDebris::AASHammerDebris() {
    PrimaryActorTick.bCanEverTick=true;SetReplicates(false);
    Scene=CreateDefaultSubobject<USceneComponent>(TEXT("Impact"));RootComponent=Scene;
    for(int32 I=0;I<18;++I) {
        auto* C=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Shard%d"),I));C->SetupAttachment(Scene);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);Chunks.Add(C);
    }
}
void AASHammerDebris::Setup(const FVector& At,const FVector& Direction,FLinearColor Color,bool Radial) {
    SetActorLocation(At);SetActorRotation(Direction.Rotation());Velocities.Empty();
    FRandomStream Rng(220022); // Stable cosmetic pattern; no gameplay RNG consumed.
    for(int32 I=0;I<Chunks.Num();++I) {
        const float A=Radial?I*UE_TWO_PI/Chunks.Num():Rng.FRandRange(-1.1f,1.1f),Speed=Rng.FRandRange(110.f,310.f);
        auto* C=Chunks[I].Get();const bool Spark=I%3==0;
        ASAssets::Style(C,TEXT("Cube"),Spark?Color:FLinearColor(.23f,.21f,.19f),Spark?.5f:0.f,Spark?3.f:0.f);
        C->SetRelativeLocation(FVector(0,0,8));C->SetRelativeScale3D(FVector(Rng.FRandRange(.035f,.085f),.035f,Rng.FRandRange(.025f,.07f)));
        Velocities.Add(FVector(FMath::Cos(A)*Speed,FMath::Sin(A)*Speed,Rng.FRandRange(180.f,380.f)));
    }
}
void AASHammerDebris::Tick(float Delta) {
    Super::Tick(Delta);if(auto* S=AS::State(this)) if(S->Frozen()) return;
    Age+=Delta;if(Age>=.65f) {Destroy();return;}
    for(int32 I=0;I<Chunks.Num()&&Velocities.IsValidIndex(I);++I) {
        FVector P=FVector(0,0,8)+Velocities[I]*Age-FVector(0,0,650.f*Age*Age);P.Z=FMath::Max(2.0,P.Z);
        Chunks[I]->SetRelativeLocation(P);Chunks[I]->SetRelativeRotation(FRotator(Age*(170.f+I*11.f),Age*230.f,I*29.f));
        if(Age>.4f) Chunks[I]->SetRelativeScale3D(Chunks[I]->GetRelativeScale3D()*FMath::Max(0.f,1.f-Delta*12.f));
    }
}
