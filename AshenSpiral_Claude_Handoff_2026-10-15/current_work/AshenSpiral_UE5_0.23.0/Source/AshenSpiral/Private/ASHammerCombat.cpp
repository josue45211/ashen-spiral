#include "ASCharacter.h"
#include "ASMovementComponent.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "ASEnemy.h"
#include "ASBreakable.h"
#include "ASSpell.h"
#include "GameFramework/RootMotionSource.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

bool AASCharacter::HammerCommitted() const {
    return !bDowned&&ASHammer::Committed(Build.Weapon,MeleeStep,AS::Time(this),AttackPoseStarted,AttackPoseDuration);
}
bool AASCharacter::HammerInvulnerable() const {
    return Build.Weapon==1&&!bDowned&&ASHammer::LeapProtected(HammerLeapState,AS::Time(this),HammerLandedAt);
}
void AASCharacter::AnimateHammer(float Now,ASHammer::Pose& P,ASHammer::Frame& F) const {
    if(HammerLeapState) {
        const float Duration=HammerLeapState==1?ASHammer::Startup:(HammerLeapState==2?ASHammer::Flight:ASHammer::Recovery);
        const float Start=HammerLeapState==3?HammerLandedAt:HammerLeapStarted+(HammerLeapState==2?ASHammer::Startup:0.f);
        const float T=ASMelee::Phase(Now,Start,Duration);P=ASHammer::Leap(HammerLeapState,T);F=ASHammer::LeapFrame(HammerLeapState,T);return;
    }
    const bool Active=AttackPoseStarted>=0&&Now<AttackPoseStarted+AttackPoseDuration;
    const float T=ASHammer::PosePhase(MeleeStep,ASMelee::Phase(Now,AttackPoseStarted,AttackPoseDuration),bHammerContact);
    P=Active?ASHammer::Swing(MeleeStep,T):ASHammer::Hold();
    F=Active?ASHammer::SwingFrame(MeleeStep,T):ASHammer::Orientation(P,P,P);
}
void AASCharacter::CancelHammerLeap() {
    const bool Active=HammerLeapState!=0;
    HammerLeapState=0;HammerLeapStarted=HammerLandedAt=-10;
    Movement()->RemoveRootMotionSource(FName(TEXT("HammerLeap")));Movement()->RefreshDashCollision();
    if(HasAuthority()&&Active) {HammerMotion(false,FVector::ForwardVector);ForceNetUpdate();}
}
void AASCharacter::RequestHammerLeap_Implementation() {
    auto* S=AS::State(this);auto* M=Movement();
    if(!S||Build.Weapon!=1||!CanUseWeapons()||HammerBusy()||bReviving||S->SimTime<NextHammerLeap
        ||!M->IsMovingOnGround()||M->DashRemaining>0) return;
    CancelMelee(true);bCharging=false;StopJumping();
    HammerLeapDirection=(AimPoint-GetActorLocation()).GetSafeNormal2D();if(HammerLeapDirection.IsNearlyZero()) HammerLeapDirection=GetActorForwardVector();
    HammerLeapState=1;HammerLeapStarted=S->SimTime;HammerLandedAt=-10;NextHammerLeap=S->SimTime+ASHammer::Cooldown;
    PendingMeleePhase=int32(S->Phase);PendingMeleeGeneration=S->Arena?S->Arena->Config.Epoch:-1;
    M->StopMovementImmediately();ForceNetUpdate();
}
void AASCharacter::HammerMotion_Implementation(bool Launch,FVector_NetQuantizeNormal Direction) {
    auto* M=Movement();M->RemoveRootMotionSource(FName(TEXT("HammerLeap")));
    if(!Launch) {M->RefreshDashCollision();return;}
    if(!HasAuthority()&&!IsLocallyControlled()) return;
    if(bDowned||Build.Weapon!=1) return;
    // Collision is swept by CharacterMovement; walls/ceilings remain solid.
    auto Leap=MakeShared<FRootMotionSource_JumpForce>();Leap->InstanceName=FName(TEXT("HammerLeap"));
    Leap->Priority=600;Leap->AccumulateMode=ERootMotionAccumulateMode::Override;Leap->Rotation=FVector(Direction).Rotation();
    Leap->Distance=ASHammer::LeapDistance(DashDuration());Leap->Height=ASHammer::Height;
    Leap->Duration=ASHammer::Flight;Leap->bDisableTimeout=false;
    Leap->FinishVelocityParams.Mode=ERootMotionFinishVelocityMode::SetVelocity;
    Leap->FinishVelocityParams.SetVelocity=FVector(0,0,-100);
    M->StopMovementImmediately();M->SetMovementMode(MOVE_Falling);M->ApplyRootMotionSource(Leap);M->RefreshDashCollision();
}
void AASCharacter::TickHammer(float Now) {
    if(Build.Weapon!=1&&HammerLeapState) {CancelHammerLeap();return;}
    const auto* S=AS::State(this);
    const bool Active=HammerLeapState||HammerCommitted()||bMeleePending;
    if(Build.Weapon==1&&Active&&(!S||!CanUseWeapons()||!S->Arena||PendingMeleePhase!=int32(S->Phase)||!ASMelee::ContextValid(PendingMeleePhase,int32(S->Phase),PendingMeleeGeneration,S->Arena->Config.Epoch))) {
        CancelMelee(true);CancelHammerLeap();return;
    }
    if(HammerLeapState==1&&Now>=HammerLeapStarted+ASHammer::Startup) {
        // Frame 2: launch. Frame 1's crouch receives normal incoming damage.
        HammerLeapState=2;HammerMotion(true,HammerLeapDirection);HammerCue(4,false,GetActorLocation(),HammerLeapDirection);ForceNetUpdate();
    } else if(HammerLeapState==2) {
        if(Movement()->IsMovingOnGround()&&Now>HammerLeapStarted+ASHammer::Startup+.06f) {
            // Recovery for a missed Landed callback (e.g. correction), still requires actual floor contact.
            FHitResult Hit;Hit.ImpactPoint=GetActorLocation()-FVector(0,0,GetCapsuleComponent()->GetScaledCapsuleHalfHeight());Landed(Hit);
        } else if(Now>HammerLeapStarted+ASHammer::Startup+ASHammer::Flight+2.f) {
            // A missed platform must never grant permanent flight immunity or phantom landing damage.
            CancelHammerLeap();
        }
    } else if(HammerLeapState==3&&Now>=HammerLandedAt+ASHammer::Recovery) {HammerLeapState=0;ForceNetUpdate();}
}
void AASCharacter::Landed(const FHitResult& Hit) {
    Super::Landed(Hit);
    if(!HasAuthority()||Build.Weapon!=1||HammerLeapState!=2||bDowned) return;
    auto* S=AS::State(this);
    if(!S||!CanUseWeapons()||!S->Arena||PendingMeleePhase!=int32(S->Phase)||!ASMelee::ContextValid(PendingMeleePhase,int32(S->Phase),PendingMeleeGeneration,S->Arena->Config.Epoch)) {CancelHammerLeap();return;}
    HammerLeapState=3;HammerLandedAt=S->SimTime;NextAttack=FMath::Max(NextAttack,HammerLandedAt+ASHammer::Recovery);
    HammerMotion(false,HammerLeapDirection);Movement()->StopMovementImmediately();Movement()->RefreshDashCollision();
    ResolveHammerLanding(Hit.ImpactPoint);ForceNetUpdate();
}
void AASCharacter::ResolveHammerLanding(const FVector& At) {
    auto* S=AS::State(this);if(!HasAuthority()||!S||!CanUseWeapons()) return;
    bool Contact=false;const auto Targets=S->Enemies;
    for(auto W:Targets) if(auto* E=W.Get()) if(E->Health>0&&CanDamage(E)&&FMath::Abs(E->GetActorLocation().Z-At.Z)<230.f
        &&FVector::DistSquared2D(At,E->GetActorLocation())<FMath::Square(ASHammer::LandingRadius+E->GetCapsuleComponent()->GetScaledCapsuleRadius())
        &&AS::ClearSight(GetWorld(),At+FVector(0,0,55),E->GetActorLocation())) {
        Contact=true;HitEnemy(E,ASHammer::LandingDamage,false,EASHitKind::Physical,true);
        if(IsValid(E)&&E->Health>0) E->Control(0,E->GetActorLocation()-At,700.f);
    }
    const auto Props=S->Breakables;
    for(auto W:Props) if(auto* B=W.Get()) if(FVector::DistSquared2D(At,B->GetActorLocation())<FMath::Square(ASHammer::LandingRadius)
        &&FMath::Abs(B->GetActorLocation().Z-At.Z)<180.f&&B->VisibleFrom(At+FVector(0,0,55))) B->Strike(this,ASHammer::LandingDamage*DamageScale(true));
    HammerCue(3,Contact,At,HammerLeapDirection);
}
void AASCharacter::HammerCue_Implementation(uint8 Kind,bool Contact,FVector_NetQuantize At,FVector_NetQuantizeNormal Direction) {
    if(GetNetMode()==NM_DedicatedServer) return;
    if(Kind<3) bHammerContact=Contact;
    if(Kind<4&&IsLocallyControlled()&&(Contact||Kind>=2)) {HammerShakeAge=0;HammerShakeStrength=Kind>=2?3.2f:.9f;}
    const TCHAR* Name=Kind==4?TEXT("S_HammerLeap"):(Kind>=2?TEXT("S_HammerSmash"):(Contact?TEXT("S_HammerHit"):TEXT("S_HammerSwing")));
    if(auto* Sound=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),Name,Name)))
        UGameplayStatics::PlaySoundAtLocation(this,Sound,At,Kind>=2?.65f:.42f,Kind==1?.94f:1.f);
    if(Kind==2||Kind==3) if(auto* FX=GetWorld()->SpawnActor<AASHammerDebris>()) FX->Setup(At,Direction,BodyColor(),Kind==3);
}
