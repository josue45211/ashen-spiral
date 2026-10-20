#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ASData.h"
#include "ASEnemyRules.h"
#include "ASEnemy.generated.h"
class USceneComponent;class UStaticMeshComponent;class AASCharacter;class USoundBase;
namespace ASEnemyPose {struct RigOut;}
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
    UPROPERTY(Replicated) uint8 Ability=0; // ASEnemyRules::Ability*: which attack is being wound up, struck or recovered from (0 = none), so every machine poses the right body
    UPROPERTY(Replicated) uint8 AIState=0; // ASEnemyRules::State*: approach, strafe, windup, attack, recover, retreat, special. The server's state machine; every machine poses from it
    UPROPERTY(Replicated) uint8 Flags=0;   // ASEnemyRules::Flag*: what every machine shows about an enemy beyond its state (enraged tint, a parry's block flash)
    UPROPERTY(Replicated) uint8 Mutation=0; // ASEnemyRules::Mutation*: an elite brute's elemental mutation (0 = none): its tint, its signature ability, a burning one's fire trail
    float SlowUntil=0,PulledUntil=0,MarkUntil=0,StunUntil=0,RootUntil=0,StunImmuneUntil=0,RootImmuneUntil=0;
    float LastHitTime=-10,NextDamageNumber=0;
    TWeakObjectPtr<AASCharacter> LastHitter;
    TWeakObjectPtr<AASEnemy> Summoner; // server only: the Broodcaller that spawned this minion (if any)
    bool Hit(float Damage,AASCharacter* Source,bool bDirect=true); // bDirect: a blow, not status damage ticking in (a parry guard holds only against blows)
    UPROPERTY(Replicated) float SpawnProtectedUntil=0;
    float FirstEngagedAt=-1;
    bool SpawnProtected() const;
    void RegisterCoopElement(EASHitKind Type,AASCharacter* Source,float Power);

    UPROPERTY(Replicated) float AttackReleaseTime=-100.f;
    UPROPERTY(Replicated) float HitReactionTime=-100.f;
    UPROPERTY(Replicated) float HitReactionStrength=0.f;
    UPROPERTY(Replicated) FVector_NetQuantizeNormal HitReactionDirection=FVector::BackwardVector;
    void BeginHitReaction(const FVector& SourceLocation,bool Heavy);
    // Buildup scales the frost added per application (0.24 on-hit reward scaling); Pool (0.24.2) is damage the proc delivers over the element's time (ASRules::ElementShare of the hit)
    void ApplyElement(EASHitKind Type,AASCharacter* Source,float Power,float Buildup=1.f,float Pool=0.f);
    // Generation: 0 = launched by a hero, 1+ = thrown on by another launched enemy (ASPartC::ImpactMaxGeneration), -1 = not a launch (a gravity pull). Source: the hero behind a launch
    // (0.24.2: Repulse deals no damage, so the old "last hitter" guess left its launches uncredited and their slams harmless).
    void Control(int32 Type,const FVector& Direction,float Strength,int32 Generation=0,AASCharacter* Source=nullptr);
    // 0.24 C-2 / 0.24.2 F1-F3 (server only): a launch is a short hop and a skid (ASPartC::LaunchFor) during which the AI is suspended (Launched); a hard one also makes the body a
    // projectile: every tick sweeps ahead along its flight for a wall (WallSlam) or another enemy (PileInto), and NotifyHit is a second opinion.
    bool bKnocked=false;float KnockStart=-10.f,KnockStrength=0.f,KnockSpeed=0.f,KnockUntil=-10.f,NextImpactAt=0.f;int32 KnockGeneration=0;TWeakObjectPtr<AASCharacter> KnockSource;
    FVector KnockVelocity=FVector::ZeroVector; // the horizontal velocity at the last tick: the speed a body had before something stopped it
    bool bSkidFriction=false;float BaseFriction=8.f; // the walker's own ground friction comes back when the skid ends
    bool Launched(float Now) const {return Now<KnockUntil;}
    void TickKnock(float Now,float Delta);
    void WallSlam(const FVector& Normal,const FVector& At,float Strength,AActor* Struck);
    void PileInto(AASEnemy* Other,float Strength,const FVector& Heading);
    void ImpactHurt(AASEnemy* Victim,float Strength,int32 Generation); // impact damage credited to the hero who launched it (nothing without one), then counted for the death screen
    virtual void NotifyHit(UPrimitiveComponent* MyComp,AActor* Other,UPrimitiveComponent* OtherComp,bool bSelfMoved,FVector HitLocation,FVector HitNormal,FVector NormalImpulse,const FHitResult& Hit) override;
    void EndWindup(); // authority: abandons whatever attack is in progress (windup, committed motion, recovery) and returns to moving, so the ground warning can never outlive the attack
    // 0.23.5: the Endless flying boss is a normal boss (Kind 4) that flies; the Gloomwing is Kind 10.
    UPROPERTY(ReplicatedUsing=RefreshLook) bool bAirborne=false;
    int32 DiveCycle=0;
    bool Flies() const {return !bTrainingDummy&&(Kind==10||bAirborne);}
    bool Boss() const {return !bTrainingDummy&&Kind==4;}
    bool Elite() const {return !bTrainingDummy&&(Kind==3||Kind==4);}
    UFUNCTION() void RefreshLook();
    // 0.24 sound cues (cosmetic, every machine, no network traffic): ASEnemyRules::Cue*, played from the replicated state; see ASEnemyCues.cpp.
    void PlayCue(int32 CueId);                                               // the delivered file if it exists, else its fallback, at the distance-scaled volume
    static USoundBase* ResolveCue(int32 CueId,float& Volume,float& Pitch);   // the sound a cue plays right now (null when there is none)
private:
    void UpdateCues(float Now);
    int32 SeenState=0,SeenAbility=0;float SeenRelease=-100.f;
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
    void ApplyRig(const ASEnemyPose::RigOut& Out,float HalfHeight,float Fade); // puts a solved pose on the armour components, the weapon, the shield and the figure (Fade scales the whole figure)
    void AnimateBlob();         // 0.24: the leaper and the broodcaller (one body) squash, stretch and tip from the same ability tracks as the armour rig
    void ThrownPose(float* D,const FVector& LocalVelocity,float Delta); // 0.24.2: adds the flung pose of a launched body (FlagLaunched) to the track's channels
    float ThrownAmount=0.f;
    float PoseTime(float Now);  // track time (ASEnemyPose::AbilityU) of the ability pose that is playing, or -1; remembers the ability after the replicated one resets
    int32 PoseAbility=0;
    int32 DeathVariant=-1;      // which baked Mannequin collapse this enemy plays (ASEnemyPose::DeathVariantFor), chosen when its death is first seen
    float AppliedGlow=-1.f;
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

    TWeakObjectPtr<AASCharacter> BurnSource,PoisonSource,BleedSource,FrostSource;
    float BurnPending=0,PoisonPending=0,BleedPending=0,FrostPending=0,NextStatusDamage=0;
    bool TickElements(float Delta);
    float NextAttack=0,NextRoute=0,ChargeUntil=0,LastContact=0,NextStuckCheck=0,StuckFor=0,NextSummon=0;
    float LeapUntil=0,BodyScale=1;
    FVector LastRoutePosition=FVector::ZeroVector;
    FVector MoveGoal=FVector::ZeroVector;
    // 0.24 fluid movement (authority only, nothing replicated; the gait reads the replicated velocity).
    ASEnemyRules::RetreatState Retreat;               // frail kinds fall back for a moment when badly hurt (bounded, paused, never for the last few)
    float NextClearProbe=0,ProbeOffsetDegrees=0,NudgeUntil=0;
    int32 ProbeMode=-1;bool bProbeBlocked=false,bDirectClear=false;
    FVector2D NudgeDirection=FVector2D::ZeroVector;   // a short sideways step out of a pin (ASEnemyRules::UnstickDirection)
    int32 Pattern=0;
    void CommitAttack();
    // 0.24 state machine (authority). Walkers run TickGround; a state is entered with SetAIState, which also keeps the replicated bWinding (telegraph on) in step.
    void TickGround(float Delta,AASCharacter* Target,float DistanceSquared);
    void SetAIState(int32 NewState);
    void BeginAbility(int32 AbilityId,const FVector& ToTarget); // Windup: pick the aim, start the telegraph
    void ReleaseAbility();                                       // end of the windup: the effect, then committed motion (Attack), a short Recover, or back to moving
    void FinishAttackPhase();                                    // after the effect or the committed motion: Recover if the ability has one, otherwise moving again
    float AbilityReadyAt[ASEnemyRules::AbilityCount]={};         // per-ability cooldowns (SimTime)
    int32 LastAbility=0;
    float RecoverUntil=0;
    bool bTraversalLeap=false;                                   // the leaper's jump across a gap: no recovery pause afterwards
    // 0.24 abilities with their own effect and flow (ASEnemyAbilities.cpp; rules and numbers in ASEnemyAbilities.h).
    enum EFlow : int32 {FlowNotMine,FlowFinish,FlowChained,FlowAttack}; // after an ability's effect: the old by-kind path / the recovery / another ability began / committed motion began
    int32 ReleaseOwnAbility(int32 AbilityId,const FVector& ToTarget);   // ReleaseAbility calls it first; FlowNotMine means CommitAttack handles the ability as before
    FVector SlamPointFor(int32 AbilityId,const FVector& Feet) const; // where a maul slam lands (the head of the maul), on the floor; the feet for any other ability
    void ShowTell(float Progress);                               // the ground warning of the ability being wound up: its shape, its radius, its fill
    TWeakObjectPtr<AASCharacter> CurrentTarget;                  // authority: the hero TickGround is working against this tick
    float HopUntil=0,RiposteAt=-1.f;                             // the archer's back-hop in flight; when the duelist's counter starts after a block
    FVector HopDirection=FVector::ZeroVector;
    bool bParryBlocked=false;
    int32 TellShape=-1,SeenFlags=0;                              // clients: which warning mesh is applied; the flag bits UpdateCues has seen
    // M6-a2: the bulwark's guard, the sniper's tracking beam and relocation, the broodcaller's eggs, the Gloomwing pack.
    float GuardBrokenUntil=0;                                    // authority: when a broken guard closes again
    FVector RelocateGoal=FVector::ZeroVector;float RelocateUntil=0; // authority: the sniper's new vantage point and when it gives up getting there
    bool bPackFollower=false;                                    // authority: this Gloomwing's next dive was set by another's call, so it does not call the pack itself
    bool bBeamLocked=false;                                      // clients: the sniper's beam is drawn in its locked look
    float NextTrail=0;bool bMutationCued=false;                  // a burning elite's next fire pool (authority); the mutation's appearance cue has played (clients)
    void StartRelocation();                                      // after a shot: pick a vantage point on a ring around the target
    void LayEggs();                                              // the broodcaller's summon: a fan of eggs that hatch into minions unless popped
    void CallPack(const FVector& TargetAt);                      // a Gloomwing about to dive sets the next dive of the idle ones near its target, one after another
    void ShowBeam(float Progress);                               // clients: amber while the sniper's beam tracks, white-amber and thicker once it has locked
    FVector ConfineToMap(const FVector& P,bool bFlier) const;       // a point pulled onto the map: inside the Endless arena's margin, or over walkable floor (rooms and corridors) in story rooms
    bool WardBossKit() const;                                    // a ground boss outside Endless wave 50+ fights with its ward's kit (the flying boss and the late Endless bosses keep the old boss attack)
};
