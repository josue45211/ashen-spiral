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
