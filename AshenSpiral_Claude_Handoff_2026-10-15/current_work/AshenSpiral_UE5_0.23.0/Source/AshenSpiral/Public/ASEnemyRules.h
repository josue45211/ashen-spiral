#pragma once
#include <algorithm>
#include <cmath>
// Engine-independent enemy rules (0.24). The kind ids are the long-standing runtime ids (see ASEnemy.cpp RefreshLook); nothing here touches Unreal types,
// so Tests/enemy_024_tests.cpp can exercise it as plain C++. TUNABLE values are marked.
namespace ASEnemyRules {
constexpr int Grunt=0,Archer=1,Duelist=2,Brute=3,BossKind=4,Leaper=5,Bulwark=6,Shaman=7,Sniper=8,Broodcaller=9,Gloomwing=10,KindCount=11;

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
