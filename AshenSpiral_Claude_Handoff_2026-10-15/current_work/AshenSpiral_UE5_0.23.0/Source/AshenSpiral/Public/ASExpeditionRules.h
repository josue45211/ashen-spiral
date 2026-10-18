#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>
#include <cstdint>
#include "ASRules.h"
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
    return int(std::min<std::int64_t>(std::int64_t(Cinders-Stake)+std::int64_t(Stake)*std::max(0,Multiplier),ASRules::MaxCurrency));
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
// Lucky shop stock (TUNABLE). Scrap items always; rare Ash items only when the shop rolled rare.
struct ShopItem {const char* Name;const char* Description;int Scrap,Ash;bool Rare;};
constexpr int ShopCount=6,ShopPickIndex=5;
inline const ShopItem& ShopStock(int Index) {
    static const ShopItem Items[ShopCount]={
        {"Field Dressing","Heal 60% of your maximum health.",60,0,false},
        {"Ward Plate","Gain 40 shield.",55,0,false},
        {"Reroll Token","Gain one extra reward reroll.",90,0,false},
        {"Ember Core","Gain one random legendary-tier trait you can use.",0,30,true},
        {"Heart of the Spiral","Gain 25 maximum health for this run.",0,22,true},
        // Ported from Astra: a trait rolled for this player's build when the shop opens. Good stock, high price (TUNABLE).
        {"Trader's Pick","A trait the trader rolled for your build.",150,0,false}};
    return Items[std::clamp(Index,0,ShopCount-1)];
}
// Server-side purchase check: one purchase of each item per player per shop, exact funds, rare stock only when open.
inline bool CanBuyShopItem(int Index,int Scrap,int Ash,int BoughtMask,bool RareOpen) {
    if(Index<0||Index>=ShopCount) return false;const auto& I=ShopStock(Index);
    return (BoughtMask&(1<<Index))==0&&(!I.Rare||RareOpen)&&Scrap>=I.Scrap&&Ash>=I.Ash;
}
inline float BloodrootGain(float EffectiveRank) {return .35f*std::max(0.f,EffectiveRank);}
// TUNABLE Endless earnings: Scrap for kills (normal / elite / boss) and Ash for checkpoints and bosses.
inline int KillScrap(bool Elite,bool Boss) {return Boss?25:(Elite?5:1);}
inline int CheckpointAsh(int Wave) {return BossWave(Wave)?10:(Wave>0?2:0);}
// ---- 0.23.1 portal text and Endless sky stages (looks only; the numbers mirror the 0.23 rules above).
inline const char* PortalName(int D) {return D==1?"ASHEN":(D==2?"BLOOD":"EMBER");}
inline const char* PortalDifficultyName(int D) {return D==1?"HARD":(D==2?"VERY HARD":"NORMAL");}
inline const char* PortalModifiers(int D) {
    static const char* Text[3]={"Enemy health x1.0, damage x1.0, reward quality +0%","Enemy health x1.5, damage x1.3, reward quality +25%","Enemy health x2.1, damage x1.6, reward quality +50%"};
    return Text[Difficulty(D)];
}
struct SkyLook {float SunIntensity,SunR,SunG,SunB,FogDensity,FogR,FogG,FogB,SkyLight,Vignette,Saturation,GainR,GainG,GainB;};
// Stage 0 equals the existing lighting rig: clear sky (waves 1-20). Stage 1 gloomier (21-49). Stage 2 red, bloody gloom (50+).
inline SkyLook SkyLookFor(int Stage) {
    static const SkyLook Looks[3]={
        {4.5f,1.f,.83f,.65f,.018f,.10f,.12f,.18f,.65f,.20f,1.f,1.f,1.f,1.f},
        {1.7f,.72f,.76f,.85f,.032f,.20f,.21f,.24f,.42f,.32f,.78f,.95f,.97f,1.f},
        {2.4f,1.f,.16f,.10f,.045f,.50f,.035f,.03f,.50f,.50f,.90f,1.25f,.62f,.58f}};
    return Looks[std::clamp(Stage,0,2)];
}
inline SkyLook LerpSky(const SkyLook& A,const SkyLook& B,float T) {
    T=std::clamp(T,0.f,1.f);const float* X=&A.SunIntensity;const float* Y=&B.SunIntensity;SkyLook R{};float* O=&R.SunIntensity;
    for(int I=0;I<14;++I) O[I]=X[I]+(Y[I]-X[I])*T;
    return R;
}
// The step into the red stage is fast so wave 50 is unmistakable; the other changes drift in slowly.
inline float SkyBlendRate(int From,int To) {return To==2&&From<2?1.4f:.35f;}
inline int EndlessSkyStage(bool Endless,bool InRun,int Wave) {return Endless&&InRun?SkyStage(Wave):0;}
inline bool Exclusive(int Id) {return Id>=ExclusiveBase&&Id<ExclusiveBase+ExclusiveCount;}
inline const char* ExclusiveName(int Id) {const char* Names[]={"Bloodroot","Close Quarters","First Blood","Unbound Fury"};return Names[std::clamp(Id-ExclusiveBase,0,3)];}
inline const char* ExclusiveDescription(int Id) {
    const char* Text[]={"Each kill adds 0.35 max HP per effective rank, healing that amount.","Kills within 350 cm restore 3 HP and grant 2 Scrap per rank.","Kills within 1 second of first engagement grant 3 Scrap per rank.","An endlessly stacking 8% primary attack damage bonus per effective rank."};
    return Text[std::clamp(Id-ExclusiveBase,0,3)];
}
// ---- 0.23.2 run reward log: counts of every reward bought or granted this run, shown in the tab menu.
// Shop purchases use their ShopStock index (0..5); chest and checkpoint rewards use the codes below.
constexpr int RunGainChestHeal=8,RunGainChestKit=9,RunGainRelic=10,RunGainStyle=11,RunGainCinders=12,RunGainAsh=13,RunGainCount=16,HeartMaxHealth=25;
inline bool RunGainValid(int Code) {return Code>=0&&Code<RunGainCount&&(Code<ShopCount||(Code>=RunGainChestHeal&&Code<=RunGainAsh));}
inline const char* RunGainName(int Code) {
    switch(Code) {
        case 0: return "Field Dressing";case 1: return "Ward Plate";case 2: return "Reroll Token";case 3: return "Ember Core";case 4: return "Heart of the Spiral";case 5: return "Trader's Pick";
        case RunGainChestHeal: return "Chest: restorative";case RunGainChestKit: return "Chest: field kit";case RunGainRelic: return "Chest: relic";
        case RunGainStyle: return "Chest: spiral style";case RunGainCinders: return "Chest: Cinders";case RunGainAsh: return "Checkpoint Ash";default: return "";
    }
}
// Uniform acquisition format for the tab menu: title "NAME  xN" and a one-line effect, for every run reward.
inline std::string AcquisitionTitle(const std::string& Name,int Count) {return Name+"  x"+std::to_string(Count<1?1:Count);}
inline std::string RunGainLine(int Code,int Count) {
    if(!RunGainValid(Code)||Count<=0) return std::string();
    return AcquisitionTitle(RunGainName(Code),Count);
}
inline std::string RunGainDetail(int Code,int Count) {
    if(!RunGainValid(Code)||Count<=0) return std::string();
    switch(Code) {
        case 0: return "Healed 60% of maximum health"+std::string(Count>1?" each time.":".");
        case 1: return "Gained 40 shield"+std::string(Count>1?" each time.":".");
        case 2: return "Extra reward rerolls gained.";
        case 3: return "A random legendary-tier trait.";
        case 4: return "+"+std::to_string(HeartMaxHealth*Count)+" maximum health this run ("+std::to_string(HeartMaxHealth)+" each).";
        case 5: return "A trait the trader rolled for your build.";
        case RunGainChestHeal: return "Restored 35% of maximum health.";
        case RunGainChestKit: return "Restored 15 health and gained 10 shield.";
        case RunGainRelic: return "A relic from the chest.";
        case RunGainStyle: return "A new Spiral style from the chest.";
        case RunGainCinders: return "Cinders from the chest.";
        case RunGainAsh: return "Ash earned at checkpoints (banked when the run ends).";
        default: return std::string();
    }
}
// ---- Arena shift sequence v2 (0.23.4). Timeline from the banner: 0-1s banner only, 1-2s the old cover sinks, the swap happens at 2s,
// 2-3s the new cover rises. Looks and sound only; the shuffle itself is unchanged.
constexpr float ShiftWarningSeconds=2.f,ShiftSinkSeconds=1.f,ShiftRiseSeconds=1.f,ShiftBannerSeconds=3.f,ShiftCoverDrop=450.f,ShiftSolidBelow=.55f;
// Ease-in: slow start, picking up speed, then an abrupt stop ("slam"). TUNABLE exponent 3.
inline float ShiftEaseIn(float X) {X=std::clamp(X,0.f,1.f);return X*X*X;}
// 0 = cover fully up, 1 = fully sunk. ShiftAt <= 0 means no shift is scheduled.
inline float ShiftCoverSunk(float Now,float ShiftAt) {
    if(!std::isfinite(Now)||!std::isfinite(ShiftAt)||ShiftAt<=0.f) return 0.f;
    const float T=Now-ShiftAt;
    return T<0.f?ShiftEaseIn(1.f+T/ShiftSinkSeconds):1.f-ShiftEaseIn(T/ShiftRiseSeconds);
}
// Collision never moves; it switches on and off at discrete moments of the sequence, so machines can only disagree by one network delay.
inline bool ShiftCoverSolid(float Now,float ShiftAt) {return ShiftCoverSunk(Now,ShiftAt)<ShiftSolidBelow;}
// ---- Lightning strikes (0.23.4): Endless wave 50+. Everything here is TUNABLE.
// A strike shows a floor indicator for LightningTelegraph seconds, then hits everyone inside LightningRadius.
constexpr int LightningFirstWave=50;
constexpr float LightningTelegraph=.5f,LightningRadius=220.f,LightningJitter=150.f,LightningClusterRadius=450.f,LightningMinSeparation=420.f;
constexpr float LightningPlayerFraction=.20f,LightningPlayerMinimum=40.f,LightningEnemyFraction=.30f,LightningBossFraction=.03f;
inline bool LightningActive(bool Endless,int Wave) {return Endless&&Wave>=LightningFirstWave;}
// Seconds until the next strike: about 6-9 at wave 50, shrinking with the wave, never below 2.5. Roll is a 0..1 random number.
inline float LightningInterval(int Wave,float Roll) {
    const float Base=std::max(2.5f,8.f-.05f*float(std::max(0,Wave-LightningFirstWave)));
    return Base*(.75f+.5f*std::clamp(std::isfinite(Roll)?Roll:.5f,0.f,1.f));
}
inline int LightningMaxAtOnce(int Wave) {return Wave>=80?3:(Wave>=65?2:1);}
inline float LightningPlayerDamage(float MaxHealth) {return std::max(LightningPlayerMinimum,std::isfinite(MaxHealth)?MaxHealth*LightningPlayerFraction:0.f);}
inline float LightningEnemyDamage(float MaxHealth,bool Boss) {return std::isfinite(MaxHealth)?MaxHealth*(Boss?LightningBossFraction:LightningEnemyFraction):0.f;}
// Crowds attract strikes: weight grows with the square of how many others stand nearby.
inline float LightningClusterWeight(int Neighbours) {const float N=1.f+float(std::max(0,Neighbours));return N*N;}
// Weighted pick; Roll in 0..1. Returns -1 for no candidates.
inline int LightningPickIndex(const std::vector<float>& Weights,float Roll) {
    if(Weights.empty()) return -1;
    float Total=0.f;for(float W:Weights) Total+=std::max(0.f,std::isfinite(W)?W:0.f);
    if(Total<=0.f) return 0;
    float Target=std::clamp(std::isfinite(Roll)?Roll:0.f,0.f,.99999f)*Total;
    for(std::size_t I=0;I<Weights.size();++I) {Target-=std::max(0.f,std::isfinite(Weights[I])?Weights[I]:0.f);if(Target<0.f) return int(I);}
    return int(Weights.size())-1;
}
// ---- Flying monster (0.23.4): the "Gloomwing" (enemy Kind 10). It hovers low enough for melee to reach it, circles the nearest player,
// telegraphs, then dives in a straight line through where the player stood. v1 is deliberately simple. Everything here is TUNABLE.
constexpr int FlierKind=10;
constexpr float FlierHealthScale=.55f,FlierHover=200.f,FlierOrbitRadius=520.f,FlierDiveSpeed=1500.f,FlierDiveSeconds=.55f,FlierWindup=.7f,FlierContactDamage=18.f;
inline float FlierAttackDelay(float Roll) {return 4.f+2.f*std::clamp(std::isfinite(Roll)?Roll:.5f,0.f,1.f);}
// Level is the room (story) or the wave (Endless). Roll is a 0..1 random number.
inline bool SpawnsFlier(bool Endless,int Level,float Roll) {
    if(!std::isfinite(Roll)) return false;
    const float Chance=Endless?(Level>=8?std::min(.28f,.08f+.004f*float(Level-8)):0.f):(Level>=4?std::min(.25f,.15f+.01f*float(Level-4)):0.f);
    return Roll<Chance;
}
}
