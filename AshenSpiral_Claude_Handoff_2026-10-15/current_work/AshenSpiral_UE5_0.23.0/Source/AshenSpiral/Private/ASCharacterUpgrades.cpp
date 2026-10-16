#include "ASCharacter.h"
#include "ASMovementComponent.h"
#include "ASPlayerController.h"
#include "ASGameState.h"
#include "ASEnemy.h"
#include "ASSpell.h"
#include "ASWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

using ASBuild::Trait;
int32 AASCharacter::TraitRank(Trait T) const {
    const int32 I=int32(T);return I>=0&&I<ASBuild::Count&&TraitRanks.IsValidIndex(I)?FMath::Clamp(TraitRanks[I],0,ASBuild::Catalog[I].Max):0;
}
int32 AASCharacter::TraitValue(Trait T) const {return TraitRank(T)+(ASBuild::Evolved(EvolutionMask,int32(T))?1:0);}
ASBuild::Context AASCharacter::UpgradeContext() const {
    ASBuild::Context C;C.Weapon=Build.Weapon;C.Last=LastUpgrade;C.Curse=CurseStacks;C.Hurt=Health<MaxHealth-.1f;
    C.Evolutions=EvolutionMask;C.PermanentCooldown=Rank(14);C.PermanentCritical=Rank(19);C.MissingRelic=false;
    if(const auto* S=AS::State(this)) {C.Room=S->Room;C.Challenge=S->Challenge;}
    for(int32 I=0;I<ASRules::RunUpgradeCount;++I) C.Stats[I]=RunRank(I);
    for(int32 I=0;I<ASBuild::Count;++I) C.Traits[I]=TraitRank(Trait(I));
    for(int32 I=0;I<Build.RelicPool;++I) if(!Relic(I)) C.MissingRelic=true;
    return C;
}
bool AASCharacter::CanApplyChoice(int32 Choice) const {return ASBuild::CanChoose(UpgradeContext(),Choice);}
int32 AASCharacter::EvolutionTarget() const {
    const auto C=UpgradeContext();const int32 Last=LastUpgrade-ASBuild::OfferBase;
    if(ASBuild::CanEvolve(C,Last)) return Last;
    int32 Best=-1;for(int32 I=0;I<ASBuild::Count;++I) if(ASBuild::CanEvolve(C,I)&&(Best<0||C.Traits[I]>C.Traits[Best])) Best=I;
    return Best;
}
bool AASCharacter::ApplyChoice(int32 Choice) {
    if(!HasAuthority()||!CanApplyChoice(Choice)) return false;
    if(ASRules::IsRunUpgradeOffer(Choice)) {if(!ApplyRunUpgrade(Choice-100)) return false;LastUpgrade=Choice;return true;}
    const int32 I=Choice-ASBuild::OfferBase;const Trait T=Trait(I);
    const int32 Evolution=EvolutionTarget(),OldCapacity=MaxDashCharges(),OldMagazine=MagazineCapacity();
    TraitRanks.SetNumZeroed(ASBuild::Count);++TraitRanks[I];
    if(T==Trait::Magazine) Ammo=FMath::Min(MagazineCapacity(),Ammo+MagazineCapacity()-OldMagazine);
    if(T==Trait::Mend) Heal(MaxHealth*.35f);

    if(T==Trait::Barrier) Shield=ASRules::GrantShield(Shield,12.f,MaxHealth);
    if(T==Trait::Reroll) Rerolls=FMath::Min(9,Rerolls+1);
    if(T==Trait::Cleanse) CurseStacks=0;
    if(T==Trait::ManaReserve) Focus=FMath::Min(MaxFocus(),Focus+10.f);
    if(T==Trait::BloodPact) {MaxHealth=FMath::Max(40.f,MaxHealth*.85f);Health=FMath::Min(Health,MaxHealth);++CurseStacks;}
    if(T==Trait::Refine&&Evolution>=0) EvolutionMask|=uint64(1)<<Evolution;
    if(T==Trait::Mystery) {
        TArray<int32> Pool;for(int32 J=0;J<Build.RelicPool;++J) if(!Relic(J)) Pool.Add(J);
        if(!Pool.IsEmpty()) RelicMask|=1<<Pool[FMath::RandRange(0,Pool.Num()-1)];
    }
    if(T==Trait::Duplicate) {const int32 Previous=LastUpgrade;ApplyChoice(Previous);}
    else if(!ASBuild::Catalog[I].Instant) LastUpgrade=Choice;
    const int32 Added=MaxDashCharges()-OldCapacity;
    if(Added>0) {Movement()->DashCharges=FMath::Min(MaxDashCharges(),Movement()->DashCharges+Added);Movement()->KnownDashCapacity=MaxDashCharges();}
    Focus=FMath::Min(Focus,MaxFocus());RefreshLook();ForceNetUpdate();return true;
}
void AASCharacter::ResetTraits() {
    TraitRanks.Init(0,ASBuild::Count);EvolutionMask=0;LastUpgrade=-1;Rerolls=1;CurseStacks=0;Focus=60;NextRune=0;
    ConduitHits=0;bPhoenixUsed=false;WarCryUntil=0;LastDamageAt=-100;RoomBuffUntil=KillBuffUntil=StillSince=GoldRemainder=0;
    NextLeech=NextSplash=NextChain=NextShock=NextPull=NextConduit=NextAura=NextOrbit=NextThorns=0;
    DashVictims.Empty();bDashTraceActive=false;Movement()->DashCharges=1;Movement()->KnownDashCapacity=1;Movement()->DashRecharge=0;
}
void AASCharacter::EnterRoomTraits(float CombatStarts) {RoomBuffUntil=CombatStarts+8.f;StillSince=CombatStarts;DashVictims.Empty();}
float AASCharacter::DamageScale(bool WeaponHit) const {
    const float Now=AS::Time(this);float Bonus=0;
    if(WeaponHit&&(Build.Weapon==0||Build.Weapon==1||Build.Weapon==5)) Bonus+=.12f*TraitValue(Trait::Melee);
    if(WeaponHit&&(Build.Weapon==2||Build.Weapon==3||Build.Weapon>=6)) Bonus+=.12f*TraitValue(Trait::Ranged);
    if(!WeaponHit||Build.Weapon==4) Bonus+=.12f*TraitValue(Trait::Magic);
    Bonus+=.45f*TraitValue(Trait::BloodPact);
    const auto* State=AS::State(this);
    if(State&&(State->Phase==EASPhase::Hub||CombatBenefits())&&WeaponHit&&Now<WarCryUntil&&(Build.Weapon==0||Build.Weapon==1||Build.Weapon==5)) Bonus+=.20f;
    if(CombatBenefits()) {
        if(Now<RoomBuffUntil) Bonus+=.10f*TraitValue(Trait::RoomBlessing);
        if(Now<KillBuffUntil) Bonus+=.05f*TraitValue(Trait::KillFlow);
        if(Now-StillSince>=1.2f) Bonus+=.12f*TraitValue(Trait::Stillness);
        if(GetVelocity().Size2D()>200.f) Bonus+=.08f*TraitValue(Trait::Momentum);
        if(Health<MaxHealth*.4f) Bonus+=.15f*TraitValue(Trait::Berserk);
    }
    return ASRules::DamageMultiplier(Rank(17),RunRank(2))*(1.f+Bonus);
}
void AASCharacter::OnKill() {
    if(!CombatBenefits()) return;
    Heal(1.5f*TraitValue(Trait::Reaper));
    if(TraitValue(Trait::KillFlow)>0) {NextSpiral=FMath::Max(AS::Time(this),NextSpiral-.25f*TraitValue(Trait::KillFlow));KillBuffUntil=AS::Time(this)+2.f;}
}
void AASCharacter::OnCinderPickup() {if(CombatBenefits()) Heal(float(TraitValue(Trait::MendingCoin)));}
void AASCharacter::SecondaryDamage(AASEnemy* E,float Amount,EASHitKind Kind) {
    if(!HasAuthority()||!CanDamage(E)||E->Health<=0||Amount<=0) return;
    const FVector At=E->GetActorLocation()+FVector(0,0,80);const float Before=E->Health;
    const bool Killed=E->Hit(Amount,this);const float Dealt=Killed?Before:Before-E->Health;
    if(!Killed&&Kind==EASHitKind::Shock) E->RegisterCoopElement(Kind,this,DamageScale(false));
    if(Dealt>0) if(auto* PC=Cast<AASPlayerController>(GetController())) PC->DamageNumber(At,Dealt,false,Kind);
}
void AASCharacter::HitProcs(AASEnemy* E,const FVector& At,float Dealt) {
    if(!CombatBenefits()||Dealt<=0) return;auto* S=AS::State(this);if(!S) return;
    const float Now=S->SimTime;
    if(Now>=NextLeech&&TraitValue(Trait::Leech)>0) {NextLeech=Now+.25f;Heal(FMath::Min(3.f,Dealt*.015f*TraitValue(Trait::Leech)));}
    if(Now>=NextConduit&&TraitValue(Trait::Conduit)>0) {
        NextConduit=Now+.15f;if(++ConduitHits>=6) {ConduitHits=0;Focus=FMath::Min(MaxFocus(),Focus+12.f);NextSpiral=FMath::Max(Now,NextSpiral-.3f);}
    }
    if(IsValid(E)&&E->Health>0) {
        const Trait Types[]={Trait::Burn,Trait::Poison,Trait::Bleed,Trait::Frost};
        const EASHitKind Kinds[]={EASHitKind::Burn,EASHitKind::Poison,EASHitKind::Bleed,EASHitKind::Frost};
        for(int32 I=0;I<4;++I) if(FMath::FRand()<ASBuild::ProcChance(TraitValue(Types[I]),.18f)) E->ApplyElement(Kinds[I],this,DamageScale(true)*(Kinds[I]==ASData::WeaponElement(Build.Weapon)?1.3f:1.f));
        if(TraitValue(Trait::Knockback)>0) E->Control(0,E->GetActorLocation()-GetActorLocation(),120.f*TraitValue(Trait::Knockback));
        if(FMath::FRand()<ASBuild::ProcChance(TraitValue(Trait::Stun),.08f)) E->Control(1,FVector::ZeroVector,0);
        if(FMath::FRand()<ASBuild::ProcChance(TraitValue(Trait::Root),.10f)) E->Control(2,FVector::ZeroVector,0);
    }
    const bool Shock=Now>=NextShock&&FMath::FRand()<ASBuild::ProcChance(TraitValue(Trait::Shock),.18f);
    if(Shock) {NextShock=Now+.35f;if(IsValid(E)) SecondaryDamage(E,Dealt*.20f,EASHitKind::Shock);}
    if((Now>=NextChain&&TraitValue(Trait::Chain)>0)||Shock) {
        const int32 Count=Shock?1:TraitValue(Trait::Chain);NextChain=Now+.35f;
        FVector Start=At;TSet<TWeakObjectPtr<AASEnemy>> Visited;if(E) Visited.Add(E);
        for(int32 I=0;I<FMath::Min(4,Count);++I) {
            AASEnemy* Best=nullptr;float Distance=FMath::Square(650.f*AreaScale());
            for(auto W:S->Enemies) if(auto* N=W.Get()) if(CanDamage(N)&&N->Health>0&&!Visited.Contains(N)) {
                const float D=FVector::DistSquared(Start,N->GetActorLocation());
                if(D<Distance&&AS::ClearSight(GetWorld(),Start,N->GetActorLocation())) {Best=N;Distance=D;}
            }
            if(!Best) break;Visited.Add(Best);const FVector End=Best->GetActorLocation();
            S->Effect(9,Start,(End-Start).GetSafeNormal(),ASData::HitColor(EASHitKind::Shock),FVector::Distance(Start,End));
            SecondaryDamage(Best,Dealt*(Shock?.20f:.30f),EASHitKind::Shock);Start=End;
        }
    }
    const bool Splash=Now>=NextSplash&&TraitValue(Trait::Splash)>0,Pull=Now>=NextPull&&TraitValue(Trait::Gravity)>0;
    if(Splash||Pull) {
        if(Splash) NextSplash=Now+.25f;if(Pull) NextPull=Now+.6f;
        const auto Enemies=S->Enemies;
        for(auto W:Enemies) if(auto* N=W.Get()) if(N!=E&&CanDamage(N)&&FVector::DistSquared2D(At,N->GetActorLocation())<FMath::Square((Splash?180.f:300.f)*AreaScale())&&AS::ClearSight(GetWorld(),At,N->GetActorLocation())) {
            if(Pull) N->Control(0,At-N->GetActorLocation(),170.f*TraitValue(Trait::Gravity));
            if(Splash) SecondaryDamage(N,Dealt*.15f*TraitValue(Trait::Splash),EASHitKind::Spiral);
        }
        if(Splash) S->Effect(6,At,FVector::UpVector,ASData::HitColor(EASHitKind::Spiral),180.f*AreaScale());
    }
}
void AASCharacter::TickTraits(float Delta) {
    auto* S=AS::State(this);if(!S||!HasAuthority()||bDowned) return;const float Now=S->SimTime;
    if(GetVelocity().Size2D()>40.f) StillSince=Now;
    if(S->Phase==EASPhase::Hub||CombatBenefits()) Focus=ASBuild::RefillFocus(Focus,Delta,TraitValue(Trait::ManaReserve),TraitValue(Trait::ManaRegen));
    if(!CombatBenefits()) return;
    if(Now-LastDamageAt>=3.f) Heal(Delta*TraitValue(Trait::Regrowth));
    const int32 Barrier=TraitValue(Trait::Barrier);
    if(Barrier>0&&Now-LastDamageAt>=5.f&&Shield<12.f*Barrier) Shield=FMath::Min(12.f*Barrier,Shield+Delta*2.f*Barrier);
    if(TraitValue(Trait::DashStrike)>0&&bDashTraceActive) {
        const auto Enemies=S->Enemies;
        for(auto W:Enemies) if(auto* E=W.Get()) if(CanDamage(E)&&!DashVictims.Contains(E)&&ASBuild::SweptCircle(DashTraceStart.X,DashTraceStart.Y,GetActorLocation().X,GetActorLocation().Y,E->GetActorLocation().X,E->GetActorLocation().Y,160.f)&&FMath::Abs(GetActorLocation().Z-E->GetActorLocation().Z)<200&&AS::ClearSight(GetWorld(),GetActorLocation(),E->GetActorLocation())) {
            DashVictims.Add(E);SecondaryDamage(E,12.f*TraitValue(Trait::DashStrike)*DamageScale(true),EASHitKind::Physical);
        }
    }
    DashTraceStart=GetActorLocation();if(Movement()->DashRemaining<=0) bDashTraceActive=false;
    if(Now>=NextAura&&TraitValue(Trait::ChillAura)>0) {
        NextAura=Now+.25f;const float Radius=(320.f+50.f*TraitValue(Trait::ChillAura))*AreaScale();
        for(auto W:S->Enemies) if(auto* E=W.Get()) if(CanDamage(E)&&FVector::DistSquared2D(GetActorLocation(),E->GetActorLocation())<Radius*Radius&&AS::ClearSight(GetWorld(),GetActorLocation(),E->GetActorLocation())) E->ApplyElement(EASHitKind::Frost,this,0.f);
    }
    if(Now>=NextOrbit&&TraitValue(Trait::Orbit)>0) {
        NextOrbit=Now+.5f;const int32 Count=FMath::Min(3,TraitValue(Trait::Orbit));
        for(int32 I=0;I<Count;++I) {
            const float A=Now*2.f+I*UE_TWO_PI/Count;const FVector P=GetActorLocation()+FVector(FMath::Cos(A)*135,FMath::Sin(A)*135,35);
            AASEnemy* Best=nullptr;float Distance=FMath::Square(220.f*AreaScale());
            for(auto W:S->Enemies) if(auto* E=W.Get()) if(CanDamage(E)&&E->Health>0) {const float D=FVector::DistSquared(P,E->GetActorLocation());if(D<Distance&&AS::ClearSight(GetWorld(),P,E->GetActorLocation())) {Best=E;Distance=D;}}
            if(Best) {const FVector End=Best->GetActorLocation();S->Effect(9,P,(End-P).GetSafeNormal(),ASData::HitColor(EASHitKind::Burn),FVector::Distance(P,End));SecondaryDamage(Best,4.f*TraitValue(Trait::Orbit)/Count*DamageScale(false),EASHitKind::Burn);}
        }
    }
}
