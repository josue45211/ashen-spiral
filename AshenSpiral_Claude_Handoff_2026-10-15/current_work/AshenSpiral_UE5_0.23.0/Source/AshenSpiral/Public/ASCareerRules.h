#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
namespace ASCareer {
enum Metric {Kills,Elites,Bosses,Rooms,Flawless,Perfects,Revives,Vaults,Casks,Wins,Elemental,Swift,Count};
constexpr int MaxValue=1000000000;
inline int Add(int A,int B) {return int(std::clamp<std::int64_t>(std::int64_t(std::max(A,0))+std::max(B,0),0,MaxValue));}
inline std::int64_t Threshold(int Level) {const std::int64_t N=std::clamp(Level,1,10000)-1;return N*100+25*N*(N-1)/2;}
inline int Cost(int Level) {return 100+25*(std::clamp(Level,1,10000)-1);}
inline int Level(int XP) {int Lo=1,Hi=10000;while(Lo<Hi) {const int M=(Lo+Hi+1)/2;if(Threshold(M)<=std::clamp(XP,0,MaxValue)) Lo=M;else Hi=M-1;}return Lo;}
inline int Progress(int XP) {return int(std::max<std::int64_t>(0,std::clamp(XP,0,MaxValue)-Threshold(Level(XP))));}
// Cash out a small part of the shared balance after all shopping is finished.
// Ten Cinders buy one Ash, at most three Ash per player per cleared chamber.
inline int Cashout(int Cinders) {return std::min(3,std::clamp(Cinders,0,MaxValue)/100);}
inline int CashoutCost(int Cinders) {return Cashout(Cinders)*10;}
struct Achievement {const char* Name;const char* Description;Metric Stat;std::array<int,3> Goals;};
constexpr std::array<Achievement,Count> Achievements{{
 {"Ash Hunter","Defeat enemies with your attacks or ongoing effects.",Kills,{100,1000,5000}},
 {"Giant Slayer","Defeat elite enemies (personal final hit).",Elites,{10,50,250}},
 {"Sovereign Breaker","Defeat ward bosses (personal final hit).",Bosses,{1,10,50}},
 {"Wayfarer","Clear expedition chambers.",Rooms,{10,100,500}},
 {"Untouched","Clear a chamber without health or shield damage.",Flawless,{1,10,50}},
 {"Perfect Rhythm","Release Perfect Spirals during combat.",Perfects,{25,100,500}},
 {"Lifeline","Complete a co-op revival.",Revives,{1,10,50}},
 {"Vaultkeeper","Recover Rune Vault caches with your party.",Vaults,{1,10,50}},
 {"Demolitionist","Detonate explosive casks during combat.",Casks,{10,50,200}},
 {"Spiral Conqueror","Finish complete expeditions.",Wins,{1,5,20}},
 {"Alchemist","Defeat enemies affected by at least two statuses.",Elemental,{25,100,500}},
 {"Quick Work","Clear a chamber within 90 seconds, including countdowns.",Swift,{1,10,50}}
}};
inline int Tier(int Value,int I) {if(I<0||I>=Count) return 0;int T=0;for(int Goal:Achievements[I].Goals) if(Value>=Goal) ++T;return T;}
inline int LevelEmbers(int LevelValue) {return std::clamp(LevelValue,1,10000)/5;}
inline int LevelTablets(int LevelValue) {return std::clamp(LevelValue,1,10000)/10;}
struct Reward {int Embers=0,Tablets=0;};
inline Reward ClaimLevelReward(int Current,int& Paid) {
    Current=std::clamp(Current,1,10000);Paid=std::clamp(Paid,1,10000);
    const Reward R{std::max(0,LevelEmbers(Current)-LevelEmbers(Paid)),std::max(0,LevelTablets(Current)-LevelTablets(Paid))};Paid=std::max(Paid,Current);return R;
}
inline Reward ClaimTierReward(int Value,int I,int& Paid) {
    const int T=Tier(Value,I);Paid=std::clamp(Paid,0,3);const Reward R{std::max(0,T-Paid),T==3&&Paid<3?1:0};Paid=std::max(Paid,T);return R;
}
inline bool MultiStatus(unsigned Bits) {Bits&=23u;return Bits&&(Bits&(Bits-1));}
}
