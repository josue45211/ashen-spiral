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
// 0.24.2 (Josue after the 0.24.1 playtest: "reduce footstep volume or make the footsteps more natural, like the default Unreal Engine robotic character"): the Game Animation
// Sample's own way - a WALK set at walking pace, a RUN set when running, STRAFE and BACKPEDAL sets when moving sideways or backwards against the facing (the hero faces its aim),
// landings and jumps - at the pitch they were recorded at (no more pitching sneakers down to sound like boots) and a good deal quieter.
enum StepSet : int {SetRun=0,SetWalk,SetStrafe,SetBack,SetCount};
constexpr int SetVariants[SetCount]={12,12,8,8};
constexpr const char* SetPrefix[SetCount]={"S_Step","S_StepWalk","S_StepStrafe","S_StepBack"}; // /Game/Audio/Imported/Foley/<prefix>NN (Tools/import_vault_assets.py)
constexpr float StepVolume=.10f;                 // the hero's own running steps (was .16); other heroes' are quieter with distance
constexpr float WalkVolume=.065f,LandVolume=.16f,JumpVolume=.07f;
constexpr float StepPitchLow=.95f,StepPitchHigh=1.04f; // as recorded, with a little variety
constexpr float MinStepSpeed=140.f;              // cm/s: slower than this is shuffling, not running
constexpr float WalkBelow=360.f;                 // cm/s: below this a step is a walking one
constexpr float LandMinFall=380.f;               // cm/s downward: a softer landing than this makes no sound
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
// Which set a step comes from: by speed, then by how the motion points against the facing (ForwardDot: the unit motion's dot with the facing, -1 straight backwards).
inline StepSet SetFor(float Speed,float ForwardDot) {
    if(!std::isfinite(Speed)||!std::isfinite(ForwardDot)) return SetRun;
    if(ForwardDot<-.45f) return Speed<WalkBelow?SetWalk:SetBack; // GASP's backpedal steps are the running ones; a slow backwards step is a walk step
    if(ForwardDot<.45f) return SetStrafe;
    return Speed<WalkBelow?SetWalk:SetRun;
}
inline float SetVolume(StepSet Set) {return Set==SetWalk?WalkVolume:StepVolume;}
// A landing: silent below LandMinFall, loudest from a long fall; Jump: a small push-off.
inline float LandGain(float FallSpeed) {return std::isfinite(FallSpeed)?std::clamp((FallSpeed-LandMinFall)/900.f+.35f,0.f,1.f)*(FallSpeed>=LandMinFall?1.f:0.f):0.f;}
// The next variant of a set with Count clips: never the one just played.
inline int NextVariantOf(int Count,int Last,std::uint32_t Roll) {
    if(Count<2) return 0;
    const int Pick=int(Roll%std::uint32_t(Count-1));
    return Last<0||Last>=Count?int(Roll%std::uint32_t(Count)):(Pick>=Last?Pick+1:Pick);
}
// Volume of a step for a hero this far from the local player's hero (the local hero is 0 away): full inside 6 m, silent beyond 32 m, as the enemy cues are.
inline float StepGain(float Distance,float Speed) {
    if(!std::isfinite(Distance)||!std::isfinite(Speed)) return 0.f;
    const float T=std::clamp((std::max(0.f,Distance)-600.f)/2600.f,0.f,1.f);
    const float Run=std::clamp((Speed-MinStepSpeed)/(640.f-MinStepSpeed),0.f,1.2f); // a little harder at full run than at a jog
    return (1.f-T)*(1.f-T)*(.6f+.4f*std::min(1.f,Run));
}

} // namespace ASFoley
