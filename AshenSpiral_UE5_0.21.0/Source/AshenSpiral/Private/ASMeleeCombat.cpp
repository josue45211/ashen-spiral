#include "ASCharacter.h"
#include "ASMovementComponent.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "ASEnemy.h"
#include "ASBreakable.h"
#include "ASSpell.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/RootMotionSource.h"

void AASCharacter::CancelMelee() {
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
    TSharedPtr<FRootMotionSource> Source;
    if(Slam) {
        auto Hop=MakeShared<FRootMotionSource_JumpForce>();Hop->Rotation=(Destination-GetActorLocation()).Rotation();
        Hop->Distance=FVector::Dist2D(Destination,GetActorLocation());Hop->Height=85.f;Hop->bDisableTimeout=false;
        M->SetMovementMode(MOVE_Falling);Source=Hop;
    } else {
        auto Lunge=MakeShared<FRootMotionSource_MoveToForce>();Lunge->StartLocation=GetActorLocation();
        Lunge->TargetLocation=Destination;Lunge->bRestrictSpeedToExpected=true;Source=Lunge;
    }
    Source->InstanceName=FName(TEXT("MeleeFinisher"));Source->Priority=500;Source->Duration=Duration;
    Source->AccumulateMode=ERootMotionAccumulateMode::Override;
    Source->FinishVelocityParams.Mode=ERootMotionFinishVelocityMode::ClampVelocity;
    Source->FinishVelocityParams.ClampVelocity=RunningSpeed();M->ApplyRootMotionSource(Source);
}
void AASCharacter::TickMelee(float Now) {
    if(!bMeleePending) return;
    auto* S=AS::State(this);
    if(!S||!CanUseWeapons()||bReviving||PendingMeleeWeapon!=Build.Weapon||PendingMeleePhase!=int32(S->Phase)
        ||!S->Arena||PendingMeleeGeneration!=S->Arena->Config.Generation) {CancelMelee();return;}
    if(MeleeStep==2&&!bMeleeMotionStarted&&Now>=MeleeMotionAt) {
        bMeleeMotionStarted=true;
        if(Now<MeleeImpactAt&&Movement()->IsMovingOnGround()&&Movement()->DashRemaining<=0) {
            FVector Forward=(AimPoint-GetActorLocation()).GetSafeNormal2D();if(Forward.IsNearlyZero()) Forward=GetActorForwardVector();
            const float Duration=FMath::Max(.025f,MeleeImpactAt-Now);
            const float Wanted=Build.Weapon==1?60.f:FMath::Min(160.f,Duration*1400.f);
            float Distance=0;
            // No automatic finisher carries the player across a ledge or wall.
            for(float D=10.f;D<=Wanted;D+=10.f) {
                if(!S->Arena->CanWalkDirectly(GetActorLocation(),GetActorLocation()+Forward*D)) break;
                Distance=D;
            }
            if(Build.Weapon==1||Distance>0) MeleeTravel(Build.Weapon==1,GetActorLocation()+Forward*Distance,Duration);
        }
    }
    if(ASMelee::ImpactDue(Now,MeleeImpactAt,bMeleePending)) {bMeleePending=false;ResolveMelee();}
}
void AASCharacter::ResolveMelee() {
    auto* S=AS::State(this);if(!HasAuthority()||!S||!CanUseWeapons()||PendingMeleeWeapon!=Build.Weapon) return;
    const bool Slam=Build.Weapon==1&&MeleeStep==2;
    const float Reach=Slam?410.f:(Build.Weapon==5?360.f:(Build.Weapon==0&&MeleeStep==2?320.f:280.f));
    const float Damage=ASArsenal::Damage(Build.Weapon,PrimaryDamageBonus())*ASMelee::DamageScale(Build.Weapon,MeleeStep);
    FVector Forward=(AimPoint-GetActorLocation()).GetSafeNormal2D();if(Forward.IsNearlyZero()) Forward=GetActorForwardVector();
    const auto Targets=S->Enemies;
    for(auto W:Targets) if(auto* E=W.Get()) if(E->Health>0&&CanDamage(E)
        &&FMath::Abs(GetActorLocation().Z-E->GetActorLocation().Z)<230.f
        &&FVector::DistSquared2D(GetActorLocation(),E->GetActorLocation())<FMath::Square(Reach+E->GetCapsuleComponent()->GetScaledCapsuleRadius())
        &&(Slam||FVector::DotProduct(Forward,(E->GetActorLocation()-GetActorLocation()).GetSafeNormal2D())>-.05f)
        &&AS::ClearSight(GetWorld(),GetActorLocation(),E->GetActorLocation()))
        HitEnemy(E,Damage+(Build.Weapon==0&&E->PulledUntil>S->SimTime?25.f:0.f),bMeleeCritical,ASData::WeaponElement(Build.Weapon),true);
    const auto Props=S->Breakables;
    for(auto W:Props) if(auto* B=W.Get()) if(FVector::DistSquared2D(GetActorLocation(),B->GetActorLocation())<Reach*Reach
        &&FMath::Abs(GetActorLocation().Z-B->GetActorLocation().Z)<180.f
        &&(Slam||FVector::DotProduct(Forward,(B->GetActorLocation()-GetActorLocation()).GetSafeNormal2D())>0)
        &&B->VisibleFrom(GetActorLocation())) B->Strike(this,Damage*DamageScale(true));
    S->Effect(Slam?6:1,GetActorLocation()-FVector(0,0,Slam?76:20),Forward,BodyColor(),Reach);
    if(Build.Weapon==1) {
        AASWell* Closest=nullptr;float Best=750;
        for(auto W:S->Wells) if(W.IsValid()&&W->CanAffectGameplay()&&W->Hero.Get()==this) {
            const float D=FVector::Dist2D(GetActorLocation(),W->GetActorLocation());if(D<Best) {Best=D;Closest=W.Get();}
        }
        if(Slam&&Closest) Closest->Collapse();else AASWell::Plant(this,GetActorLocation()+Forward*260-FVector(0,0,78),Slam,.8f);
    }
}
