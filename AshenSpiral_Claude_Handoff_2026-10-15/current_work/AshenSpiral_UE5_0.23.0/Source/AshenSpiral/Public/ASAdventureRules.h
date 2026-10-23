#pragma once
#include <algorithm>
#include <cmath>
namespace ASAdventure {
constexpr int AbilityCount=9; // 0.26: the three team skills (Ashen Brand, Ember Tether, Twin Collapse: ASTeamRules.h) are 6-8
inline float CameraDistance(float Current,float Wheel) {if(!std::isfinite(Current)) Current=528.f;if(!std::isfinite(Wheel)) Wheel=0;return std::clamp(Current-Wheel*45.f,360.f,780.f);}
inline int Biome(int Room) {return Room>=16&&Room<24?4:(Room>=8&&Room<12?2:std::clamp(Room/8,0,3));}
inline bool InReach(float DistanceSquared,float Height) {return DistanceSquared<=320.f*320.f&&std::abs(Height)<=300.f;}
inline float TrapSpeed(int Ward) {return 1.f+.15f*std::clamp(Ward,0,3);}
inline float TrapDamage(int Ward) {return 16.f+4.f*std::clamp(Ward,0,3);}
inline float ShrineCost(float MaxHealth) {return std::max(1.f,std::ceil(MaxHealth*.12f));}
inline bool CanSacrifice(float Health,float MaxHealth) {return Health> ShrineCost(MaxHealth)+1.f;}
inline float NovaAreaRadius(float Base,bool Nova) {return std::max(0.f,Base)+(Nova?300.f:0.f);}
inline float AbilityCost(int I) {constexpr float Cost[]={25,30,32,35,28,30,30,35,40};return Cost[std::clamp(I,0,AbilityCount-1)];}
inline float AbilityCooldown(int I,float Recovery) {constexpr float CD[]={1.2f,6.f,5.f,7.f,9.f,6.f,10.f,16.f,16.f};return CD[std::clamp(I,0,AbilityCount-1)]*std::clamp(Recovery,.5f,1.f);}
inline float Shuttle(float Time,float Speed) {return .5f-.5f*std::cos(Time*Speed*1.2f);}
}
