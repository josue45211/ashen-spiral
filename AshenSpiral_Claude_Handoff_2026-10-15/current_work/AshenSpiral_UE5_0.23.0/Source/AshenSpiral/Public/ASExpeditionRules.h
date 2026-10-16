#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
// Engine-independent expedition rules. Wave numbers are one-based; wave zero is a normal expedition.
namespace ASExpedition {
inline int Difficulty(int D) {return std::clamp(D,0,2);}
// TUNABLE unlock rules: beating Normal unlocks Hard, beating Hard unlocks Very Hard, and a Normal clear unlocks Endless.
// Mask bit 0/1/2 = Normal/Hard/Very Hard beaten. Older saves count as Normal-beaten when they have any win.
inline int ClearedMask(int Wins,int Mask) {return (Mask&7)|(Wins>0?1:0);}
inline bool DifficultyUnlocked(int D,int Mask) {D=Difficulty(D);return D==0||((Mask>>(D-1))&1)!=0;}
inline bool EndlessUnlocked(int Mask) {return (Mask&1)!=0;}
inline const char* UnlockCondition(int D) {return D==1?"Beat Normal to unlock":(D==2?"Beat Hard to unlock":"");}
inline float Quality(int D) {return 1.f+.25f*Difficulty(D);}
inline float EnemyHealthScale(int D,int Wave=0,int Tier=0) {
    const float Base[]={1.f,1.5f,2.1f};
    if(Wave<=0) return Base[Difficulty(D)];
    const float W=float(std::clamp(Wave,1,10000)-1),K=std::max(0.f,W-38.f);
    const float Rate=Tier==2?1.875f:(Tier==1?1.25f:1.f);
    return Base[Difficulty(D)]*(1.f+Rate*(W*.09f+K*K*.008f));
}
inline float EnemyDamageScale(int D,int Wave=0,int Tier=0) {
    const float Base[]={1.f,1.3f,1.6f};
    if(Wave<=0) return Base[Difficulty(D)];
    const float W=float(std::clamp(Wave,1,10000)-1),K=std::max(0.f,W-38.f);
    const float Rate=Tier==2?1.875f:(Tier==1?1.25f:1.f);
    return Base[Difficulty(D)]*(1.f+Rate*(W*.035f+K*K*.003f));
}
inline bool BossWave(int W) {return W>0&&W%10==0;}
inline bool RewardWave(int W) {return W>0&&(W<=20||BossWave(W)||(W<=50?W%2==0:(W-50)%3==0));}
inline int SkyStage(int W) {return W>=50?2:(W>=21?1:0);}
inline float AngerSpeed(int W) {return W<50?1.f:std::min(1.3f,1.12f+.003f*(W-50));}
inline int EnemyCount(int W,int Players) {return std::clamp(7+W/2+(Players>1?4:0),8,36);}
inline float TraitPower(int Picks,int Cap) {
    Picks=std::clamp(Picks,0,10000);Cap=std::max(1,Cap);float Power=float(std::min(Picks,Cap));
    for(int Extra=std::max(0,Picks-Cap),Block=0;Extra>0;++Block) {
        const int N=std::min(5,Extra);Power+=N*std::max(.1f,.8f-.1f*Block);Extra-=N;
    }
    return Power;
}
inline int ChoiceCount(int Rank) {return 3+std::clamp(Rank,0,2);}
inline int ChoiceCost(int Rank) {return Rank==0?150:(Rank==1?750:-1);}
inline bool CanBleed(float Remaining,bool Reviving) {return Remaining>0.f&&!Reviving;}
inline float Bleed(float Remaining,float Delta,bool Reviving) {return std::max(0.f,Remaining-(CanBleed(Remaining,Reviving)?std::max(0.f,Delta):0.f));}
inline int ShuffleWaves(std::uint32_t Roll) {
    // Symmetric discrete distribution: range 1..7, mean four, mode four.
    constexpr int Choices[]={1,2,3,3,4,4,4,5,5,6,7};return Choices[Roll%11];
}
inline float ShuffleSeconds(float Roll) {return 120.f+60.f*std::clamp(Roll,0.f,1.f);}
inline bool ShopAppears(int Wave,float Roll) {return Wave>=10&&Roll<.15f;}
// Wishing-well stakes are chosen by index; the server derives the amount so a client can never name its own stake.
constexpr int GambleMinimum=10;
inline int GambleStake(int Cinders,int Index) {
    if(Cinders<GambleMinimum) return 0;
    const int Percent[]={25,50,100};const int P=Percent[std::clamp(Index,0,2)];
    return std::clamp(std::max(GambleMinimum,int(std::int64_t(Cinders)*P/100)),GambleMinimum,Cinders);
}
// Returns the balance after one resolved spin. Multipliers include the stake return.
inline int GambleBalance(int Cinders,int Stake,int Multiplier) {
    Cinders=std::max(0,Cinders);Stake=std::clamp(Stake,0,Cinders); // a stake can never exceed what is held
    return int(std::min<std::int64_t>(std::int64_t(Cinders-Stake)+std::int64_t(Stake)*std::max(0,Multiplier),1000000000));
}
struct GambleOdds {int Loss,Refund,Double,Five,Ten;};
inline GambleOdds Odds(int Ash) {return Ash<=50?GambleOdds{45,25,24,5,1}:(Ash>=1000?GambleOdds{50,20,22,7,1}:GambleOdds{60,20,17,2,1});}
inline int GambleMultiplier(int Roll,int Ash) {
    const auto O=Odds(Ash);Roll=std::clamp(Roll,0,99);
    if(Roll<O.Loss) return 0;Roll-=O.Loss;if(Roll<O.Refund) return 1;Roll-=O.Refund;
    if(Roll<O.Double) return 2;Roll-=O.Double;return Roll<O.Five?5:10;
}
inline int Synergy(int A,int B) {
    // EASHitKind numeric ABI: Burn2, Poison3, Frost4, Shock8. Physical never marks.
    if(A>B) std::swap(A,B);
    if(A==2&&B==8) return 0;if(A==4&&B==8) return 1;if(A==2&&B==3) return 2;return -1;
}
inline float SynergyDamage(int Kind,float MaxHP,float Power,bool Boss) {
    const float Base=Kind==0?60.f:(Kind==1?45.f:50.f);
    return Base*std::max(1.f,Power)+(Boss?std::max(0.f,MaxHP)*.035f:0.f);
}
constexpr int ExclusiveBase=400,ExclusiveCount=4;
inline float BloodrootGain(float EffectiveRank) {return .35f*std::max(0.f,EffectiveRank);}
// TUNABLE Endless earnings: Scrap for kills (normal / elite / boss) and Ash for checkpoints and bosses.
inline int KillScrap(bool Elite,bool Boss) {return Boss?25:(Elite?5:1);}
inline int CheckpointAsh(int Wave) {return BossWave(Wave)?10:(Wave>0?2:0);}
inline bool Exclusive(int Id) {return Id>=ExclusiveBase&&Id<ExclusiveBase+ExclusiveCount;}
inline const char* ExclusiveName(int Id) {const char* Names[]={"Bloodroot","Close Quarters","First Blood","Unbound Fury"};return Names[std::clamp(Id-ExclusiveBase,0,3)];}
inline const char* ExclusiveDescription(int Id) {
    const char* Text[]={"Each kill adds 0.35 max HP per effective rank, healing that amount.","Kills within 350 cm restore 3 HP and grant 2 Scrap per rank.","Kills within 1 second of first engagement grant 3 Scrap per rank.","An endlessly stacking 8% primary attack damage bonus per effective rank."};
    return Text[std::clamp(Id-ExclusiveBase,0,3)];
}
}
