#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASBreakable.generated.h"
class UStaticMeshComponent;class AASCharacter;class AASEnemy;
UCLASS() class ASHENSPIRAL_API AASBreakable : public AActor {
    GENERATED_BODY()
public:
    AASBreakable();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(ReplicatedUsing=RefreshLook) int32 Kind=0; // 0 reward crystal, 1 explosive cask, 2 brood egg (0.24)
    UPROPERTY(Replicated) float Health=35.f;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    UFUNCTION() void RefreshLook();
    void Strike(AASCharacter* Hero,float Damage);
    bool VisibleFrom(const FVector& From) const;
    static void Spawn(UWorld* World,const FVector& At,int32 Type);
    // 0.24 brood egg (Kind 2): laid by a Broodcaller, hatches into a minion at HatchAt unless a hero pops it first. Null when no egg may be laid.
    static AASBreakable* LayEgg(UWorld* World,const FVector& At,AASEnemy* Mother);
    UPROPERTY(Replicated) float HatchAt=0.f;
    TWeakObjectPtr<AASEnemy> Mother; // server only
private:
    bool bBroken=false;
    void Hatch();
};
