#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ASData.h"
#include "ASArsenalRules.h"
#include "ASMeleeRules.h"
#include "ASExpeditionRules.h"
#include "ASAnimationRules.h"
#include "ASPartCRules.h"
#include "ASCharacter.generated.h"
class USpringArmComponent;class UCameraComponent;class UStaticMeshComponent;class UASMovementComponent;class AASEnemy;class UAnimMontage;
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
    virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override; // drops a movement base no client can resolve (ASNetBase.h)
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(ReplicatedUsing=RefreshLook) FASBuild Build;
    UPROPERTY(Replicated) int32 RelicMask=0;
    UPROPERTY(Replicated) TArray<int32> RunRanks;
    // Per-run reward log (counts by ASExpedition::RunGain code), reset on a fresh run; read by the tab menu.
    UPROPERTY(Replicated) TArray<int32> RunGains;
    void AddRunGain(int32 Code) {if(HasAuthority()&&ASExpedition::RunGainValid(Code)) {RunGains.SetNumZeroed(ASExpedition::RunGainCount);++RunGains[Code];ForceNetUpdate();}}
    int32 RunGain(int32 Code) const {return RunGains.IsValidIndex(Code)?RunGains[Code]:0;}
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
    // True while the server sees this character dashing; lets every remote machine draw the dash trail. Skipped for the owner.
    UPROPERTY(Replicated) bool bDashActive=false;
    UPROPERTY(Replicated) float HammerLeapStarted=-10;
    UPROPERTY(Replicated) float HammerLandedAt=-10;
    UPROPERTY(Replicated) float NextHammerLeap=0;
    UFUNCTION(Server,Reliable) void RequestHammerLeap();
    UFUNCTION(NetMulticast,Reliable) void HammerMotion(bool Launch,FVector_NetQuantizeNormal Direction,float PlanDistance,float PlanHeight);
    UFUNCTION(NetMulticast,Reliable) void HammerCue(uint8 Kind,bool Contact,FVector_NetQuantize At,FVector_NetQuantizeNormal Direction);
    bool HammerCommitted() const;
    bool HammerBusy() const {return Build.Weapon==1&&(HammerCommitted()||HammerLeapState!=0);}
    bool HammerInvulnerable() const;
    bool HammerPhasing() const {return Build.Weapon==1&&HammerLeapState==2&&!bDowned;}
    UPROPERTY(ReplicatedUsing=OnAttackPose) int32 AttackSequence=0;
    UPROPERTY(ReplicatedUsing=RefreshLook) int32 ColorIndex=0;
    UPROPERTY(ReplicatedUsing=RefreshLook) int32 ColorChoice=-1;
    FLinearColor BodyColor() const {return ASData::ArmorColor(ColorChoice,ColorIndex);}
    // 0.23.5: no friend/foe hue tint any more; every player shows their own colour (the HUD triangle marks allies).
    FLinearColor ViewColor() const {return BodyColor();}
    // The colour this machine draws the character's own light in (the rig's visor and core, the Mannequin's paint, the dash afterimages): the chosen colour, or, with none chosen,
    // azure for the player's own character on its own screen. 0.24.2: one rule for all three (the afterimages used ViewColor and came out orange beside a blue hero).
    FLinearColor OwnColor() const {return ColorChoice<0&&IsLocallyControlled()?FLinearColor(.08f,.55f,1.f):ViewColor();}
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
    // 0.24 Part C (server only): C-3 perfect dash, C-1 proc chains, C-6 metamorphoses.
    float DashInvulEnd=0,LastPerfectDashAt=-100.f,NextProcChain=0,PerfectBoostUntil=0; // PerfectBoostUntil: the perfect-dash damage boost lasts until then
    void OnPerfectDash();
    void ProcEcho(AASEnemy* From,EASHitKind Kind,float Power,float Pool,int32 Depth,TSet<TWeakObjectPtr<AASEnemy>>& Visited); // an element proc jumps to a neighbour (ASPartC::ChainMaxDepth hops, weaker each time) Pool (0.24.2): the proc's hit-scaled damage, carried at the echo's strength
    bool Metamorphosed(ASPartC::Metamorphosis M) const; // the trait behind M is at its catalogue maximum
    void NoteMetamorphosis(ASPartC::Metamorphosis M);   // first time per run: banner + the run fact for the death screen
    int32 WildfireDepth=0;                               // recursion guard: explosions that kill burning enemies set off more
    uint8 MetamorphosisAnnounced=0;                      // bit per metamorphosis already announced this run
    void HitProcs(AASEnemy* Enemy,const FVector& At,float Dealt);
    float DamageScale(bool WeaponHit) const;
    float AreaScale() const {return 1.f+.15f*TraitValue(ASBuild::Trait::Area);}
    float MaxFocus() const {return 60.f+10.f*TraitValue(ASBuild::Trait::ManaReserve);}
    float CriticalPower() const {return ASBuild::CriticalMultiplier(TraitValue(ASBuild::Trait::Deadeye));}
    float ElementalDamageMultiplier() const {return ASBuild::ElementalDamageMultiplier(TraitValue(ASBuild::Trait::Attunement));}
    float MeleeReachMultiplier() const {return ASBuild::MeleeReachMultiplier(TraitValue(ASBuild::Trait::WideArc));}
    float OnHitScale() const; // 0.24: the equipped weapon's on-hit reward scale (its BASE swing/shot interval against a reference; see ASArsenal::OnHitScale)
    int32 TeamworkRank() const {return TraitValue(ASBuild::Trait::Teamwork);}
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
    float MeleeReachFor(int32 Step) const; // the swing's reach in cm for a combo step, Wide Arc included (also read by the HUD's in-reach crosshair)
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
    // Meshes that make up this character right now (body parts, cape, weapon); the dash trail copies them as afterimages.
    void CollectTrailMeshes(TArray<UStaticMeshComponent*>& Out) const;
    bool IsSkeletalHero() const {return bSkeletalHero;} // 0.24.2 Phase F: drawn as the Unreal Mannequin (ASCharacterSkeletal.cpp)
private:
    // 0.24.2 Phase F: the hero as the Unreal Mannequin; the 21-part rig stays as the fallback (as.hero.skeletal 0, or the assets missing on this machine).
    bool bSkeletalHero=false,bSkelDashSeen=false,bSkelDownedSeen=false;uint8 SkelLeapSeen=0;float SkelReloadSeen=0.f;
    TWeakObjectPtr<UAnimMontage> SkelStance;int32 SkelStanceWeapon=-1;FString SkelStanceClip; // the weapon's stance looping on the upper body between moves, the weapon it is for, and the clip
    void SetupSkeletalHero();
    bool SkeletalMovePlaying() const; // a move (an attack, a dash, a flinch, a cast, the fall) holds the slots, not the stance
    void AnimateSkeletal(float Delta,bool Dashing,bool Airborne);
    UAnimMontage* PlayHeroClip(const TCHAR* Clip,FName Slot,float Seconds,float BlendIn=.06f,float BlendOut=.14f); // the clip as a one-off montage on that slot, stretched over Seconds
    void PlayAttackClip();
    void PlayHitClip(const FVector& Direction,float Strength);
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
    float NextSplashFx=0,NextChainFx=0,NextAura=0,NextOrbit=0,NextThorns=0; // 0.24: the on-hit reward cooldowns are gone; the first two only throttle visuals
    bool bAttackHeld=false;
    bool bMeleePending=false,bMeleeMotionStarted=false,bMeleeCritical=false;
    int32 NextMeleeStep=0,PendingMeleeWeapon=-1,PendingMeleePhase=-1,PendingMeleeGeneration=-1;
    float MeleeImpactAt=0,MeleeMotionAt=0,LastMeleeEnd=-10,HammerComboWindow=1.2f;
    FVector HammerLeapDirection=FVector::ForwardVector;
    FVector2D HammerLeapPlan=FVector2D::ZeroVector; // planned arc (distance, height) toward the crosshair point; sent with the launch
    float HammerShakeAge=10,HammerShakeStrength=0;
    void TickHammer(float Now);
    void CancelHammerLeap();
    void ResolveHammerLanding(const FVector& At);
    void AnimateHammer(float Now,ASHammer::Pose& OutPose,ASHammer::Frame& OutFrame) const;
    float PoseAge=10,WalkPhase=0,ResonanceAt=0,AllyHealAt=0;
    int32 LastStepVariant=-1;                         // 0.25 phase E: the footstep sound played last (ASFoleyRules.h)
    void PlayFootstep(float Speed);                   // a step sound for this hero, at the volume its distance from the local hero allows
    void PlayFoley(const TCHAR* Prefix,int32 Count,float Volume); // 0.24.2: one clip of a foley set (landings, jumps), distance-scaled like the steps
    float FallSpeed=0.f;int32 LastFoleyVariant=-1;    // 0.24.2: the fastest downward speed of the current fall (a landing's loudness)
    float LocomotionWeight=0,LandingAge=10,AirWeight=0,CapeLean=0;
    bool bWasAirborne=false;
    FVector AnimationVelocity=FVector::ZeroVector,PreviousVisualVelocity=FVector::ZeroVector;
    FVector FootAnchors[2]={FVector::ZeroVector,FVector::ZeroVector};
    bool bFootPlanted[2]={false,false};
    float TurnVelocity=0,PreviousVisualYaw=0,CapeTurn=0,CapeLift=0;
    ASAnimation::CapeChain CapeSim;FVector CapeAccelLocal=FVector::ZeroVector;
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
    float NextDashGhost=0;int8 LastTintLocal=-1;
};
