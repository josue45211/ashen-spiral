#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASPortal.generated.h"
class UStaticMeshComponent;class USceneComponent;
// Hub portals (0.23.1 looks): Ember / Ashen / Blood difficulty archways and the Endless portal.
// Placeholder-quality gothic stone arches. Locked portals are chained across the archway; Blood (and Endless) drip.
UCLASS() class ASHENSPIRAL_API AASPortal : public AActor {
    GENERATED_BODY()
public:
    AASPortal();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    // 0 Ember (Normal), 1 Ashen (Hard), 2 Blood (Very Hard), 3 Endless.
    UPROPERTY(ReplicatedUsing=OnLook) int32 Kind=0;
    // Chained (difficulty) or dormant (Endless) until the unlock rule is met. Set by the server.
    UPROPERTY(ReplicatedUsing=OnLook) bool bLocked=true;
    UFUNCTION() void OnLook();
private:
    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Arch;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Lens;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Rim;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ChainParts;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Drips;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Flames;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Relics;
    float DripPhase[8]={0,.13f,.31f,.47f,.62f,.78f,.9f,.55f};
    bool bStyled=false;
    void Refresh();
};
