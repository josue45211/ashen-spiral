#pragma once
#include "ASRules.h"
#include "ASMeleeRules.h"
namespace ASArsenal {
constexpr int Scattergun=6,Rotary=7,Launcher=8;
inline int Pellets(int Weapon) {return Weapon==Scattergun?6:1;}
inline bool UsesMagazine(int Weapon) {return !ASMelee::IsMelee(Weapon);}
inline float Rate(int Weapon) {constexpr float Values[]={.28f,.72f,.30f,.48f,.34f,.40f,.72f,.105f,1.05f,.19f};return Values[std::clamp(Weapon,0,ASRules::WeaponCount-1)];}
// ---- On-hit reward scaling (0.24) ----
// Every on-hit reward (element procs, the weapon's own element, stun, root, shock, chain, splash, pull, knockback, leech, conduit) is scaled by the weapon's BASE swing/shot
// interval relative to a reference: slower weapons reward more per hit, faster weapons less, so rewards per second stay comparable between weapons - the way Crab Champions
// gives each weapon its own mod chances. Only the weapon's BASE rate is used, never attack speed or any live rate, so attack-speed upgrades still mean more hits and
// therefore proportionally more rewards. The limits keep the fastest and slowest weapons from being nullified or absurd. TUNABLE.
constexpr float OnHitReferenceInterval=.40f,OnHitScaleMin=.30f,OnHitScaleMax=2.6f; // the ten weapons' ideal scales run .26 (Rotary), .48 (Twin Claws) to 2.63 (Mortar); the limits barely clip the Rotary and the Mortar
// Seconds per swing/shot at base speed. Melee uses the combo's average step length (the hammer's slam and the blade's lunge are slower than their first swings).
inline float BaseInterval(int Weapon) {const int W=std::clamp(Weapon,0,ASRules::WeaponCount-1);return Rate(W)*(ASMelee::IsMelee(W)?ASMelee::AverageTimeScale(W):1.f);}
inline float OnHitScale(int Weapon) {return std::clamp(BaseInterval(Weapon)/OnHitReferenceInterval,OnHitScaleMin,OnHitScaleMax);}
// Splits "BaseChance x Scale" into a probability (never above 1) and a strength multiplier (never below 1), so the expected reward is exactly BaseChance x Scale:
// below 1 the reward simply happens less often at full strength; past certainty the excess becomes extra strength.
struct ScaledReward {float Chance=0.f,Magnitude=1.f;};
inline ScaledReward ScaleReward(float BaseChance,float Scale) {
    BaseChance=std::clamp(std::isfinite(BaseChance)?BaseChance:0.f,0.f,1.f);
    Scale=std::clamp(std::isfinite(Scale)?Scale:1.f,0.f,10.f);
    const float Want=BaseChance*Scale;
    if(Want<=0.f) return {0.f,1.f};
    return Want<=1.f?ScaledReward{Want,1.f}:ScaledReward{1.f,Want};
}
// Battle Conduit pays out every six hits at Scale 1; slower weapons need fewer hits, faster ones more (1..24).
inline int ConduitHitsNeeded(float Scale) {
    if(!std::isfinite(Scale)) return 6;
    return std::clamp(int(std::lround(6.f/std::max(.1f,Scale))),1,24);
}
inline int BaseCapacity(int Weapon) {constexpr int Values[]={0,0,10,7,14,0,6,40,3,0};return Values[std::clamp(Weapon,0,ASRules::WeaponCount-1)];}
inline int Capacity(int Weapon,int Rank) {return BaseCapacity(Weapon)+int(std::ceil(BaseCapacity(Weapon)*.20f-.00001f))*std::clamp(Rank,0,5);}
inline float ReloadSeconds(int Weapon,int Rank=0) {constexpr float Values[]={0.f,0.f,1.25f,1.30f,1.45f,0.f,1.65f,1.85f,2.10f,0.f};return Values[std::clamp(Weapon,0,ASRules::WeaponCount-1)]/(1.f+.15f*std::clamp(Rank,0,5));}
inline float SpreadScale(int Rank) {return std::max(.25f,1.f-.15f*std::clamp(Rank,0,5));}
inline bool CanReload(int Ammo,int Max,bool Active,bool Allowed) {return Allowed&&!Active&&Ammo>=0&&Ammo<Max;}
inline bool Consume(int& Ammo,bool Active) {if(Active||Ammo<=0) return false;--Ammo;return true;}
inline float ReloadProgress(float Start,float End,float Now) {return End>Start?std::clamp((Now-Start)/(End-Start),0.f,1.f):0.f;}
inline bool ReloadComplete(float End,float Now) {return End>0&&Now>=End;}
inline float EnemySpeedScale(bool EliteOrBoss) {return EliteOrBoss?1.35f:1.25f;}

// 0.26: Twin Claws is 30 a swing at .19 s (about 158 a second against ONE target; the blade's 143 hits everything in its arc, so the claws trade the arc for speed and the pounce).
inline float Damage(int Weapon,float Bonus=0.f) {
    constexpr float Values[]={40.f,62.f,39.f,33.f,31.f,41.f,96.f,14.f,132.f,30.f};
    const float Base=Values[std::clamp(Weapon,0,ASRules::WeaponCount-1)];
    // Flat damage upgrades add the same burst DPS to each new firing style; the claws swing nearly twice as often as the blade, so they get the same normalisation.
    return (Base+std::max(0.f,Bonus)*(ASRules::IsGun(Weapon)||Weapon==ASRules::Claws?Rate(Weapon)/.32f:1.f))/Pellets(Weapon);
}
inline float Speed(int Weapon) {return Weapon==Launcher?1600.f:4200.f;}
inline float Lifetime(int Weapon) {return Weapon==Scattergun?.43f:(Weapon==Rotary?.95f:3.5f);}
inline float ScatterFalloff(float Distance) {return std::clamp(1.f-(std::max(0.f,Distance)-600.f)/1800.f,.35f,1.f);}
inline float BlastFalloff(float Distance,float Radius) {return Radius>0?std::clamp(1.f-.65f*std::max(0.f,Distance)/Radius,.35f,1.f):0.f;}
inline float Heat(float Current,float Delta,bool Firing) {
    if(!std::isfinite(Current)) Current=0;
    if(!std::isfinite(Delta)||Delta<0) Delta=0;
    return std::clamp(Current+Delta*(Firing?.7f:-1.8f),0.f,1.f);
}
}
