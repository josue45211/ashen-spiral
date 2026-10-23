#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASSpell.generated.h"
class USceneComponent;class UStaticMeshComponent;class UPoseableMeshComponent;class UMaterialInstanceDynamic;class AASCharacter;class AASEnemy;
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
    bool bFinisherArc=false; // Kind 17 (0.24 swing arc): the combo's third swing is drawn brighter and holds a little longer
    FVector Anchor=FVector::ZeroVector; // Kind 18 (0.24 meteor): the ground point it falls onto
    UPROPERTY() TObjectPtr<UStaticMeshComponent> GlintBars[2]; // Kind 20 (0.24.2 weapon glint): the star's two crossed rays
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

// Cosmetic-only dash afterimage: a stretched glowing body silhouette that fades and shrinks. Spawned locally.
UCLASS() class ASHENSPIRAL_API AASDashGhost : public AActor {
    GENERATED_BODY()
public:
    AASDashGhost();
    virtual void Tick(float Delta) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void Setup(const FVector& At,float Yaw,FLinearColor Color);
    // Afterimage of the character itself: copies its current body parts, cape and weapon at their present pose, tinted in the player's colour.
    void SetupFromCharacter(const AASCharacter* Source,FLinearColor Color);
    static int32 Active;
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Copies;
    UPROPERTY() TObjectPtr<UPoseableMeshComponent> Pose; // 0.24.2 Phase F: the Mannequin hero's afterimage, its pose frozen at the moment
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Shared;
    float Age=0;bool bCounted=false;
};

// 0.26 Twin Claws: cosmetic effects, spawned locally on every machine by the character's multicasts (ASClawEffects.cpp). One actor class, several looks:
//   rake   three glowing streaks drawn across the struck body in a flash (the diagonal alternates with the combo step)
//   sparks bits thrown out of an impact toward the attacker, tinted by the weapon's element
//   dust   brown debris thrown up round a point on the floor (a leap's launch, a landing)
//   ring   a flat ring spreading out on the floor
//   bond   a thin pulsing ring on the floor under a pounce's target, for as long as the hero holds on
UCLASS() class ASHENSPIRAL_API AASClawFx : public AActor {
    GENERATED_BODY()
public:
    AASClawFx();
    virtual void Tick(float Delta) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void SetupRake(const FVector& At,const FVector& From,FLinearColor Color,int32 Step,float Power);
    void SetupSparks(const FVector& At,const FVector& From,FLinearColor Color,float Power);
    void SetupDust(const FVector& At,float Radius,float Power);
    void SetupRing(const FVector& At,FLinearColor Color,float Radius,float Seconds);
    void SetupBond(AASCharacter* Hero,FLinearColor Color);
    static int32 Active;
private:
    enum class Look : uint8 {Rake,Sparks,Dust,Ring,Bond};
    struct FBit {TObjectPtr<UStaticMeshComponent> Mesh;FVector Start=FVector::ZeroVector,Velocity=FVector::ZeroVector,Axis=FVector::ForwardVector,Scale0=FVector::OneVector,Scale1=FVector::ZeroVector;FRotator Spin=FRotator::ZeroRotator;float Length=0.f,Gravity=0.f;};
    UStaticMeshComponent* AddBit(const TCHAR* Mesh,UMaterialInterface* Material);
    TArray<FBit> Bits;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Glow;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Matte;
    TWeakObjectPtr<AASCharacter> Holder;
    Look Kind=Look::Rake;float Age=0.f,Life=.2f,Emission=20.f,Radius=0.f;bool bCounted=false;
};
namespace ASClawFx {
// One of Variants numbered sounds (/Game/Audio/<Name>1..N; Variants 1: /Game/Audio/<Name>) at a point, quieter with the distance from the local player like every cue of another player's.
void Sound(const UObject* Context,const TCHAR* Name,int32 Variants,const FVector& At,float Volume,float PitchMin=1.f,float PitchMax=1.f);
}

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
