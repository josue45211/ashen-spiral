#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASLightning.generated.h"
class UStaticMeshComponent;
// Endless wave 50+: a lightning strike. The server spawns it; it replicates to every client, which draw the floor indicator
// (an outline at the true radius plus a ring that fills over the dodge window), the bolt flash and the thunder. The server deals the damage.
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
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Bolt;
    bool bFlashed=false,bResolved=false;
    float FlashAge=0.f;
};
