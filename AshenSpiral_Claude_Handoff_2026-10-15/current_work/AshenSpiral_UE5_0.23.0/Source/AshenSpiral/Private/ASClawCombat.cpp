#include "ASCharacter.h"
#include "ASMovementComponent.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "ASEnemy.h"
#include "ASBreakable.h"
#include "ASSpell.h"
#include "Components/CapsuleComponent.h"
#include "HAL/IConsoleManager.h"
static TAutoConsoleVariable<int32> CVarASClawLog(TEXT("as.claw.log"),0,TEXT("1: log every claw hit's show (the rhythm count and the sparks' power) as AshenClaw lines."),ECVF_Default);
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
    if(Best) {
        // the blow lands before its show is sent: a kill inside HitEnemy must not leave the rake pointing at a body that is already gone
        const FVector Chest=Best->GetActorLocation()+FVector(0,0,18);
        HitEnemy(Best,Damage,bMeleeCritical,ASData::WeaponElement(Build.Weapon),true);
        ClawHit(Chest,bMeleeCritical?.9f:.45f,uint8(MeleeStep&1));
    }
    const auto Props=S->Breakables;
    for(auto W:Props) if(auto* B=W.Get()) if(FVector::DistSquared2D(Me,B->GetActorLocation())<Reach*Reach
        &&FMath::Abs(Me.Z-B->GetActorLocation().Z)<180.f&&FVector::DotProduct(Forward,(B->GetActorLocation()-Me).GetSafeNormal2D())>0.f
        &&B->VisibleFrom(Me)) {B->Strike(this,Damage*DamageScale(true));ClawHit(B->GetActorLocation(),.3f,uint8(MeleeStep&1));}
}
// A claw landed: the three-streak rake across the body (its diagonal alternates with the step), sparks in the bleed colour thrown back toward the wielder, a tearing hit, and for the wielder a small
// shake (the camera's own, the same one the maul's blows use). Every machine runs this from the server's word, so a partner's claws look and sound like yours, quieter with the distance.
// The faster the blows come the more they show: each blow within ASClaws::ChainWindow of the last lifts the sparks (and the shake) and nudges the pitch up, a pause lets it fall back. Looks only: no number
// here feeds a rule.
void AASCharacter::ClawHit_Implementation(FVector_NetQuantize At,float Power,uint8 Step) {
    if(GetNetMode()==NM_DedicatedServer) return;
    UWorld* World=GetWorld();if(!World||!FMath::IsFinite(Power)) return;
    const float Now=World->GetTimeSeconds();ClawChain=ASClaws::NextChain(ClawChain,Now-ClawChainAt);ClawChainAt=Now;
    const float Rise=ASClaws::ChainRise(ClawChain);
    Power=FMath::Clamp(Power,0.f,1.f);Power=FMath::Clamp(Power+ASClaws::ChainPower*Rise,0.f,1.f);const FLinearColor Blood=ASData::HitColor(EASHitKind::Bleed);const FVector Point(At),From=GetActorLocation()+FVector(0,0,40);
    if(CVarASClawLog.GetValueOnGameThread()) UE_LOG(LogTemp,Display,TEXT("AshenClaw: %s hit at %s: step %d, chain %d, power %.2f"),*GetName(),*FVector(At).ToCompactString(),int32(Step),ClawChain,Power);
    if(auto* Rake=World->SpawnActor<AASClawFx>()) Rake->SetupRake(Point,From,Blood,int32(Step),Power);
    if(auto* Sparks=World->SpawnActor<AASClawFx>()) Sparks->SetupSparks(Point,From,Blood,Power);
    ASClawFx::Sound(this,TEXT("S_ClawHit"),3,Point,.36f+.22f*Power,.94f+ASClaws::ChainPitch*Rise,1.08f+ASClaws::ChainPitch*Rise);
    if(IsLocallyControlled()) {HammerShakeAge=0;HammerShakeStrength=.35f+.7f*Power;}
}
