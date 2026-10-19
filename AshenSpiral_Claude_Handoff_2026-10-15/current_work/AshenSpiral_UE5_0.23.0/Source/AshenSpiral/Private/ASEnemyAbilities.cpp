#include "ASEnemy.h"
#include "ASEnemyRules.h"
#include "ASEnemyAbilities.h"
#include "ASEnemyHazards.h"
#include "ASBreakable.h"
#include "ASGameMode.h"
#include "ASExpeditionRules.h"
#include "ASCharacter.h"
#include "ASGameState.h"
#include "ASSpell.h"
#include "ASWorld.h"
#include "ASData.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
// The 0.24 enemy abilities that have their own effect and flow: the grunt's shove, the archer's back-hop, the duelist's parry and riposte, the brute's ground pound and enrage,
// the leaper's rift blink and its strike, the shaman's meteor rain. The geometry and every number are ASEnemyAbilities.h / ASEnemyRules.h (tested as plain C++ in
// Tests/enemy_ability_024_tests.cpp); this file applies them in the world, on the server. Everything runs through the replicated state machine, so the other machines pose
// and draw it from AIState, Ability, WindupStart/End, AttackReleaseTime and Flags, and the effects below are multicast or replicated actors.

// The warning on the floor for the ability being wound up (the server never draws it). A half disc for the swings, a full disc for the ground pound; the dim shape shows the whole
// area at once, the bright one grows over it as the wind-up completes. Neither is a ring: a ring on screen is not a thing an enemy puts on the floor.
void AASEnemy::ShowTell(float Progress) {
    namespace ER=ASEnemyRules;
    if(!Tell||!TellFill||!GetCapsuleComponent()) return;
    const int32 Shape=ER::SpecFor(Ability).Tell;const bool Disc=Shape==ER::TellDisc,HasSector=ASAssets::HasMesh(TEXT("SM_StrikeSector"));
    if(Shape!=TellShape) { // a different shape than the one last drawn: swap both meshes (this happens only when an enemy with two kinds of warning switches)
        const FString TellMesh=Disc?FString(TEXT("Cylinder")):(HasSector?FString(TEXT("SM_StrikeSector")):FString(TEXT("SM_Ring")));
        ASAssets::Style(Tell,TellMesh,FLinearColor(.55f,.05f,.02f),0,1.5f);ASAssets::Style(TellFill,TellMesh,FLinearColor(1.f,.16f,.03f),0,6.f);
        Tell->SetCastShadow(false);TellFill->SetCastShadow(false);TellShape=Shape;
    }
    const float Radius=ER::TellRadiusFor(Ability,Elite()),Unit=Disc?50.f:(HasSector?50.f:53.f),Full=Radius/Unit,Grow=Full*FMath::Max(.04f,Progress),Flat=Disc?.02f:1.f; // the basic cylinder is 100 cm across: flattened it is a disc
    Tell->SetRelativeScale3D(FVector(Full,Full,Flat));TellFill->SetRelativeScale3D(FVector(Grow,Grow,Flat*1.5f));
    if(auto* M=Cast<UMaterialInstanceDynamic>(TellFill->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),3+Progress*6);
}

// What happens when an ability's wind-up ends, for the abilities in this file. Returns what comes next (see EFlow); FlowNotMine leaves the ability to CommitAttack, the older
// by-kind code (slash, shot, pounce, charge, blast, snipe, summon and the boss).
int32 AASEnemy::ReleaseOwnAbility(int32 Id,const FVector& ToTarget) {
    namespace ER=ASEnemyRules;
    auto* S=AS::State(this);auto* World=GetWorld();
    if(!S||!World||SpawnProtected()) return Id>=ER::AbilityShove?FlowFinish:FlowNotMine; // an enemy that is still being introduced does nothing (the old CommitAttack also stood down)
    const float Now=S->SimTime;const FVector Here=GetActorLocation();
    const float HalfHeight=GetCapsuleComponent()?GetCapsuleComponent()->GetScaledCapsuleHalfHeight():83.f;
    const FVector Feet=Here-FVector(0,0,HalfHeight-6.f);
    AASCharacter* Target=CurrentTarget.Get();
    // A frontal half-disc swing: the same shape the warning drew (ASEnemyRules::StrikeHits) decides who is hit; Knock > 0 throws the heroes it hits (unless a dash's
    // invulnerability or a fresh hit protects them, in which case nothing happens to them at all).
    auto Swing=[&](float Radius,float Damage,float Knock,float Lift) {
        S->Effect(17,Here-FVector(0,0,35),Aim,FLinearColor(1.f,.12f,.03f,3.f),Radius); // the red crescent; the alpha 3 marks a hostile swing
        for(auto W:S->Heroes) if(auto* Hero=W.Get()) {
            if(Hero->bDowned) continue;
            const FVector To=Hero->GetActorLocation()-Here;
            if(!ER::StrikeHits(To.Size2D(),FVector::DotProduct(Aim,To.GetSafeNormal2D()),Radius)||!AS::ClearSight(World,Here,Hero->GetActorLocation())) continue;
            const bool Vulnerable=Now>=Hero->InvulnerableUntil&&!Hero->HammerInvulnerable();
            Hero->Hurt(Damage,this);
            if(Knock>0.f&&Vulnerable) Hero->LaunchCharacter(Aim*Knock+FVector(0,0,Lift),true,true);
        }
    };
    switch(Id) {
    case ER::AbilityShove: { // a shoulder-barge: short, wide, throws the hero back
        ++Pattern;Swing(ER::ShoveRadius,ER::ShoveDamage,ER::ShoveKnockback,ER::ShoveLift);
        return FlowFinish;
    }
    case ER::AbilityBackHop: { // a sudden hop away from a hero that has closed in; the direction is the nearest clear one around "straight away"
        ++Pattern;
        if(!Target||!GetCharacterMovement()) return FlowFinish;
        const FVector To=Target->GetActorLocation()-Here;float Dx=0.f,Dy=0.f;
        const bool Found=ER::BackHopDirection(To.X,To.Y,[&](float X,float Y) {
            if(!S->Arena) return true;
            for(float D=ER::BackHopProbeStep;D<=ER::BackHopDistance+1.f;D+=ER::BackHopProbeStep) if(!S->Arena->CanWalkDirectly(Here,Here+FVector(X,Y,0.f)*D)) return false;
            return true;
        },Dx,Dy);
        if(!Found) return FlowFinish; // boxed in: no hop (it recovers and shoots instead)
        HopDirection=FVector(Dx,Dy,0.f);HopUntil=Now+ER::BackHopSeconds;
        GetCharacterMovement()->Velocity=HopDirection*ER::BackHopSpeed; // full speed at once: the push-off is not a ramp
        S->Effect(11,Feet,FVector::UpVector,FLinearColor(.55f,.55f,.6f),1.f); // a puff of dust where it left
        return FlowAttack;
    }
    case ER::AbilityParry: // the stance ran out; if it blocked a blow in its last moments the counter is not late
        if(bParryBlocked) {RiposteAt=-1.f;bParryBlocked=false;BeginAbility(ER::AbilityRiposte,ToTarget);return FlowChained;}
        return FlowFinish;
    case ER::AbilityRiposte: // the fast counter-slash out of a block
        ++Pattern;Swing(ER::RiposteRadius,ER::RiposteDamage,0.f,0.f);
        return FlowFinish;
    case ER::AbilityPound: { // a shockwave ring from the feet; ground contact when its front passes is what hurts
        ++Pattern;
        AASShockwave::Spawn(World,Feet,this,ER::PoundDamage,ER::PoundMaxRadius);
        S->Effect(6,Feet,FVector::UpVector,FLinearColor(1.f,.3f,.05f),360.f);                // the impact flash (and its sound)
        S->Effect(11,Feet,FVector::UpVector,FLinearColor(.75f,.6f,.45f),2.f);                // thrown rubble
        return FlowFinish;
    }
    case ER::AbilityEnrage: { // the roar is over: faster, quicker to wind up, quicker to recharge, and hot red from now on
        Flags|=uint8(ER::FlagEnraged);ForceNetUpdate();
        S->Effect(11,Here+FVector(0,0,70),FVector::UpVector,FLinearColor(1.f,.2f,.04f),3.f);
        return FlowFinish;
    }
    case ER::AbilityBlink: { // vanish and reappear behind the hero, then strike (the strike has its own, shorter, warning)
        ++Pattern;
        if(!Target) return FlowFinish;
        const FVector HeroAt=Target->GetActorLocation();FVector Face=Target->GetActorForwardVector();Face.Z=0.f;
        float Ox[5],Oy[5];const int32 Count=ER::BlinkCandidates(Face.X,Face.Y,ER::BlinkBehind,Ox,Oy);
        bool Found=false;FVector Dest=Here;
        for(int32 I=0;I<Count&&!Found;++I) {
            const FVector P(HeroAt.X+Ox[I],HeroAt.Y+Oy[I],Here.Z);
            if(S->Arena&&!S->Arena->CanWalkDirectly(FVector(HeroAt.X,HeroAt.Y,Here.Z),P)) continue; // the spot must be walkable from where the hero stands
            bool Crowded=false;
            for(auto W:S->Enemies) if(const auto* Other=W.Get()) if(Other!=this&&FVector::DistSquared2D(Other->GetActorLocation(),P)<140.f*140.f) {Crowded=true;break;}
            for(auto W:S->Heroes) if(const auto* Hero=W.Get()) if(Hero!=Target&&FVector::DistSquared2D(Hero->GetActorLocation(),P)<120.f*120.f) {Crowded=true;break;}
            if(!Crowded) {Dest=P;Found=true;}
        }
        if(!Found) return FlowFinish; // nowhere to land: the rift fizzles and it recovers where it stands
        const FLinearColor Violet(.55f,.25f,1.f);
        S->Effect(11,Here,FVector::UpVector,Violet,2.f);                                              // it leaves in a burst of violet shards...
        S->Effect(10,Here,(Dest-Here).GetSafeNormal(),Violet,FVector::Dist(Here,Dest));               // ...along a thin violet trail...
        SetActorLocation(Dest,false,nullptr,ETeleportType::TeleportPhysics);
        if(auto* Move=GetCharacterMovement()) Move->StopMovementImmediately();
        LastRoutePosition=Dest;NextRoute=0.f;
        S->Effect(11,Dest,FVector::UpVector,Violet,2.f);                                              // ...and arrives in another
        BeginAbility(ER::AbilityBlinkStrike,HeroAt-Dest);
        return FlowChained;
    }
    case ER::AbilityBlinkStrike: // the slash after a blink
        ++Pattern;Swing(ER::BlinkStrikeRadius,ER::BlinkStrikeDamage,0.f,0.f);
        return FlowFinish;
    case ER::AbilityMeteors: { // a volley: the first meteors land on each hero (so you must move), the others scatter around them, one after another
        ++Pattern;
        float Hx[4],Hy[4],Hz[4];int32 Heroes=0;
        for(auto W:S->Heroes) if(const auto* Hero=W.Get()) if(!Hero->bDowned&&Heroes<4) {const FVector At=Hero->GetActorLocation();Hx[Heroes]=At.X;Hy[Heroes]=At.Y;Hz[Heroes]=At.Z-77.f;++Heroes;}
        ER::MeteorPoint Points[ER::MeteorMax];
        const int32 Count=ER::MeteorVolley(Hx,Hy,Heroes,ER::MeteorCount(S->PlayerArray.Num()),GetUniqueID()*31u+uint32(Pattern),Points);
        for(int32 I=0;I<Count;++I) {
            FVector At(Points[I].X,Points[I].Y,Hz[I%Heroes]);
            if(S->Arena) At=S->Arena->ClampToArena(At,-80.f); // never into a wall
            if(auto* Field=AASField::Spawn(World,0,At,ER::MeteorRadius,ER::MeteorDamage,Points[I].Delay,Points[I].Delay+.4f)) Field->EnemySource=this; // the ground warning, then the blast
            S->Effect(18,At,FVector::UpVector,FLinearColor(1.f,.4f,.08f),Points[I].Delay);                 // the meteor itself falls for as long as the warning lasts
        }
        return FlowFinish;
    }
    default: return FlowNotMine;
    }
}

// The sniper's new vantage point after a shot: a point 13 to 16 m from its target on a bearing of its own, clamped into the arena and moved to the nearest free spot; it runs there
// for a few seconds at 1.5 times its speed (TickGround) and may not attack until it arrives.
void AASEnemy::StartRelocation() {
    namespace ER=ASEnemyRules;
    auto* S=AS::State(this);AASCharacter* Target=CurrentTarget.Get();if(!S||!Target) return;
    float Ox=0.f,Oy=0.f;ER::RelocateOffset(GetUniqueID()*7u+uint32(Pattern),Ox,Oy);
    FVector Goal=Target->GetActorLocation()+FVector(Ox,Oy,0.f);
    if(S->Arena) {
        Goal=S->Arena->ClampToArena(Goal,-160.f);FVector Free;
        if(!S->Arena->FreeEnemyPoint(Goal,Free)) return; // nowhere free near that spot: it stays where it is
        Goal=Free;
    }
    RelocateGoal=Goal;RelocateUntil=S->SimTime+ER::RelocateSeconds;
}

// The broodcaller's summon: eggs in a fan in front of it, each hatching into a minion after a few seconds unless a hero pops it (AASBreakable kind 2). When no egg can be laid
// (too many already, or no ground for it) the minion simply appears, as it did before 0.24.
void AASEnemy::LayEggs() {
    namespace ER=ASEnemyRules;
    auto* S=AS::State(this);auto* World=GetWorld();auto* GM=World?World->GetAuthGameMode<AASGameMode>():nullptr;if(!S||!World||!GM) return;
    int32 Live=0;for(auto W:S->Enemies) if(const auto* M=W.Get()) if(M->Summoner.Get()==this&&M->Health>0) ++Live;
    for(auto W:S->Breakables) if(const auto* Egg=W.Get()) if(Egg->Kind==2&&Egg->Mother.Get()==this) ++Live;
    const int32 Cap=S->PlayerArray.Num()>1?ER::BroodMinionCapCoop:ER::BroodMinionCap;
    const int32 Count=FMath::Min(ER::EggCount(S->PlayerArray.Num()),FMath::Max(0,Cap-Live));
    float Ox[3],Oy[3];const int32 Laid=ER::EggOffsets(Aim.X,Aim.Y,Count,Ox,Oy);
    const FVector Here=GetActorLocation();const float Floor=Here.Z-(GetCapsuleComponent()?GetCapsuleComponent()->GetScaledCapsuleHalfHeight():83.f);
    for(int32 I=0;I<Laid;++I) {
        const FVector Spot(Here.X+Ox[I],Here.Y+Oy[I],Floor);
        const bool Ground=!S->Arena||S->Arena->CanWalkDirectly(Here,FVector(Spot.X,Spot.Y,Here.Z));
        AASBreakable* Egg=Ground?AASBreakable::LayEgg(World,Spot+FVector(0,0,36),this):nullptr;
        if(Egg) S->Effect(11,Spot+FVector(0,0,20),FVector::UpVector,FLinearColor(.2f,1.f,.35f),1.f); // a small green burst where it was laid
        else GM->SpawnMinion(Here+FVector(Ox[I],Oy[I],0.f),this);
    }
}

// A Gloomwing about to dive calls the pack: the idle Gloomwings near the same target get their next dive set one stagger apart, in order of their ids, so one screech starts a chain
// of dives. Only the first of a volley calls (the rest are marked as followers), and a Gloomwing that is already winding up or diving is left alone.
void AASEnemy::CallPack(const FVector& TargetAt) {
    namespace ER=ASEnemyRules;
    auto* S=AS::State(this);if(!S) return;
    const float Now=S->SimTime;constexpr int32 MaxSeen=16;
    unsigned Ids[MaxSeen];AASEnemy* Members[MaxSeen];int32 Count=0;
    Ids[Count]=GetUniqueID();Members[Count++]=this;
    for(auto W:S->Enemies) if(auto* E=W.Get()) {
        if(Count>=MaxSeen) break;
        if(E==this||E->Kind!=ASExpedition::FlierKind||E->Boss()||E->Health<=0||E->bWinding||E->DiveUntil>Now||E->SpawnProtected()) continue;
        if(FVector::DistSquared2D(E->GetActorLocation(),TargetAt)>FMath::Square(ER::PackRadius)) continue;
        Ids[Count]=E->GetUniqueID();Members[Count++]=E;
    }
    if(Count<2) return;
    float Delays[MaxSeen];
    if(ER::PackDelays(Ids,Count,GetUniqueID(),Delays)<2) return;
    for(int32 I=0;I<Count;++I) if(Members[I]!=this&&Delays[I]>0.f) {Members[I]->NextAttack=Now+Delays[I];Members[I]->bPackFollower=true;}
    Flags|=uint8(ER::FlagPackCall);ForceNetUpdate(); // every machine hears this wind-up as the pack's call
}

// The sniper's aiming beam: amber while it tracks the hero, white-amber, thicker and brighter once it has locked (the last 30% of the wind-up).
void AASEnemy::ShowBeam(float Progress) {
    if(!Beam) return;
    const bool Locked=ASEnemyRules::SnipeLocked(Progress);
    if(Locked==bBeamLocked) return;
    bBeamLocked=Locked;if(Locked) PlayCue(ASEnemyRules::CueSnipeLock); // the aim locks: a click, and the whine cuts off
    ASAssets::Style(Beam,TEXT("Cube"),Locked?FLinearColor(1.f,.93f,.62f):FLinearColor(1.f,.62f,.12f),0,Locked?12.f:5.f);
    Beam->SetRelativeScale3D(FVector(17.f,Locked?.075f:.035f,Locked?.075f:.035f));
}
