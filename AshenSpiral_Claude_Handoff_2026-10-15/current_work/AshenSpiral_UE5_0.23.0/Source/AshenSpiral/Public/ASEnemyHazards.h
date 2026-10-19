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
    static AASShockwave* Spawn(UWorld* World,const FVector& Feet,AASEnemy* Source,float Damage,float MaxRadius,float Delay=0.f); // Feet: the floor under the enemy; Delay: it starts this many seconds from now
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Ring;
    float PrevRadius=0.f;bool bSwept=false;
    TSet<TWeakObjectPtr<AASCharacter>> Struck;
};

// A lingering ground zone (the elite mutations' fire trails, frost, storm and venom; the ward bosses' blooms and roots). It has a warning phase (a dim disc at its full size with a
// bright copy growing over it, red for everything except lightning, which is violet-white, like the sky strikes) and, when the warning is over, it arms: an optional one-off impact,
// then it hurts a hero standing in it every half second until it shrinks away. Size, time and damage come from ASEnemyAbilities.h (HazardFor).
UCLASS() class ASHENSPIRAL_API AASHazard : public AActor {
    GENERATED_BODY()
public:
    AASHazard();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) float Born=0.f;   // SimTime when the warning started
    UPROPERTY(Replicated) float ArmAt=0.f;  // when the warning ends and the zone is live (== Born for a zone with no warning)
    UPROPERTY(Replicated) uint8 Style=0;    // ASEnemyRules::Hazard*
    float ImpactDamage=0.f;                 // server only: hurts everyone inside once, where it arms
    TWeakObjectPtr<AASEnemy> EnemySource;
    static AASHazard* Spawn(UWorld* World,const FVector& Feet,int32 Style,AASEnemy* Source,float WarnSeconds,float ImpactDamage); // null at the cap
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Disc;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Fill;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Bolt; // storm only: the sky column
    float NextTick=0.f;
    bool bArmed=false,bDrawnArmed=false,bHeardArm=false,bCounted=false;
    static int32 Alive;
};
