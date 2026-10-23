#include "ASCharacter.h"
#include "ASMovementComponent.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "ASEnemy.h"
#include "ASBreakable.h"
#include "ASSpell.h"
#include "Components/CapsuleComponent.h"
// 0.26 Twin Claws: a swing shreds ONE target. Where the blade and the scythe sweep everything in a frontal arc, the claws pick the enemy that best fits the swing (the one the crosshair and the
// wielder's facing favour, nearer ones first) and rake it. A latched wielder (the pounce) swings at its target, whatever the crosshair says. Props in reach still break.
void AASCharacter::ResolveClaws() {
    auto* S=AS::State(this);if(!HasAuthority()||!S||!CanUseWeapons()||PendingMeleeWeapon!=Build.Weapon) return;
    const bool Latched=PounceLatched();
    const FVector Me=GetActorLocation();
    float Reach=MeleeReachFor(MeleeStep);if(Latched) Reach*=ASClaws::ReachWhenLatched/ASClaws::Reach; // a latched wielder drifts a little on its ring and still lands every swing
    const float Damage=ASArsenal::Damage(Build.Weapon,PrimaryDamageBonus())*ASMelee::DamageScale(Build.Weapon,MeleeStep);
    FVector Forward=(AimPoint-Me).GetSafeNormal2D();if(Forward.IsNearlyZero()) Forward=GetActorForwardVector();
    if(Latched) {const FVector To=(PounceTarget->GetActorLocation()-Me).GetSafeNormal2D();if(!To.IsNearlyZero()) Forward=To;}
    // how well an enemy fits the swing: the facing, then closeness; false when it is out of reach, out of the frontal half, unreachable or not ours to hit
    const auto Fit=[&](AASEnemy* E,float& Score)->bool {
        if(!E||E->Health<=0||!CanDamage(E)) return false;
        const FVector To=E->GetActorLocation()-Me;const float Distance=float(To.Size2D());const float Dot=float(FVector::DotProduct(Forward,To.GetSafeNormal2D()));
        const float Radius=E->GetCapsuleComponent()->GetScaledCapsuleRadius();
        if(!ASMelee::TargetInReach(Distance,Reach,Radius,float(To.Z),Dot,-.05f,ASMelee::VerticalReachFor(E->Flies()))) return false;
        if(!AS::ClearSight(GetWorld(),Me,E->GetActorLocation())) return false;
        Score=Dot-Distance/(Reach+Radius)*.5f;return true;
    };
    AASEnemy* Best=nullptr;float BestScore=-TNumericLimits<float>::Max();
    {float Score=0.f;if(Latched&&Fit(PounceTarget,Score)) Best=PounceTarget;}
    if(!Best) {const auto Targets=S->Enemies;for(auto W:Targets) {float Score=0.f;AASEnemy* E=W.Get();if(Fit(E,Score)&&Score>BestScore) {Best=E;BestScore=Score;}}}
    bool Contact=false;
    if(Best) {
        Contact=true;
        S->Effect(16,Best->GetActorLocation()-FVector(0,0,Best->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-4.f),FVector::UpVector,FLinearColor::White,0.f); // the faint hit-confirm tick
        HitEnemy(Best,Damage,bMeleeCritical,ASData::WeaponElement(Build.Weapon),true);
    }
    const auto Props=S->Breakables;
    for(auto W:Props) if(auto* B=W.Get()) if(FVector::DistSquared2D(Me,B->GetActorLocation())<Reach*Reach
        &&FMath::Abs(Me.Z-B->GetActorLocation().Z)<180.f&&FVector::DotProduct(Forward,(B->GetActorLocation()-Me).GetSafeNormal2D())>0.f
        &&B->VisibleFrom(Me)) {Contact=true;B->Strike(this,Damage*DamageScale(true));}
    FLinearColor Tint=BodyColor();Tint.A=float(FMath::Clamp(MeleeStep,0,2));
    S->Effect(17,Me-FVector(0,0,35),Forward,Tint,Reach); // the swing's crescent for now; the claws' own rake is the feel pass
    (void)Contact;
}
