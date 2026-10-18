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
    DamageLook L{1.f+.3f*std::exp(-Age/.06f),Age*55.f,0.f,0.f,Fade,0.f,0.f,0,0.f}; // every number pops in quickly
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
struct BannerPose {float Slide,Alpha,Stretch,TitleScale;}; // Slide: -1 = off left, 0 = centred, +1 = off right.
inline BannerPose BannerAt(float Age) {
    if(Age<0.f||Age>=BannerLife) return {-1.f,0.f,0.f,1.f};
    const float Pop=1.f+1.1f*std::exp(-std::max(0.f,Age-.05f)/.07f)*(Age<.05f?Age/.05f:1.f); // fast scale-in overshoot
    if(Age<BannerIn) {const float T=Age/BannerIn;const float Ease=1.f-(1.f-T)*(1.f-T)*(1.f-T);return {-1.f+Ease,std::min(1.f,T*2.f),1.f-.5f*Ease,Pop};}
    if(Age>BannerLife-BannerOut) {const float T=(Age-(BannerLife-BannerOut))/BannerOut;return {T*T,1.f-T,.5f,1.f};}
    return {0.f,1.f,.5f+.04f*std::sin((Age-BannerIn)*3.f),Pop};
}

// ---- Kill moments (looks only): shard burst size by enemy tier, and the hit-stop that sells elite/boss deaths.
struct KillBurst {int Shards;float Speed,Size,Life,StopScale,StopSeconds;};
inline KillBurst KillBurstFor(bool Elite,bool Boss) {
    if(Boss) return {30,720.f,2.6f,1.1f,.15f,.05f};
    if(Elite) return {16,520.f,1.7f,.85f,.2f,.035f}; // spec: 30-50ms hit-stop on elite/boss kills
    return {8,380.f,1.f,.65f,1.f,0.f};
}
// Ballistic shard: position offset from the burst centre after Age seconds (cm), launched at Velocity.
inline float ShardHeight(float Vz,float Age) {return Vz*Age-.5f*980.f*Age*Age;}
inline float ShardScale(float Age,float Life) {const float T=std::clamp(Age/std::max(.05f,Life),0.f,1.f);return T<.7f?1.f:(1.f-T)/.3f;}

// ---- Pickup flow (Crab Champions style): drops are tossed out, bounce, settle into a glowing hover, and are
// pulled to a player who needs them with a whoosh that speeds up as they close in. Looks and feel only.
constexpr float PickupMagnetRadius=520.f,PickupGrabRadius=170.f;
inline float PickupHeight(float Age) {
    Age=std::max(0.f,Age);
    const float Bounce=std::fabs(std::sin(Age*8.f))*150.f*std::exp(-Age*2.6f);
    const float Hover=(14.f+6.f*std::sin(Age*3.f))*(1.f-std::exp(-Age*3.f));
    return Bounce+Hover;
}
inline float PickupPulse(float Age) {return 1.f+.08f*std::sin(Age*5.f);}
// Pull speed (cm/s) at a distance from the player; rises steeply as the pickup closes in.
inline float PickupMagnetSpeed(float Distance) {
    if(Distance>=PickupMagnetRadius) return 0.f;
    const float Close=1.f-std::clamp(Distance/PickupMagnetRadius,0.f,1.f);return 380.f+2200.f*Close*Close;
}

// ---- Boss intro (looks only): letterbox bars, name slam and a camera push-in over the boss's 2s spawn protection.
constexpr float BossIntroLife=2.f;
struct BossIntroPose {float Bars,NameScale,NameAlpha,FovPush;}; // Bars 0..1 of full bar height; FovPush in degrees.
inline float IntroSmooth(float T) {T=std::clamp(T,0.f,1.f);return T*T*(3.f-2.f*T);}
inline BossIntroPose BossIntroAt(float Age,bool ReduceMotion) {
    if(Age<0.f||Age>=BossIntroLife) return {0.f,1.f,0.f,0.f};
    const float Bars=IntroSmooth(Age/.25f)*(1.f-IntroSmooth((Age-1.6f)/.4f));
    float Scale=1.f,Alpha=0.f;
    if(Age>=.3f) {const float Slam=std::exp(-(Age-.3f)/.09f);Scale=1.f+1.6f*Slam;Alpha=std::min(1.f,(Age-.3f)/.06f)*(1.f-IntroSmooth((Age-1.6f)/.4f));}
    const float Push=ReduceMotion?0.f:10.f*IntroSmooth(Age/.5f)*(1.f-IntroSmooth((Age-1.3f)/.7f));
    return {Bars,Scale,Alpha,Push};
}

// ---- Portal entry juice: FOV kick and a short fade to black over the 1.2s walk-in. Looks only.
constexpr float PortalWalkSeconds=1.2f;
inline float PortalKickFov(float Age,bool ReduceMotion) {
    if(ReduceMotion||Age<0.f||Age>PortalWalkSeconds+.3f) return 0.f;
    return 14.f-8.f*IntroSmooth((Age-.25f)/.95f)*(Age>.25f?1.f:0.f)-(Age<.25f?14.f*(1.f-IntroSmooth(Age/.25f)):0.f);
}
inline float PortalFadeAlpha(float Age) {return Age<0.f?0.f:.92f*IntroSmooth((Age-.7f)/.5f);}
// ---- Low-HP vignette: off at full health, a pulsing red frame that quickens as health drops.
struct VignetteLook {float Alpha,Hertz;};
inline VignetteLook LowHpVignette(float HealthFraction,float Time) {
    const float F=std::isfinite(HealthFraction)?std::clamp(HealthFraction,0.f,1.f):1.f;
    if(F>=.999f) return {0.f,0.f};
    const float Hz=1.f+3.f*(1.f-F),Intensity=std::pow(1.f-F,1.6f)*.62f;
    const float Pulse=.5f+.5f*std::sin(Time*6.2831853f*Hz);
    return {Intensity*(.55f+.45f*Pulse),Hz};
}

// ---- Reward fanfare: floating "+X" text and the bigger trait banner. Looks only.
constexpr float FloatTextLife=1.7f;
struct FloatTextPose {float Rise,Alpha,Scale;};
inline FloatTextPose FloatTextAt(float Age,bool Big) {
    if(Age<0.f||Age>=FloatTextLife) return {0.f,0.f,1.f};
    const float Pop=1.f+(Big?.9f:.55f)*std::exp(-Age/.09f);
    const float Fade=std::clamp((FloatTextLife-Age)/.45f,0.f,1.f);
    return {(Big?36.f:80.f)*IntroSmooth(Age/FloatTextLife),Fade,Pop};
}
// Chest pop: the opened chest swells and settles. Multiplier on the chest's own scale.
constexpr float ChestPopLife=.45f;
inline float ChestPopScale(float Age) {
    if(Age<0.f||Age>=ChestPopLife) return 1.f;
    return 1.f+.28f*std::sin(3.14159265f*Age/ChestPopLife);
}

// ---- Enemy death: topple, glow, shrink. Looks only; every machine animates it from Health<=0.
constexpr float EnemyDeathSeconds=.75f;
struct EnemyDeathLook {float Topple,Scale,Glow;};
inline EnemyDeathLook EnemyDeathAt(float Age) {
    const float T=std::isfinite(Age)?std::clamp(Age/EnemyDeathSeconds,0.f,1.f):1.f;
    const float Fall=std::min(1.f,T/.55f),Shrink=std::clamp((T-.55f)/.45f,0.f,1.f);
    return {Fall*Fall*(3.f-2.f*Fall),1.f-.97f*Shrink*Shrink,(1.f-T)*(1.f-T)};
}
// ---- Dash trail: fading afterimages dropped while moving at dash speed. Looks only; spawned locally on every client.
constexpr float DashGhostSpacing=.06f,DashGhostLife=.28f;constexpr int DashGhostCap=12; // full-body copies cost ~20-40 meshes each, so fewer, spaced wider
// TrailActive is true only while a dash or hammer leap is actually happening; walking and running never qualify.
inline bool DashGhostDue(bool TrailActive,float Now,float NextAt) {return TrailActive&&std::isfinite(Now)&&Now>=NextAt;}
inline float DashGhostAlpha(float Age) {const float T=std::clamp(Age/DashGhostLife,0.f,1.f);return (1.f-T)*(1.f-T);}
inline float DashGhostShrink(float Age) {const float T=std::clamp(Age/DashGhostLife,0.f,1.f);return 1.f-.45f*T;}

// ---- Menu button juice: hover swell, quick press squash, eased between states. Looks only.
constexpr float ButtonHover=1.045f,ButtonPress=.94f,ButtonTween=.09f;
inline float ButtonScaleTarget(bool Hover,bool Press,bool Enabled) {return !Enabled?1.f:(Press?ButtonPress:(Hover?ButtonHover:1.f));}
inline float ButtonScaleAt(float From,float To,float Age) {
    const float T=std::clamp(Age/ButtonTween,0.f,1.f);const float Ease=1.f-(1.f-T)*(1.f-T);return From+(To-From)*Ease;
}
}
