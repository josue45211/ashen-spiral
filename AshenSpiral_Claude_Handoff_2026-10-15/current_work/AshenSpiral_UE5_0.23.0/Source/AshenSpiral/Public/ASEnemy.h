#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ASData.h"
#include "ASEnemy.generated.h"
class USceneComponent;class UStaticMeshComponent;class AASCharacter;
UCLASS() class ASHENSPIRAL_API AASEnemy : public ACharacter {
    GENERATED_BODY()
public:
    AASEnemy();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(ReplicatedUsing=RefreshLook) int32 Kind=0;
    UPROPERTY(Replicated) int32 Ward=0;
    UPROPERTY(ReplicatedUsing=RefreshLook) int32 Theme=0;
    UPROPERTY(Replicated) float Health=70;
    UPROPERTY(Replicated) float MaxHealth=70;
    UPROPERTY(ReplicatedUsing=RefreshLook) bool bTrainingDummy=false;
    UPROPERTY(Replicated) float LastDamageTime=-100.f;
    UPROPERTY(Replicated) uint8 StatusBits=0; // status is shown by TickStatusTint (a body tint pulse); there is no ring component any more
    UPROPERTY(Replicated) float WindupEnd=0;
    UPROPERTY(Replicated) float WindupStart=0;
    UPROPERTY(Replicated) FVector Aim=FVector::ForwardVector;
    UPROPERTY(Replicated) bool bWinding=false;
    UPROPERTY(Replicated) uint8 Ability=0; // ASEnemyRules::Ability*: which attack is being wound up (0 = none), so every machine poses the right wind-up
    float SlowUntil=0,PulledUntil=0,MarkUntil=0,StunUntil=0,RootUntil=0,StunImmuneUntil=0,RootImmuneUntil=0;
    float LastHitTime=-10,NextDamageNumber=0;
    TWeakObjectPtr<AASCharacter> LastHitter;
    TWeakObjectPtr<AASEnemy> Summoner; // server only: the Broodcaller that spawned this minion (if any)
    bool Hit(float Damage,AASCharacter* Source);
    UPROPERTY(Replicated) float SpawnProtectedUntil=0;
    float FirstEngagedAt=-1;
    bool SpawnProtected() const;
    void RegisterCoopElement(EASHitKind Type,AASCharacter* Source,float Power);

    UPROPERTY(Replicated) float AttackReleaseTime=-100.f;
    UPROPERTY(Replicated) float HitReactionTime=-100.f;
    UPROPERTY(Replicated) float HitReactionStrength=0.f;
    UPROPERTY(Replicated) FVector_NetQuantizeNormal HitReactionDirection=FVector::BackwardVector;
    void BeginHitReaction(const FVector& SourceLocation,bool Heavy);
    void ApplyElement(EASHitKind Type,AASCharacter* Source,float Power);
    void Control(int32 Type,const FVector& Direction,float Strength);
    void EndWindup(); // authority: clears bWinding on every path that abandons an attack, so the ground warning can never outlive the attack
    // 0.23.5: the Endless flying boss is a normal boss (Kind 4) that flies; the Gloomwing is Kind 10.
    UPROPERTY(ReplicatedUsing=RefreshLook) bool bAirborne=false;
    int32 DiveCycle=0;
    bool Flies() const {return !bTrainingDummy&&(Kind==10||bAirborne);}
    bool Boss() const {return !bTrainingDummy&&Kind==4;}
    bool Elite() const {return !bTrainingDummy&&(Kind==3||Kind==4);}
    UFUNCTION() void RefreshLook();
private:
    UPROPERTY() TObjectPtr<USceneComponent> ArmorFigure;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ArmorParts;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ArmorWeapon;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ArmorShield;
    float GaitPhase=0,VisualMovementWeight=0,VisualTurn=0,PreviousVisualYaw=0;
    FVector VisualVelocity=FVector::ZeroVector;
    FVector FootAnchors[2]={FVector::ZeroVector,FVector::ZeroVector};
    bool bFootPlanted[2]={false,false};
    bool bVisualInitialized=false;
    void AnimateArmor(float Delta,float Windup);
    void AnimateFlightPose(float Delta,float Windup); // the armoured flying boss: legs trail, torso leans into its motion, arms rise with the wind-up (no walking gait in the air)
    void AnimateDeath();
    void AnimateFlier();
    void ConfineFlight(float CapsuleHalfHeight); // authority: keep a flier inside the arena (sides, floor, ceiling); fliers have no world collision
    void Molt(); // authority: the flying boss's half-health scream; bursts out hatchlings from its wings
    bool bMolted=false;
    bool ShowsStrikeCircle() const;
    void TickStatusTint();
    bool bTintApplied=false;
    TArray<FLinearColor> BaseTints;
    void TickFlier(float Delta,AASCharacter* Target,float DistanceSquared);
    FVector DiveDirection=FVector::ForwardVector,DiveTarget=FVector::ZeroVector;float DiveUntil=0,OrbitAngle=0,OrbitSign=1;
    float DeathSeenAt=-1.f;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Eye;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> WingL; // flier wings (membrane); hidden on every other kind
    UPROPERTY() TObjectPtr<UStaticMeshComponent> WingR;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> WingBoneL; // the wing's bone frame, a second mesh so it can take its own colour; flaps with the membrane
    UPROPERTY() TObjectPtr<UStaticMeshComponent> WingBoneR;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Tell;     // the melee ground warning: a dim half disc at the true strike radius
    UPROPERTY() TObjectPtr<UStaticMeshComponent> TellFill; // ... and a bright half disc that grows over it as the wind-up completes
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Crown;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Beam;
    ASRules::ElementState Elements;
    TWeakObjectPtr<AASCharacter> SynergySources[9];
    float SynergyTimes[9]={-100,-100,-100,-100,-100,-100,-100,-100,-100};
    float NextSynergy=0;

    TWeakObjectPtr<AASCharacter> BurnSource,PoisonSource,BleedSource;
    float BurnPending=0,PoisonPending=0,BleedPending=0,NextStatusDamage=0;
    bool TickElements(float Delta);
    float NextAttack=0,NextRoute=0,ChargeUntil=0,LastContact=0,NextStuckCheck=0,StuckFor=0,NextSummon=0;
    float LeapUntil=0,BodyScale=1;
    FVector LastRoutePosition=FVector::ZeroVector;
    FVector MoveGoal=FVector::ZeroVector;
    int32 Pattern=0;
    void CommitAttack();
};
