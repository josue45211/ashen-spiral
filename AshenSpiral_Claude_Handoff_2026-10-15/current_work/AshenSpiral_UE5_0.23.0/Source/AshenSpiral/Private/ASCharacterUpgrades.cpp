#include "ASCharacter.h"
#include "ASArsenalRules.h"
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
int32 AASCharacter::TraitValue(Trait T) const {
    int32 Rank=TraitRank(T);const int32 Cap=ASBuild::Catalog[int32(T)].Max;
    // Ranks above the normal cap exist only in Endless: full strength to the cap, then each block of five steps down.
    if(Rank>Cap) Rank=FMath::RoundToInt(ASExpedition::TraitPower(Rank,Cap));
    return Rank+(ASBuild::Evolved(EvolutionMask,int32(T))?1:0);
}
ASBuild::Context AASCharacter::UpgradeContext() const {
    ASBuild::Context C;C.Weapon=Build.Weapon;C.Last=LastUpgrade;C.Curse=CurseStacks;C.Hurt=Health<MaxHealth-.1f;
    C.Evolutions=EvolutionMask;C.PermanentCooldown=Rank(14);C.PermanentCritical=Rank(19);C.MissingRelic=false;
    if(const auto* S=AS::State(this)) {C.Room=S->Room;C.Challenge=S->Challenge;C.Difficulty=S->Difficulty;C.Endless=S->bEndless;C.EndlessWave=S->EndlessWave;C.Coop=S->PlayerArray.Num()>1;}
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
    for(int32 M=1;M<int32(ASPartC::Metamorphosis::Count);++M) if(Metamorphosed(ASPartC::Metamorphosis(M))) NoteMetamorphosis(ASPartC::Metamorphosis(M));
    Focus=FMath::Min(Focus,MaxFocus());RefreshLook();ForceNetUpdate();return true;
}
// 0.24 C-6: at the catalogue maximum four traits change what they do. The trait behind each is fixed here; the effects live where the trait already acts (HitProcs, OnKill, the enemy's impact code).
bool AASCharacter::Metamorphosed(ASPartC::Metamorphosis M) const {
    Trait T;
    switch(M) {case ASPartC::Metamorphosis::Wildfire: T=Trait::Burn;break;case ASPartC::Metamorphosis::Shatter: T=Trait::Frost;break;case ASPartC::Metamorphosis::Pinball: T=Trait::Knockback;break;case ASPartC::Metamorphosis::StormWeb: T=Trait::Chain;break;default: return false;}
    return ASPartC::AtMax(TraitValue(T),ASBuild::Catalog[int32(T)].Max);
}
void AASCharacter::NoteMetamorphosis(ASPartC::Metamorphosis M) {
    const uint8 Bit=uint8(1u<<int32(M));if(!HasAuthority()||(MetamorphosisAnnounced&Bit)) return;
    MetamorphosisAnnounced|=Bit;
    if(auto* Player=GetPlayerState<AASPlayerState>()) Player->bRunMetamorphosed=true;
    if(auto* PC=Cast<AASPlayerController>(GetController())) PC->Notify(FString::Printf(TEXT("%s / %s"),UTF8_TO_TCHAR(ASPartC::MetamorphosisName(M)),UTF8_TO_TCHAR(ASPartC::MetamorphosisText(M))));
}
// 0.24 C-1: an element proc jumps from enemy to enemy (depth 0 -> 1 -> 2), weaker each hop. Visited keeps it from bouncing back; the caller owns the per-chain cooldown.
void AASCharacter::ProcEcho(AASEnemy* From,EASHitKind Kind,float Power,int32 Depth,TSet<TWeakObjectPtr<AASEnemy>>& Visited) {
    auto* S=AS::State(this);if(!HasAuthority()||!S||!IsValid(From)||!ASPartC::ChainMayContinue(Depth)||!FMath::IsFinite(Power)||Power<=0.f) return;
    AASEnemy* Best=nullptr;float Distance=FMath::Square(ASPartC::ChainRange*AreaScale());const FVector Start=From->GetActorLocation();
    const auto Enemies=S->Enemies;
    for(auto W:Enemies) if(auto* N=W.Get()) if(CanDamage(N)&&N->Health>0&&!Visited.Contains(N)) {
        const float D=FVector::DistSquared(Start,N->GetActorLocation());
        if(D<Distance&&AS::ClearSight(GetWorld(),Start,N->GetActorLocation())) {Best=N;Distance=D;}
    }
    if(!Best) return;
    Visited.Add(Best);const FVector End=Best->GetActorLocation();const float Magnitude=ASPartC::ChainMagnitude(Depth);
    S->Effect(9,Start+FVector(0,0,50),(End-Start).GetSafeNormal(),ASData::HitColor(Kind),FVector::Distance(Start,End));
    if(auto* Player=GetPlayerState<AASPlayerState>()) Player->RunEchoes=FMath::Min(Player->RunEchoes+1,100000);
    Best->ApplyElement(Kind,this,Power*Magnitude,Magnitude);
    ProcEcho(Best,Kind,Power,Depth+1,Visited);
}
void AASCharacter::ResetTraits() {
    TraitRanks.Init(0,ASBuild::Count);EvolutionMask=0;LastUpgrade=-1;Rerolls=1;CurseStacks=0;Focus=60;NextRune=0;
    ConduitHits=0;bPhoenixUsed=false;WarCryUntil=0;LastDamageAt=-100;RoomBuffUntil=KillBuffUntil=StillSince=GoldRemainder=0;
    NextSplashFx=NextChainFx=NextAura=NextOrbit=NextThorns=0;
    DashInvulEnd=0;LastPerfectDashAt=-100.f;NextProcChain=0;PerfectBoostUntil=0;MetamorphosisAnnounced=0;WildfireDepth=0;
    DashVictims.Empty();bDashTraceActive=false;Movement()->DashCharges=1;Movement()->KnownDashCapacity=1;Movement()->DashRecharge=0;
}
void AASCharacter::EnterRoomTraits(float CombatStarts) {RoomBuffUntil=CombatStarts+ASBuild::VanguardSeconds;StillSince=CombatStarts;DashVictims.Empty();}
float AASCharacter::DamageScale(bool WeaponHit) const {
    const float Now=AS::Time(this);float Bonus=0;
    if(WeaponHit&&(Build.Weapon==0||Build.Weapon==1||Build.Weapon==5)) Bonus+=.12f*TraitValue(Trait::Melee);
    if(WeaponHit&&(Build.Weapon==2||Build.Weapon==3||Build.Weapon>=6)) Bonus+=.12f*TraitValue(Trait::Ranged);
    if(!WeaponHit||Build.Weapon==4) Bonus+=.12f*TraitValue(Trait::Magic);
    Bonus+=.45f*TraitValue(Trait::BloodPact);
    if(WeaponHit) Bonus+=.08f*EndlessValue(3); // Unbound Fury (Endless exclusive): 8% primary damage per effective rank
    const auto* State=AS::State(this);
    if(State&&(State->Phase==EASPhase::Hub||CombatBenefits())&&WeaponHit&&Now<WarCryUntil&&(Build.Weapon==0||Build.Weapon==1||Build.Weapon==5)) Bonus+=.20f;
    if(CombatBenefits()) {
        if(Now<RoomBuffUntil) Bonus+=ASBuild::VanguardBonus(TraitValue(Trait::RoomBlessing));
        if(Now<KillBuffUntil) Bonus+=.05f*TraitValue(Trait::KillFlow);
        if(ASPartC::PerfectBoostActive(Now,PerfectBoostUntil)) Bonus+=ASPartC::PerfectDashBoost; // 0.24.1: a perfect dash
        if(Now-StillSince>=1.2f) Bonus+=.12f*TraitValue(Trait::Stillness);
        if(GetVelocity().Size2D()>200.f) Bonus+=.08f*TraitValue(Trait::Momentum);
        if(Health<MaxHealth*.4f) Bonus+=.15f*TraitValue(Trait::Berserk);
    }
    return ASRules::DamageMultiplier(Rank(17),RunRank(2))*(1.f+Bonus);
}
void AASCharacter::OnKill(const AASEnemy* Killed) {
    if(!CombatBenefits()) return;
    if(const auto* S=AS::State(this)) if(S->bEndless&&HasAuthority()) {
        auto* PS=GetPlayerState<AASPlayerState>();
        // Bloodroot: each kill permanently adds max HP (and heals the same amount).
        if(const float V=EndlessValue(0);V>0.f) {const float Gain=ASExpedition::BloodrootGain(V);MaxHealth=FMath::Min(100000.f,MaxHealth+Gain);Health=FMath::Min(MaxHealth,Health+Gain);}
        // Close Quarters: kills within 350 cm heal and pay Scrap.
        if(const float V=EndlessValue(1);V>0.f&&Killed&&FVector::DistSquared2D(GetActorLocation(),Killed->GetActorLocation())<=350.f*350.f) {
            Heal(3.f*V);if(PS) PS->AvailableScrap=ASRules::Credit(PS->AvailableScrap,int32(FMath::RoundToInt(2.f*V)));
        }
        // First Blood: a kill within one second of first engaging the enemy pays Scrap.
        if(const float V=EndlessValue(2);V>0.f&&Killed&&Killed->FirstEngagedAt>=0.f&&AS::Time(this)-Killed->FirstEngagedAt<1.f) {
            if(PS) PS->AvailableScrap=ASRules::Credit(PS->AvailableScrap,int32(FMath::RoundToInt(3.f*V)));
        }
        ForceNetUpdate();
    }
    Heal(1.5f*TraitValue(Trait::Reaper));
    // 0.24 C-6: Wildfire - a burning enemy explodes when it dies; Shatter - a frozen one shatters. Each hurts and re-applies its element to what is near; kills inside can set off more (depth-capped).
    if(HasAuthority()&&Killed&&WildfireDepth<3) {
        const bool Burning=(Killed->StatusBits&1)!=0&&Metamorphosed(ASPartC::Metamorphosis::Wildfire),Frozen=(Killed->StatusBits&8)!=0&&Metamorphosed(ASPartC::Metamorphosis::Shatter);
        if(Burning||Frozen) if(auto* S=AS::State(this)) {
            ++WildfireDepth;
            const EASHitKind Element=Burning?EASHitKind::Burn:EASHitKind::Frost;const float Radius=(Burning?ASPartC::WildfireRadius:ASPartC::ShatterRadius)*AreaScale();
            const float Damage=Burning?ASPartC::WildfireDamage(Killed->MaxHealth):ASPartC::ShatterDamage(Killed->MaxHealth);const FVector At=Killed->GetActorLocation();
            S->Effect(8,At+FVector(0,0,30),FVector::UpVector,ASData::HitColor(Element),Radius);S->Effect(6,At,FVector::UpVector,ASData::HitColor(Element),Radius);
            const auto Enemies=S->Enemies;
            for(auto W:Enemies) if(auto* N=W.Get()) if(N!=Killed&&CanDamage(N)&&N->Health>0&&FVector::DistSquared2D(At,N->GetActorLocation())<FMath::Square(Radius)&&AS::ClearSight(GetWorld(),At+FVector(0,0,55),N->GetActorLocation())) {
                SecondaryDamage(N,Damage,Element);
                if(IsValid(N)&&N->Health>0) N->ApplyElement(Element,this,DamageScale(false)*.5f,1.f);
            }
            --WildfireDepth;
        }
    }
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
float AASCharacter::OnHitScale() const {return ASArsenal::OnHitScale(Build.Weapon);}
void AASCharacter::HitProcs(AASEnemy* E,const FVector& At,float Dealt) {
    if(!CombatBenefits()||Dealt<=0) return;auto* S=AS::State(this);if(!S) return;
    const float Now=S->SimTime;
    // 0.24: every on-hit reward below is scaled by the equipped weapon's BASE swing/shot interval (ASArsenal::OnHitScale): slower weapons reward more per hit, faster ones less,
    // so rewards per second stay comparable between weapons. Chance rewards go through ScaleReward (exact expected value; whatever would exceed certainty becomes extra strength);
    // steady rewards simply multiply. The old per-reward cooldowns are gone - the scale does their job - except two purely visual throttles (chain beams, splash flash).
    const float Scale=OnHitScale();
    const int32 LeechRank=TraitValue(Trait::Leech);
    if(LeechRank>0) Heal(FMath::Min(3.f,Dealt*.015f*float(LeechRank))*Scale);
    if(TraitValue(Trait::Conduit)>0&&++ConduitHits>=ASArsenal::ConduitHitsNeeded(Scale)) {ConduitHits=0;Focus=FMath::Min(MaxFocus(),Focus+12.f);NextSpiral=FMath::Max(Now,NextSpiral-.3f);}
    if(IsValid(E)&&E->Health>0) {
        const Trait Types[]={Trait::Burn,Trait::Poison,Trait::Bleed,Trait::Frost};
        const EASHitKind Kinds[]={EASHitKind::Burn,EASHitKind::Poison,EASHitKind::Bleed,EASHitKind::Frost};
        int32 Fired=-1;float FiredPower=0.f;
        for(int32 I=0;I<4;++I) {
            const auto R=ASArsenal::ScaleReward(ASBuild::ProcChance(TraitValue(Types[I]),.18f),Scale);
            if(R.Chance>0.f&&FMath::FRand()<R.Chance) {
                const float Power=DamageScale(true)*(Kinds[I]==ASData::WeaponElement(Build.Weapon)?1.3f:1.f)*R.Magnitude;
                E->ApplyElement(Kinds[I],this,Power,R.Magnitude);Fired=I;FiredPower=Power;
            }
        }
        // 0.24 C-1: with a conductor (Chain Lightning, Storm Touch or Impact Bloom) an element proc echoes to a neighbour, and from there once more; one chain per ASPartC::ChainCooldown.
        if(Fired>=0&&ASPartC::ChainConducts(TraitValue(Trait::Chain),TraitValue(Trait::Shock),TraitValue(Trait::Splash))&&ASPartC::ChainMayStart(Now,NextProcChain)&&IsValid(E)) {
            NextProcChain=Now+ASPartC::ChainCooldown;TSet<TWeakObjectPtr<AASEnemy>> Visited;Visited.Add(E);
            ProcEcho(E,Kinds[Fired],FiredPower,0,Visited);
        }
        if(TraitValue(Trait::Knockback)>0) E->Control(0,E->GetActorLocation()-GetActorLocation(),120.f*float(TraitValue(Trait::Knockback))*Scale);
        const auto Stun=ASArsenal::ScaleReward(ASBuild::ProcChance(TraitValue(Trait::Stun),.08f),Scale),Root=ASArsenal::ScaleReward(ASBuild::ProcChance(TraitValue(Trait::Root),.10f),Scale);
        if(Stun.Chance>0.f&&FMath::FRand()<Stun.Chance) E->Control(1,FVector::ZeroVector,0);
        if(Root.Chance>0.f&&FMath::FRand()<Root.Chance) E->Control(2,FVector::ZeroVector,0);
    }
    const auto ShockReward=ASArsenal::ScaleReward(ASBuild::ProcChance(TraitValue(Trait::Shock),.18f),Scale);
    const bool Shock=ShockReward.Chance>0.f&&FMath::FRand()<ShockReward.Chance;
    if(Shock&&IsValid(E)) SecondaryDamage(E,Dealt*.20f*ShockReward.Magnitude,EASHitKind::Shock);
    if(TraitValue(Trait::Chain)>0||Shock) {
        // A shock proc chains once at its own strength (as before); the Chain trait chains up to four links on every hit at 30% x the weapon's scale.
        const bool Web=!Shock&&Metamorphosed(ASPartC::Metamorphosis::StormWeb); // 0.24 C-6 Storm Web: two more links, 40% further reach
        const int32 Count=Shock?1:ASPartC::StormWebLinks(TraitValue(Trait::Chain),Web);
        const float LinkFraction=Shock?.20f*ShockReward.Magnitude:.30f*Scale;
        const bool ShowBeams=Now>=NextChainFx;
        FVector Start=At;TSet<TWeakObjectPtr<AASEnemy>> Visited;if(E) Visited.Add(E);
        for(int32 I=0;I<FMath::Min(Web?6:4,Count);++I) {
            AASEnemy* Best=nullptr;float Distance=FMath::Square(650.f*AreaScale()*(Web?ASPartC::StormWebRangeMultiplier:1.f));
            for(auto W:S->Enemies) if(auto* N=W.Get()) if(CanDamage(N)&&N->Health>0&&!Visited.Contains(N)) {
                const float D=FVector::DistSquared(Start,N->GetActorLocation());
                if(D<Distance&&AS::ClearSight(GetWorld(),Start,N->GetActorLocation())) {Best=N;Distance=D;}
            }
            if(!Best) break;Visited.Add(Best);const FVector End=Best->GetActorLocation();
            if(ShowBeams) S->Effect(9,Start,(End-Start).GetSafeNormal(),ASData::HitColor(EASHitKind::Shock),FVector::Distance(Start,End));
            SecondaryDamage(Best,Dealt*LinkFraction,EASHitKind::Shock);Start=End;
        }
        if(ShowBeams) NextChainFx=Now+.1f; // visual throttle only: fast weapons do not flood the network with beam effects
    }
    const bool Splash=TraitValue(Trait::Splash)>0,Pull=TraitValue(Trait::Gravity)>0;
    if(Splash||Pull) {
        const auto Enemies=S->Enemies;
        for(auto W:Enemies) if(auto* N=W.Get()) if(N!=E&&CanDamage(N)&&FVector::DistSquared2D(At,N->GetActorLocation())<FMath::Square((Splash?180.f:300.f)*AreaScale())&&AS::ClearSight(GetWorld(),At,N->GetActorLocation())) {
            if(Pull) N->Control(0,At-N->GetActorLocation(),170.f*float(TraitValue(Trait::Gravity))*Scale,-1); // -1: a pull is not a launch (no impact damage as the pulled crowd converges)
            if(Splash) SecondaryDamage(N,Dealt*.15f*float(TraitValue(Trait::Splash))*Scale,EASHitKind::Spiral);
        }
        if(Splash&&Now>=NextSplashFx) {NextSplashFx=Now+.12f;S->Effect(6,At,FVector::UpVector,ASData::HitColor(EASHitKind::Spiral),180.f*AreaScale());}
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
