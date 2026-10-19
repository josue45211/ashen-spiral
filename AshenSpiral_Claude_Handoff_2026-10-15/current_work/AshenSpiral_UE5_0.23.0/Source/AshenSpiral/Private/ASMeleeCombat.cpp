#include "ASCharacter.h"
#include "ASMovementComponent.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "ASEnemy.h"
#include "ASBreakable.h"
#include "ASSpell.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/RootMotionSource.h"

void AASCharacter::CancelMelee(bool Force) {
    if(!Force&&HammerCommitted()) return;
    const bool Active=ASMelee::IsMelee(Build.Weapon)&&(bMeleePending||AttackPoseStarted>=0);
    bMeleePending=false;NextMeleeStep=0;PendingMeleeWeapon=-1;LastMeleeEnd=-10;
    if(ASMelee::IsMelee(Build.Weapon)) AttackPoseStarted=-10;
    Movement()->RemoveRootMotionSource(FName(TEXT("MeleeFinisher")));
    if(HasAuthority()&&Active) {MeleeTravel(false,GetActorLocation(),0);ForceNetUpdate();}
}
void AASCharacter::MeleeTravel_Implementation(bool Slam,FVector_NetQuantize Destination,float Duration) {
    // Root-motion sources participate in CharacterMovement's correction/replay.
    // Simulated proxies receive the replicated movement, not a second local force.
    if(!HasAuthority()&&!IsLocallyControlled()) return;
    auto* M=Movement();M->RemoveRootMotionSource(FName(TEXT("MeleeFinisher")));
    if(Duration<=0||bDowned||M->DashRemaining>0) return;
    if(Slam) return; // The 0.22 hammer finisher never launches the character.
    auto Source=MakeShared<FRootMotionSource_MoveToForce>();Source->StartLocation=GetActorLocation();
    Source->TargetLocation=Destination;Source->bRestrictSpeedToExpected=true;
    Source->InstanceName=FName(TEXT("MeleeFinisher"));Source->Priority=500;Source->Duration=Duration;
    Source->AccumulateMode=ERootMotionAccumulateMode::Override;
    Source->FinishVelocityParams.Mode=ERootMotionFinishVelocityMode::ClampVelocity;
    Source->FinishVelocityParams.ClampVelocity=RunningSpeed();M->ApplyRootMotionSource(Source);
}
void AASCharacter::TickMelee(float Now) {
    if(!bMeleePending) return;
    auto* S=AS::State(this);
    if(!S||!CanUseWeapons()||bReviving||PendingMeleeWeapon!=Build.Weapon
        ||!S->Arena||!ASMelee::ContextValid(PendingMeleePhase,int32(S->Phase),PendingMeleeGeneration,S->Arena->Config.Epoch)) {CancelMelee(true);return;}
    if(Build.Weapon==0&&MeleeStep==2&&!bMeleeMotionStarted&&Now>=MeleeMotionAt) {
        bMeleeMotionStarted=true;
        if(Now<MeleeImpactAt&&Movement()->IsMovingOnGround()&&Movement()->DashRemaining<=0) {
            FVector Forward=(AimPoint-GetActorLocation()).GetSafeNormal2D();if(Forward.IsNearlyZero()) Forward=GetActorForwardVector();
            const float Duration=FMath::Max(.025f,MeleeImpactAt-Now);
            const float Wanted=FMath::Min(160.f,Duration*1400.f);
            float Distance=0;
            // No automatic finisher carries the player across a ledge or wall.
            for(float D=10.f;D<=Wanted;D+=10.f) {
                if(!S->Arena->CanWalkDirectly(GetActorLocation(),GetActorLocation()+Forward*D)) break;
                Distance=D;
            }
            if(Distance>0) MeleeTravel(false,GetActorLocation()+Forward*Distance,Duration);
        }
    }
    if(ASMelee::ImpactDue(Now,MeleeImpactAt,bMeleePending)) {bMeleePending=false;ResolveMelee();}
}
float AASCharacter::MeleeReachFor(int32 Step) const {
    const bool Slam=Build.Weapon==1&&Step==2;
    return MeleeReachMultiplier()*(Slam?410.f:(Build.Weapon==5?360.f:(Build.Weapon==0&&Step==2?320.f:280.f)));
}
void AASCharacter::ResolveMelee() {
    auto* S=AS::State(this);if(!HasAuthority()||!S||!CanUseWeapons()||PendingMeleeWeapon!=Build.Weapon) return;
    const bool Slam=Build.Weapon==1&&MeleeStep==2;
    const float Reach=MeleeReachFor(MeleeStep);
    const float Oath=MeleeStep==2&&bVengeanceFinisherReady?ASBuild::VengeanceFinisherMultiplier:1.f;
    if(Oath>1.f) bVengeanceFinisherReady=false;
    const float Damage=Oath*ASArsenal::Damage(Build.Weapon,PrimaryDamageBonus())*ASMelee::DamageScale(Build.Weapon,MeleeStep);
    FVector Forward=(AimPoint-GetActorLocation()).GetSafeNormal2D();if(Forward.IsNearlyZero()) Forward=GetActorForwardVector();
    bool Contact=false,RangeTicked=false;
    const auto Targets=S->Enemies;
    for(auto W:Targets) if(auto* E=W.Get()) if(E->Health>0&&CanDamage(E)
        &&FMath::Abs(GetActorLocation().Z-E->GetActorLocation().Z)<230.f
        &&FVector::DistSquared2D(GetActorLocation(),E->GetActorLocation())<FMath::Square(Reach+E->GetCapsuleComponent()->GetScaledCapsuleRadius())
        &&(Build.Weapon==1?ASHammer::Frontal(FVector::DotProduct(Forward,(E->GetActorLocation()-GetActorLocation()).GetSafeNormal2D())):FVector::DotProduct(Forward,(E->GetActorLocation()-GetActorLocation()).GetSafeNormal2D())>-.05f)
        &&AS::ClearSight(GetWorld(),GetActorLocation(),E->GetActorLocation()))
        {
            Contact=true;
            if(!RangeTicked) {RangeTicked=true;S->Effect(16,E->GetActorLocation()-FVector(0,0,E->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-4.f),FVector::UpVector,FLinearColor::White,0.f);} // faint hit-confirm tick, once per swing
            HitEnemy(E,Damage+(Build.Weapon==0&&E->PulledUntil>S->SimTime?25.f:0.f),bMeleeCritical,ASData::WeaponElement(Build.Weapon),true);
            if(Build.Weapon==1&&IsValid(E)&&E->Health>0) E->Control(0,Forward,Slam?950.f:230.f);
        }
    const auto Props=S->Breakables;
    for(auto W:Props) if(auto* B=W.Get()) if(FVector::DistSquared2D(GetActorLocation(),B->GetActorLocation())<Reach*Reach
        &&FMath::Abs(GetActorLocation().Z-B->GetActorLocation().Z)<180.f
        &&(FVector::DotProduct(Forward,(B->GetActorLocation()-GetActorLocation()).GetSafeNormal2D())>0)
        &&B->VisibleFrom(GetActorLocation())) {Contact=true;B->Strike(this,Damage*DamageScale(true));}
    if(Build.Weapon==1) {bHammerContact=Contact;HammerCue(uint8(MeleeStep),Contact,GetActorLocation()+Forward*(Slam?100.f:115.f)-FVector(0,0,Slam?85.f:10.f),Forward);ForceNetUpdate();}
    else {
        // 0.24: Blade and Scythe draw a frontal crescent whose outer edge is exactly this swing's reach, so every swing teaches the real range.
        // The colour's alpha carries the combo step (0, 1, 2 = finisher) to the effect.
        FLinearColor Tint=BodyColor();Tint.A=float(FMath::Clamp(MeleeStep,0,2));
        S->Effect(17,GetActorLocation()-FVector(0,0,35),Forward,Tint,Reach);
    }
    // Hammer swings no longer plant or collapse gravity wells (removed 2026-10-16; may return reworked).
}
