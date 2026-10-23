#include "ASEnemyAbilities.h"
#include "ASEnemy.h"
#include "ASEnemyRules.h"
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

// Where a maul slam lands: in front of the brute, at the reach of the head of its maul (ASEnemyRules::SlamReach), on the floor. Every other ability has no reach and lands at the feet.
FVector AASEnemy::SlamPointFor(int32 AbilityId,const FVector& Feet) const {
    FVector Forward=GetActorForwardVector();Forward.Z=0.f;
    return Feet+Forward.GetSafeNormal()*ASEnemyRules::SlamReach(AbilityId,BodyScale);
}

// The warning on the floor for the ability being wound up (the server never draws it). A half disc for the swings, a full disc for the ground pound; the dim shape shows the whole
// area at once, the bright one grows over it as the wind-up completes. Neither is a ring: a ring on screen is not a thing an enemy puts on the floor.
void AASEnemy::ShowTell(float Progress) {
    namespace ER=ASEnemyRules;
    if(!Tell||!TellFill||!GetCapsuleComponent()) return;
    const ER::AbilitySpec& Spec=ER::SpecFor(Ability);
    const int32 Shape=Spec.Tell;const bool Disc=Shape==ER::TellDisc,Line=Shape==ER::TellLine,Ring=Shape==ER::TellRing,HasSector=ASAssets::HasMesh(TEXT("SM_StrikeSector"));
    if(Shape!=TellShape) { // a different shape than the one last drawn: swap both meshes (this happens only when an enemy with two kinds of warning switches)
        if(Ring) { // a travelling shockwave: an outline where the wave ends, two hot ripples racing out to it (never a filled disc, which means "leave this area")
            ASAssets::Style(Tell,TEXT("SM_Ring"),FLinearColor(1.f,.20f,.03f),0,3.f);ASAssets::Style(TellFill,TEXT("SM_Ring"),FLinearColor(1.f,.78f,.30f),0,8.f);
            if(TellRipple) {ASAssets::Style(TellRipple,TEXT("SM_Ring"),FLinearColor(1.f,.78f,.30f),0,8.f);TellRipple->SetCastShadow(false);}
        } else {
            const FString TellMesh=Disc?FString(TEXT("Cylinder")):(Line?FString(TEXT("Cube")):(HasSector?FString(TEXT("SM_StrikeSector")):FString(TEXT("SM_Ring"))));
            ASAssets::Style(Tell,TellMesh,FLinearColor(.55f,.05f,.02f),0,1.5f);ASAssets::Style(TellFill,TellMesh,FLinearColor(1.f,.16f,.03f),0,6.f);
        }
        Tell->SetCastShadow(false);TellFill->SetCastShadow(false);TellShape=Shape;
    }
    const float Floor=-GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    if(Ring) { // 0.24.2 (F6): the ripples quicken as the wind-up completes, like a heartbeat; each eases out as it nears the outline, the way a wave slows
        const float Radius=ER::TellRadiusFor(Ability,Elite()),Reach=ER::SlamReach(Ability,BodyScale),Step=GetWorld()?GetWorld()->GetDeltaSeconds():.016f;
        Tell->SetRelativeLocation(FVector(Reach,0.f,Floor+3.f));Tell->SetRelativeScale3D(FVector(Radius/50.f,Radius/50.f,4.f));
        RipplePhase=FMath::Fmod(RipplePhase+Step*FMath::Lerp(1.3f,3.2f,Progress),1.f);
        for(int32 I=0;I<2;++I) if(UStaticMeshComponent* Ripple=I==0?TellFill.Get():TellRipple.Get()) {
            const float Phase=FMath::Fmod(RipplePhase+float(I)*.5f,1.f),Out=FMath::Max(25.f,Radius*(1.f-FMath::Square(1.f-Phase)));
            Ripple->SetRelativeLocation(FVector(Reach,0.f,Floor+5.f));Ripple->SetRelativeScale3D(FVector(Out/50.f*1.f,Out/50.f*1.f,3.f+5.f*Progress)); // a band 9-24 cm tall: a wave, not a line
            if(auto* M=Cast<UMaterialInstanceDynamic>(Ripple->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),(4.f+8.f*Progress)*(1.f-.6f*Phase));
        }
        return;
    }
    if(Line) { // a strip from the enemy along its aim: the dim strip is the whole path, the bright one grows along it (the basic cube is 100 cm each way)
        const float Length=Spec.TellA,Width=Spec.TellB>0.f?Spec.TellB:200.f,Grown=Length*FMath::Max(.04f,Progress);
        Tell->SetRelativeLocation(FVector(Length*.5f,0.f,Floor+3.f));Tell->SetRelativeScale3D(FVector(Length/100.f,Width/100.f,.02f));
        TellFill->SetRelativeLocation(FVector(Grown*.5f,0.f,Floor+4.5f));TellFill->SetRelativeScale3D(FVector(Grown/100.f,Width/100.f*.95f,.03f));
    } else {
        const float Radius=ER::TellRadiusFor(Ability,Elite()),Unit=Disc?50.f:(HasSector?50.f:53.f),Full=Radius/Unit,Grow=Full*FMath::Max(.04f,Progress),Flat=Disc?.02f:1.f; // the basic cylinder is 100 cm across: flattened it is a disc
        const float Reach=ER::SlamReach(Ability,BodyScale); // a maul slam lands where the head of the maul does (0 for every other ability), so the warning is drawn there too
        Tell->SetRelativeLocation(FVector(Reach,0.f,Floor+3.f));TellFill->SetRelativeLocation(FVector(Reach,0.f,Floor+4.5f));
        Tell->SetRelativeScale3D(FVector(Full,Full,Flat));TellFill->SetRelativeScale3D(FVector(Grow,Grow,Flat*1.5f));
    }
    if(auto* M=Cast<UMaterialInstanceDynamic>(TellFill->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),3+Progress*6);
}

// A ground boss outside Endless wave 50+ fights with its ward's kit (ASEnemyAbilities.h, BossKitFor). The flying boss keeps its dive-and-attack loop, and the Endless wave 50+
// bosses keep the old boss attack with its lethal telegraphed patterns.
bool AASEnemy::WardBossKit() const {
    if(!Boss()||bAirborne) return false;
    const auto* S=AS::State(this);
    return !(S&&S->bEndless&&S->EndlessWave>50);
}

// A point pulled onto the map. In the Endless arena that is inside its walls by a margin (a flier stays further in), in story rooms it is over walkable floor, a room or a corridor
// between rooms, pulled in from its edge: the room boxes alone missed the corridors (a hero standing in one had its meteors dropped in the nearest room) and, with the old
// 3 m of slack, let fliers hover outside the map where no hero could reach them (0.24 playtest).
FVector AASEnemy::ConfineToMap(const FVector& P,bool bFlier) const {
    const auto* S=AS::State(this);if(!S||!S->Arena) return P;
    if(S->bEndless) return S->Arena->ClampToArena(P,-(bFlier?ASEnemyRules::FlightEdgeMargin:80.f));
    return S->Arena->ClampToFloor(P,bFlier?ASEnemyRules::FlightFloorInset:80.f);
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
    // The points of an area ability that targets every hero and scatters around them (meteors, lightning, pods): fills Points and Floor (the floor under the hero each belongs to),
    // returns how many. Deterministic in Seed (ASEnemyRules::MeteorVolley).
    auto Volley=[&](int32 Count,uint32 Seed,ER::MeteorPoint* Points,float* Floor)->int32 {
        float Hx[4],Hy[4],Hz[4];int32 Heroes=0;
        for(auto W:S->Heroes) if(const auto* Hero=W.Get()) if(!Hero->bDowned&&Heroes<4) {const FVector At=Hero->GetActorLocation();Hx[Heroes]=At.X;Hy[Heroes]=At.Y;Hz[Heroes]=At.Z-Hero->GetSimpleCollisionHalfHeight();++Heroes;}
        const int32 N=ER::MeteorVolley(Hx,Hy,Heroes,Count,Seed,Points);
        for(int32 I=0;I<N;++I) Floor[I]=Hz[I%Heroes];
        return N;
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
    case ER::AbilityPound: { // a shockwave ring from where the head of the maul meets the floor; ground contact when its front passes is what hurts
        ++Pattern;
        const FVector Slam=SlamPointFor(Id,Feet);
        AASShockwave::Spawn(World,Slam,this,ER::PoundDamage,ER::PoundMaxRadius);
        S->Effect(6,Slam,FVector::UpVector,FLinearColor(1.f,.3f,.05f),360.f);                // the impact flash (and its sound)
        S->Effect(11,Slam,FVector::UpVector,FLinearColor(.75f,.6f,.45f),2.f);                // thrown rubble
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
        ER::MeteorPoint Points[ER::MeteorMax];float Floor[ER::MeteorMax];
        const int32 Count=Volley(ER::MeteorCount(S->PlayerArray.Num()),GetUniqueID()*31u+uint32(Pattern),Points,Floor);
        for(int32 I=0;I<Count;++I) {
            FVector At(Points[I].X,Points[I].Y,Floor[I]+11.f); // the old blasts sat 77 cm under the hero's centre: 11 cm over the floor
            if(S->Arena) At=ConfineToMap(At,false);  // never into a wall
            if(auto* Field=AASField::Spawn(World,0,At,ER::MeteorRadius,ER::MeteorDamage,Points[I].Delay,Points[I].Delay+.4f)) Field->EnemySource=this; // the ground warning, then the blast
            S->Effect(18,At,FVector::UpVector,FLinearColor(1.f,.4f,.08f),Points[I].Delay);                 // the meteor itself falls for as long as the warning lasts
        }
        return FlowFinish;
    }
    case ER::AbilityGlacial: { // the frostbound elite's slam: everyone inside its warning disc is hurt, and a frost zone stays where it landed
        ++Pattern;
        const FVector Slam=SlamPointFor(Id,Feet);                                                          // where the head of the maul lands: the warning disc is drawn there too
        for(auto W:S->Heroes) if(auto* Hero=W.Get()) if(!Hero->bDowned&&FVector::Dist2D(Hero->GetActorLocation(),Slam)<ER::GlacialRadius&&AS::ClearSight(World,Here,Hero->GetActorLocation())) Hero->Hurt(ER::GlacialDamage,this);
        AASHazard::Spawn(World,Slam,ER::HazardFrost,this,0.f,0.f);
        S->Effect(6,Slam,FVector::UpVector,FLinearColor(.12f,1.f,1.f),ER::GlacialRadius);                  // a cyan flash: frost, and only frost, is blue
        S->Effect(11,Slam,FVector::UpVector,FLinearColor(.55f,.95f,1.f),2.f);                             // ice shards
        return FlowFinish;
    }
    case ER::AbilityCinders: { // the burning elite's fire pools in a line toward the hero, each with its own warning, one after another
        ++Pattern;
        for(int32 I=0;I<ER::CinderPools;++I) {
            const FVector At=Feet+Aim*(ER::CinderSpacing*float(I+1));
            if(S->Arena&&!S->Arena->CanWalkDirectly(Here,FVector(At.X,At.Y,Here.Z))) break; // the line stops at a wall
            AASHazard::Spawn(World,At,ER::HazardFire,this,ER::CinderFirstDelay+ER::CinderStagger*float(I),ER::CinderImpact);
        }
        S->Effect(11,Feet,FVector::UpVector,FLinearColor(1.f,.35f,.05f),2.f);
        return FlowFinish;
    }
    case ER::AbilityStorm: { // the shocking elite's lightning on each hero and around them: a violet-white warning, a strike, then a lingering static field
        ++Pattern;
        ER::MeteorPoint Points[ER::MeteorMax];float Floor[ER::MeteorMax];
        const int32 Count=Volley(ER::MeteorCount(S->PlayerArray.Num()),GetUniqueID()*17u+uint32(Pattern),Points,Floor);
        for(int32 I=0;I<Count;++I) {
            FVector At(Points[I].X,Points[I].Y,Floor[I]+4.f);
            if(S->Arena) At=ConfineToMap(At,false);
            AASHazard::Spawn(World,At,ER::HazardStorm,this,Points[I].Delay,ER::StormImpact);
        }
        return FlowFinish;
    }
    case ER::AbilitySeeds: { // the venomous elite's pods: they fall onto each hero and around them and burst into venom pools
        ++Pattern;
        ER::MeteorPoint Points[ER::MeteorMax];float Floor[ER::MeteorMax];
        const int32 Count=Volley(S->PlayerArray.Num()>1?4:3,GetUniqueID()*13u+uint32(Pattern),Points,Floor);
        for(int32 I=0;I<Count;++I) {
            FVector At(Points[I].X,Points[I].Y,Floor[I]+4.f);
            if(S->Arena) At=ConfineToMap(At,false);
            const float Delay=ER::SeedFirstDelay+ER::SeedStagger*float(I);
            if(AASHazard::Spawn(World,At,ER::HazardVenom,this,Delay,ER::SeedImpact)) S->Effect(18,At,FVector::UpVector,FLinearColor(.25f,1.f,.30f),Delay); // the pod falls for as long as the warning lasts
        }
        return FlowFinish;
    }
    // ---- The ward bosses (ASEnemyAbilities.h, BossKitFor). Phase two (a replicated flag) widens and doubles up. ----
    case ER::AbilityBossSlam: { // a huge shockwave ring; in the second phase a second ring follows it
        ++Pattern;const bool Two=(Flags&ER::FlagPhaseTwo)!=0;
        AASShockwave::Spawn(World,Feet,this,ER::BossSlamDamage,ER::BossSlamRadius);
        if(Two) AASShockwave::Spawn(World,Feet,this,ER::BossSlamDamage,ER::BossSlamRadius,ER::BossTollGap);
        S->Effect(6,Feet,FVector::UpVector,FLinearColor(1.f,.3f,.05f),420.f);S->Effect(11,Feet,FVector::UpVector,FLinearColor(.75f,.6f,.45f),3.f);
        return FlowFinish;
    }
    case ER::AbilityBossMarks: { // a blast mark on every hero, each with its own warning; the second phase marks wider and adds a scattered volley
        ++Pattern;const bool Two=(Flags&ER::FlagPhaseTwo)!=0;
        for(auto W:S->Heroes) if(const auto* Hero=W.Get()) if(!Hero->bDowned) {
            const FVector At=Hero->GetActorLocation()-FVector(0,0,77);
            if(auto* Field=AASField::Spawn(World,0,At,Two?ER::BossMarkRadiusTwo:ER::BossMarkRadius,ER::BossMarkDamage,ER::BossMarkDelay,ER::BossMarkDelay+.4f)) Field->EnemySource=this;
        }
        if(Two) {
            ER::MeteorPoint Points[ER::MeteorMax];float Floor[ER::MeteorMax];
            const int32 Count=Volley(3,GetUniqueID()*19u+uint32(Pattern),Points,Floor);
            for(int32 I=0;I<Count;++I) {
                FVector At(Points[I].X,Points[I].Y,Floor[I]+11.f);if(S->Arena) At=ConfineToMap(At,false);
                const float Delay=ER::BossMarkExtraDelay+.3f*float(I);
                if(auto* Field=AASField::Spawn(World,0,At,ER::BossMarkRadius,ER::BossMarkDamage,Delay,Delay+.4f)) Field->EnemySource=this;
            }
        }
        return FlowFinish;
    }
    case ER::AbilityBossRing: { // a ring of shots, turned a little further each time so the gaps are never in the same place
        ++Pattern;const int32 Count=(Flags&ER::FlagPhaseTwo)?ER::BossRingShotsTwo:ER::BossRingShots;
        for(int32 I=0;I<Count;++I) {
            const FVector Dir=FRotator(0.f,float(Pattern)*ER::BossRingTurn+float(I)*360.f/float(Count),0.f).Vector();
            if(auto* Shot=AASProjectile::Fire(World,Here+Dir*75.f+FVector(0,0,15),Dir,nullptr,2,ER::BossRingDamage,true)) Shot->EnemySource=this;
        }
        return FlowFinish;
    }
    case ER::AbilityBossCharge: { // the stampede: along the red strip, contact damage (TickGround's Attack state), faster in the second phase
        ++Pattern;ChargeUntil=Now+ER::BossChargeSeconds;
        S->Effect(11,Feet,FVector::UpVector,FLinearColor(.75f,.6f,.45f),2.f);
        return FlowAttack;
    }
    case ER::AbilityBossToll: { // two rings, one after the other: the first is jumped, the second catches the one who lands
        ++Pattern;
        AASShockwave::Spawn(World,Feet,this,ER::BossTollDamage,ER::BossTollRadius);
        AASShockwave::Spawn(World,Feet,this,ER::BossTollDamage,ER::BossTollRadius,ER::BossTollGap);
        S->Effect(6,Feet,FVector::UpVector,FLinearColor(1.f,.3f,.05f),420.f);
        return FlowFinish;
    }
    case ER::AbilityBossSeeds: { // venom pods onto each hero and around them
        ++Pattern;ER::MeteorPoint Points[ER::MeteorMax];float Floor[ER::MeteorMax];
        const int32 Count=Volley((Flags&ER::FlagPhaseTwo)?ER::BossSeedsTwo:ER::BossSeeds,GetUniqueID()*23u+uint32(Pattern),Points,Floor);
        for(int32 I=0;I<Count;++I) {
            FVector At(Points[I].X,Points[I].Y,Floor[I]+4.f);if(S->Arena) At=ConfineToMap(At,false);
            const float Delay=ER::BossSeedFirstDelay+ER::BossSeedStagger*float(I);
            if(AASHazard::Spawn(World,At,ER::HazardVenom,this,Delay,ER::BossSeedImpact)) S->Effect(18,At,FVector::UpVector,FLinearColor(.25f,1.f,.30f),Delay);
        }
        return FlowFinish;
    }
    case ER::AbilityBossBloom: { // void blooms that open under and around the heroes and stay
        ++Pattern;ER::MeteorPoint Points[ER::MeteorMax];float Floor[ER::MeteorMax];
        const int32 Count=Volley((Flags&ER::FlagPhaseTwo)?ER::BossBloomsTwo:ER::BossBlooms,GetUniqueID()*29u+uint32(Pattern),Points,Floor);
        for(int32 I=0;I<Count;++I) {
            FVector At(Points[I].X,Points[I].Y,Floor[I]+4.f);if(S->Arena) At=ConfineToMap(At,false);
            AASHazard::Spawn(World,At,ER::HazardVoid,this,ER::BossBloomFirstDelay+ER::BossBloomStagger*float(I),ER::BossBloomImpact);
        }
        return FlowFinish;
    }
    case ER::AbilityBossRoots: { // roots erupt under every hero after a second; the second phase adds two more near them
        ++Pattern;
        for(auto W:S->Heroes) if(const auto* Hero=W.Get()) if(!Hero->bDowned) {
            FVector At=Hero->GetActorLocation()-FVector(0,0,Hero->GetSimpleCollisionHalfHeight()-4.f);if(S->Arena) At=ConfineToMap(At,false);
            AASHazard::Spawn(World,At,ER::HazardVoid,this,ER::BossRootDelay,ER::BossRootImpact);
        }
        if(Flags&ER::FlagPhaseTwo) {
            ER::MeteorPoint Points[ER::MeteorMax];float Floor[ER::MeteorMax];
            const int32 Count=Volley(2,GetUniqueID()*37u+uint32(Pattern),Points,Floor);
            for(int32 I=0;I<Count;++I) {
                FVector At(Points[I].X,Points[I].Y,Floor[I]+4.f);if(S->Arena) At=ConfineToMap(At,false);
                AASHazard::Spawn(World,At,ER::HazardVoid,this,ER::BossRootExtraDelay+.3f*float(I),ER::BossRootImpact);
            }
        }
        return FlowFinish;
    }
    case ER::AbilityBossGrasp: { // every hero in reach is flung toward the boss, then a ring sweeps out: land inside it and you must jump
        ++Pattern;
        for(auto W:S->Heroes) if(auto* Hero=W.Get()) if(!Hero->bDowned) {
            const FVector To=Here-Hero->GetActorLocation();const float Distance=To.Size2D();
            if(Distance>ER::GraspRadius) continue;
            const FVector Dir=To.GetSafeNormal2D();const float Speed=ER::GraspSpeed(Distance);
            if(Speed>0.f&&!Dir.IsNearlyZero()) Hero->LaunchCharacter(Dir*Speed+FVector(0,0,ER::GraspLift),true,true);
        }
        AASShockwave::Spawn(World,Feet,this,ER::GraspRingDamage,ER::GraspRingRadius,ER::GraspRingDelay);
        S->Effect(6,Feet,FVector::UpVector,FLinearColor(.42f,.12f,.75f),ER::GraspRadius);
        return FlowFinish;
    }
    case ER::AbilityBossPhase: { // the roar: the second phase begins, and the ward's set piece happens
        Flags|=uint8(ER::FlagPhaseTwo);ForceNetUpdate();
        S->Effect(6,Feet,FVector::UpVector,FLinearColor(1.f,.5f,.1f),1100.f);S->Effect(11,Here+FVector(0,0,80),FVector::UpVector,FLinearColor(.8f,.25f,.9f),3.f);
        auto Summon=[&](int32 Count) { // grunts in a ring around the boss (nothing spawns past the enemy cap)
            for(int32 I=0;I<Count;++I) {
                const float Angle=UE_TWO_PI*float(I)/float(FMath::Max(1,Count))+.5f;
                if(auto* GM=World->GetAuthGameMode<AASGameMode>()) if(GM->SpawnMinion(Here+FVector(FMath::Cos(Angle),FMath::Sin(Angle),0.f)*420.f,this)) S->Effect(11,Here+FVector(FMath::Cos(Angle),FMath::Sin(Angle),0.f)*420.f,FVector::UpVector,FLinearColor(.8f,.25f,.9f),2.f);
            }
        };
        if(Ward==ER::BossWardIndexer) Summon(ER::PhaseClerks);       // the clerks the Indexer calls up
        else if(Ward==ER::BossWardBellwether) Summon(ER::PhaseHerd);  // the herd the Bellwether calls
        else if(Ward==ER::BossWardHorticulturist) { // the wells turn against you, and a ring of blooms opens around the boss
            for(auto W:S->Wells) if(W.IsValid()) W->bInverted=true;
            for(int32 I=0;I<ER::PhaseBloomsRing;++I) {
                const float Angle=UE_TWO_PI*float(I)/float(ER::PhaseBloomsRing);FVector At=Feet+FVector(FMath::Cos(Angle),FMath::Sin(Angle),0.f)*ER::PhaseBloomRadius;
                if(S->Arena) At=ConfineToMap(At,false);
                AASHazard::Spawn(World,At,ER::HazardVoid,this,ER::PhaseBloomDelay,ER::BossBloomImpact);
            }
        } else { // the Sovereign's roots rise near the heroes
            ER::MeteorPoint Points[ER::MeteorMax];float Floor[ER::MeteorMax];
            const int32 Count=Volley(ER::PhaseBloomsNear,GetUniqueID()*41u+uint32(Pattern),Points,Floor);
            for(int32 I=0;I<Count;++I) {
                FVector At(Points[I].X,Points[I].Y,Floor[I]+4.f);if(S->Arena) At=ConfineToMap(At,false);
                AASHazard::Spawn(World,At,ER::HazardVoid,this,ER::PhaseBloomDelay+.2f*float(I),ER::BossBloomImpact);
            }
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
        Goal=ConfineToMap(Goal,false);FVector Free;
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
