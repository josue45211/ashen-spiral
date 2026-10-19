#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASEnemyHazards.generated.h"
class UStaticMeshComponent;class AASCharacter;class AASEnemy;
// 0.24 hazards left behind by enemy abilities. Replicated actors: the server decides who is hurt (the geometry is ASEnemyAbilities.h, tested as plain C++), and every machine
// draws the same thing from the replicated start time. Colours follow Docs/TELEGRAPH_LANGUAGE_0.24.md (red-orange is damage).

// The brute's ground pound: a ring that sweeps out from its feet. A hero standing on the ground when the front passes is hurt once; a jump (or a dash's invulnerability) avoids it.
UCLASS() class ASHENSPIRAL_API AASShockwave : public AActor {
    GENERATED_BODY()
public:
    AASShockwave();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) float Born=0.f;        // SimTime when the ring started
    UPROPERTY(Replicated) float MaxRadius=720.f; // where it ends
    UPROPERTY(Replicated) float Damage=18.f;
    TWeakObjectPtr<AASEnemy> EnemySource;
    static AASShockwave* Spawn(UWorld* World,const FVector& Feet,AASEnemy* Source,float Damage,float MaxRadius); // Feet: the floor under the enemy
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Ring;
    float PrevRadius=0.f;bool bSwept=false;
    TSet<TWeakObjectPtr<AASCharacter>> Struck;
};
