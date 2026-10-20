#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

// 0.25 phase E: the heroes' footsteps. Pure rules, no Unreal types, tested as plain C++ (Tests/foley_025_tests.cpp). A step sounds each time the gait passes a half stride (the feet already swap there:
// the gait phase is a sine over WalkPhase), only while a hero is really running on the ground, at a low volume that falls with distance for the other hero, and with a small random pitch so a long run
// does not machine-gun. The sounds are 12 concrete running steps from the Game Animation Sample, imported locally as /Game/Audio/Imported/Foley/S_Step01..12 (Tools/import_vault_assets.py);
// a machine without them is silent, as before. Everything here is TUNABLE.
namespace ASFoley {

constexpr int StepVariants=12;
constexpr float StepVolume=.16f;                 // the hero's own steps; other heroes' are quieter with distance
constexpr float StepPitchLow=.82f,StepPitchHigh=.96f; // armoured boots: a little lower than the sneakers they were recorded in
constexpr float MinStepSpeed=140.f;              // cm/s: slower than this is shuffling, not running
constexpr float TwoPi=6.28318530717958647692f;

// How many half strides (multiples of pi) the gait phase passed going from Prev to Now: 0 normally, 1 on a step, never more than 2 (a long frame must not play a burst).
inline int StepsCrossed(float Prev,float Now) {
    if(!std::isfinite(Prev)||!std::isfinite(Now)||Now<=Prev) return 0;
    const float Pi=TwoPi*.5f;
    return std::clamp(int(std::floor(Now/Pi))-int(std::floor(Prev/Pi)),0,2);
}
// Whether a step is heard at all: on the ground, not dashing, not downed, at running speed.
inline bool Audible(bool Airborne,bool Dashing,bool Downed,float Speed) {return !Airborne&&!Dashing&&!Downed&&std::isfinite(Speed)&&Speed>=MinStepSpeed;}
// The next variant: never the one just played, always in range. Last is -1 for the first step; Roll is any random number.
inline int NextVariant(int Last,std::uint32_t Roll) {
    if(StepVariants<2) return 0;
    const int Pick=int(Roll%std::uint32_t(StepVariants-1));
    return Last<0||Last>=StepVariants?int(Roll%std::uint32_t(StepVariants)):(Pick>=Last?Pick+1:Pick);
}
// Volume of a step for a hero this far from the local player's hero (the local hero is 0 away): full inside 6 m, silent beyond 32 m, as the enemy cues are.
inline float StepGain(float Distance,float Speed) {
    if(!std::isfinite(Distance)||!std::isfinite(Speed)) return 0.f;
    const float T=std::clamp((std::max(0.f,Distance)-600.f)/2600.f,0.f,1.f);
    const float Run=std::clamp((Speed-MinStepSpeed)/(640.f-MinStepSpeed),0.f,1.2f); // a little harder at full run than at a jog
    return (1.f-T)*(1.f-T)*(.6f+.4f*std::min(1.f,Run));
}

} // namespace ASFoley
