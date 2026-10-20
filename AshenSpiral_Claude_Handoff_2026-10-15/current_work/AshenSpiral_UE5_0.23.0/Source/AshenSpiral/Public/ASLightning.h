#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASLightning.generated.h"
class UStaticMeshComponent;class USceneComponent;class UPointLightComponent;
// Endless wave 50+: a lightning strike. The server spawns it; it replicates to every client, which draw the floor indicator and the thunder. The server deals the damage.
// 0.24 look: violet-white and deliberately unlike the enemy telegraphs (red/orange rings): a filled violet disc at the true radius, eight spokes that spin faster as
// the strike nears, a pale ring that fills toward the edge, and a sky column that grows while it charges. Blue stays reserved for frost.
// 0.24.2 (F8; playtest: "try to implement the lightning assets I downloaded" - none is on this PC, see MERGE_LOG): the strike itself is now a real bolt - a jagged, branching
// channel from the sky that flickers through three shapes in a third of a second - with a flash that lights the arena, a burst of sparks and a scorch mark that lingers.
UCLASS() class ASHENSPIRAL_API AASLightningStrike : public AActor {
    GENERATED_BODY()
public:
    AASLightningStrike();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) float StrikeAt=0.f; // SimTime of the impact
    UPROPERTY(Replicated) float Radius=220.f;
    enum : int32 {MainSegments=11,BranchSegments=7}; // the bolt's segments: down the main channel, then in each of two forks
private:
    void Resolve();
    void ShapeBolt(int32 Seed); // lays the bolt's segments along a fresh jagged path (cosmetic, every machine)
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Outline;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Fill;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Bolt; // the sky column while it charges: thin, flickering, growing toward the ground
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Disc;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Scorch; // the burn left on the floor after the strike
    UPROPERTY() TObjectPtr<USceneComponent> SpokePivot;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Spokes;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Channel; // the bolt: MainSegments down the middle, then BranchSegments in two forks
    UPROPERTY() TObjectPtr<UPointLightComponent> Flash;
    bool bFlashed=false,bResolved=false;
    float FlashAge=0.f;
    int32 WarnTicks=0,Shape=-1;
};
