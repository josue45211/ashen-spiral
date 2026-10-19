#pragma once
#include <algorithm>
#include <cmath>
// Engine-independent enemy rules (0.24). The kind ids are the long-standing runtime ids (see ASEnemy.cpp RefreshLook); nothing here touches Unreal types,
// so Tests/enemy_024_tests.cpp can exercise it as plain C++. TUNABLE values are marked.
namespace ASEnemyRules {
constexpr int Grunt=0,Archer=1,Duelist=2,Brute=3,BossKind=4,Leaper=5,Bulwark=6,Shaman=7,Sniper=8,Broodcaller=9,Gloomwing=10,KindCount=11;

// ---- Melee swing (kinds 0, 2, 3) ----
// The ground warning and the damage test share these numbers: the warning is a half disc (SM_StrikeSector, +-90 degrees) of StrikeRadius in front of
// the enemy, and a hero is hit only inside exactly that shape. A circle on screen therefore always means "damage lands here". TUNABLE.
constexpr float MeleeRadius=210.f,EliteMeleeRadius=340.f,MeleeDamage=12.f,EliteMeleeDamage=24.f;
inline float StrikeRadius(bool Elite) {return Elite?EliteMeleeRadius:MeleeRadius;}
inline float StrikeDamage(bool Elite) {return Elite?EliteMeleeDamage:MeleeDamage;}
// Distance2D is measured from the enemy's centre; ForwardDot is dot(aim, direction to the hero), both flat.
inline bool StrikeHits(float Distance2D,float ForwardDot,float Radius) {
    return std::isfinite(Distance2D)&&std::isfinite(ForwardDot)&&std::isfinite(Radius)&&Distance2D<Radius&&ForwardDot>0.f;
}

// ---- Abilities (replicated byte; 0 = none) ----
// The id of the attack being wound up, so every machine can pose the right wind-up. New kits extend this list; ids are never reused.
constexpr int AbilityNone=0,AbilityDive=1,AbilityMolt=2;

// ---- Fliers: where they may go (Gloomwing and the flying boss) ----
// Fliers ignore walls and floor collision, so they are kept inside the arena by explicit limits. TUNABLE.
constexpr float FlightFloorClearance=25.f;   // the capsule's bottom never goes below this height above the floor
constexpr float FlightCeiling=1100.f;        // the capsule's centre never goes above this height above the floor
constexpr float FlightEdgeMargin=260.f;      // Endless: stay this far inside the arena walls
constexpr float FlightRoomSlack=300.f;       // story rooms: may overhang a room by this much (corridors between rooms)
// Height band for a flier's CENTRE.
inline float ClampFlightHeight(float Z,float FloorZ,float CapsuleHalfHeight) {
    const float Low=FloorZ+std::max(0.f,CapsuleHalfHeight)+FlightFloorClearance,High=std::max(Low+50.f,FloorZ+FlightCeiling);
    if(std::isnan(Z)) return Low; // NaN lands on the lowest legal height; +-infinity clamps to the nearest limit
    return std::clamp(Z,Low,High);
}
// Clamp a point into a rectangle: NaN lands on the rectangle's centre, +-infinity on the nearest edge. The rectangle is normalised if its limits are crossed.
inline void ClampToRect(float MinX,float MaxX,float MinY,float MaxY,float& X,float& Y) {
    if(MinX>MaxX) MinX=MaxX=(MinX+MaxX)*.5f;
    if(MinY>MaxY) MinY=MaxY=(MinY+MaxY)*.5f;
    X=std::isnan(X)?(MinX+MaxX)*.5f:std::clamp(X,MinX,MaxX);
    Y=std::isnan(Y)?(MinY+MaxY)*.5f:std::clamp(Y,MinY,MaxY);
}
// A flier circles its target. The radius "breathes" between Min and Max so melee players get regular windows when it sweeps within reach:
// Min at the start of each cycle, Max half a cycle later.
inline float OrbitRadius(float Seconds,float Min,float Max,float CycleSeconds) {
    if(!std::isfinite(Seconds)||!(CycleSeconds>0.f)) return Max;
    const float Phase=Seconds/CycleSeconds*6.2831853f;
    return Min+(Max-Min)*(.5f-.5f*std::cos(Phase));
}

// ---- Flying boss: the molt ----
// At half health the boss rears back (MoltWindupSeconds, telegraphed by the wings), then bursts out hatchlings (small Gloomwings) from its wings.
// MoltDue is true once per fight. TUNABLE.
constexpr float MoltHealthFraction=.5f,MoltWindupSeconds=1.5f,HatchlingHealthScale=.6f;
constexpr int MoltHatchlingsSolo=4,MoltHatchlingsCoop=6;
inline bool MoltDue(bool AlreadyMolted,float Health,float MaxHealth) {
    return !AlreadyMolted&&std::isfinite(Health)&&std::isfinite(MaxHealth)&&MaxHealth>0.f&&Health>0.f&&Health<=MaxHealth*MoltHealthFraction;
}
inline int MoltHatchlings(int Players) {return Players>1?MoltHatchlingsCoop:MoltHatchlingsSolo;}

// ---- Broodcaller ----
// It summons minions in pairs whenever it has room under the cap and its summon cooldown is over; otherwise it shoots. (Before 0.24 it summoned on its
// first two attacks only, because a lifetime attack counter was compared with 2 and never reset.) TUNABLE.
constexpr int BroodMinionPair=2,BroodMinionCap=4,BroodMinionCapCoop=6;
constexpr float BroodSummonCooldown=6.5f;
inline bool BroodShouldSummon(int Live,int Cap,float Now,float NextSummon) {
    if(!std::isfinite(Now)||!std::isfinite(NextSummon)) return false;
    return Now>=NextSummon&&std::max(0,Live)+BroodMinionPair<=std::max(BroodMinionPair,Cap);
}
}
