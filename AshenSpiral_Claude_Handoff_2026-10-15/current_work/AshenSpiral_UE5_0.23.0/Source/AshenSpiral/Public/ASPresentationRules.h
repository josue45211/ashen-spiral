#pragma once
#include <algorithm>
#include <cmath>

namespace ASPresentation {
inline float HUDScale(float Value) {return std::isfinite(Value)?float(std::clamp(int(std::lround(std::clamp(Value,.25f,1.f)*4.f)),1,4))*.25f:1.f;}
constexpr float NoticeLife=9.f,NoticeHold=4.5f;
inline float NoticeAlpha(float Age) {return Age<0?0.f:std::clamp((NoticeLife-Age)/(NoticeLife-NoticeHold),0.f,1.f);}
inline float NumberScale(float Value) {
    if(!std::isfinite(Value)) return 1.2f;
    return .8f+.2f*std::clamp(int(std::lround((std::clamp(Value,.8f,1.6f)-.8f)/.2f)),0,4);
}
inline float CrosshairScale(float Value) {return std::isfinite(Value)?std::clamp(Value,.5f,2.f):1.f;}
// Includes the larger critical-hit flash, so text never covers the aim point.
inline float CrosshairClearance(float Scale) {return 24.f*CrosshairScale(Scale)+16.f;}
inline float DamageTop(float CenterY,float Scale,float TextHeight,float Age,int Lane,bool Incoming) {
    return CenterY-CrosshairClearance(Scale)-std::max(0.f,TextHeight)-std::max(0.f,Age)*55.f
        -float(std::clamp(Lane,0,2))*24.f-(Incoming?76.f:0.f);
}

// ---- Damage-number personalities (looks only). Every special damage source moves, pops and fades in its own way,
// so a crit, a synergy or a status tick reads from the number's behaviour, never from a label.
enum class NumberPersona {Plain,Crit,Burn,Poison,Frost,Bleed,Shock,Spiral,SynergyLightning,SynergyStorm,SynergyWild};
struct DamageLook {float Scale,Rise,X,Y,Alpha,Flash,Heat;int Rings;float RingT;};
inline float NumberNoise(float Seed,float Step) {
    const float V=std::sin(Seed*12.9898f+Step*78.233f)*43758.5453f;return V-std::floor(V)-.5f;
}
inline bool IsSynergyPersona(NumberPersona P) {return P>=NumberPersona::SynergyLightning;}
// Age in seconds since the number spawned; Life is its total lifetime; Seed is a stable per-number random.
inline DamageLook DamageLookFor(NumberPersona P,float Age,float Life,float Seed) {
    Age=std::max(0.f,Age);Life=std::max(.1f,Life);
    const float T=std::clamp(Age/Life,0.f,1.f);const float Fade=std::clamp((1.f-T)/.3f,0.f,1.f);
    DamageLook L{1.f,Age*55.f,0.f,0.f,Fade,0.f,0.f,0,0.f};
    switch(P) {
    case NumberPersona::Crit: { // Hard punch: overshoots, freezes for a beat, then shoves up with a decaying shake.
        const float Punch=std::exp(-Age/.07f);L.Scale=1.5f+1.0f*Punch;L.Flash=Punch;
        L.Rise=Age<.09f?0.f:(Age-.09f)*45.f;const float Shake=7.f*std::exp(-Age/.12f);
        L.X=NumberNoise(Seed,std::floor(Age*60.f))*Shake;L.Y=NumberNoise(Seed+3.f,std::floor(Age*60.f))*Shake;
        L.Rings=1;L.RingT=std::clamp(Age/.22f,0.f,1.f);break;}
    case NumberPersona::Burn: { // Embers: hot start, accelerating rise, flicker, cooling to dark red.
        L.Scale=1.05f+.14f*std::sin(Age*38.f+Seed*6.f);L.Rise=Age*60.f+Age*Age*110.f;L.X=std::sin(Age*17.f+Seed*9.f)*5.f;L.Heat=1.f-T;
        L.Alpha*=.8f+.2f*std::sin(Age*45.f+Seed);break;}
    case NumberPersona::Poison: { // Sluggish and wobbling; drips downward as it dies.
        L.Scale=1.f+.05f*std::sin(Age*5.f);L.Rise=Age*26.f;L.X=std::sin(Age*5.5f+Seed*6.f)*13.f;
        const float Drip=std::max(0.f,T-.6f);L.Y=Drip*Drip*260.f;L.Alpha*=.85f+.15f*std::sin(Age*9.f);break;}
    case NumberPersona::Frost: { // Snaps in oversized, hangs perfectly still with a shiver, then shatters outward.
        const float Snap=std::exp(-Age/.05f);L.Scale=1.f+.3f*Snap;L.Rise=0.f;L.Flash=Snap;
        L.X=NumberNoise(Seed,std::floor(Age*45.f))*2.f;
        if(T>.8f) {const float K=(T-.8f)/.2f;L.Scale*=1.f+.35f*K;L.Rings=1;L.RingT=K;}break;}
    case NumberPersona::Bleed: { // Heavy: no lift, swells like a drop and falls.
        L.Scale=1.f+.1f*T;L.Rise=0.f;L.Y=Age*Age*150.f;L.X=NumberNoise(Seed,0.f)*10.f;break;}
    case NumberPersona::Shock: { // Erratic: re-jitters in discrete steps and flickers.
        const float Step=std::floor(Age*28.f);L.X=NumberNoise(Seed,Step)*16.f;L.Y=NumberNoise(Seed+5.f,Step)*10.f;
        L.Scale=1.f+.14f*NumberNoise(Seed+9.f,Step);L.Rise=Age*40.f;L.Alpha*=NumberNoise(Seed+2.f,Step)>0.f?1.f:.6f;L.Flash=NumberNoise(Seed,Step)>.35f?1.f:0.f;break;}
    case NumberPersona::Spiral: { // Orbits as it climbs.
        L.Scale=1.05f;L.Rise=Age*48.f;L.X=std::cos(Age*8.f+Seed*6.f)*18.f*std::min(1.f,Age*3.f);L.Y=std::sin(Age*8.f+Seed*6.f)*7.f;break;}
    default:
        if(IsSynergyPersona(P)) { // Biggest thing on screen: slam-in, long hold, shockwave rings, trembling.
            const float Slam=std::exp(-Age/.1f);L.Scale=1.9f+1.3f*Slam;L.Flash=Slam;L.Rise=Age<.2f?0.f:(Age-.2f)*30.f;
            const float Shake=9.f*std::exp(-Age/.18f);L.X=NumberNoise(Seed,std::floor(Age*70.f))*Shake;L.Y=NumberNoise(Seed+4.f,std::floor(Age*70.f))*Shake;
            L.Rings=3;L.RingT=std::clamp(Age/.45f,0.f,1.f);
        }
        break;
    }
    return L;
}

// ---- Wave banner: a band sweeps in from the left, holds, then exits right. Looks only.
constexpr float BannerLife=1.9f,BannerIn=.24f,BannerOut=.32f;
struct BannerPose {float Slide,Alpha,Stretch;}; // Slide: -1 = off left, 0 = centred, +1 = off right.
inline BannerPose BannerAt(float Age) {
    if(Age<0.f||Age>=BannerLife) return {-1.f,0.f,0.f};
    if(Age<BannerIn) {const float T=Age/BannerIn;const float Ease=1.f-(1.f-T)*(1.f-T)*(1.f-T);return {-1.f+Ease,std::min(1.f,T*2.f),1.f-.5f*Ease};}
    if(Age>BannerLife-BannerOut) {const float T=(Age-(BannerLife-BannerOut))/BannerOut;return {T*T,1.f-T,.5f};}
    return {0.f,1.f,.5f+.04f*std::sin((Age-BannerIn)*3.f)};
}

// ---- Kill moments (looks only): shard burst size by enemy tier, and the hit-stop that sells elite/boss deaths.
struct KillBurst {int Shards;float Speed,Size,Life,StopScale,StopSeconds;};
inline KillBurst KillBurstFor(bool Elite,bool Boss) {
    if(Boss) return {30,720.f,2.6f,1.1f,.12f,.16f};
    if(Elite) return {16,520.f,1.7f,.85f,.15f,.07f};
    return {8,380.f,1.f,.65f,1.f,0.f};
}
// Ballistic shard: position offset from the burst centre after Age seconds (cm), launched at Velocity.
inline float ShardHeight(float Vz,float Age) {return Vz*Age-.5f*980.f*Age*Age;}
inline float ShardScale(float Age,float Life) {const float T=std::clamp(Age/std::max(.05f,Life),0.f,1.f);return T<.7f?1.f:(1.f-T)/.3f;}
}
