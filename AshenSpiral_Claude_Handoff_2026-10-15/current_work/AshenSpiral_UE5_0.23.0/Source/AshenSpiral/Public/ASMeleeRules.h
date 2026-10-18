#pragma once
#include <algorithm>
#include <cmath>
#include "ASHammerRules.h"
// Shared timing and grip foundation. One complete pose cycle is one attack.
namespace ASMelee {
// A pending swing stays valid while the game phase and the arena epoch are unchanged. An Endless arena shuffle does not change the epoch.
inline bool ContextValid(int PhaseThen,int PhaseNow,int EpochThen,int EpochNow) {return PhaseThen==PhaseNow&&EpochThen==EpochNow;}
inline bool IsMelee(int W) {return W==0||W==1||W==5;}
inline int ComboLength(int W) {return W==0||W==1?3:1;}
inline float BaseSeconds(int W) {return W==0?.28f:(W==1?.72f:.40f);}
inline float TimeScale(int W,int Step) {return Step==2?(W==1?1.65f:(W==0?1.25f:1.f)):1.f;}
inline float AverageTimeScale(int W) {return ComboLength(W)==3?(2.f+TimeScale(W,2))/3.f:1.f;}
inline float DamageScale(int W,int Step) {return Step==2?(W==1?2.6f:(W==0?1.3f:1.f)):1.f;}
inline float ImpactFraction(int W,int Step) {return W==1?(Step==2?.76f:.56f):(W==0&&Step==2?.60f:.48f);}
inline float MotionFraction(int W) {return W==1?.24f:.18f;}
inline float Phase(float Now,float Start,float Duration) {return Duration>0?std::clamp((Now-Start)/Duration,0.f,1.f):1.f;}
inline bool ImpactDue(float Now,float At,bool Pending) {return Pending&&Now>=At;}
inline float Smooth(float T) {T=std::clamp(T,0.f,1.f);return T*T*(3.f-2.f*T);}
using Pose=ASHammer::Pose;
inline Pose Blend(const Pose& A,const Pose& B,float T) {
    T=Smooth(T);return {A.X+(B.X-A.X)*T,A.Y+(B.Y-A.Y)*T,A.Z+(B.Z-A.Z)*T,
        A.Pitch+(B.Pitch-A.Pitch)*T,A.Yaw+(B.Yaw-A.Yaw)*T,A.Roll+(B.Roll-A.Roll)*T,
        A.BodyYaw+(B.BodyYaw-A.BodyYaw)*T,A.BodyPitch+(B.BodyPitch-A.BodyPitch)*T};
}
inline Pose Hold(int W) {
    if(W==1) return ASHammer::Hold();
    if(W==5) return {40,8,104,30,-40,0,0,1};
    return {32,38,96,24,-14,0,0,0};
}
inline float SupportHeight(int W) {return W==1?24.f:30.f;}
inline Pose Swing(int W,int Step,float PhaseValue,bool Active=true) {
    const Pose Rest=Hold(W);if(!Active) return Rest;
    if(W==1) return ASHammer::Swing(Step,PhaseValue);
    const float P=std::clamp(PhaseValue,0.f,1.f),Impact=ImpactFraction(W,Step);
    Pose Wind,Strike,Follow;
    if(W==5) {
        Wind={52,4,125,30,65,0,-14,-3};Strike={54,4,105,45,-50,0,10,5};Follow={40,5,109,40,-65,0,16,3};
    } else if(Step==2) {
        Wind={36,42,116,65,30,0,-15,-7};Strike={72,20,114,90,0,0,12,16};Follow={64,22,112,82,-4,0,11,12};
    } else {
        const float Side=Step==1?-1.f:1.f;
        Wind={40,34+Side*15,125,55,Side*65,0,-Side*16,-2};
        Strike={52,24,113,82,-Side*10,0,Side*9,5};Follow={40,26-Side*28,104,85,-Side*65,0,Side*19,3};
    }
    // Blade: a quick coil and almost no follow-through. Same cycle length and impact fraction.
    const float WindEnd=W==1&&Step==2?.42f:(W==0?.15f:.22f),FollowEnd=Impact+.12f;
    if(W==0&&Step!=2) {const float K=.5f;Follow={Strike.X+(Follow.X-Strike.X)*K,Strike.Y+(Follow.Y-Strike.Y)*K,Strike.Z+(Follow.Z-Strike.Z)*K,Strike.Pitch+(Follow.Pitch-Strike.Pitch)*K,Strike.Yaw+(Follow.Yaw-Strike.Yaw)*K,Follow.Roll,Strike.BodyYaw+(Follow.BodyYaw-Strike.BodyYaw)*K,Strike.BodyPitch+(Follow.BodyPitch-Strike.BodyPitch)*K};}
    if(P<WindEnd) return ASMelee::Blend(Rest,Wind,P/WindEnd);
    if(P<Impact) return ASMelee::Blend(Wind,Strike,(P-WindEnd)/(Impact-WindEnd));
    if(P<FollowEnd) return ASMelee::Blend(Strike,Follow,(P-Impact)/.12f);
    return ASMelee::Blend(Follow,Rest,(P-FollowEnd)/(1.f-FollowEnd));
}
// Smoothly converge to normal attack-range movement. Maximum ordinary boost +35%.
inline float ApproachScale(float Distance,float AttackRange,bool Boss) {
    const float Near=std::max(700.f,AttackRange+200.f),Far=Near+1700.f;
    return 1.f+(Boss?.15f:.35f)*Smooth((Distance-Near)/(Far-Near));
}
}
