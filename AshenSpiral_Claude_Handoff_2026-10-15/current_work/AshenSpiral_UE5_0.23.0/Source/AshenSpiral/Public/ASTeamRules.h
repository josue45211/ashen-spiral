#pragma once
// 0.26 Phase 2: the team abilities, three new active skills (F) built for two players. Engine-independent rules shared by the game and the native tests (Tests/team_026_tests.cpp).
//   Ashen Brand   (Mark & Pop)  brand an enemy; the hits your partner lands on it fill the brand, and when it is full it bursts for what it stored, splashing the crowd around it
//   Ember Tether  (Link)        bind yourself to your partner with a line of embers: it lashes every enemy it crosses, and both of you hit harder while you stay close
//   Twin Collapse (Twin Cast)   cast together inside a short window: each cast is a small collapse of its own, and the two together pull every enemy in to the midpoint and detonate it
// Every one of them has a weaker form for a hero on its own (solo: the brand fills from your own hits at a third of the rate, the tether anchors to an ember stake, a lone Twin Collapse is the
// small collapse): usable, never as good as two.
#include <algorithm>
#include <cmath>
namespace ASTeam {
constexpr int Brand=6,Tether=7,Twin=8,FirstAbility=6;           // ASAdventure ability ids (0-5 are the 0.24 skills)
constexpr int SynergyBrand=3,SynergyTether=4,SynergyTwin=5;    // the SynergyCue triggers these raise (0-2 are Lightning Blast, Stormbreak, Wildfire)
inline bool AbilityIsTeam(int Ability) {return Ability>=FirstAbility&&Ability<FirstAbility+3;}
// The cues the server sends every machine (AASCharacter::TeamCue): what to show and play.
enum Cue : int {CueBrandSet=0,CueBrandTick,CueBrandPop,CueBrandDeathPop,CueBrandFizzle,CueTetherOn,CueTetherZap,CueTetherEnd,CueTwinCall,CueTwinLesser,CueTwinConverge,CueTwinBlast,CueTwinTooFar,
    CueWard,CuePredator,CuePierce,CueEcho};                         // 0.26 Phase 3 (the skill tree): Shared Ward, Predator's refund, Piercing Rounds opening, a Synergy Echo
constexpr int EchoLesser=6;                                    // Synergy Echo's trigger number for the small collapse (0-5 are the SynergyCue numbers)

// ---- Ashen Brand ----
constexpr float BrandSeconds=8.f;                              // how long a brand holds
constexpr float BrandOwnShare=.35f;                            // the branding hero's own hits fill the brand at this share; a partner's fill it whole
constexpr float BrandSplashShare=.60f,BrandDeathShare=.50f;    // the burst's splash on the others, as a share of what was stored; halved when the branded enemy died first
constexpr float BrandSplashRadius=380.f,BrandBossBonus=.035f;  // cm; a boss also takes 3.5% of its health, as the elemental synergies do
constexpr float BrandDeathMinFill=.50f;                        // a brand that dies with this much stored still splashes
constexpr float BrandAssist=260.f,BrandReach=1800.f;           // the cast finds the enemy under the crosshair, else the nearest within BrandAssist of where the aim ends
inline float BrandNeed(float MaxHealth,bool Boss) {
    if(!std::isfinite(MaxHealth)||MaxHealth<=0.f) return 70.f;
    return Boss?std::clamp(MaxHealth*.04f,120.f,500.f):std::clamp(MaxHealth*.25f,70.f,400.f);
}
inline float BrandCharge(float Dealt,bool FromOwner) {return (std::isfinite(Dealt)&&Dealt>0.f)?Dealt*(FromOwner?BrandOwnShare:1.f):0.f;}
inline int BrandFillPercent(float Stored,float Need) {return (Need>0.f&&std::isfinite(Stored))?std::clamp(int(std::lround(Stored/Need*100.f)),0,100):0;}
inline int BrandMilestone(int FillPercent) {return std::clamp(FillPercent,0,100)/25;}  // 0..4: a tick as the fill passes each quarter
inline float BrandBurstDamage(float Stored,float MaxHealth,bool Boss) {return std::max(0.f,Stored)+(Boss&&std::isfinite(MaxHealth)?std::max(0.f,MaxHealth)*BrandBossBonus:0.f);}
inline float BrandSplashDamage(float Stored,bool EnemyDied) {return std::max(0.f,Stored)*BrandSplashShare*(EnemyDied?BrandDeathShare:1.f);}
inline bool BrandDeathBursts(float Stored,float Need) {return Need>0.f&&Stored>=Need*BrandDeathMinFill;}

// ---- Ember Tether ----
enum LinkKind {LinkNone=0,LinkPair=1,LinkAnchor=2};
constexpr float TetherSeconds=7.f;
constexpr float TetherFormRange=1800.f;                        // a partner farther than this cannot be tethered to: the tether anchors to a stake instead
constexpr float TetherBuffRange=900.f;                         // the damage bonus holds while the two stand this close
constexpr float TetherSnap=2400.f,AnchorSnap=1500.f;           // the tether breaks when its ends are pulled this far apart (a hero and its stake: this far)
constexpr float TetherDamageBonus=.15f;                        // both tethered heroes' damage, paired and close
constexpr float LashInterval=.45f,LashHalfWidth=90.f,LashVertical=220.f,LashDamage=18.f,LashAnchorScale=.55f;
constexpr float AnchorPlaceRange=900.f;                        // how far from the caster the stake can be planted
constexpr int LashCuesPerPulse=4;                              // the show of one pulse is capped (the damage is not)
inline float LashDamageFor(bool Pair) {return LashDamage*(Pair?1.f:LashAnchorScale);}
inline float TetherBuff(bool Pair,float Separation) {return (Pair&&std::isfinite(Separation)&&Separation<=TetherBuffRange)?TetherDamageBonus:0.f;}
inline bool TetherSnaps(int Mode,float Separation) {return Mode==LinkPair?!(Separation<=TetherSnap):(Mode==LinkAnchor?!(Separation<=AnchorSnap):false);}
// Distance on the floor from a point to the segment AB, and where along it (0 at A, 1 at B) the closest point is.
inline float SegmentDistance2D(float Px,float Py,float Ax,float Ay,float Bx,float By,float* AlongOut=nullptr) {
    const float Dx=Bx-Ax,Dy=By-Ay,L2=Dx*Dx+Dy*Dy;
    float T=L2>1e-6f?((Px-Ax)*Dx+(Py-Ay)*Dy)/L2:0.f;T=std::clamp(T,0.f,1.f);
    if(AlongOut) *AlongOut=T;
    const float Cx=Ax+Dx*T-Px,Cy=Ay+Dy*T-Py;return std::sqrt(Cx*Cx+Cy*Cy);
}
// Whether an enemy body (a point and a radius) is crossed by the line from A to B: within the lash's half width of it on the floor and within LashVertical of the line's height there.
inline bool LashCrosses(const float P[3],float Radius,const float A[3],const float B[3]) {
    for(int I=0;I<3;++I) if(!std::isfinite(P[I])||!std::isfinite(A[I])||!std::isfinite(B[I])) return false;
    float T=0.f;const float D=SegmentDistance2D(P[0],P[1],A[0],A[1],B[0],B[1],&T);
    return D<=LashHalfWidth+std::max(0.f,Radius)&&std::fabs(P[2]-(A[2]+(B[2]-A[2])*T))<=LashVertical;
}

// ---- Twin Collapse ----
constexpr float TwinWindow=1.5f;                               // a second cast this soon after the first (and close enough) joins it
constexpr float TwinMaxSeparation=1600.f;                      // cm between the two heroes
constexpr float TwinDelay=.65f,TwinPullTick=.10f,TwinPull=520.f; // the pull-in, then the detonation
constexpr float TwinRadius=700.f,TwinDamage=95.f,TwinBossBonus=.05f,TwinStun=1.0f,TwinShield=25.f;
constexpr float TwinLesserRadius=330.f,TwinLesserDamage=24.f,TwinLesserShield=10.f,TwinLesserStun=.4f; // every cast: the small collapse of the caster's own
inline bool TwinJoins(float Now,float PartnerCallUntil,float Separation) {return PartnerCallUntil>0.f&&Now<=PartnerCallUntil&&std::isfinite(Separation)&&Separation<=TwinMaxSeparation;}
inline float TwinBlastDamage(float MaxHealth,bool Boss) {return TwinDamage+(Boss&&std::isfinite(MaxHealth)?std::max(0.f,MaxHealth)*TwinBossBonus:0.f);}
}
