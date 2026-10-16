#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASBreakable.generated.h"
class UStaticMeshComponent;class AASCharacter;
UCLASS() class ASHENSPIRAL_API AASBreakable : public AActor {
    GENERATED_BODY()
public:
    AASBreakable();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(ReplicatedUsing=RefreshLook) int32 Kind=0; // 0 reward crystal, 1 explosive cask
    UPROPERTY(Replicated) float Health=35.f;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    UFUNCTION() void RefreshLook();
    void Strike(AASCharacter* Hero,float Damage);
    bool VisibleFrom(const FVector& From) const;
    static void Spawn(UWorld* World,const FVector& At,int32 Type);
private:
    bool bBroken=false;
};
