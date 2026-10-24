#pragma once
// 0.26 Phase 3: the skill tree. Three lanes of eight permanent nodes (MELEE, RANGED, TEAM) bought with Ash in the hub, kept on the profile with the hearth upgrades. Engine-independent rules shared by
// the game and the native tests (Tests/tree_026_tests.cpp).
//   Storage: a node is one more rank in FASBuild::Ranks, appended after the 20 hearth upgrades (index ASTree::FirstRank + node), the way ranks have always been appended: old profiles load with the new
//   ranks at 0, nothing that was saved changes meaning. The node's names and texts are the hearth table's (ASData::Hearth) so the profile code and the menu need nothing new.
//   Shape of a lane (slot 0-7): two openers, three middle nodes, two late nodes and a CAPSTONE that changes how you play. A node needs a number of the lane's other nodes first (Required), so a lane is
//   walked in order, but lanes are not locks: spread over all three, or finish one for its capstone (the chase).
//   Growth is real but capped: every number below is small, none stacks with itself, and the capstones are behaviours, not percentages.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
namespace ASTree {
constexpr int FirstRank=20;               // FASBuild::Ranks[FirstRank + node]: the tree starts after the hearth's 20 upgrades
constexpr int Lanes=3,PerLane=8,Nodes=Lanes*PerLane;
enum Lane {Melee=0,Ranged=1,Team=2};
// node ids: lane * 8 + slot
enum Node {
    HonedEdge=0,LongReach,QuickHands,IronSkin,BloodRush,LeapMaster,Executioner,Predator,               // MELEE; Predator is the capstone
    SteadyHand=8,DeepMagazine,QuickReload,EagleEye,FleetAim,LongShot,Marksman,PiercingRounds,           // RANGED; PiercingRounds
    SharedWard=16,Wellspring,Rhythm,SteadfastHands,BrandHunter,TetherWarden,Convergence,SynergyEcho     // TEAM; SynergyEcho
};
constexpr int Required[PerLane]={0,0,2,2,2,5,5,7};   // nodes of the same lane already owned before this slot can be bought
constexpr int CostOf[PerLane]={15,15,30,30,30,50,50,120}; // Ash
inline bool Valid(int N) {return N>=0&&N<Nodes;}
inline int LaneOf(int N) {return Valid(N)?N/PerLane:0;}
inline int SlotOf(int N) {return Valid(N)?N%PerLane:0;}
inline bool IsCapstone(int N) {return Valid(N)&&SlotOf(N)==PerLane-1;}
inline int Tier(int N) {const int S=SlotOf(N);return S<2?1:(S<5?2:(S<7?3:4));}   // 1 openers, 2 middle, 3 late, 4 the capstone
inline int Cost(int N) {return Valid(N)?CostOf[SlotOf(N)]:0;}
inline int TotalCost() {int T=0;for(int N=0;N<Nodes;++N) T+=Cost(N);return T;}
inline int LaneCost(int Lane) {int T=0;for(int S=0;S<PerLane;++S) T+=CostOf[S];(void)Lane;return T;}

// A set of owned nodes as bits (bit N = node N).
using Owned=std::uint32_t;
inline Owned Bit(int N) {return Valid(N)?(Owned(1)<<N):0u;}
inline bool Owns(Owned M,int N) {return Valid(N)&&(M&Bit(N))!=0;}
inline int CountInLane(Owned M,int Lane) {int C=0;for(int S=0;S<PerLane;++S) if(M&Bit(Lane*PerLane+S)) ++C;return C;}
inline Owned FromRankData(const int* Ranks,int Count) {Owned M=0;if(!Ranks) return M;for(int N=0;N<Nodes;++N) if(FirstRank+N<Count&&Ranks[FirstRank+N]>0) M|=Bit(N);return M;}
inline Owned FromRanks(const std::vector<int>& Ranks) {return FromRankData(Ranks.data(),int(Ranks.size()));}
inline bool LaneComplete(Owned M,int Lane) {return CountInLane(M,Lane)==PerLane;}
// What a node needs before it can be bought: it is not owned yet, and the lane already holds Required nodes (the capstone: the other seven).
inline bool Available(Owned M,int N) {
    if(!Valid(N)||Owns(M,N)) return false;
    return CountInLane(M,LaneOf(N))>=Required[SlotOf(N)];
}
// Ash after buying (or -1 when it cannot be bought now).
inline int Buy(Owned& M,int Ash,int N) {
    if(!Available(M,N)||Ash<Cost(N)) return -1;
    M|=Bit(N);return Ash-Cost(N);
}

// ---- the effects (TUNABLE; each is a permanent, capped bonus or a behaviour) ----
// MELEE (while a melee weapon is in your hands, the claws included)
constexpr float HonedEdgeDamage=.05f;            // melee weapon damage
constexpr float LongReachBonus=.08f;             // melee reach
constexpr float QuickHandsSpeed=.06f;            // melee attack speed
constexpr float IronSkinGuard=.06f;              // damage taken
constexpr float BloodRushHeal=3.f;               // health per melee kill
constexpr float LeapMasterCooldown=.15f;         // the maul's leap and the claws' pounce recover this much faster
constexpr float ExecutionerBonus=.25f,ExecutionerBelow=.30f; // melee damage against an enemy under 30% health
constexpr float PredatorLockout=1.5f;            // the capstone: a melee kill refunds the leap or pounce (a dash charge for the blade and the scythe), at most this often
// RANGED (while a weapon that is not melee is in your hands)
constexpr float SteadyHandDamage=.05f;
constexpr float DeepMagazineBonus=.15f;          // magazine size, at least one more round
constexpr float QuickReloadSpeed=.12f;           // reload time
constexpr float EagleEyeCritical=.04f;           // critical chance (the 50% ceiling stands)
constexpr float FleetAimSpeed=.05f;              // running speed
constexpr float LongShotSpeed=.12f;              // projectile speed (and so range)
constexpr float MarksmanBonus=.15f;              // damage against elites and bosses
constexpr int PierceRounds=2;constexpr float PierceSeconds=5.f; // the capstone: a perfect dash makes your rounds pierce this many enemies for this long
// TEAM (the lone hero gets what applies; the partner's share needs the partner)
constexpr float SharedWardShield=12.f,SharedWardPartner=12.f;   // shield on entering a fight, and again while the partner is up
constexpr float WellspringFocus=15.f;            // maximum Focus
constexpr float RhythmCooldown=.12f;             // the team skills recover this much faster
constexpr float SteadfastRevive=.30f;            // you revive a partner this much faster
constexpr float BrandHunterNeed=.15f,BrandHunterBurst=.15f;      // the brand needs this much less stored, and bursts this much harder
constexpr float TetherWardenSeconds=2.f,TetherWardenBonus=.05f;  // the tether lasts longer and its damage bonus is higher
constexpr float ConvergenceWindow=.6f,ConvergenceDamage=.20f;    // Twin Collapse: a longer call, a harder detonation
constexpr float EchoScale=.5f,EchoDelay=.8f;                     // the capstone: every synergy burst repeats later at this share of its damage

// pure helpers so the game and the tests agree
inline float MeleeDamageBonus(Owned M) {return Owns(M,HonedEdge)?HonedEdgeDamage:0.f;}
inline float RangedDamageBonus(Owned M) {return Owns(M,SteadyHand)?SteadyHandDamage:0.f;}
inline int MagazineWith(Owned M,int Base) {return (Base>0&&Owns(M,DeepMagazine))?Base+std::max(1,int(std::lround(float(Base)*DeepMagazineBonus))):Base;}
inline float ReloadScale(Owned M) {return Owns(M,QuickReload)?1.f-QuickReloadSpeed:1.f;}
inline float CriticalWith(Owned M,float Chance) {return Owns(M,EagleEye)?std::min(.50f,Chance+EagleEyeCritical):Chance;}
inline float LeapScale(Owned M) {return Owns(M,LeapMaster)?1.f-LeapMasterCooldown:1.f;}
inline float TeamCooldownScale(Owned M) {return Owns(M,Rhythm)?1.f-RhythmCooldown:1.f;}
inline float BrandNeedScale(Owned M) {return Owns(M,BrandHunter)?1.f-BrandHunterNeed:1.f;}
inline float BrandBurstScale(Owned M) {return Owns(M,BrandHunter)?1.f+BrandHunterBurst:1.f;}
inline float TetherSecondsFor(Owned M,float Base) {return Base+(Owns(M,TetherWarden)?TetherWardenSeconds:0.f);}
inline float TetherBonusFor(Owned M,float Base) {return Base+(Owns(M,TetherWarden)?TetherWardenBonus:0.f);}
inline float TwinWindowFor(Owned M,float Base) {return Base+(Owns(M,Convergence)?ConvergenceWindow:0.f);}
inline float TwinDamageScale(Owned M) {return Owns(M,Convergence)?1.f+ConvergenceDamage:1.f;}
inline float ExecutionerScale(Owned M,float Health,float MaxHealth) {return (Owns(M,Executioner)&&MaxHealth>0.f&&Health<=MaxHealth*ExecutionerBelow)?1.f+ExecutionerBonus:1.f;}
inline float MarksmanScale(Owned M,bool EliteOrBoss) {return (Owns(M,Marksman)&&EliteOrBoss)?1.f+MarksmanBonus:1.f;}
inline bool PredatorReady(Owned M,float Now,float NextAt) {return Owns(M,Predator)&&Now>=NextAt;}
// what the rest of the nodes do, one rule each (the weapon an effect needs in hand is the caller's: melee for the MELEE lane's, anything else for the RANGED lane's)
inline float MeleeIntervalScale(Owned M) {return Owns(M,QuickHands)?1.f/(1.f+QuickHandsSpeed):1.f;}   // +6% attack speed: the time between swings shrinks by this factor
inline float MeleeReachScale(Owned M) {return Owns(M,LongReach)?1.f+LongReachBonus:1.f;}
inline float MeleeGuard(Owned M) {return Owns(M,IronSkin)?IronSkinGuard:0.f;}                       // damage reduction, added to the other reductions (which the game caps)
inline float KillHeal(Owned M) {return Owns(M,BloodRush)?BloodRushHeal:0.f;}                          // health per melee kill
inline float RunSpeedScale(Owned M) {return Owns(M,FleetAim)?1.f+FleetAimSpeed:1.f;}
inline float ProjectileSpeedScale(Owned M) {return Owns(M,LongShot)?1.f+LongShotSpeed:1.f;}
inline float ReviveScale(Owned M) {return Owns(M,SteadfastHands)?1.f+SteadfastRevive:1.f;}          // the speed at which this hero lifts a downed partner
inline float FocusBonus(Owned M) {return Owns(M,Wellspring)?WellspringFocus:0.f;}
inline float WardShield(Owned M,bool PartnerUp) {return Owns(M,SharedWard)?SharedWardShield+(PartnerUp?SharedWardPartner:0.f):0.f;}
inline int PierceFor(Owned M,bool Active) {return (Owns(M,PiercingRounds)&&Active)?PierceRounds:0;}  // extra enemies a round passes through while the perfect dash's window is open
inline float EchoDamage(float Original) {return (std::isfinite(Original)&&Original>0.f)?Original*EchoScale:0.f;}
// Whether a synergy's echo is owed: either hero of a burst holding the capstone makes one (the owner of the other half of a co-op synergy shares it, one echo however many own it).
inline bool EchoOwed(Owned Source,Owned Partner) {return Owns(Source,SynergyEcho)||Owns(Partner,SynergyEcho);}
}
