#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ASData.h"
#include "ASArsenalRules.h"
#include "ASMeleeRules.h"
#include "ASExpeditionRules.h"
#include "ASCharacter.generated.h"
class USpringArmComponent;class UCameraComponent;class UStaticMeshComponent;class UASMovementComponent;class AASEnemy;
UCLASS() class ASHENSPIRAL_API AASCharacter : public ACharacter {
    GENERATED_BODY()
public:
    AASCharacter(const FObjectInitializer& Init);
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float Delta) override;
    virtual void Landed(const FHitResult& Hit) override;
    virtual bool CanJumpInternal_Implementation() const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Boom;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(ReplicatedUsing=RefreshLook) FASBuild Build;
    UPROPERTY(Replicated) int32 RelicMask=0;
    UPROPERTY(Replicated) TArray<int32> RunRanks;
    UPROPERTY(Replicated) float Health=110;
    UPROPERTY(Replicated) float MaxHealth=110;
    UPROPERTY(Replicated) float Shield=0;
    UPROPERTY(Replicated) float ChargeStart=0;
    UPROPERTY(Replicated) float NextSpiral=0;
    UPROPERTY(Replicated) float NextAttack=0;
    float RotaryHeat=0;
    UPROPERTY(Replicated) int32 Ammo=9;
    UPROPERTY(Replicated) float ReloadStart=0;
    UPROPERTY(Replicated) float ReloadEnd=0;
    UFUNCTION(Server,Reliable) void RequestReload();
    UFUNCTION(NetMulticast,Reliable) void ReloadCue(bool Complete);
    UFUNCTION(NetMulticast,Unreliable) void DamageReaction(FVector_NetQuantizeNormal Direction,float Strength);
    UFUNCTION(NetMulticast,Unreliable) void AbilityGesture();
    UFUNCTION(NetMulticast,Reliable) void MeleeTravel(bool Slam,FVector_NetQuantize Destination,float Duration);
    int32 MagazineCapacity() const {return ASArsenal::Capacity(Build.Weapon,TraitRank(ASBuild::Trait::Magazine));}
    float ReloadDuration() const {return ASArsenal::ReloadSeconds(Build.Weapon,TraitRank(ASBuild::Trait::Reload));}
    float SpreadScale() const {return ASArsenal::SpreadScale(TraitRank(ASBuild::Trait::Precision));}
    bool IsReloading() const {return ReloadEnd>0;}
    float ReloadProgress() const;
    void ResetMagazine();
    UPROPERTY(Replicated) float InvulnerableUntil=0;
    UPROPERTY(Replicated) float ReviveProgress=0;
    UPROPERTY(Replicated) bool bCharging=false;
    UPROPERTY(ReplicatedUsing=RefreshLook) bool bDowned=false;
    UPROPERTY(Replicated) bool bReviving=false;
    UPROPERTY(Replicated) float AttackPoseDuration=.28f;
    UPROPERTY(Replicated) float AttackPoseStarted=-10;
    UPROPERTY(Replicated) int32 MeleeStep=0;
    UPROPERTY(Replicated) bool bHammerContact=false;
    UPROPERTY(Replicated) uint8 HammerLeapState=0;
    UPROPERTY(Replicated) float HammerLeapStarted=-10;
    UPROPERTY(Replicated) float HammerLandedAt=-10;
    UPROPERTY(Replicated) float NextHammerLeap=0;
    UFUNCTION(Server,Reliable) void RequestHammerLeap();
    UFUNCTION(NetMulticast,Reliable) void HammerMotion(bool Launch,FVector_NetQuantizeNormal Direction);
    UFUNCTION(NetMulticast,Reliable) void HammerCue(uint8 Kind,bool Contact,FVector_NetQuantize At,FVector_NetQuantizeNormal Direction);
    bool HammerCommitted() const;
    bool HammerBusy() const {return Build.Weapon==1&&(HammerCommitted()||HammerLeapState!=0);}
    bool HammerInvulnerable() const;
    bool HammerPhasing() const {return Build.Weapon==1&&HammerLeapState==2&&!bDowned;}
    UPROPERTY(ReplicatedUsing=OnAttackPose) int32 AttackSequence=0;
    UPROPERTY(ReplicatedUsing=RefreshLook) int32 ColorIndex=0;
    UPROPERTY(ReplicatedUsing=RefreshLook) int32 ColorChoice=-1;
    FLinearColor BodyColor() const {return ASData::ArmorColor(ColorChoice,ColorIndex);}
    UPROPERTY(Replicated) TArray<int32> TraitRanks;
    UPROPERTY(Replicated) uint64 EvolutionMask=0;
    UPROPERTY(Replicated) int32 LastUpgrade=-1;
    UPROPERTY(Replicated) int32 Rerolls=1;
    UPROPERTY(Replicated) int32 CurseStacks=0;
    UPROPERTY(Replicated) float Focus=60;
    UPROPERTY(Replicated) float NextRune=0;
    UPROPERTY(Replicated) int32 ConduitHits=0;
    UPROPERTY(Replicated) bool bPhoenixUsed=false;
    UPROPERTY(Replicated) bool bVengeanceUsed=false;
    UPROPERTY(Replicated) bool bVengeanceFinisherReady=false;
    UPROPERTY(Replicated) float BleedRemaining=25.f;
    UPROPERTY(Replicated) bool bEliminated=false;
    UPROPERTY(Replicated) TArray<int32> EndlessRanks;
    float EndlessValue(int32 Index) const {return EndlessRanks.IsValidIndex(Index)?ASExpedition::TraitPower(EndlessRanks[Index],5):0.f;}

    UPROPERTY(Replicated) float WarCryUntil=0;
    float CameraZoom=528.f;
    FVector MuzzleLocation() const;
    float LastDamageAt=-100,RoomBuffUntil=0,KillBuffUntil=0,StillSince=0,GoldRemainder=0;
    int32 TraitRank(ASBuild::Trait Type) const;
    int32 TraitValue(ASBuild::Trait Type) const;
    ASBuild::Context UpgradeContext() const;
    bool CanApplyChoice(int32 Choice) const;
    bool ApplyChoice(int32 Choice);
    int32 EvolutionTarget() const;
    void ResetTraits();
    void EnterRoomTraits(float CombatStarts);
    void TickTraits(float Delta);
    void OnKill(const class AASEnemy* Killed=nullptr);
    void OnCinderPickup();
    void SecondaryDamage(AASEnemy* Enemy,float Damage,EASHitKind Kind);
    void HitProcs(AASEnemy* Enemy,const FVector& At,float Dealt);
    float DamageScale(bool WeaponHit) const;
    float AreaScale() const {return 1.f+.15f*TraitValue(ASBuild::Trait::Area);}
    float MaxFocus() const {return 60.f+10.f*TraitValue(ASBuild::Trait::ManaReserve);}
    float CriticalPower() const {return ASBuild::CriticalMultiplier(TraitValue(ASBuild::Trait::Deadeye));}
    float PrimaryDamageBonus() const {return 4.f*TraitValue(ASBuild::Trait::Sharpened);}
    float DashDuration() const {return ASBuild::DashDuration(TraitValue(ASBuild::Trait::LongDash));}
    int32 MaxDashCharges() const {return ASBuild::MaxDashCharges(TraitValue(ASBuild::Trait::DashCharge));}
    void DownedPose(TArray<FVector>& Points) const;
    UFUNCTION(Server,Reliable) void CastRune();
    bool bMercyUsed=false,bSecondWindUsed=false,bNextCritical=false;
    float RallyUntil=0,GuardianUntil=0,UnbrokenUntil=0,NextAllyWellHeal=0;
    FVector LastSafe=FVector(0,0,100);
    void ApplyBuild(const FASBuild& Value,bool Fresh);
    void Hurt(float Amount,AASEnemy* Source=nullptr,bool EnemyAttack=false);
    void Heal(float Amount);
    void OnDash();
    void HitEnemy(AASEnemy* Enemy,float Amount,bool Critical=false,EASHitKind Kind=EASHitKind::Spiral,bool ApplyElement=false,bool AllowProcs=true);
    bool CanUseWeapons() const;
    bool CombatBenefits() const;
    bool CanDamage(const AASEnemy* Enemy) const;
    void Area(const FVector& Position,float Radius,float Damage,bool Critical=false);
    int32 Rank(int32 I) const {return Build.Rank(I);}
    int32 RunRank(int32 I) const {return RunRanks.IsValidIndex(I)?ASRules::UpgradeRank(RunRanks[I]):0;}
    float SpiralCooldown() const {return ASRules::SpiralCooldown(Rank(14),RunRank(0));}
    float RunningSpeed() const {return ASRules::MoveSpeed(Rank(18),RunRank(4));}
    bool CanApplyRunUpgrade(int32 I) const {return ASRules::CanUpgradeRunStat(I,RunRank(I),I==0?Rank(14):(I==5?Rank(19):0));}
    bool ApplyRunUpgrade(int32 I);
    bool CanRevive(const AASCharacter* Target) const;
    bool Relic(int32 I) const {return (RelicMask&(1<<I))!=0;}
    bool Talent(int32 I) const {return Build.Talent(I);}
    UASMovementComponent* Movement() const;
    UFUNCTION() void RefreshLook();
    UFUNCTION() void OnAttackPose();
    UFUNCTION(Server,Reliable) void SetAttack(bool Held);
    UFUNCTION(Server,Reliable) void SetSpiral(bool Held);
    UFUNCTION(Server,Unreliable) void SetAim(FVector_NetQuantize Position);
    UFUNCTION(Server,Reliable) void SetRevive(bool Held);
    void ResetInput(bool Force=false);
private:
    UPROPERTY() TObjectPtr<USceneComponent> Figure;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> WeaponMesh;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ChargeMesh;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ArrowMesh;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BowStrings;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Wisps;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> FlowingCape;
    void CreateCape();
    void RefreshCape();
    void AnimateCape(float Delta,float AttackFlare,float BodyTwist);
    TSet<TWeakObjectPtr<AASEnemy>> DashVictims;
    FVector DashTraceStart=FVector::ZeroVector;
    bool bDashTraceActive=false;
    float NextLeech=0,NextSplash=0,NextChain=0,NextShock=0,NextPull=0,NextConduit=0,NextAura=0,NextOrbit=0,NextThorns=0;
    bool bAttackHeld=false;
    bool bMeleePending=false,bMeleeMotionStarted=false,bMeleeCritical=false;
    int32 NextMeleeStep=0,PendingMeleeWeapon=-1,PendingMeleePhase=-1,PendingMeleeGeneration=-1;
    float MeleeImpactAt=0,MeleeMotionAt=0,LastMeleeEnd=-10,HammerComboWindow=1.2f;
    FVector HammerLeapDirection=FVector::ForwardVector;
    float HammerShakeAge=10,HammerShakeStrength=0;
    void TickHammer(float Now);
    void CancelHammerLeap();
    void ResolveHammerLanding(const FVector& At);
    void AnimateHammer(float Now,ASHammer::Pose& OutPose,ASHammer::Frame& OutFrame) const;
    float PoseAge=10,WalkPhase=0,ResonanceAt=0,AllyHealAt=0;
    float LocomotionWeight=0,LandingAge=10,AirWeight=0,CapeLean=0;
    bool bWasAirborne=false;
    FVector AnimationVelocity=FVector::ZeroVector,PreviousVisualVelocity=FVector::ZeroVector;
    FVector FootAnchors[2]={FVector::ZeroVector,FVector::ZeroVector};
    bool bFootPlanted[2]={false,false};
    float TurnVelocity=0,PreviousVisualYaw=0,CapeTurn=0;
    bool bVisualInitialized=false;
    float DamageReactionAge=10,DamageReactionStrength=0,AbilityGestureAge=10;
    FVector DamageReactionDirection=FVector::ZeroVector;
    FVector AimPoint=FVector(1000,0,100);
    void Attack();
    void ResolveMelee();
    void TickMelee(float Now);
    void CancelMelee(bool Force=false);
    void CastSpiral(bool Perfect);
    void Animate(float Delta);
};
