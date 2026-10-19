// Native tests for the 0.24 enemy foundation: kits (the numbers ASEnemy::Tick used to spell out), fluid movement planning, retreat, pack spreading and unsticking.
// Plain C++ over ASEnemyRules.h; Unreal movement, navigation, collision and replication are not executed.
#include "../Source/AshenSpiral/Public/ASEnemyRules.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <fstream>
#include <set>
#include <string>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.001f) {return std::fabs(A-B)<E;}
static unsigned Seed=24u;
static float Rand() {Seed=Seed*1664525u+1013904223u;return float((Seed>>8)&0xFFFF)/65535.f;}
static float Range(float A,float B) {return A+(B-A)*Rand();}
using namespace ASEnemyRules;
int main(int argc,char** argv) {
    const std::string Root=argc>1?argv[1]:"";
    const float NaN=std::numeric_limits<float>::quiet_NaN(),Inf=std::numeric_limits<float>::infinity();
    // ---- Kits reproduce the old hard-coded numbers exactly ----
    for(int Kind=0;Kind<=9;++Kind) {
        const bool Boss=Kind==4;
        const float OldSpeed=Boss?235.f:(Kind==5?390.f:(Kind==6?230.f:(Kind==0?340.f:275.f)));
        const float OldRange=Kind==8?1700.f:((Kind==1||Kind==7||Kind==9||Boss)?1300.f:(Kind==5||Kind==6?650.f:180.f));
        const float OldWindup=Kind==8?1.5f:(Kind==9?1.4f:(Boss||Kind==6?1.1f:.85f));
        const float OldCooldown=Boss?2.f:1.8f;
        const Kit& K=KitFor(Kind);
        Check(K.Speed==OldSpeed&&K.AttackRange==OldRange&&K.Windup==OldWindup&&K.Cooldown==OldCooldown,"every kit row matches the numbers ASEnemy::Tick used before 0.24");
    }
    Check(&KitFor(-3)==&KitFor(0)&&&KitFor(99)==&KitFor(KindCount-1),"an out-of-range kind clamps to a real row");
    for(int Kind=0;Kind<KindCount;++Kind) {
        const Kit& K=KitFor(Kind);
        Check(std::isfinite(K.Speed)&&K.Speed>0.f&&K.AttackRange>0.f&&K.Windup>=.5f&&K.Cooldown>=1.5f,"kits have positive speed and range, a readable wind-up (>= .5 s) and a real cooldown");
        Check(K.Strafe>=0.f&&K.Strafe<=1.f&&K.RetreatBelow>=0.f&&K.RetreatBelow<=.5f,"strafe is a fraction and a retreat never starts above half health");
        if(K.BandMax>0.f) Check(K.BandMin>0.f&&K.BandMin<K.BandMax&&K.BandMax<=K.AttackRange&&K.Circle==0.f,"a shooter's band is ordered, inside its attack range, and it does not also circle");
        else Check(K.BandMin==0.f,"a melee kit has no band");
        if(K.Circle>0.f) {
            Check(K.Circle>HoldDistance&&K.Circle<=400.f,"a melee circle sits outside the hold distance and close enough to re-engage at once");
            if(K.AttackRange<=300.f) Check(K.Circle>=K.AttackRange,"a close-range melee kind circles outside the distance it starts its attack from (the leaper and bulwark attack from afar, so they may circle inside theirs)");
        }
    }
    Check(KitFor(Archer).RetreatBelow>0.f&&KitFor(Shaman).RetreatBelow>0.f&&KitFor(Sniper).RetreatBelow>0.f&&KitFor(Broodcaller).RetreatBelow>0.f,"the four frail ranged kinds can fall back");
    Check(KitFor(Grunt).RetreatBelow==0.f&&KitFor(Brute).RetreatBelow==0.f&&KitFor(BossKind).RetreatBelow==0.f,"melee kinds and bosses never run");

    // ---- PlanMovement: invariants over random states ----
    long long Modes[5]={0,0,0,0,0};
    for(int I=0;I<200000;++I) {
        const Kit& K=KitFor(int(Rand()*10.f)%10);
        const float Angle=Range(0.f,6.2831853f),Distance=Range(0.f,2500.f);
        const float ToX=std::cos(Angle)*Distance,ToY=std::sin(Angle)*Distance;
        const float RouteAngle=Range(0.f,6.2831853f);
        const MovePlan P=PlanMovement(K,Distance,ToX,ToY,Rand()<.2f?0.f:std::cos(RouteAngle),Rand()<.2f?0.f:std::sin(RouteAngle),Rand()<.5f,Rand()<.8f,Rand()<.15f,Rand()<.5f?1.f:-1.f,Range(-60.f,60.f));
        const float Length=std::sqrt(P.X*P.X+P.Y*P.Y);
        Check(std::isfinite(P.X)&&std::isfinite(P.Y)&&std::isfinite(P.Speed),"a plan is always finite");
        Check(P.Mode>=ModeHold&&P.Mode<=ModeRetreat,"a plan always has a known mode");
        if(P.Mode==ModeHold) Check(Length==0.f||Near(Length,1.f,.002f),"a hold plan may point nowhere");
        else Check(Near(Length,1.f,.002f),"every moving plan is a unit direction");
        Check(P.Speed>=0.f&&P.Speed<=RetreatSpeed+.0001f,"a plan never asks for more than the retreat speed");
        ++Modes[P.Mode];
    }
    Check(Modes[ModeAdvance]>0&&Modes[ModeCircle]>0&&Modes[ModeBackpedal]>0&&Modes[ModeRetreat]>0&&Modes[ModeHold]>0,"all five modes are reachable");
    // Degenerate inputs give a quiet hold, never garbage.
    {
        const Kit& K=KitFor(Grunt);
        const MovePlan A=PlanMovement(K,NaN,1,0,1,0,true,true,false,1,0),B=PlanMovement(K,100,NaN,0,1,0,true,true,false,1,0),C=PlanMovement(K,100,0,0,1,0,true,true,false,1,0),D=PlanMovement(K,500,Inf,Inf,1,0,true,true,false,1,0);
        Check(A.Mode==ModeHold&&B.Mode==ModeHold&&C.Mode==ModeHold&&D.Mode==ModeHold&&A.Speed==0.f,"non-finite or zero-length inputs hold still");
        const MovePlan E=PlanMovement(K,500,500,0,NaN,NaN,true,true,false,1,NaN);
        Check(E.Mode==ModeAdvance&&Near(E.X,1.f,.001f)&&Near(E.Y,0.f,.001f),"a broken route direction falls back to the straight line, and a broken flank angle to none");
    }
    // ---- PlanMovement: specific behaviour ----
    {   // Melee, attack ready: close in along the route; hold once at the hold distance with sight.
        const Kit& K=KitFor(Grunt);
        const MovePlan Far=PlanMovement(K,900,900,0,1,0,true,true,false,1,0);
        Check(Far.Mode==ModeAdvance&&Near(Far.X,1.f)&&Far.Speed==1.f&&!Far.Face,"a ready grunt advances along the route at full speed, facing its motion");
        const MovePlan Close=PlanMovement(K,HoldDistance-5.f,HoldDistance-5.f,0,1,0,true,true,false,1,0);
        Check(Close.Mode==ModeHold&&Close.Face,"a ready grunt that has closed in stands and faces the target");
        const MovePlan Blind=PlanMovement(K,HoldDistance-5.f,HoldDistance-5.f,0,0,1,true,false,false,1,0);
        Check(Blind.Mode==ModeAdvance&&Near(Blind.Y,1.f),"without sight it keeps following the route even when close");
        const MovePlan Waiting=PlanMovement(K,K.Circle,K.Circle,0,1,0,false,true,false,1,0);
        Check(Waiting.Mode==ModeCircle&&Waiting.Face&&Near(Waiting.Speed,K.Strafe)&&Near(Waiting.X,0.f,.05f)&&Waiting.Y>.9f,"a grunt on cooldown at its circle radius walks sideways at strafe speed, facing the target");
        const MovePlan Mirror=PlanMovement(K,K.Circle,K.Circle,0,1,0,false,true,false,-1,0);
        Check(Mirror.Mode==ModeCircle&&Mirror.Y<-.9f,"the other side circles the other way");
        const MovePlan TooClose=PlanMovement(K,150.f,150.f,0,1,0,false,true,false,1,0);
        Check(TooClose.Mode==ModeCircle&&TooClose.X<-.3f,"a grunt on cooldown that is too close drifts outward while circling");
        const MovePlan TooFar=PlanMovement(K,K.Circle+100.f,K.Circle+100.f,0,1,0,false,true,false,1,0);
        Check(TooFar.Mode==ModeCircle&&TooFar.X>.3f,"... and one a little too far drifts inward");
        const MovePlan Distant=PlanMovement(K,K.Circle+CircleReachSlack+40.f,K.Circle+CircleReachSlack+40.f,0,1,0,false,true,false,1,0);
        Check(Distant.Mode==ModeAdvance,"beyond its circle plus the slack it closes in even on cooldown");
        const MovePlan Flank=PlanMovement(K,900,900,0,1,0,true,true,false,1,30.f);
        Check(Flank.Mode==ModeAdvance&&Near(Flank.X,std::cos(30.f*0.017453292f),.01f)&&Near(Flank.Y,std::sin(30.f*0.017453292f),.01f),"a flank angle rotates the advance");
        const MovePlan Clamp=PlanMovement(K,900,900,0,1,0,true,true,false,1,500.f);
        Check(Near(Clamp.X,std::cos(45.f*0.017453292f),.01f),"the flank angle is capped at 45 degrees");
    }
    {   // A boss has no circle: it closes in and waits exactly as before until its own kit arrives.
        const Kit& K=KitFor(BossKind);
        Check(PlanMovement(K,200,200,0,1,0,false,true,false,1,0).Mode==ModeAdvance,"a boss on cooldown at 200 cm still closes in (no circling yet)");
        Check(PlanMovement(K,100,100,0,1,0,false,true,false,1,0).Mode==ModeHold,"... and holds at the hold distance");
    }
    {   // Shooters keep their band.
        const Kit& K=KitFor(Archer);
        const MovePlan Near_=PlanMovement(K,K.BandMin-100.f,K.BandMin-100.f,0,1,0,false,true,false,1,0);
        Check(Near_.Mode==ModeBackpedal&&Near_.X<-.8f&&Near_.Face&&Near(Near_.Speed,BackpedalSpeed),"an archer that is too close backs away (diagonally) facing the target");
        const MovePlan Far=PlanMovement(K,K.BandMax+200.f,K.BandMax+200.f,0,1,0,false,true,false,1,0);
        Check(Far.Mode==ModeAdvance&&Far.X>.99f,"... closes in when it is beyond the band");
        const MovePlan In=PlanMovement(K,.5f*(K.BandMin+K.BandMax),.5f*(K.BandMin+K.BandMax),0,1,0,false,true,false,1,0);
        Check(In.Mode==ModeCircle&&In.Face&&Near(In.Speed,K.Strafe)&&Near(In.X,0.f,.05f),"inside the band it side-steps at strafe speed");
        const MovePlan Blind=PlanMovement(K,.5f*(K.BandMin+K.BandMax),.5f*(K.BandMin+K.BandMax),0,0,1,false,false,false,1,0);
        Check(Blind.Mode==ModeAdvance&&Near(Blind.Y,1.f),"without sight it follows the route to get a line");
        const MovePlan Hurt=PlanMovement(K,800,800,0,1,0,false,true,true,1,0);
        Check(Hurt.Mode==ModeRetreat&&Hurt.X<-.8f&&Hurt.Speed==RetreatSpeed&&!Hurt.Face,"a retreating archer runs away at retreat speed, turned from the target");
    }
    // ---- Kinematic simulations: the planner as a controller ----
    {   // An archer dropped far away walks into its band and stays in it while the target stands still.
        const Kit& K=KitFor(Archer);float X=2200.f,Y=0.f,Worst=0.f,Best=1e9f;
        for(int Step=0;Step<600;++Step) { // 60 simulated seconds at 10 Hz
            const float Distance=std::sqrt(X*X+Y*Y);
            const MovePlan P=PlanMovement(K,Distance,-X,-Y,-X/Distance,-Y/Distance,false,true,false,StrafeSide(7u,Step*.1f),0.f);
            X+=P.X*P.Speed*K.Speed*.1f;Y+=P.Y*P.Speed*K.Speed*.1f;
            if(Step>=300) {const float D=std::sqrt(X*X+Y*Y);Worst=std::max(Worst,D);Best=std::min(Best,D);}
        }
        Check(Best>=K.BandMin-110.f&&Worst<=K.BandMax+110.f,"a kiting archer settles inside its distance band and stays there for 30 seconds");
    }
    {   // A hurt-free archer rushed from point blank backs out to its band.
        const Kit& K=KitFor(Archer);float X=250.f,Y=0.f;
        for(int Step=0;Step<200;++Step) {
            const float Distance=std::sqrt(X*X+Y*Y);
            const MovePlan P=PlanMovement(K,Distance,-X,-Y,-X/Distance,-Y/Distance,false,true,false,1.f,0.f);
            X+=P.X*P.Speed*K.Speed*.1f;Y+=P.Y*P.Speed*K.Speed*.1f;
        }
        const float D=std::sqrt(X*X+Y*Y);
        Check(D>=K.BandMin-110.f&&D<=K.BandMax+110.f,"an archer rushed to point blank backs out into its band");
    }
    {   // A grunt on cooldown walks in to its circle and then really goes around the target.
        const Kit& K=KitFor(Grunt);float X=900.f,Y=0.f,Swept=0.f,Previous=0.f,MinD=1e9f,MaxD=0.f;bool HavePrev=false;
        for(int Step=0;Step<400;++Step) {
            const float Distance=std::sqrt(X*X+Y*Y);
            const MovePlan P=PlanMovement(K,Distance,-X,-Y,-X/Distance,-Y/Distance,false,true,false,1.f,0.f);
            X+=P.X*P.Speed*K.Speed*.1f;Y+=P.Y*P.Speed*K.Speed*.1f;
            if(Step>=100) {
                const float Angle=std::atan2(Y,X),D=std::sqrt(X*X+Y*Y);MinD=std::min(MinD,D);MaxD=std::max(MaxD,D);
                if(HavePrev) {float Delta=Angle-Previous;while(Delta>3.14159265f) Delta-=6.2831853f;while(Delta<-3.14159265f) Delta+=6.2831853f;Swept+=Delta;}
                Previous=Angle;HavePrev=true;
            }
        }
        Check(MinD>=K.Circle-90.f&&MaxD<=K.Circle+100.f,"a grunt on cooldown holds its circle radius");
        Check(std::fabs(Swept)>6.2831853f,"... and travels more than a full lap around the target in 30 seconds");
    }
    {   // A retreat opens real distance in its two seconds.
        const Kit& K=KitFor(Shaman);float X=700.f,Y=0.f;
        for(int Step=0;Step<20;++Step) {
            const float Distance=std::sqrt(X*X+Y*Y);
            const MovePlan P=PlanMovement(K,Distance,-X,-Y,-X/Distance,-Y/Distance,false,true,true,1.f,0.f);
            X+=P.X*P.Speed*K.Speed*.1f;Y+=P.Y*P.Speed*K.Speed*.1f;
        }
        Check(std::sqrt(X*X+Y*Y)>700.f+450.f,"a two-second retreat gains over 450 cm");
    }

    // ---- Circle direction flips, per enemy rhythm ----
    {
        std::set<int> Seen;bool FlipsEverywhere=true;
        for(unsigned Id=0;Id<400u;++Id) {
            int Flips=0;float Previous=StrafeSide(Id,0.f);
            for(int T=1;T<=420;++T) {const float Side=StrafeSide(Id,T*.1f);Check(Side==1.f||Side==-1.f,"a side is +1 or -1");if(Side!=Previous) ++Flips;Previous=Side;Seen.insert(int(Side));}
            if(Flips<10||Flips>20) FlipsEverywhere=false; // 42 s with a 2.2..4.0 s period: 42/4.0 = 10.5 up to 42/2.2 = 19.1 boundaries, so 10..20 flips
        }
        Check(Seen.size()==2&&FlipsEverywhere,"every enemy flips its circling side every 2.2 to 4.0 seconds, in both directions");
        Check(StrafeSide(5u,NaN)==1.f&&StrafeSide(5u,Inf)==1.f,"a non-finite clock gives a fixed side");
        int Plus=0;for(unsigned Id=0;Id<1000u;++Id) if(StrafeSide(Id,10.f)>0.f) ++Plus;
        Check(Plus>350&&Plus<650,"at any instant the crowd circles both ways in roughly equal numbers");
    }
    // ---- Pack spreading ----
    {
        float MinSeen=1e9f,MaxSeen=-1e9f,Sum=0.f;
        for(unsigned Id=0;Id<20000u;++Id) {
            const float Far=FlankDegrees(Id,2000.f);Sum+=Far;MinSeen=std::min(MinSeen,Far);MaxSeen=std::max(MaxSeen,Far);
            Check(std::fabs(Far)<=FlankSpread+.001f,"a flank angle never exceeds its spread");
            Check(FlankDegrees(Id,FlankNear)==0.f&&FlankDegrees(Id,10.f)==0.f&&FlankDegrees(Id,-50.f)==0.f,"no flanking at close range");
            Check(FlankDegrees(Id,2000.f)==FlankDegrees(Id,5000.f),"fully faded in beyond the far distance");
            float Previous=0.f;
            for(float D=FlankNear;D<=FlankFar+50.f;D+=25.f) {const float A=std::fabs(FlankDegrees(Id,D));Check(A+.0001f>=Previous,"the flank angle grows monotonically with distance");Previous=A;}
        }
        Check(MinSeen<-FlankSpread*.95f&&MaxSeen>FlankSpread*.95f&&std::fabs(Sum/20000.f)<1.5f,"flank angles use the whole range on both sides and balance out");
        Check(FlankDegrees(3u,NaN)==0.f,"a non-finite distance gives no flank");
    }
    // ---- Retreat: once, bounded, never for the last few ----
    {
        const Kit& K=KitFor(Archer);RetreatState S;
        Check(!UpdateRetreat(K,.9f,10,0.f,S),"a healthy archer does not retreat");
        Check(!UpdateRetreat(K,.1f,2,1.f,S)&&!S.Used,"... nor when only two enemies are left, and that refusal does not use the retreat up");
        Check(!UpdateRetreat(K,.0f,10,1.f,S)&&!UpdateRetreat(K,NaN,10,1.f,S)&&!UpdateRetreat(K,.1f,10,NaN,S),"a dead or non-finite state never retreats");
        Check(UpdateRetreat(K,.1f,10,2.f,S)&&S.Used,"a badly hurt archer with company falls back");
        Check(UpdateRetreat(K,.1f,10,2.f+RetreatSeconds-.01f,S)&&UpdateRetreat(K,.1f,1,2.f+RetreatSeconds-.01f,S),"... for the full two seconds, even if the company thins out meanwhile");
        Check(!UpdateRetreat(K,.1f,10,2.f+RetreatSeconds+.01f,S),"... and then stops");
        for(int Step=0;Step<3000;++Step) Check(!UpdateRetreat(K,.02f,12,5.f+Step*.1f,S),"it never retreats a second time, however hurt and however crowded");
        RetreatState T;Check(!UpdateRetreat(KitFor(Grunt),.01f,20,5.f,T)&&!UpdateRetreat(KitFor(BossKind),.01f,20,5.f,T),"melee kinds and bosses never retreat at any health");
        // Over a long fight the total time spent retreating is exactly one retreat.
        RetreatState U;float Spent=0.f;for(int Step=0;Step<6000;++Step) if(UpdateRetreat(K,.1f,12,Step*.1f,U)) Spent+=.1f;
        Check(Spent<=RetreatSeconds+.15f&&Spent>=RetreatSeconds-.15f,"an archer stuck at low health spends two seconds retreating in a ten minute fight");
        // The last stragglers are never chased off: with two left, no retreat starts at any health, and it is still available once company returns.
        RetreatState V;for(int Step=0;Step<2000;++Step) Check(!UpdateRetreat(K,.05f,RetreatMinLiving-1,Step*.1f,V),"with fewer than three enemies alive nothing falls back");
        Check(UpdateRetreat(K,.05f,RetreatMinLiving,300.f,V),"... and it is still there when enough enemies are alive again");
    }
    // ---- Kit fields added with the state machine ----
    for(int Kind=0;Kind<KindCount;++Kind) {
        const Kit& K=KitFor(Kind);
        Check(K.Accel>=800.f&&K.Accel<=4000.f&&K.Brake>=800.f&&K.Brake<=4000.f,"acceleration and braking sit between a heavy shuffle and a snap");
        Check(K.TurnRate>=180.f&&K.TurnRate<=900.f,"turn rate is between 180 and 900 degrees a second");
        Check(K.AnimSet>=AnimHumanoid&&K.AnimSet<=AnimSkeletal,"every kind belongs to a known animation family");
        Check(K.Abilities[0]>AbilityNone&&K.Abilities[0]<AbilityCount,"every kind owns at least one ability");
        bool Ended=false;std::set<int> Seen;
        for(int I=0;I<MaxKitAbilities;++I) {
            const int Id=K.Abilities[I];
            if(Id==AbilityNone) {Ended=true;continue;}
            Check(!Ended,"an ability list has no gaps: after the first 0 everything is 0");
            Check(Id>AbilityNone&&Id<AbilityCount,"every listed ability is a real id");
            Check(Seen.insert(Id).second,"an ability appears once in a list");
        }
    }
    Check(KitFor(Brute).Accel<KitFor(Grunt).Accel&&KitFor(Bulwark).Accel<KitFor(Archer).Accel&&KitFor(BossKind).TurnRate<KitFor(Grunt).TurnRate,"heavy kinds accelerate and turn slower than light ones");
    Check(KitFor(Leaper).AnimSet==AnimBlob&&KitFor(Broodcaller).AnimSet==AnimBlob&&KitFor(Gloomwing).AnimSet==AnimFlier&&KitFor(Grunt).AnimSet==AnimHumanoid,"animation families match the bodies the kinds are drawn with");
    // ---- The ability table ----
    {
        Check(StateCount==7&&StateApproach==0&&StateStrafe==1&&StateWindup==2&&StateAttack==3&&StateRecover==4&&StateRetreat==5&&StateSpecial==6,"the seven AI states have stable ids");
        Check(&SpecFor(-1)==&SpecFor(0)&&&SpecFor(999)==&SpecFor(AbilityCount-1),"an out-of-range ability clamps to a real row");
        for(int Id=0;Id<AbilityCount;++Id) {
            const AbilitySpec& A=SpecFor(Id);
            Check(A.Windup>=0.f&&A.Attack>=0.f&&A.Recover>=0.f&&A.Cooldown>=0.f&&A.MinRange>=0.f&&A.MaxRange>=0.f&&A.Weight>=0.f,"no negative timings, ranges or weights");
            if(A.MaxRange>0.f) Check(A.MaxRange>A.MinRange,"a distance window is ordered");
            Check(A.Tell>=TellNone&&A.Tell<=TellLine,"a warning shape is a known shape");
            if(A.Tell!=TellNone) Check(A.TellA>0.f,"a warning has a size");
            if(Id!=AbilityNone) Check(A.Weight>0.f||Id==AbilityRiposte||Id==AbilityBlinkStrike||(A.Flags&OneShotFlag),"every real ability can be chosen, except the follow-ups chained after another (riposte, blink strike) and the one-shot triggers (enrage)");
        }
        // Every ability a kit owns has a readable wind-up (resolved against the kit) and, when it is a walker's, a recovery.
        for(int Kind=0;Kind<KindCount;++Kind) for(int I=0;I<MaxKitAbilities;++I) {
            const int Id=KitFor(Kind).Abilities[I];if(Id==AbilityNone) break;
            if(SpecFor(Id).Weight>0.f) Check(WindupFor(KitFor(Kind),Id)>=.5f,"every ability a kit can start telegraphs for at least half a second (a chained follow-up rides on the long telegraph before it)");
            if(Kind!=Gloomwing) Check(SpecFor(Id).Recover>0.f,"a walker's every ability ends with a short exposed recovery");
        }
        // The numbers the old code used for committed motion.
        Check(SpecFor(AbilityPounce).Attack==.65f&&SpecFor(AbilityCharge).Attack==.6f&&(SpecFor(AbilityPounce).Flags&NeedsClearPath)&&(SpecFor(AbilityCharge).Flags&NeedsClearPath),"the leaper's leap and the bulwark's charge keep their old durations and path requirement");
        Check(SpecFor(AbilitySlash).Tell==TellSector&&SpecFor(AbilitySlash).TellA==MeleeRadius,"the slash warning is the half disc of the melee strike radius");
        Check(!(SpecFor(AbilitySlash).Flags&NeedsClearPath)&&!(SpecFor(AbilityShot).Flags&NeedsClearPath)&&!(SpecFor(AbilityBlast).Flags&NeedsClearPath),"only leaps and charges need a clear path");
    }
    // ---- Sound cues ----
    {
        static const char* Existing[]={"S_Attack","S_Collapse","S_Dash","S_HammerHit","S_HammerLeap","S_HammerSmash","S_HammerSwing","S_Hit","S_ReloadReady","S_ReloadStart","S_Reward","S_Spiral"};
        Check(CueNone==0&&CueCount==45,"the cue ids are stable: 44 cues");
        Check(&CueFor(-4)==&CueFor(0)&&&CueFor(9999)==&CueFor(CueCount-1),"an out-of-range cue clamps to a real row");
        std::set<std::string> Stems;
        for(int Id=1;Id<CueCount;++Id) {
            const CueSpec& C=CueFor(Id);const std::string Stem=C.Stem,Fallback=C.Fallback;
            Check(Stem.size()>3&&Stem.compare(0,2,"E_")==0&&Stems.insert(Stem).second,"every cue names its own E_ file, once");
            bool Known=Fallback.empty();for(const char* E:Existing) Known|=Fallback==E;
            Check(Known,"a fallback is one of the twelve existing sounds (or silent until the file is delivered)");
            Check(C.Volume>0.f&&C.Volume<=.7f&&C.Pitch>=.4f&&C.Pitch<=2.5f,"cue volume and pitch are sane");
            Check(C.Note&&C.Note[0]&&C.Note[1],"every cue says what it should sound like");
            if(!Fallback.empty()&&!Root.empty()) {std::ifstream File(Root+"/SourceArt/Audio/"+Fallback+".wav",std::ios::binary);Check(File.good(),"the fallback sound exists as a source file");}
        }
        for(int Id=1;Id<AbilityCount;++Id) {
            const AbilitySpec& A=SpecFor(Id);
            Check(A.CueWindup>CueNone&&A.CueWindup<CueCount&&A.CueRelease>=CueNone&&A.CueRelease<CueCount,"every real ability telegraphs with a real cue and its release cue is valid");
        }
        Check(SpecFor(AbilityNone).CueWindup==CueNone&&SpecFor(AbilityNone).CueRelease==CueNone,"no ability, no cue");
        Check(CueDistanceGain(0.f)==1.f&&CueDistanceGain(600.f)==1.f&&CueDistanceGain(3200.f)==0.f&&CueDistanceGain(99999.f)==0.f&&CueDistanceGain(-50.f)==1.f&&CueDistanceGain(NaN)==0.f,"cue gain is full inside 6 m and silent from 32 m");
        float Last=2.f;bool Falls=true;for(float D=0.f;D<4000.f;D+=10.f) {const float G=CueDistanceGain(D);if(G>Last+1e-6f||G<0.f||G>1.f) Falls=false;Last=G;}
        Check(Falls,"cue gain never rises with distance and stays within 0..1");
        Check(CueDistanceGain(1900.f)>.2f&&CueDistanceGain(1900.f)<.3f,"half way out a cue is a quarter as loud");
    }
    // ---- ChooseAbility ----
    {
        Context C;C.Now=10.f;
        for(int Kind=0;Kind<=9;++Kind) {
            Kit K=KitFor(Kind);const int Only=K.Abilities[0];for(int I=1;I<MaxKitAbilities;++I) K.Abilities[I]=AbilityNone; // the primary attack on its own (the extra abilities are tested below)
            C.Distance=K.AttackRange-1.f;C.Sight=true;C.ClearPath=true;
            Check(ChooseAbility(K,C,nullptr,AbilityNone,.3f)==Only,"in range with sight, a single-ability kit chooses its ability");
            C.Distance=K.AttackRange;Check(ChooseAbility(K,C,nullptr,AbilityNone,.3f)==AbilityNone,"exactly at the attack range is out of range (strictly inside, as before)");
            C.Distance=K.AttackRange-1.f;C.Sight=false;Check(ChooseAbility(K,C,nullptr,AbilityNone,.3f)==AbilityNone,"without sight nothing starts");
            C.Sight=true;C.ClearPath=false;
            const bool Needs=(SpecFor(Only).Flags&NeedsClearPath)!=0;
            Check((ChooseAbility(K,C,nullptr,AbilityNone,.3f)==AbilityNone)==Needs,"a blocked path stops leaps and charges and nothing else");
            C.ClearPath=true;
            float Ready[AbilityCount]={};for(float& R:Ready) R=C.Now+1.f;
            Check(ChooseAbility(K,C,Ready,AbilityNone,.3f)==AbilityNone,"an ability on its own cooldown is not chosen");
            Ready[Only]=C.Now;Check(ChooseAbility(K,C,Ready,AbilityNone,.3f)==Only,"... and is again the moment the cooldown ends");
        }
        {   // The extra abilities of the 0.24 kits open and close at their own distances.
            auto Pick=[&](int Kind,float Distance,float Roll,int Last=AbilityNone) {Context X;X.Now=10.f;X.Distance=Distance;return ChooseAbility(KitFor(Kind),X,nullptr,Last,Roll);};
            Check(Pick(Grunt,100.f,.0f)==AbilitySlash&&Pick(Grunt,200.f,.5f)==AbilityShove&&Pick(Grunt,240.f,.5f)==AbilityNone,"the grunt slashes up close, shoves from a little farther, and nothing beyond the shove's reach");
            Check(Pick(Archer,400.f,.0f)==AbilityShot&&Pick(Archer,400.f,.9f)==AbilityBackHop&&Pick(Archer,700.f,.9f)==AbilityShot,"the archer back-hops only when a hero has closed within 5 m, otherwise it shoots");
            Check(Pick(Duelist,100.f,.0f)==AbilitySlash&&Pick(Duelist,300.f,.5f)==AbilityParry&&Pick(Duelist,500.f,.5f)==AbilityNone,"the duelist raises its guard at mid distance and never chooses its riposte");
            int Counts[AbilityCount]={};for(int I=0;I<1000;++I) ++Counts[Pick(Brute,100.f,float(I)/1000.f)];
            Check(Counts[AbilitySlash]>0&&Counts[AbilityPound]>0&&Counts[AbilityEnrage]==0&&Counts[AbilityRiposte]==0,"the brute slashes and pounds, and its enrage is never picked by the chooser");
            Check(Pick(Brute,500.f,.5f)==AbilityPound,"the brute pounds from a distance too");
            Check(Pick(Leaper,200.f,.5f)==AbilityPounce&&Pick(Leaper,200.f,.99f)==AbilityPounce&&Pick(Leaper,500.f,.0f)==AbilityPounce&&Pick(Leaper,500.f,.99f)==AbilityBlink&&Pick(Leaper,900.f,.0f)==AbilityBlink&&Pick(Leaper,1200.f,.5f)==AbilityNone,"the leaper pounces when close, chooses between pounce and blink from 3.5 to 6.5 m, and only blinks out to 11 m");
            Check(Pick(Shaman,500.f,.0f)==AbilityBlast&&Pick(Shaman,500.f,.99f)==AbilityMeteors,"the shaman alternates ground blasts and meteor rain");
            Check(Pick(Duelist,300.f,.5f,AbilityParry)==AbilityParry,"a lone eligible ability repeats when there is nothing else");
            for(int Kind=0;Kind<KindCount;++Kind) for(int I=0;I<400;++I) {
                Context X;X.Now=10.f;X.Distance=float(I)*4.f;X.ClearPath=(I&1)!=0;
                const int Id=ChooseAbility(KitFor(Kind),X,nullptr,AbilityNone,float(I%97)/97.f);
                Check(Id==AbilityNone||(SpecFor(Id).Weight>0.f&&!(SpecFor(Id).Flags&OneShotFlag)),"the chooser never returns a chained follow-up or a one-shot trigger");
            }
        }
        C.Distance=NaN;Check(ChooseAbility(KitFor(Grunt),C,nullptr,0,.3f)==AbilityNone,"a non-finite distance chooses nothing");
        C.Distance=100.f;C.Now=NaN;Check(ChooseAbility(KitFor(Grunt),C,nullptr,0,.3f)==AbilityNone,"a non-finite clock chooses nothing");
        // A synthetic three-ability kit: all eligible, equal weights, and no immediate repeats.
        Kit K=KitFor(Grunt);K.Abilities[0]=AbilitySlash;K.Abilities[1]=AbilityShot;K.Abilities[2]=AbilityBlast;K.AttackRange=300.f;
        Context D;D.Distance=100.f;D.Now=5.f;
        int Hits[AbilityCount]={};
        for(int I=0;I<9000;++I) ++Hits[ChooseAbility(K,D,nullptr,AbilityNone,float(I)/9000.f)];
        Check(Hits[AbilitySlash]==3000&&Hits[AbilityShot]==3000&&Hits[AbilityBlast]==3000,"equal weights split a sweep of rolls into equal thirds");
        for(int Last:{int(AbilitySlash),int(AbilityShot),int(AbilityBlast)}) {
            int After[AbilityCount]={};
            for(int I=0;I<9000;++I) ++After[ChooseAbility(K,D,nullptr,Last,float(I)/9000.f)];
            Check(After[Last]==0,"the ability just used is never chosen twice running when others are eligible");
            int Total=0;for(int Id=0;Id<AbilityCount;++Id) Total+=After[Id];Check(Total==9000,"every roll still chooses something");
        }
        K.Abilities[1]=AbilityNone;K.Abilities[2]=AbilityNone;
        Check(ChooseAbility(K,D,nullptr,AbilitySlash,.5f)==AbilitySlash,"a lone ability repeats when there is nothing else");
        Check(ChooseAbility(K,D,nullptr,AbilityNone,NaN)==AbilitySlash&&ChooseAbility(K,D,nullptr,AbilityNone,-5.f)==AbilitySlash&&ChooseAbility(K,D,nullptr,AbilityNone,5.f)==AbilitySlash,"a non-finite or out-of-range roll still gives a valid answer");
    }
    // ---- Unsticking ----
    {
        for(unsigned Id=0;Id<50u;++Id) for(int Seconds=0;Seconds<12;++Seconds) {
            float X,Y;UnstickDirection(1.f,0.f,float(Seconds),Id,X,Y);
            const float Length=std::sqrt(X*X+Y*Y),Degrees=std::acos(std::clamp(X,-1.f,1.f))*57.2957795f;
            Check(Near(Length,1.f,.002f),"an unstick direction is a unit vector");
            Check(Near(Degrees,75.f,.2f)||Near(Degrees,90.f,.2f),"... 75 or 90 degrees off the wanted direction");
        }
        float X1,Y1,X2,Y2;UnstickDirection(1.f,0.f,1.f,2u,X1,Y1);UnstickDirection(1.f,0.f,2.f,2u,X2,Y2);
        Check(Y1*Y2<0.f,"the side alternates as the stuck time grows");
        float Xn,Yn;UnstickDirection(NaN,0.f,1.f,0u,Xn,Yn);Check(Xn==0.f&&Yn==0.f,"no wanted direction, no unstick direction");
        UnstickDirection(0.f,0.f,1.f,0u,Xn,Yn);Check(Xn==0.f&&Yn==0.f,"a zero wanted direction gives a zero result");
        UnstickDirection(1.f,0.f,NaN,0u,Xn,Yn);Check(Xn==0.f&&Yn==0.f,"a non-finite stuck time gives a zero result");
        UnstickDirection(1.f,0.f,1e30f,3u,Xn,Yn);Check(std::isfinite(Xn)&&std::isfinite(Yn),"a huge stuck time is clamped");
    }
    // ---- ChooseClearDirection ----
    {
        float X,Y;
        Check(ChooseClearDirection(1.f,0.f,[](float,float) {return true;},X,Y)&&Near(X,1.f)&&Near(Y,0.f),"open ground: straight ahead");
        Check(ChooseClearDirection(1.f,0.f,[](float Dx,float) {return Dx<.5f;},X,Y)&&Near(X,std::cos(80.f*0.017453292f),.005f)&&Near(Y,std::sin(80.f*0.017453292f),.005f),"a wall ahead: the nearest clear fan direction (80 degrees, after 0 and 40 either way are blocked)");
        Check(!ChooseClearDirection(1.f,0.f,[](float,float) {return false;},X,Y)&&X==0.f&&Y==0.f,"enclosed: nothing is chosen");
        Check(!ChooseClearDirection(NaN,0.f,[](float,float) {return true;},X,Y)&&!ChooseClearDirection(0.f,0.f,[](float,float) {return true;},X,Y),"no wanted direction: nothing is chosen");
        int Calls=0;Check(ChooseClearDirection(0.f,1.f,[&Calls](float Dx,float Dy) {++Calls;Check(Near(Dx*Dx+Dy*Dy,1.f,.002f),"the probe is always a unit direction");return false;},X,Y)==false&&Calls==7,"at most seven probes are made");
    }
    std::printf("PASS: %lld enemy foundation checks (kits equal the old numbers, ability table and chooser, movement planning over 200000 random states, kiting/circling/retreat simulations, pack spreading, one-time retreat, unsticking).\nShared rules only; Unreal movement, navigation and gameplay are not executed.\n",Checks);
    return 0;
}
