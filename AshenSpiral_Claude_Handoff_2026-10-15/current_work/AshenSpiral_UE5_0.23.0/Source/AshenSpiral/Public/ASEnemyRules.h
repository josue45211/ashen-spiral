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
