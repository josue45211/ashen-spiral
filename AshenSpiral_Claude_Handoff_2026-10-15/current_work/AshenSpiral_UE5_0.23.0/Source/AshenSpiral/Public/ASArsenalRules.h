#pragma once
#include "ASRules.h"
#include "ASMeleeRules.h"
namespace ASArsenal {
constexpr int Scattergun=6,Rotary=7,Launcher=8;
inline int Pellets(int Weapon) {return Weapon==Scattergun?6:1;}
inline bool UsesMagazine(int Weapon) {return !ASMelee::IsMelee(Weapon);}
inline float Rate(int Weapon) {constexpr float Values[]={.28f,.72f,.30f,.48f,.34f,.40f,.72f,.105f,1.05f};return Values[std::clamp(Weapon,0,8)];}
inline int BaseCapacity(int Weapon) {constexpr int Values[]={0,0,10,7,14,0,6,40,3};return Values[std::clamp(Weapon,0,8)];}
inline int Capacity(int Weapon,int Rank) {return BaseCapacity(Weapon)+int(std::ceil(BaseCapacity(Weapon)*.20f-.00001f))*std::clamp(Rank,0,5);}
inline float ReloadSeconds(int Weapon,int Rank=0) {constexpr float Values[]={0.f,0.f,1.25f,1.30f,1.45f,0.f,1.65f,1.85f,2.10f};return Values[std::clamp(Weapon,0,8)]/(1.f+.15f*std::clamp(Rank,0,5));}
inline float SpreadScale(int Rank) {return std::max(.25f,1.f-.15f*std::clamp(Rank,0,5));}
inline bool CanReload(int Ammo,int Max,bool Active,bool Allowed) {return Allowed&&!Active&&Ammo>=0&&Ammo<Max;}
inline bool Consume(int& Ammo,bool Active) {if(Active||Ammo<=0) return false;--Ammo;return true;}
inline float ReloadProgress(float Start,float End,float Now) {return End>Start?std::clamp((Now-Start)/(End-Start),0.f,1.f):0.f;}
inline bool ReloadComplete(float End,float Now) {return End>0&&Now>=End;}
inline float EnemySpeedScale(bool EliteOrBoss) {return EliteOrBoss?1.35f:1.25f;}

inline float Damage(int Weapon,float Bonus=0.f) {
    constexpr float Values[]={40.f,62.f,39.f,33.f,31.f,41.f,96.f,14.f,132.f};
    const float Base=Values[std::clamp(Weapon,0,8)];
    // Flat damage upgrades add the same burst DPS to each new firing style.
    return (Base+std::max(0.f,Bonus)*(Weapon>=6?Rate(Weapon)/.32f:1.f))/Pellets(Weapon);
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
