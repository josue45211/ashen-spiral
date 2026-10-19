#pragma once
#include <algorithm>
#include <cmath>
#include "ASEnemyRules.h"
// The geometry and numbers of the 0.24 enemy abilities, as plain C++ (nothing here touches Unreal types; Tests/enemy_ability_024_tests.cpp exercises all of it). ASEnemy.cpp and
// ASEnemyAbilities.cpp apply these rules in the world; every number is TUNABLE. Colours follow Docs/TELEGRAPH_LANGUAGE_0.24.md.
namespace ASEnemyRules {

// ---- Shove (grunt): a short, wide shoulder-barge. Damage lands in the same half disc the warning draws (StrikeHits), and the hero is thrown back. ----
constexpr float ShoveRadius=260.f,ShoveDamage=7.f,ShoveKnockback=520.f,ShoveLift=140.f;

// ---- Back-hop (archer): a sudden hop away from a hero that has closed in, with a smoke puff at each end. ----
constexpr float BackHopDistance=520.f,BackHopSpeed=1900.f,BackHopSeconds=.28f,BackHopProbeStep=130.f;
// The direction to hop: straight away from the target, or the nearest clear direction in a fan around it (ChooseClearDirection). False when every direction is blocked.
template<class ClearFn> inline bool BackHopDirection(float ToX,float ToY,ClearFn&& Clear,float& OutX,float& OutY) {return ChooseClearDirection(-ToX,-ToY,Clear,OutX,OutY);}

// ---- Parry (duelist): a guard stance that takes almost no damage and, if it blocked a hit, answers with a fast counter-slash. ----
constexpr float ParryStanceSeconds=.95f,ParryDamageScale=.15f,ParryRiposteDelay=.15f,RiposteRadius=260.f,RiposteDamage=16.f;
inline float ParryDamage(bool InStance,float Damage) {return InStance?Damage*ParryDamageScale:Damage;}
// The guard holds against a direct blow from its front half (a source-less blow, a bolt of lightning, counts as frontal); status damage ticking in and a blow from behind
// (a second hero flanking it) go through at full strength, and neither draws a counter.
constexpr float ParryFrontDot=0.f;
inline bool ParryBlocks(bool InStance,bool Direct,float ForwardDot) {return InStance&&Direct&&(!std::isfinite(ForwardDot)||ForwardDot>ParryFrontDot);}

// ---- Ground pound (brute): a shockwave ring expanding from its feet. Standing on the ground when the front passes hurts; jumping over it does not. ----
constexpr float PoundStartRadius=140.f,PoundMaxRadius=720.f,PoundSpeed=900.f,PoundBand=70.f,PoundJumpClearance=45.f,PoundDamage=18.f;
inline float ShockwaveRadius(float Age,float Start,float Speed,float Max) {
    if(!std::isfinite(Age)||!std::isfinite(Start)||!std::isfinite(Speed)||!std::isfinite(Max)) return 0.f;
    return std::clamp(Start+std::max(0.f,Age)*Speed,0.f,std::max(0.f,Max));
}
// Whether a hero is caught by the front as it moved from PrevRadius to Radius this tick: it must be on the ground and inside the swept band, widened by Band on each side so
// a slow frame can never step over anyone. Height is the hero's feet above the floor, Clearance how high a jump must lift it. The first tick of a ring is swept from radius 0 (not
// from its start radius), so a hero standing right under the enemy is caught as well.
inline bool ShockwaveHits(float PrevRadius,float Radius,float Distance,bool Grounded,float Height,float Band,float Clearance) {
    if(!std::isfinite(PrevRadius)||!std::isfinite(Radius)||!std::isfinite(Distance)||!std::isfinite(Height)) return false;
    if(!Grounded||Height>Clearance) return false;
    return Distance>=PrevRadius-Band&&Distance<=Radius+Band;
}
constexpr float EnrageHealthFraction=.5f,EnrageRoarSeconds=1.2f,EnrageSpeedScale=1.25f,EnrageWindupScale=.8f,EnrageCooldownScale=.6f;
inline bool EnrageDue(bool Enraged,float Health,float MaxHealth) {
    return !Enraged&&std::isfinite(Health)&&std::isfinite(MaxHealth)&&MaxHealth>0.f&&Health>0.f&&Health<=MaxHealth*EnrageHealthFraction;
}

// ---- Hazards: lingering ground zones that hurt a hero standing in them. A zone is drawn as a flat disc in its own colour, shrinks away in its last fifth, and ticks. ----
constexpr int HazardFire=0,HazardFrost=1,HazardVenom=2,HazardStorm=3,HazardVoid=4,HazardStyles=5;
struct HazardSpec {float Radius,Seconds,Damage,Interval;float R,G,B;}; // colour: fire red-orange, frost cyan (blue is frost only), venom green, storm violet-white, void deep violet
inline const HazardSpec& HazardFor(int Style) {
    static constexpr HazardSpec Table[HazardStyles]={
        {170.f,3.5f,7.f,.5f,1.00f,.20f,.03f}, // fire (burning elite's trail, the horticulturist's embers)
        {300.f,5.0f,5.f,.5f,.15f,.95f,1.00f}, // frost (frostbound elite's slam)
        {240.f,6.0f,6.f,.5f,.25f,1.00f,.30f}, // venom (venomous elite, seed bombs)
        {260.f,3.0f,9.f,.5f,.86f,.74f,1.00f}, // storm
        {330.f,6.0f,8.f,.5f,.42f,.12f,.75f},  // void (the horticulturist's blooms, the sovereign's roots)
    };
    return Table[std::clamp(Style,0,HazardStyles-1)];
}
inline bool HazardContains(float Distance2D,float DeltaZ,float Radius) {return std::isfinite(Distance2D)&&std::isfinite(DeltaZ)&&Distance2D<Radius&&std::fabs(DeltaZ)<=150.f;}
// The disc keeps its size for the first 80% of its life, then shrinks to a fifth.
inline float HazardScale(float Age,float Seconds) {
    if(!std::isfinite(Age)||!std::isfinite(Seconds)||!(Seconds>0.f)) return 0.f;
    const float T=std::clamp(Age/Seconds,0.f,1.f);
    return T<.8f?1.f:1.f-.8f*((T-.8f)/.2f);
}

// ---- Guard break (bulwark): a heavy blow from the front (the hammer's slam is one) rips its guard open for a few seconds and staggers it. ----
constexpr float GuardBreakFraction=.12f,GuardBreakSeconds=3.5f,GuardBreakStagger=.8f,GuardFrontDot=.35f;
inline bool GuardBreaks(float Damage,float MaxHealth,bool FromFront,bool AlreadyBroken) {
    return !AlreadyBroken&&FromFront&&std::isfinite(Damage)&&std::isfinite(MaxHealth)&&MaxHealth>0.f&&Damage>=MaxHealth*GuardBreakFraction;
}
// The old front-guard rule: half damage from in front, unless the guard is broken.
inline float GuardDamageScale(bool Broken,float ForwardDot) {return (!Broken&&std::isfinite(ForwardDot)&&ForwardDot>GuardFrontDot)?.5f:1.f;}

// ---- Meteors (shaman): a volley whose first meteors land on each hero (so you must move) and whose others scatter around the heroes, kept apart, landing one after another. ----
constexpr int MeteorsSolo=3,MeteorsCoop=5,MeteorMax=8;
constexpr float MeteorRadius=200.f,MeteorDamage=18.f,MeteorFirstDelay=.85f,MeteorStagger=.30f,MeteorSpacing=260.f,MeteorScatterMin=120.f,MeteorScatterMax=420.f;
struct MeteorPoint {float X=0.f,Y=0.f,Delay=0.f;};
inline int MeteorCount(int Players) {return Players>1?MeteorsCoop:MeteorsSolo;}
// Fills Out[0..Count) and returns how many it wrote (0 with no heroes). Deterministic in Seed; the caller clamps the points into the arena.
inline int MeteorVolley(const float* HeroX,const float* HeroY,int Heroes,int Count,unsigned Seed,MeteorPoint* Out) {
    if(!HeroX||!HeroY||!Out||Heroes<=0||Count<=0) return 0;
    Count=std::min(Count,MeteorMax);
    unsigned State=Seed*2654435761u+12345u;
    auto Next=[&State]() {State=State*1664525u+1013904223u;return float((State>>8)&0xFFFF)/65535.f;};
    for(int I=0;I<Count;++I) {
        MeteorPoint P;const int Who=I%Heroes;
        if(I<Heroes) {P.X=HeroX[Who];P.Y=HeroY[Who];}
        else {
            for(int Try=0;Try<8;++Try) {
                const float Angle=Next()*6.2831853f,Distance=MeteorScatterMin+Next()*(MeteorScatterMax-MeteorScatterMin);
                P.X=HeroX[Who]+std::cos(Angle)*Distance;P.Y=HeroY[Who]+std::sin(Angle)*Distance;
                bool Apart=true;for(int J=0;J<I;++J) if(std::hypot(P.X-Out[J].X,P.Y-Out[J].Y)<MeteorSpacing) Apart=false;
                if(Apart) break;
            }
        }
        P.Delay=MeteorFirstDelay+float(I)*MeteorStagger;Out[I]=P;
    }
    return Count;
}

// ---- Rift blink (leaper): vanish and reappear behind the hero, leaving a violet trail, then strike. ----
constexpr float BlinkBehind=260.f,BlinkStrikeRadius=260.f,BlinkStrikeDamage=16.f;
// Candidate reappearance points as offsets from the hero, best first: directly behind it (opposite its facing), then either side behind, then beside it, then in front.
inline int BlinkCandidates(float FaceX,float FaceY,float Distance,float* OutX,float* OutY) {
    float x=FaceX,y=FaceY;
    if(!std::isfinite(x)||!std::isfinite(y)||!Detail::Unit(x,y)) {x=1.f;y=0.f;}
    static constexpr float Angles[]={180.f,140.f,-140.f,90.f,-90.f};
    for(int I=0;I<5;++I) {float Rx,Ry;Detail::Rotate(x,y,Angles[I],Rx,Ry);OutX[I]=Rx*Distance;OutY[I]=Ry*Distance;}
    return 5;
}

// ---- Egg sacs (broodcaller): it lays eggs instead of conjuring minions outright. An egg hatches into a minion after EggHatchSeconds unless a hero pops it first. ----
constexpr float EggHatchSeconds=3.2f,EggHealth=22.f,EggDistance=230.f,EggSpreadDegrees=50.f;
constexpr int EggMaxLive=6,EggsSolo=2,EggsCoop=3;
inline int EggCount(int Players) {return Players>1?EggsCoop:EggsSolo;}
// Where the eggs go, as offsets (cm) from the mother in the world frame: a fan around its aim, one egg straight ahead, two to either side, three the two and the middle. Returns how
// many were written (0..3).
inline int EggOffsets(float AimX,float AimY,int Count,float* OutX,float* OutY) {
    float x=AimX,y=AimY;
    if(!std::isfinite(x)||!std::isfinite(y)||!Detail::Unit(x,y)) {x=1.f;y=0.f;}
    static constexpr float Fan[4][3]={{0.f,0.f,0.f},{0.f,0.f,0.f},{-1.f,1.f,0.f},{-1.f,0.f,1.f}};
    Count=std::clamp(Count,0,3);
    for(int I=0;I<Count;++I) {float Rx,Ry;Detail::Rotate(x,y,Fan[Count][I]*EggSpreadDegrees,Rx,Ry);OutX[I]=Rx*EggDistance;OutY[I]=Ry*EggDistance;}
    return Count;
}
inline bool EggHatchDue(float Now,float HatchAt) {return std::isfinite(Now)&&std::isfinite(HatchAt)&&Now>=HatchAt;}
// How far through its hatching an egg is, 0..1 (it pulses faster as this nears 1).
inline float EggProgress(float Now,float HatchAt,float Seconds) {
    if(!std::isfinite(Now)||!std::isfinite(HatchAt)||!(Seconds>0.f)) return 0.f;
    return std::clamp(1.f-(HatchAt-Now)/Seconds,0.f,1.f);
}

// ---- Pack dive (Gloomwing): the fliers of a pack attack in a volley instead of one at a time. When one is ready to dive it calls the pack: the others near the same target have
// their next dive set in order of their ids, PackStagger seconds apart, so one screech starts a chain of dives a player can read. At most PackMax take part. ----
constexpr float PackRadius=1700.f,PackStagger=.38f;
constexpr int PackMax=4;
// Fills OutDelays[i] with the delay (seconds) before member i's dive: the caller 0, the rest in ascending id order PackStagger apart, and -1 for any member beyond PackMax (not in
// this volley). Returns how many are in the volley (0 when the caller is not among the members).
inline int PackDelays(const unsigned* Ids,int Count,unsigned Caller,float* OutDelays) {
    if(!Ids||!OutDelays||Count<=0) return 0;
    for(int I=0;I<Count;++I) OutDelays[I]=-1.f;
    int InPack=0;
    for(int I=0;I<Count;++I) if(Ids[I]==Caller) {OutDelays[I]=0.f;InPack=1;break;}
    if(InPack==0) return 0;
    while(InPack<PackMax) {
        int Best=-1;
        for(int I=0;I<Count;++I) if(OutDelays[I]<0.f&&Ids[I]!=Caller&&(Best<0||Ids[I]<Ids[Best])) Best=I;
        if(Best<0) break;
        OutDelays[Best]=float(InPack)*PackStagger;++InPack;
    }
    return InPack;
}

// ---- Elite mutations (0.24): an elite brute may spawn with a mutation. It adds one signature ability to its kit (the attack set changes, not only the numbers), tints its body
// in the mutation's colour (a slow pulse, never a ring on the floor) and a burning one leaves fire behind it as it walks. Frost is the only blue in the game; storm is violet-white;
// fire is orange-red; venom is poison green. ----
constexpr int MutationNone=0,MutationFrost=1,MutationFire=2,MutationStorm=3,MutationVenom=4,MutationCount=5;
inline int MutationFromRoll(int Roll) {return 1+std::clamp(Roll,0,MutationCount-2);} // Roll in 0..3
inline int MutationAbility(int Mutation) {
    switch(Mutation) {
    case MutationFrost: return AbilityGlacial;
    case MutationFire:  return AbilityCinders;
    case MutationStorm: return AbilityStorm;
    case MutationVenom: return AbilitySeeds;
    default: return AbilityNone;
    }
}
inline int MutationCue(int Mutation) {
    switch(Mutation) {
    case MutationFrost: return CueMutationFrost;
    case MutationFire:  return CueMutationFire;
    case MutationStorm: return CueMutationStorm;
    case MutationVenom: return CueMutationVenom;
    default: return CueNone;
    }
}
inline void MutationColor(int Mutation,float& R,float& G,float& B) {
    switch(Mutation) {
    case MutationFrost: R=.12f;G=1.f;B=1.f;break;   // cyan
    case MutationFire:  R=1.f;G=.35f;B=.05f;break;  // orange-red
    case MutationStorm: R=.86f;G=.74f;B=1.f;break;  // violet-white
    case MutationVenom: R=.25f;G=1.f;B=.30f;break;  // poison green
    default: R=G=B=0.f;break;
    }
}
// The mutated kit: the base kit with the mutation's signature ability in its first free slot (once).
inline Kit KitWithMutation(const Kit& Base,int Mutation) {
    Kit Out=Base;const int Id=MutationAbility(Mutation);
    if(Id==AbilityNone) return Out;
    for(int I=0;I<MaxKitAbilities;++I) {
        if(Out.Abilities[I]==Id) return Out;
        if(Out.Abilities[I]==AbilityNone) {Out.Abilities[I]=Id;return Out;}
    }
    return Out;
}
constexpr float TrailInterval=1.6f;                                               // a burning elite drops a fire pool this often while it walks
constexpr float GlacialRadius=380.f,GlacialDamage=20.f;                            // the frostbound elite's slam: damage inside its warning disc, and a frost zone left behind
constexpr int CinderPools=3;constexpr float CinderSpacing=280.f,CinderFirstDelay=.80f,CinderStagger=.25f,CinderImpact=10.f; // fire pools in a line toward the hero
constexpr float StormImpact=18.f;                                                  // each lightning strike of the shocking elite (positions: MeteorVolley, delays: MeteorFirstDelay/MeteorStagger)
constexpr float SeedImpact=8.f,SeedFirstDelay=1.0f,SeedStagger=.25f;               // each pod of the venomous elite
// The sound a hazard of each style makes where it arms (the warning is over and it bites): fire a blast, frost a slam, storm a boom with crackle, venom a squelch.
inline int HazardArmCue(int Style) {
    switch(Style) {
    case HazardFire:  return CueBlastBurst;
    case HazardFrost: return CuePoundSlam;
    case HazardStorm: return CueMeteorImpact;
    case HazardVenom: return CueEggLay;
    default:          return CueBlastBurst;
    }
}

// ---- Sniper: the aiming beam tracks the hero for the first part of the wind-up, then locks and fires along the locked line. ----
constexpr float SnipeTrackFraction=.70f,SnipeTurnRate=75.f;
inline bool SnipeLocked(float Progress) {return std::isfinite(Progress)&&Progress>=SnipeTrackFraction;}
// Turns Current toward Desired (degrees) by at most Rate * Dt, the short way round.
inline float TrackYaw(float Current,float Desired,float Dt,float Rate) {
    if(!std::isfinite(Current)||!std::isfinite(Desired)||!std::isfinite(Dt)||!std::isfinite(Rate)) return std::isfinite(Current)?Current:0.f;
    float Delta=std::fmod(Desired-Current,360.f);
    if(Delta>180.f) Delta-=360.f;
    if(Delta<-180.f) Delta+=360.f;
    const float Step=std::max(0.f,Dt)*std::max(0.f,Rate);
    return Current+std::clamp(Delta,-Step,Step);
}
// After a shot the sniper moves to a new vantage point on a ring around its target.
constexpr float RelocateMin=1300.f,RelocateMax=1600.f,RelocateSeconds=3.f,RelocateSpeed=1.5f,RelocateArrive=140.f;
inline void RelocateOffset(unsigned Seed,float& OutX,float& OutY) {
    unsigned State=Seed*2654435761u+977u;
    auto Next=[&State]() {State=State*1664525u+1013904223u;return float((State>>8)&0xFFFF)/65535.f;};
    const float Angle=Next()*6.2831853f,Distance=RelocateMin+Next()*(RelocateMax-RelocateMin);
    OutX=std::cos(Angle)*Distance;OutY=std::sin(Angle)*Distance;
}
}
