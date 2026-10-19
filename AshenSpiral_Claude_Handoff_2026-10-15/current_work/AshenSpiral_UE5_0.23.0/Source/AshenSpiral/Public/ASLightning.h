#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASLightning.generated.h"
class UStaticMeshComponent;class USceneComponent;
// Endless wave 50+: a lightning strike. The server spawns it; it replicates to every client, which draw the floor indicator and the thunder. The server deals the damage.
// 0.24 look: violet-white and deliberately unlike the enemy telegraphs (red/orange rings): a filled violet disc at the true radius, eight spokes that spin faster as
// the strike nears, a pale ring that fills toward the edge, and a sky column that grows into the bolt. Blue stays reserved for frost.
UCLASS() class ASHENSPIRAL_API AASLightningStrike : public AActor {
    GENERATED_BODY()
public:
    AASLightningStrike();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) float StrikeAt=0.f; // SimTime of the impact
    UPROPERTY(Replicated) float Radius=220.f;
private:
    void Resolve();
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Outline;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Fill;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Bolt; // the sky column: thin and growing while warning, thick and bright at impact
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Disc;
    UPROPERTY() TObjectPtr<USceneComponent> SpokePivot;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Spokes;
    bool bFlashed=false,bResolved=false;
    float FlashAge=0.f;
    int32 WarnTicks=0;
};
