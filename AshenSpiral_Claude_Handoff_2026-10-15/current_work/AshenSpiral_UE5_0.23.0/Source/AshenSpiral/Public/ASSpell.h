#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASSpell.generated.h"
class USceneComponent;class UStaticMeshComponent;class AASCharacter;class AASEnemy;
UCLASS() class ASHENSPIRAL_API AASProjectile : public AActor {
    GENERATED_BODY()
public:
    AASProjectile();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) FVector Velocity=FVector::ZeroVector;
    UPROPERTY(Replicated) TObjectPtr<AASCharacter> Hero;
    UPROPERTY(Replicated) bool bEnemy=false;
    UPROPERTY(Replicated) int32 Weapon=2;
    UPROPERTY(Replicated) float Damage=30;
    UPROPERTY(Replicated) bool bCritical=false;
    UPROPERTY(Replicated) bool bCosmetic=false;
    UPROPERTY(Replicated) bool bPractice=false;
    UPROPERTY(Replicated) bool bSecondary=false;
    // Endless wave 50+: slow, large, homing and lethal; only a well-timed dash avoids it.
    UPROPERTY(Replicated) bool bHoming=false;
    TWeakObjectPtr<AASCharacter> HomingTarget;
    TWeakObjectPtr<AASEnemy> EnemySource;
    TSet<TWeakObjectPtr<AActor>> HitActors;
    int32 Bounces=0,Pierces=0;
    bool bSplit=false;
    static AASProjectile* Fire(UWorld* World,const FVector& Position,const FVector& Direction,AASCharacter* OwnerHero,int32 Weapon,float Damage,bool Enemy=false,bool Critical=false,bool Secondary=false);
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Trail;
    float Expires=0;
    float Travelled=0;
    void Detonate(const FVector& Position,AASEnemy* DirectTarget=nullptr);
};
UCLASS() class ASHENSPIRAL_API AASWell : public AActor {
    GENERATED_BODY()
public:
    AASWell();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) TObjectPtr<AASCharacter> Hero;
    UPROPERTY(Replicated) float Radius=420;
    UPROPERTY(Replicated) float Expires=0;
    UPROPERTY(Replicated) bool bPerfect=false;
    UPROPERTY(Replicated) bool bInverted=false;
    UPROPERTY(Replicated) bool bCosmetic=false;
    UPROPERTY(Replicated) bool bPractice=false;
    bool bGuardUsed=false,bCollapsed=false;
    float OrbitUntil=0;
    TSet<TWeakObjectPtr<AASCharacter>> Crossed;
    bool CanAffectGameplay() const;
    void Collapse();
    static AASWell* Plant(AASCharacter* OwnerHero,const FVector& Position,bool Perfect,float Scale=1.f,float LifeOverride=0.f,bool CombatCarry=false);
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Ring;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Inner;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Core;
    float NextTick=0;
    bool bStyled=false;
};
UCLASS() class ASHENSPIRAL_API AASField : public AActor {
    GENERATED_BODY()
public:
    AASField();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) int32 Kind=0; // 0 tell, 1 trail, 2 spikes, 3 echo, 4 flame, 5 blades, 6 wall jet, 7 shuttle saw, 8 venom, 9 mine
    UPROPERTY(Replicated) bool bVault=false;
    UPROPERTY(Replicated) int32 Tier=0;
    UPROPERTY(Replicated) float Radius=250;
    UPROPERTY(Replicated) float Damage=24;
    UPROPERTY(Replicated) float FireAt=0;
    UPROPERTY(Replicated) bool bCosmetic=false;
    UPROPERTY(Replicated) bool bPractice=false;
    UPROPERTY(Replicated) float Expires=0;
    UPROPERTY(Replicated) FVector End=FVector::ZeroVector;
    UPROPERTY(Replicated) TObjectPtr<AASCharacter> Hero;
    TWeakObjectPtr<AASEnemy> EnemySource;
    float Cycle() const;
    bool Disabled() const;
    static AASField* Spawn(UWorld* World,int32 Type,const FVector& At,float Radius,float Damage,float Delay,float Life,AASCharacter* Hero=nullptr,const FVector& End=FVector::ZeroVector);
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Ring;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Spikes;
    float NextTick=0;
    bool bFired=false;
};
UCLASS() class ASHENSPIRAL_API AASPickup : public AActor {
    GENERATED_BODY()
public:
    AASPickup();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(ReplicatedUsing=RefreshLook) int32 Kind=0;
    UPROPERTY(Replicated) float Amount=12;
    UFUNCTION() void RefreshLook();
    static AASPickup* Spawn(UWorld* World,const FVector& At,int32 Kind,float Amount);
private:
    UPROPERTY() TObjectPtr<USceneComponent> PickupRoot;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    float Expires=0,Born=0;
};
UCLASS() class ASHENSPIRAL_API AASVisualEffect : public AActor {
    GENERATED_BODY()
public:
    AASVisualEffect();
    virtual void Tick(float Delta) override;
    void Setup(uint8 Type,const FVector& At,const FVector& Direction,FLinearColor Color,float Size);
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    float Age=0,Duration=.35f,Size=1;
    uint8 Kind=0;
};

// Cosmetic-only kill burst: a handful of glowing shards thrown outward with gravity. Spawned locally by the GameState effect multicast.
UCLASS() class ASHENSPIRAL_API AASShardBurst : public AActor {
    GENERATED_BODY()
public:
    AASShardBurst();
    virtual void Tick(float Delta) override;
    void Setup(const FVector& At,FLinearColor Color,float Tier);
    static int32 Active;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    struct FShard {TObjectPtr<UStaticMeshComponent> Mesh;FVector Velocity;float Spin;FVector BaseScale;};
    TArray<FShard> Shards;float Age=0,Life=.65f,Size=1.f;FVector Origin=FVector::ZeroVector;
};

// Cosmetic-only hammer impact, spawned locally by the character's multicast.
UCLASS() class ASHENSPIRAL_API AASHammerDebris : public AActor {
    GENERATED_BODY()
public:
    AASHammerDebris();
    void Setup(const FVector& At,const FVector& Direction,FLinearColor Color,bool Radial);
    virtual void Tick(float Delta) override;
private:
    UPROPERTY() TObjectPtr<USceneComponent> Scene;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Chunks;
    TArray<FVector> Velocities;
    float Age=0;
};
