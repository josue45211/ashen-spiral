#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// 0.24 Part C (the Crab Champions takeaways): pure rules, no Unreal types, tested as plain C++ (Tests/partc_024_tests.cpp).
// Every number here is TUNABLE; the game code only asks these functions.
namespace ASPartC {

// ---- C-3 perfect dash: a blow that lands inside the dash's invulnerability, or within PerfectDashWindow seconds after it ends, is "perfect" ----
constexpr float PerfectDashWindow=.15f,PerfectDashCooldown=.6f,PerfectDashBonusProtection=.15f;
inline bool PerfectDash(float Now,float DashInvulEnd,float LastPerfectAt) {
    if(!std::isfinite(Now)||!std::isfinite(DashInvulEnd)||!std::isfinite(LastPerfectAt)||DashInvulEnd<=0.f) return false;
    return Now<DashInvulEnd+PerfectDashWindow&&Now>=LastPerfectAt+PerfectDashCooldown;
}

// ---- C-1 proc chains: an element proc may echo to a neighbour (depth 0 -> 1 -> 2), at most one chain per cooldown ----
constexpr int ChainMaxDepth=2;
constexpr float ChainCooldown=.45f,ChainRange=450.f,ChainFalloff=.6f;
inline bool ChainMayStart(float Now,float NextChainAt) {return std::isfinite(Now)&&std::isfinite(NextChainAt)&&Now>=NextChainAt;}
inline bool ChainMayContinue(int Depth) {return Depth>=0&&Depth<ChainMaxDepth;}
inline float ChainMagnitude(int Depth) {return std::pow(ChainFalloff,float(std::clamp(Depth,0,8)+1));} // the first echo is .6, the second .36 of the original proc
// Which of the four element traits echoes: a conductor trait (Chain Lightning, Storm Touch or Impact Bloom) is needed, and the echo needs at least one element rank.
inline bool ChainConducts(int ChainRank,int ShockRank,int SplashRank) {return ChainRank>0||ShockRank>0||SplashRank>0;}

// ---- C-2 knockback as joy: a launched enemy that slams a wall or another enemy takes impact damage, and a body hit is thrown on ----
constexpr float ImpactMinStrength=250.f,ImpactWindow=.7f,ImpactEnemyRadius=120.f,ImpactSpeedLossFraction=.45f,ImpactCooldown=.15f;
constexpr int ImpactMaxGeneration=2; // a launched body can launch another (generation 1), which can launch one more (2); then it stops
inline bool ImpactWorthy(float Strength) {return std::isfinite(Strength)&&Strength>=ImpactMinStrength;}
inline float ImpactDamage(float Strength,int Generation,float Multiplier=1.f) {
    if(!std::isfinite(Strength)||!std::isfinite(Multiplier)||Strength<=0.f) return 0.f;
    const float Base=6.f+Strength*.045f; // 250 -> 17, 420 -> 25, 800 -> 42
    return std::min(160.f,Base*std::pow(.7f,float(std::clamp(Generation,0,ImpactMaxGeneration)))*std::max(0.f,Multiplier));
}
// The engine reports the contact (AActor::NotifyHit); how hard it was is the launch strength that is left, which fades linearly over the window down to a floor.
inline float RemainingStrength(float Strength,float Elapsed) {
    if(!std::isfinite(Strength)||!std::isfinite(Elapsed)||Strength<=0.f||Elapsed<0.f||Elapsed>=ImpactWindow) return 0.f;
    return Strength*std::max(ImpactSpeedLossFraction,1.f-Elapsed/ImpactWindow);
}
inline float ThrownOnStrength(float Strength) {return std::isfinite(Strength)?Strength*.65f:0.f;} // the body that was hit flies off at 65% of the strength

// ---- C-6 metamorphoses: at MAX rank four traits change what they do (rank equal to the catalogue maximum) ----
enum class Metamorphosis : int {None=0,Wildfire,Shatter,Pinball,StormWeb,Count};
inline bool AtMax(int Rank,int Max) {return Max>0&&Rank>=Max;}
constexpr float WildfireRadius=380.f,WildfireDamageFraction=.50f,ShatterRadius=340.f,ShatterDamageFraction=.60f,PinballImpactMultiplier=2.5f,StormWebExtraLinks=2,StormWebRangeMultiplier=1.4f;
inline int StormWebLinks(int ChainRank,bool Metamorphosed) {return std::min(6,std::max(0,ChainRank)+(Metamorphosed?int(StormWebExtraLinks):0));}
inline float WildfireDamage(float VictimMaxHealth) {return std::isfinite(VictimMaxHealth)?std::clamp(VictimMaxHealth*WildfireDamageFraction,10.f,400.f):10.f;}
inline float ShatterDamage(float VictimMaxHealth) {return std::isfinite(VictimMaxHealth)?std::clamp(VictimMaxHealth*ShatterDamageFraction,12.f,400.f):12.f;}
inline const char* MetamorphosisName(Metamorphosis M) {
    switch(M) {case Metamorphosis::Wildfire: return "WILDFIRE";case Metamorphosis::Shatter: return "SHATTER";case Metamorphosis::Pinball: return "PINBALL";case Metamorphosis::StormWeb: return "STORM WEB";default: return "";}
}
inline const char* MetamorphosisText(Metamorphosis M) {
    switch(M) {
        case Metamorphosis::Wildfire: return "Burning enemies explode when they die, setting their neighbours alight.";
        case Metamorphosis::Shatter: return "Frozen enemies shatter when they die, chilling and hurting everything near them.";
        case Metamorphosis::Pinball: return "Launched enemies hit like wrecking balls: impacts do 2.5x damage and hurl whatever they strike.";
        case Metamorphosis::StormWeb: return "Chain Lightning gains two links and reaches 40% further.";
        default: return "";
    }
}

// ---- C-5 reward ceremony without slow-mo: a camera push-in and a short hit-stop, then the choices ----
constexpr float CeremonyPushSeconds=.9f,CeremonyPushFov=-9.f,CeremonyHitStopSeconds=.07f;
inline float CeremonyPush(float Elapsed) { // 0..1 smooth in, then it holds; the caller adds CeremonyPushFov x this to the field of view
    if(!std::isfinite(Elapsed)||Elapsed<=0.f) return 0.f;
    const float T=std::min(1.f,Elapsed/CeremonyPushSeconds);return T*T*(3.f-2.f*T);
}

// ---- C-7 "what you learned": lines for the death screen, picked from what the run actually did ----
struct RunFacts {
    float Seconds=0.f,DamageDealt=0.f,DamageTaken=0.f;
    int Kills=0,PerfectDashes=0,Impacts=0,Echoes=0,Traits=0,Room=0,Wave=0;
    bool Endless=false,Coop=false,Metamorphosed=false;
};
struct Lines {std::array<const char*,3> Text{};int Count=0;};
inline Lines Learned(const RunFacts& F) {
    Lines L;
    auto Add=[&L](const char* T) {if(L.Count<int(L.Text.size())) L.Text[size_t(L.Count++)]=T;};
    // Most specific first: what the build did, then what the player did, then a plain note on where the run ended.
    if(F.Metamorphosed) Add("Your build transformed - a trait at full rank changes what it does. Aim to get there sooner.");
    if(F.Echoes>=3) Add("Element procs echoed from enemy to enemy. Chain Lightning, Storm Touch or Impact Bloom carry them further.");
    if(F.Impacts>=3) Add("Launched enemies hurt whatever they hit. Heavy Impact turns walls and crowds into weapons.");
    if(F.PerfectDashes>=2) Add("You dashed through blows in the last instant. A perfect dash refunds your dash and shields you a moment longer.");
    else if(F.PerfectDashes==0&&F.DamageTaken>F.DamageDealt*.5f&&F.Kills>0) Add("Dash just as a blow lands: a perfect dash costs nothing and refunds your dash.");
    if(F.Traits<=2&&F.Seconds>240.f) Add("Few traits taken - chests and shops are where a build comes from; stack one theme.");
    if(F.Coop&&F.Kills>0) Add("Co-op synergy attacks hit harder than elements. Fight close to your partner.");
    if(F.Endless&&F.Wave>=50) Add("Past wave 50 the sky turns red and lightning falls. Spread out and watch the floor.");
    if(F.Endless&&F.Wave<20&&F.Wave>0) Add("Endless starts gently, but the arena shifts and the waves grow. Keep moving between them.");
    if(!F.Endless&&F.Room>0) Add("Each room's elite is a remix, not a bigger bar. Read its colour and its wind-up before you commit.");
    if(L.Count==0) Add("Every run teaches something. Die enlightened: try a different weapon or trait theme next time.");
    return L;
}

} // namespace ASPartC
