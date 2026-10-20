#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// 0.24 Part C (the Crab Champions takeaways): pure rules, no Unreal types, tested as plain C++ (Tests/partc_024_tests.cpp).
// Every number here is TUNABLE; the game code only asks these functions.
namespace ASPartC {

// ---- C-3 perfect dash: a blow that lands inside the dash's invulnerability, or within PerfectDashWindow seconds after it ends, is "perfect" ----
constexpr float PerfectDashWindow=.15f,PerfectDashCooldown=.6f,PerfectDashBonusProtection=.15f;
// 0.24.1 (playtest: the dash cooldown is already short, so a refund meant little): a perfect dash boosts the damage you deal for a few seconds instead.
constexpr float PerfectDashBoost=.30f,PerfectDashBoostSeconds=4.f;
inline bool PerfectBoostActive(float Now,float BoostUntil) {return std::isfinite(Now)&&std::isfinite(BoostUntil)&&Now<BoostUntil;}
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
// 0.24.2 (F1-F3; playtest: launches "don't go far enough", wall slams and pile-ups "don't work"): a launch used to be a flat shove along the floor. Ground friction, braking and
// the enemy's own AI (which stops dead while it winds up or recovers) killed it in about 0.1 s and 20 cm, so a launched body almost never reached a wall or another enemy and the
// contact code never ran. Now a launch is a short hop (off the floor nothing brakes it) followed by a low-friction skid; the AI is suspended until the body comes to rest; and
// contacts are found by sweeping ahead along the flight every tick, with the engine's NotifyHit as a second opinion. The STRENGTH callers pass is unchanged (950 for the maul's
// slam, 700 for its leap landing, 600 for Repulse, 120 x rank x the weapon's on-hit scale for Heavy Impact); these rules turn it into speeds, times and distances.
constexpr float ImpactMinStrength=250.f,ImpactCooldown=.15f;
constexpr int ImpactMaxGeneration=2; // a launched body can launch another (generation 1), which can launch one more (2); then it stops
inline bool ImpactWorthy(float Strength) {return std::isfinite(Strength)&&Strength>=ImpactMinStrength;}
inline float ImpactDamage(float Strength,int Generation,float Multiplier=1.f) {
    if(!std::isfinite(Strength)||!std::isfinite(Multiplier)||Strength<=0.f) return 0.f;
    const float Base=10.f+Strength*.055f; // 250 -> 24, 600 -> 43, 950 -> 62 (0.24.2: was 6 + 4.5%); the game also multiplies by the hero's damage scale
    return std::min(220.f,Base*std::pow(.7f,float(std::clamp(Generation,0,ImpactMaxGeneration)))*std::max(0.f,Multiplier));
}
inline float ThrownOnStrength(float Strength) {return std::isfinite(Strength)?Strength*.65f:0.f;} // the body that was hit flies off at 65% of the strength
// The flight. Horizontal speed and lift grow with strength; airtime is the hop's (gravity 980 cm/s^2, the enemies' gravity scale is 1); the skid that follows brakes with
// SkidFriction (per second, proportional) and SkidBraking (cm/s^2, constant), the way CharacterMovement brakes a walker. Elites fly a little over half as far as they used to.
constexpr float LaunchSpeedPerStrength=1.f,LaunchSpeedMax=1500.f,LaunchLiftPerStrength=.30f,LaunchLiftMin=130.f,LaunchLiftMax=360.f,LaunchGravity=980.f;
constexpr float SkidFriction=3.f,SkidBraking=1000.f,EliteLaunchScale=.55f,LaunchMaxSeconds=1.6f;
struct Flight {float Horizontal=0.f,Vertical=0.f,Airtime=0.f,Skid=0.f,Seconds=0.f,Distance=0.f;};
inline float SkidSeconds(float Speed) { // time for the skid's braking (friction plus constant braking) to bring Speed to rest
    if(!std::isfinite(Speed)||Speed<=0.f) return 0.f;
    const float Floor=SkidBraking/SkidFriction;return std::log((Speed+Floor)/Floor)/SkidFriction;
}
inline float SkidDistance(float Speed) {
    const float T=SkidSeconds(Speed);if(T<=0.f) return 0.f;
    const float Floor=SkidBraking/SkidFriction;return (Speed+Floor)/SkidFriction*(1.f-std::exp(-SkidFriction*T))-Floor*T;
}
inline Flight LaunchFor(float Strength) {
    Flight F;if(!std::isfinite(Strength)||Strength<=0.f) return F;
    F.Horizontal=std::min(LaunchSpeedMax,Strength*LaunchSpeedPerStrength);
    F.Vertical=std::clamp(Strength*LaunchLiftPerStrength,LaunchLiftMin,LaunchLiftMax);
    F.Airtime=2.f*F.Vertical/LaunchGravity;F.Skid=SkidSeconds(F.Horizontal);
    F.Seconds=std::min(LaunchMaxSeconds,F.Airtime+F.Skid);F.Distance=F.Horizontal*F.Airtime+SkidDistance(F.Horizontal);
    return F;
}
// A contact counts while the body still moves at ImpactMinSpeed or more; how hard it hits is the launch strength scaled by how much of the launch speed is left.
constexpr float ImpactMinSpeed=260.f,ImpactContactSlack=30.f; // slack: added to the two capsule radii to call two bodies touching
inline float ContactStrength(float Strength,float LaunchSpeed,float SpeedNow) {
    if(!std::isfinite(Strength)||!std::isfinite(LaunchSpeed)||!std::isfinite(SpeedNow)||Strength<=0.f||LaunchSpeed<=0.f||SpeedNow<ImpactMinSpeed) return 0.f;
    return Strength*std::min(1.f,SpeedNow/LaunchSpeed);
}
// After a wall slam the body rebounds off the wall at a fraction of its speed with a small lift, and is dazed for a moment (a window to punish it).
constexpr float SlamBounce=.35f,SlamBounceLift=170.f,SlamDaze=.5f,PileCarry=.3f; // PileCarry: the share of its speed a projectile keeps after striking another body
// Two bodies only collide if one is closing on the other: a crowd flung together by one slam flies side by side and must not count as a pile-up (the in-engine check found that).
constexpr float PileClosingSpeed=160.f;
inline bool Closing(float RelativeSpeedAlongContact) {return std::isfinite(RelativeSpeedAlongContact)&&RelativeSpeedAlongContact>=PileClosingSpeed;}

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
constexpr float CeremonyPushSeconds=.9f,CeremonyPushFov=-24.f,CeremonyHitStopSeconds=.07f; // 0.24.1: -9 was too subtle in the playtest (the push-in could zoom further)
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
    if(F.PerfectDashes>=2) Add("You dashed through blows in the last instant. A perfect dash shields you a moment longer and boosts your damage by 30% for four seconds.");
    else if(F.PerfectDashes==0&&F.DamageTaken>F.DamageDealt*.5f&&F.Kills>0) Add("Dash just as a blow lands: a perfect dash costs nothing and boosts your damage by 30% for four seconds.");
    if(F.Traits<=2&&F.Seconds>240.f) Add("Few traits taken - chests and shops are where a build comes from; stack one theme.");
    if(F.Coop&&F.Kills>0) Add("Co-op synergy attacks hit harder than elements. Fight close to your partner.");
    if(F.Endless&&F.Wave>=50) Add("Past wave 50 the sky turns red and lightning falls. Spread out and watch the floor.");
    if(F.Endless&&F.Wave<20&&F.Wave>0) Add("Endless starts gently, but the arena shifts and the waves grow. Keep moving between them.");
    if(!F.Endless&&F.Room>0) Add("Each room's elite is a remix, not a bigger bar. Read its colour and its wind-up before you commit.");
    if(L.Count==0) Add("Every run teaches something. Die enlightened: try a different weapon or trait theme next time.");
    return L;
}

} // namespace ASPartC
