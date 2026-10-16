#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASRouteGuide.generated.h"
class UStaticMeshComponent;
// Local world geometry: no viewport arrow, replicated actors, or gameplay collision.
UCLASS() class ASHENSPIRAL_API AASRouteGuide : public AActor {
    GENERATED_BODY()
public:
    AASRouteGuide();
    bool JumpAhead=false;
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
private:
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Chevrons;
    TArray<FVector> Route;
    TArray<bool> JumpLegs;
    FVector PlannedFrom=FVector::ZeroVector;
    float RepathAt=0;
    int32 Generation=-1;
};
