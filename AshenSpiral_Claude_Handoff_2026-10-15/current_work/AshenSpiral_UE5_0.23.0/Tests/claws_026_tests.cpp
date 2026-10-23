// Native tests for 0.26 Twin Claws (ASClawRules.h, and the weapon plumbing in ASRules.h / ASMeleeRules.h / ASArsenalRules.h / ASBuildRules.h). Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASClawRules.h"
#include "../Source/AshenSpiral/Public/ASArsenalRules.h"
#include "../Source/AshenSpiral/Public/ASBuildRules.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.01f) {return std::fabs(A-B)<E;}
int main() {
    using namespace ASClaws;
    const float NaN=std::numeric_limits<float>::quiet_NaN(),Inf=std::numeric_limits<float>::infinity();
    // ---- the weapon: id, class, combo, cadence ----
    Check(ASRules::WeaponCount==10&&ASRules::Claws==9&&ASRules::WeaponMask==1023,"ten weapons; Twin Claws is the tenth, with the new top bit");
    for(int W=0;W<ASRules::WeaponCount;++W) {
        const bool Gun=W>=6&&W<=8;
        Check(ASRules::IsGun(W)==Gun,"IsGun is exactly the three firearms (Twin Claws is not one)");
        Check(ASMelee::IsMelee(W)==(W==0||W==1||W==5||W==9),"melee weapons: blade, maul, scythe and the claws");
        Check(ASMelee::LeapsOnRmb(W)==(W==1||W==9),"the maul and the claws put their own move on the right mouse button");
        Check(ASArsenal::UsesMagazine(W)==!ASMelee::IsMelee(W),"the claws never reload");
    }
    Check(ASMelee::ComboLength(9)==2&&ASMelee::ComboLength(0)==3&&ASMelee::ComboLength(1)==3&&ASMelee::ComboLength(5)==1,"the claws are a two-step combo; the blade and maul keep three");
    Check(Near(ASArsenal::Rate(9),.19f)&&Near(ASMelee::BaseSeconds(9),ASArsenal::Rate(9)),"the claws swing every .19 s and the pose clock agrees");
    for(int Step=0;Step<2;++Step) Check(ASMelee::TimeScale(9,Step)==1.f&&ASMelee::DamageScale(9,Step)==1.f&&Near(ASMelee::ImpactFraction(9,Step),.38f),"neither claw swing is a finisher: no longer, no harder, and the hit lands at 38%");
    Check(ASMelee::AverageTimeScale(9)==1.f&&Near(ASArsenal::BaseInterval(9),.19f),"the combo's average step length is one swing");
    Check(ASArsenal::Rate(9)<ASArsenal::Rate(0)&&ASArsenal::Rate(9)<ASArsenal::Rate(5)&&ASArsenal::Rate(9)<ASArsenal::Rate(1),"the claws are the fastest melee weapon");
    Check(ASArsenal::BaseCapacity(9)==0&&ASArsenal::Capacity(9,5)==0&&ASArsenal::ReloadSeconds(9,5)==0.f,"no magazine, no reload");
    // ---- damage: the fastest weapon is the best against one target, and flat upgrades add what they add elsewhere ----
    const float ClawDPS=ASArsenal::Damage(9)/ASArsenal::Rate(9),BladeDPS=ASArsenal::Damage(0)/ASArsenal::Rate(0);
    Check(ClawDPS>BladeDPS&&ClawDPS<BladeDPS*1.25f,"against one target the claws out-damage the blade, but not by much (the blade also hits everything in its arc)");
    Check(Near(ASArsenal::Damage(9),30.f),"a claw swing is 30");
    for(int Rank=0;Rank<=5;++Rank) Check(Near((ASArsenal::Damage(9,4.f*Rank)-ASArsenal::Damage(9))/ASArsenal::Rate(9),12.5f*Rank,.01f),"a flat primary upgrade adds the same 12.5 DPS a rank to the claws as to a firearm");
    Check(Near(ASArsenal::OnHitScale(9),.475f,.001f)&&ASArsenal::OnHitScale(9)>ASArsenal::OnHitScaleMin,"on-hit rewards scale to .48 per claw hit: many hits, a fair reward rate");
    Check(Near(ASRules::AttackInterval(ASArsenal::Rate(9),5,5,true)*5.f,ASArsenal::Rate(9)*5.f/((1.f+.4f+.6f)*1.25f),.001f),"attack speed scales the claw swing like any other");
    // ---- who may take which trait (weapon masks) ----
    ASBuild::Context C;C.Weapon=9;
    const auto Can=[&](ASBuild::Trait T) {return ASBuild::CanChoose(C,ASBuild::OfferBase+ASBuild::Index(T));};
    Check(Can(ASBuild::Trait::Melee)&&Can(ASBuild::Trait::WideArc)&&Can(ASBuild::Trait::Sharpened)&&Can(ASBuild::Trait::Deadeye)&&Can(ASBuild::Trait::Bleed),"the claws take melee traits, the universal ones and the bleed touch");
    Check(!Can(ASBuild::Trait::Ranged)&&!Can(ASBuild::Trait::Pierce)&&!Can(ASBuild::Trait::Split)&&!Can(ASBuild::Trait::Magazine)&&!Can(ASBuild::Trait::Reload)&&!Can(ASBuild::Trait::Precision),"and not the ranged, projectile or magazine ones");
    for(int Seed=1;Seed<=500;++Seed) for(int Pick:ASBuild::Offers(C,Seed)) Check(!(ASBuild::IsTrait(Pick)&&!(ASBuild::Catalog[Pick-ASBuild::OfferBase].WeaponMask&(1<<9))),"no draft ever offers the claws a trait that is not for them");
    for(int I=0;I<ASBuild::Count;++I) Check(ASBuild::Catalog[I].WeaponMask>0&&(ASBuild::Catalog[I].WeaponMask&~ASRules::WeaponMask)==0,"every trait's weapon mask is within the ten weapons");
    {ASBuild::Context Blade;Blade.Weapon=0;for(int Seed=1;Seed<=200;++Seed) for(int Pick:ASBuild::Offers(Blade,Seed)) if(ASBuild::IsTrait(Pick)) Check(Pick!=ASBuild::OfferBase+ASBuild::Index(ASBuild::Trait::Ranged),"a blade is still never offered the ranged damage trait");}
    // ---- the pounce: leap and arc ----
    Check(Near(FlightSeconds(0.f),FlightMin)&&Near(FlightSeconds(300.f),FlightMin)&&Near(FlightSeconds(700.f),.25f)&&Near(FlightSeconds(PounceRange),PounceRange/FlightSpeed)&&Near(FlightSeconds(9999.f),FlightMax),"a leap takes its distance over the flight speed, between the shortest and the longest");
    Check(PounceRange/FlightSpeed<=FlightMax+1e-4f,"the longest pounce is not clamped: its flight fits the longest allowed");
    Check(FlightSeconds(NaN)==FlightMax&&FlightSeconds(Inf)==FlightMax&&FlightSeconds(-5.f)==FlightMin,"a bad distance still gives a sane flight");
    for(float D=0.f;D<2000.f;D+=10.f) Check(FlightSeconds(D)>=FlightSeconds(D-10.f)&&FlightSeconds(D)>=FlightMin&&FlightSeconds(D)<=FlightMax,"flight time never decreases with distance and stays in bounds");
    Check(Near(ArcHeight(0.f),ArcMin)&&Near(ArcHeight(500.f),50.f)&&Near(ArcHeight(5000.f),ArcMax)&&ArcHeight(NaN)==ArcMin,"the rise is a tenth of the leap, between 18 and 120 cm");
    Check(Near(RingRadius(34.f,40.f),34.f+40.f+RingGap)&&RingRadius(-5.f,-5.f)==RingGap,"the ring is the two bodies plus a gap");
    Check(Near(Cooldown(false),6.f)&&Near(Cooldown(true),2.4f),"a pounce is 6 s; a lunge at nothing 40% of that");
    // ---- the ring point and the latch ----
    {const Vec2 P=RingPoint({100.f,50.f},{3.f,4.f},50.f);Check(Near(P.X,130.f,.001f)&&Near(P.Y,90.f,.001f),"the ring point lies on the line out from the target, a ring radius away");}
    {const Vec2 P=RingPoint({100.f,50.f},{0.f,0.f},50.f);Check(Near(P.X,150.f)&&Near(P.Y,50.f),"no direction falls back to +X");}
    {const Vec2 P=RingPoint({0.f,0.f},{NaN,1.f},50.f);Check(std::isfinite(P.X)&&std::isfinite(P.Y),"a bad direction never makes a bad point");}
    // a stationary target: the wielder is drawn onto the ring and stops there, never faster than the cap
    {
        Vec2 Self{400.f,-250.f};const Vec2 Ring{100.f,0.f};float Previous=1e9f;
        for(int Frame=0;Frame<180;++Frame) {
            const Vec2 V=GlueVelocity(Self,Ring,{0.f,0.f});Check(Length(V)<=GlueMaxSpeed+.01f,"the pull never exceeds the cap");
            Self.X+=V.X/60.f;Self.Y+=V.Y/60.f;
            const float Error=Length({Self.X-Ring.X,Self.Y-Ring.Y});Check(Error<=Previous+.001f,"the error to the ring never grows against a stationary target");Previous=Error;
        }
        Check(Previous<.1f,"after three seconds the wielder is on the ring");
    }
    // a runner: the target moves at 700 cm/s and turns; the wielder keeps within a few cm of its ring point (the feed-forward carries it) and is never faster than the cap
    {
        Vec2 Target{0.f,0.f},Self{-220.f,60.f};float Worst=0.f;
        for(int Frame=0;Frame<600;++Frame) {
            const float T=float(Frame)/60.f;const Vec2 TV{700.f*std::cos(T*.9f),700.f*std::sin(T*.9f)};
            Target.X+=TV.X/60.f;Target.Y+=TV.Y/60.f;
            const Vec2 Ring=RingPoint(Target,{-1.f,0.f},88.f);
            const Vec2 V=GlueVelocity(Self,Ring,TV);Check(Length(V)<=GlueMaxSpeed+.01f,"the pull never exceeds the cap");
            Self.X+=V.X/60.f;Self.Y+=V.Y/60.f;
            if(Frame>90) Worst=std::max(Worst,Length({Self.X-Ring.X,Self.Y-Ring.Y}));
        }
        Check(Worst<60.f,"latched to a runner (700 cm/s, turning), the wielder stays within 60 cm of its ring point");
    }
    {const Vec2 V=GlueVelocity({0.f,0.f},{1e6f,0.f},{0.f,0.f});Check(Near(Length(V),GlueMaxSpeed,.01f),"an absurd error is capped, not obeyed");}
    {const Vec2 V=GlueVelocity({NaN,0.f},{0.f,0.f},{0.f,0.f});Check(V.X==0.f&&V.Y==0.f,"bad positions give no velocity");}
    {const Vec2 V=GlueVelocity({100.f,0.f},{100.f,0.f},{300.f,-200.f});Check(Near(V.X,300.f)&&Near(V.Y,-200.f),"on the ring, the wielder simply goes where the target goes");}
    // ---- what the crosshair picks ----
    {
        const float Origin[3]={-600.f,0.f,100.f},Dir[3]={1.f,0.f,0.f},Self[3]={-300.f,0.f,100.f}; // the ray runs along the enemies' chest height
        std::vector<Candidate> E;
        Candidate A;A.X=300;A.Y=0;A.Z=100;A.Radius=40;E.push_back(A);       // dead ahead, 600 cm along
        Candidate B;B.X=300;B.Y=90;B.Z=100;B.Radius=40;E.push_back(B);      // 90 cm to the side
        Candidate Behind;Behind.X=-900;Behind.Y=0;Behind.Z=100;Behind.Radius=40;E.push_back(Behind); // behind the camera
        Candidate Far;Far.X=900;Far.Y=0;Far.Z=100;Far.Radius=40;E.push_back(Far); // out of the wielder's range
        Check(PickTarget(Origin,Dir,Self,E)==0,"the enemy the ray runs through is the one picked");
        E[0].Valid=false;Check(PickTarget(Origin,Dir,Self,E)==1,"an enemy that is not valid (no line of sight, dead) is skipped; the next nearest the ray is picked");
        E[1].Valid=false;Check(PickTarget(Origin,Dir,Self,E)==-1,"nothing behind the camera or out of range is ever picked");
        E[0].Valid=E[1].Valid=true;E[1].Y=400;Check(PickTarget(Origin,Dir,Self,E)==0,"an enemy far off the ray is not picked over one on it");
        E[0].Valid=false;Check(PickTarget(Origin,Dir,Self,E)==-1,"an enemy 400 cm off the ray is not under the crosshair at all");
        E[0].Valid=true;E[0].Y=40+AimSlack-1.f;Check(PickTarget(Origin,Dir,Self,E)==0,"inside the slack beside a body still counts as under the crosshair");
        E[0].Y=40+AimSlack+5.f;Check(PickTarget(Origin,Dir,Self,E)==-1,"just outside it does not");
        const float NoDir[3]={0.f,0.f,0.f},BadDir[3]={NaN,0.f,0.f};
        Check(PickTarget(Origin,NoDir,Self,E)==-1&&PickTarget(Origin,BadDir,Self,E)==-1,"a bad ray picks nothing");
        Check(PickTarget(Origin,Dir,Self,{})==-1,"no enemies, no target");
        std::vector<Candidate> Same;Candidate N1;N1.X=100;N1.Y=0;N1.Z=100;N1.Radius=40;Same.push_back(N1);Candidate N2=N1;N2.X=500;Same.push_back(N2);
        Check(PickTarget(Origin,Dir,Self,Same)==0,"of two enemies on the same ray the nearer is picked");
    }
    // ---- when the latch lets go, and why ----
    {
        LatchFrame F;Check(ReleaseReason(F)==Release::None,"a healthy latch holds");
        F=LatchFrame();F.Elapsed=GlueSeconds-.01f;Check(ReleaseReason(F)==Release::None,"it holds to the last moment");
        F.Elapsed=GlueSeconds;Check(ReleaseReason(F)==Release::Duration,"and lets go when its time is up");
        F=LatchFrame();F.TargetAlive=false;Check(ReleaseReason(F)==Release::TargetDead,"the target dying ends it");
        F=LatchFrame();F.TargetTouchable=false;Check(ReleaseReason(F)==Release::TargetUntouchable,"so does the target becoming untouchable (a phase, a shield, a transition)");
        F=LatchFrame();F.TargetJump=TeleportJump-1.f;Check(ReleaseReason(F)==Release::None,"a big step is not yet a teleport");
        F.TargetJump=TeleportJump;Check(ReleaseReason(F)==Release::Displaced,"a teleport is");
        F=LatchFrame();F.TargetLaunched=true;Check(ReleaseReason(F)==Release::Displaced,"and so is being thrown");
        F=LatchFrame();F.Dashed=true;Check(ReleaseReason(F)==Release::Dashed,"a dash breaks it");
        F=LatchFrame();F.OffRingSeconds=GlueLoseSeconds-.01f;Check(ReleaseReason(F)==Release::None,"a moment off the ring is forgiven");
        F.OffRingSeconds=GlueLoseSeconds;Check(ReleaseReason(F)==Release::Lost,"half a second off it is not");
        F=LatchFrame();F.AirborneSeconds=AirborneLoseSeconds;Check(ReleaseReason(F)==Release::Airborne,"neither is being off the ground");
        F=LatchFrame();F.Interrupted=true;F.Dashed=true;F.TargetAlive=false;Check(ReleaseReason(F)==Release::Interrupted,"an interruption outranks every other reason");
        F=LatchFrame();F.Dashed=true;F.TargetAlive=false;Check(ReleaseReason(F)==Release::Dashed,"a dash outranks the target's death (the wielder chose to leave)");
        F=LatchFrame();F.TargetAlive=false;F.Elapsed=GlueSeconds+1.f;Check(ReleaseReason(F)==Release::TargetDead,"a kill is credited as a kill, not as the clock running out");
    }
    std::printf("PASS: %lld Twin Claws checks (weapon plumbing and masks, cadence and damage, flight and arc, the ring and the latch's pull against a runner, the crosshair pick, every release reason).\nShared rules only; Unreal movement, replication and gameplay are not executed.\n",Checks);
    return 0;
}
