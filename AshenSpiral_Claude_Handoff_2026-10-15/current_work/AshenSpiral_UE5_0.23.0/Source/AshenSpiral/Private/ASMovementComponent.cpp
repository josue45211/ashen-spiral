#include "ASMovementComponent.h"
#include "ASCharacter.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "ASBuildRules.h"
#include "ASHammerRules.h"
#include "ASEnemy.h"
#include "Components/CapsuleComponent.h"
void UASMovementComponent::RefreshDashCollision() {
    auto* H=Cast<AASCharacter>(CharacterOwner);auto* S=AS::State(this);if(!H) return;
    const bool PhaseThrough=(DashRemaining>0&&!H->bDowned&&H->TraitValue(ASBuild::Trait::DashStrike)>0)||H->HammerPhasing();
    if(!PhaseThrough) {
        for(auto W:DashIgnoredActors) if(auto* E=Cast<AASEnemy>(W.Get())) {H->GetCapsuleComponent()->IgnoreActorWhenMoving(E,false);E->GetCapsuleComponent()->IgnoreActorWhenMoving(H,false);}
        DashIgnoredActors.Empty();return;
    }
    // Actor-specific ignores preserve walls, floors, props and ally collision.
    // Runs in movement prediction as well as ServerMove, before the swept move.
    if(S) for(auto W:S->Enemies) if(auto* E=W.Get()) if(!E->bTrainingDummy&&!DashIgnoredActors.Contains(E)) {
        DashIgnoredActors.Add(E);H->GetCapsuleComponent()->IgnoreActorWhenMoving(E,true);E->GetCapsuleComponent()->IgnoreActorWhenMoving(H,true);
    }
}
void UASMovementComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* TickFunction) {
    auto* S=AS::State(this);if(S && S->Frozen()) {Velocity=FVector::ZeroVector;return;}
    Super::TickComponent(Delta,Type,TickFunction);
}
void UASMovementComponent::PerformMovement(float Delta) {
    // ServerMove can enter here without TickComponent. Freeze both execution paths.
    if(auto* S=AS::State(this)) if(S->Frozen()) {Velocity=FVector::ZeroVector;return;}
    Super::PerformMovement(Delta);
}
float UASMovementComponent::GetMaxSpeed() const {
    const auto* Hero=Cast<AASCharacter>(CharacterOwner);auto* S=AS::State(this);
    if(Hero && (Hero->bDowned||Hero->HammerLeapState!=0)) return 0;
    if(S && S->Frozen()) return 0;
    // The hammer finisher no longer roots: movement continues at 20% speed (80% slow).
    const float Speed=(Hero?Hero->RunningSpeed():640.f)*(Hero&&Hero->HammerCommitted()?ASHammer::FinisherSpeedScale():1.f);
    if(S && S->Arena && S->Arena->InWater(CharacterOwner->GetActorLocation())) return Speed*.45f;
    return Speed;
}
void UASMovementComponent::UpdateCharacterStateBeforeMovement(float Delta) {
    Super::UpdateCharacterStateBeforeMovement(Delta);if(IsMovingOnGround()) AirDashesUsed=0;
    auto* H=Cast<AASCharacter>(CharacterOwner);auto* S=AS::State(this);
    const int32 Capacity=H?H->MaxDashCharges():1;
    const float Recharge=H?FMath::Max(.5f,1.1f-H->Rank(8)*.06f)*(H->Talent(0)?.85f:1.f):1.1f;
    if(Capacity>KnownDashCapacity) DashCharges=FMath::Min(Capacity,DashCharges+Capacity-KnownDashCapacity);
    KnownDashCapacity=Capacity;
    ASBuild::RefillDash(DashCharges,DashRecharge,Delta,Capacity,Recharge);DashCooldown=DashCharges>0?0.f:DashRecharge;
    if(H&&H->bDowned) {
        bWantsDash=false;DashRemaining=0;
        if(MovementMode==MOVE_Custom) SetMovementMode(MOVE_Falling);
        RefreshDashCollision();return;
    }
    if(H&&H->HammerLeapState!=0&&H->HammerLeapState!=2) {Velocity.X=0;Velocity.Y=0;}
    if(bWantsDash && H && !H->HammerBusy() && !H->bDowned && S && !S->Frozen() && ASRules::CanAirDash(IsMovingOnGround(),IsFalling(),AirDashesUsed,Capacity,DashCharges) && ASRules::CanUseWeapons(int(S->Phase))) {
        DashDirection=Acceleration.GetSafeNormal2D();if(DashDirection.IsNearlyZero()) DashDirection=H->GetActorForwardVector();
        DashDirection.Z=0;DashDirection.Normalize();DashRemaining=H->DashDuration();
        --DashCharges;if(DashRecharge<=0) DashRecharge=Recharge;DashCooldown=DashCharges>0?0.f:DashRecharge;
        bAirDashing=IsFalling();DashVerticalVelocity=bAirDashing?Velocity.Z:0.f;if(bAirDashing) ++AirDashesUsed;
        H->bReviving=false;SetMovementMode(MOVE_Custom,1);Velocity=DashDirection*2350.f+FVector(0,0,DashVerticalVelocity);H->OnDash();
    }
    bWantsDash=false;RefreshDashCollision();
}
void UASMovementComponent::PhysCustom(float Delta,int32 Iterations) {
    if(CustomMovementMode!=1) {Super::PhysCustom(Delta,Iterations);return;}
    RefreshDashCollision();
    const float Used=FMath::Min(Delta,DashRemaining);FHitResult Hit;
    const float Gravity=bAirDashing?GetGravityZ():0.f;
    const FVector Step=DashDirection*2350.f*Used+FVector(0,0,DashVerticalVelocity*Used+.5f*Gravity*Used*Used);
    SafeMoveUpdatedComponent(Step,UpdatedComponent->GetComponentQuat(),true,Hit);DashVerticalVelocity+=Gravity*Used;
    DashRemaining-=Used;
    if(Hit.IsValidBlockingHit() || DashRemaining<=0) {
        DashRemaining=0;Velocity=DashDirection*GetMaxSpeed()+FVector(0,0,DashVerticalVelocity);bAirDashing=false;SetMovementMode(MOVE_Falling);RefreshDashCollision();
        if(Delta>Used) StartNewPhysics(Delta-Used,Iterations+1);
    }
}
void UASMovementComponent::UpdateFromCompressedFlags(uint8 Flags) {Super::UpdateFromCompressedFlags(Flags);bWantsDash=(Flags&FSavedMove_Character::FLAG_Custom_0)!=0;}
FNetworkPredictionData_Client* UASMovementComponent::GetPredictionData_Client() const {
    if(!ClientPredictionData) {auto* Mutable=const_cast<UASMovementComponent*>(this);Mutable->ClientPredictionData=new FPredictionData_AS(*this);}
    return ClientPredictionData;
}
void FSavedMove_AS::Clear() {Super::Clear();bSavedDash=bSavedAirDashing=false;SavedAirDashesUsed=0;SavedCooldown=SavedRemaining=SavedVertical=0;SavedDirection=FVector::ForwardVector;SavedCharges=SavedCapacity=1;SavedRecharge=0;}
uint8 FSavedMove_AS::GetCompressedFlags() const {return Super::GetCompressedFlags()|(bSavedDash?FLAG_Custom_0:0);}
bool FSavedMove_AS::CanCombineWith(const FSavedMovePtr& M,ACharacter* C,float MaxDelta) const {
    const auto* Other=static_cast<const FSavedMove_AS*>(M.Get());
    if(SavedCharges!=Other->SavedCharges||SavedCapacity!=Other->SavedCapacity) return false;
    if(SavedAirDashesUsed!=Other->SavedAirDashesUsed||bSavedAirDashing!=Other->bSavedAirDashing) return false;
    if(bSavedDash || Other->bSavedDash || SavedRemaining>0 || Other->SavedRemaining>0) return false;
    return Super::CanCombineWith(M,C,MaxDelta);
}
void FSavedMove_AS::SetMoveFor(ACharacter* C,float Delta,const FVector& A,FNetworkPredictionData_Client_Character& Data) {
    Super::SetMoveFor(C,Delta,A,Data);const auto* M=Cast<UASMovementComponent>(C->GetCharacterMovement());
    SavedCharges=M->DashCharges;SavedCapacity=M->KnownDashCapacity;SavedRecharge=M->DashRecharge;
    bSavedDash=M->bWantsDash;SavedAirDashesUsed=M->AirDashesUsed;bSavedAirDashing=M->bAirDashing;SavedVertical=M->DashVerticalVelocity;SavedCooldown=M->DashCooldown;SavedRemaining=M->DashRemaining;SavedDirection=M->DashDirection;
}
void FSavedMove_AS::PrepMoveFor(ACharacter* C) {
    Super::PrepMoveFor(C);auto* M=Cast<UASMovementComponent>(C->GetCharacterMovement());
    M->DashCharges=SavedCharges;M->KnownDashCapacity=SavedCapacity;M->DashRecharge=SavedRecharge;
    M->bWantsDash=bSavedDash;M->AirDashesUsed=SavedAirDashesUsed;M->bAirDashing=bSavedAirDashing;M->DashVerticalVelocity=SavedVertical;M->DashCooldown=SavedCooldown;M->DashRemaining=SavedRemaining;M->DashDirection=SavedDirection;
}
