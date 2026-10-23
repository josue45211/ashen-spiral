#pragma once
// Twin Claws (0.26): the single-target shredder with the pounce. Engine-independent rules shared by the game and the native tests (Tests/claws_026_tests.cpp).
#include <algorithm>
#include <cmath>
#include <vector>
#include "ASRules.h"
namespace ASClaws {
// ---- the swing ----
// A swing shreds ONE target: the claws trade the blade's frontal arc for speed (.19 s a swing) and a latch (the pounce). Reach is centre to centre minus the target's own radius,
// like every melee weapon here; the pounce parks the wielder well inside it.
constexpr float Reach=215.f,ReachWhenLatched=300.f;           // cm; a latched wielder may drift a little and still hit
constexpr float HitStop=.045f;                                // seconds of freeze on a hit that lands (presentation; the camera holds a hair longer than the swing)
// ---- the pounce ----
constexpr float PounceCooldown=6.f,WhiffCooldownScale=.4f;    // a pounce at nothing (a lunge) costs 40% of the cooldown
constexpr float PounceRange=1100.f;                           // farthest centre-to-centre distance a target can be pounced from (the longest flight, FlightMax, at FlightSpeed)
constexpr float AimSlack=120.f;                               // cm beyond an enemy's body that the crosshair ray may pass and still pick it
constexpr float Windup=.10f;                                  // the crouch before the leap (the maul's is .22)
constexpr float FlightSpeed=2800.f,FlightMin=.15f,FlightMax=.40f;
constexpr float WhiffDistance=480.f;                          // the lunge when nothing is under the crosshair
constexpr float ArcPerDistance=.10f,ArcMin=18.f,ArcMax=120.f; // the leap's peak height: a tenth of its length, between these
constexpr float FlightProtection=.12f;                        // invulnerability after the flight's end (the flight itself is protected)
constexpr float StrikeDamageScale=2.2f;                       // the arrival hit, in claw swings
// ---- the latch ----
constexpr float GlueSeconds=3.5f;                             // how long a pounce holds
constexpr float RingGap=14.f;                                 // cm between the two capsules' surfaces: the wielder stands this close to the target
constexpr float GlueGain=14.f;                                // 1/s: how hard the wielder is pulled back onto its ring point
constexpr float GlueMaxSpeed=1400.f;                          // cm/s, the fastest a latched wielder is carried after a target (a charging brute runs at about 700)
constexpr float GlueLoseDistance=420.f,GlueLoseSeconds=.5f;   // farther than this from the ring point for this long: the latch lets go (a wall, a ledge)
constexpr float TeleportJump=650.f;                           // a target that moves this far in one frame has teleported or phased: the latch lets go
constexpr float AirborneLoseSeconds=.9f;                      // off the ground this long while latched: let go (a guest's leap lands on the server a round trip after the server's own latch began)

inline float FlightSeconds(float Distance) {return std::clamp(std::isfinite(Distance)?Distance/FlightSpeed:FlightMax,FlightMin,FlightMax);}
inline float ArcHeight(float Distance) {return std::clamp(std::isfinite(Distance)?Distance*ArcPerDistance:ArcMin,ArcMin,ArcMax);}
inline float RingRadius(float SelfRadius,float TargetRadius) {return std::max(0.f,SelfRadius)+std::max(0.f,TargetRadius)+RingGap;}
inline float Cooldown(bool Whiff) {return PounceCooldown*(Whiff?WhiffCooldownScale:1.f);}

struct Vec2 {float X=0.f,Y=0.f;};
inline float Length(Vec2 V) {return std::sqrt(V.X*V.X+V.Y*V.Y);}
// The horizontal velocity that holds the latch: the target's own velocity (so a runner does not outpace the wielder) plus a pull onto the ring point, capped.
inline Vec2 GlueVelocity(Vec2 Self,Vec2 Ring,Vec2 TargetVelocity,float MaxSpeed=GlueMaxSpeed,float Gain=GlueGain) {
    Vec2 V{TargetVelocity.X+(Ring.X-Self.X)*Gain,TargetVelocity.Y+(Ring.Y-Self.Y)*Gain};
    if(!std::isfinite(V.X)||!std::isfinite(V.Y)) return {};
    const float L=Length(V);
    if(L>MaxSpeed&&L>0.f) {V.X*=MaxSpeed/L;V.Y*=MaxSpeed/L;}
    return V;
}
// The ring point: on the line from the target out to the wielder (Direction need not be normalised), one ring radius from the target's centre. A zero direction falls back to +X.
inline Vec2 RingPoint(Vec2 Target,Vec2 Direction,float Radius) {
    float L=Length(Direction);
    if(!std::isfinite(L)||L<1e-4f) {Direction={1.f,0.f};L=1.f;}
    return {Target.X+Direction.X/L*Radius,Target.Y+Direction.Y/L*Radius};
}

// What the crosshair is on: the enemy whose body the camera ray passes closest to (within AimSlack of its surface), in front of the camera and inside the pounce range of the wielder.
struct Candidate {float X=0.f,Y=0.f,Z=0.f,Radius=40.f;bool Valid=true;};
inline int PickTarget(const float Origin[3],const float Dir[3],const float Self[3],const std::vector<Candidate>& Enemies,float Range=PounceRange,float Slack=AimSlack) {
    const float DL=std::sqrt(Dir[0]*Dir[0]+Dir[1]*Dir[1]+Dir[2]*Dir[2]);
    if(!std::isfinite(DL)||DL<1e-4f) return -1;
    const float D[3]={Dir[0]/DL,Dir[1]/DL,Dir[2]/DL};
    int Best=-1;float BestMiss=1e30f,BestAlong=1e30f;
    for(int I=0;I<int(Enemies.size());++I) {
        const Candidate& E=Enemies[size_t(I)];if(!E.Valid) continue;
        const float To[3]={E.X-Origin[0],E.Y-Origin[1],E.Z-Origin[2]};
        const float Along=To[0]*D[0]+To[1]*D[1]+To[2]*D[2];
        if(!std::isfinite(Along)||Along<=0.f) continue;                              // behind the camera
        const float Px=To[0]-D[0]*Along,Py=To[1]-D[1]*Along,Pz=To[2]-D[2]*Along;
        const float Miss=std::sqrt(Px*Px+Py*Py+Pz*Pz)-E.Radius;                      // how far the ray passes from the enemy's surface
        const float Dx=E.X-Self[0],Dy=E.Y-Self[1];
        if(std::sqrt(Dx*Dx+Dy*Dy)>Range+E.Radius) continue;
        if(Miss>Slack) continue;
        if(Miss<BestMiss-1.f||(std::fabs(Miss-BestMiss)<=1.f&&Along<BestAlong)) {Best=I;BestMiss=Miss;BestAlong=Along;}
    }
    return Best;
}

// Whether a latch lets go this frame, and why. Reasons are stable codes for the log and the tests.
// Displaced: the target teleported or phased (a jump of TeleportJump in one frame) or was thrown (launched) by something.
enum class Release : int {None=0,Duration,TargetDead,TargetUntouchable,Displaced,Dashed,Lost,Airborne,Interrupted};
struct LatchFrame {float Elapsed=0.f,TargetJump=0.f,OffRingSeconds=0.f,AirborneSeconds=0.f;bool TargetAlive=true,TargetTouchable=true,TargetLaunched=false,Dashed=false,Interrupted=false;};
inline Release ReleaseReason(const LatchFrame& F) {
    if(F.Interrupted) return Release::Interrupted;
    if(F.Dashed) return Release::Dashed;
    if(!F.TargetAlive) return Release::TargetDead;
    if(!F.TargetTouchable) return Release::TargetUntouchable;
    if(F.TargetJump>=TeleportJump||F.TargetLaunched) return Release::Displaced;
    if(F.Elapsed>=GlueSeconds) return Release::Duration;
    if(F.OffRingSeconds>=GlueLoseSeconds) return Release::Lost;
    if(F.AirborneSeconds>=AirborneLoseSeconds) return Release::Airborne;
    return Release::None;
}
}
