// Native tests for 0.25 phase E: the heroes' footstep rules (ASFoleyRules.h). Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASFoleyRules.h"
#include "../Source/AshenSpiral/Public/ASEnemyRules.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
using namespace ASFoley;
int main() {
    const float Pi=TwoPi*.5f,Nan=std::numeric_limits<float>::quiet_NaN(),Inf=std::numeric_limits<float>::infinity();
    // ---- Steps: one per half stride, never a burst ----
    Check(StepsCrossed(0.f,.1f)==0&&StepsCrossed(.5f,Pi-.01f)==0,"no step inside a half stride");
    Check(StepsCrossed(Pi-.01f,Pi+.01f)==1&&StepsCrossed(TwoPi-.01f,TwoPi+.01f)==1,"a step on each multiple of pi");
    Check(StepsCrossed(0.f,TwoPi*10.f)==2,"a very long frame plays at most two steps");
    Check(StepsCrossed(3.f,2.f)==0&&StepsCrossed(2.f,2.f)==0,"running backwards or standing still plays nothing");
    Check(StepsCrossed(Nan,1.f)==0&&StepsCrossed(0.f,Nan)==0&&StepsCrossed(0.f,Inf)==0&&StepsCrossed(-Inf,0.f)==0,"bad phases play nothing");
    {int Total=0;float Phase=0.f;for(int I=0;I<10000;++I) {const float Next=Phase+.05f;Total+=StepsCrossed(Phase,Next);Phase=Next;}
     const int Expected=int(std::floor(Phase/Pi));Check(Total==Expected,"over a long run every half stride sounds exactly once");}
    // ---- When a step is heard ----
    Check(Audible(false,false,false,500.f)&&Audible(false,false,false,MinStepSpeed),"a hero running on the ground is heard");
    Check(!Audible(true,false,false,500.f)&&!Audible(false,true,false,500.f)&&!Audible(false,false,true,500.f)&&!Audible(false,false,false,MinStepSpeed-1.f),"airborne, dashing, downed or crawling heroes make no step");
    Check(!Audible(false,false,false,Nan)&&!Audible(false,false,false,-5.f),"a bad speed is silent");
    // ---- Variants: never the same twice, always in range, and all of them get used ----
    {std::set<int> Seen;int Last=-1;std::uint32_t R=7u;
     for(int I=0;I<20000;++I) {R=R*1664525u+1013904223u;const int V=NextVariant(Last,R>>8);Check(V>=0&&V<StepVariants,"a variant is a real sound");Check(V!=Last,"the same step never plays twice in a row");Seen.insert(V);Last=V;}
     Check(int(Seen.size())==StepVariants,"every variant gets used");}
    Check(NextVariant(-1,0u)>=0&&NextVariant(-1,0xFFFFFFFFu)<StepVariants&&NextVariant(99,5u)<StepVariants&&NextVariant(-7,5u)>=0,"first steps and out-of-range lasts are safe");
    // ---- Volume ----
    Check(StepGain(0.f,640.f)>.99f&&StepGain(0.f,640.f)<=1.001f,"your own full-speed step is at full gain");
    Check(StepGain(0.f,MinStepSpeed)<StepGain(0.f,640.f)&&StepGain(0.f,MinStepSpeed)>=.59f,"a slow step is quieter, never silent");
    Check(StepGain(1500.f,640.f)<StepGain(600.f,640.f)&&StepGain(3300.f,640.f)==0.f&&StepGain(9000.f,640.f)==0.f,"steps fall with distance and vanish beyond 33 m");
    Check(StepGain(Nan,640.f)==0.f&&StepGain(100.f,Nan)==0.f&&StepGain(Inf,640.f)==0.f,"bad inputs are silent");
    for(float D=0.f;D<5000.f;D+=25.f) for(float S=0.f;S<900.f;S+=30.f) {const float G=StepGain(D,S);Check(std::isfinite(G)&&G>=0.f&&G<=1.001f,"gain stays in range");}
    Check(std::fabs(StepGain(1234.f,640.f)-ASEnemyRules::CueDistanceGain(1234.f))<.001f,"the distance falloff is the enemy cues' falloff");
    // ---- The constants make sense ----
    Check(StepVolume>.05f&&StepVolume<.4f&&StepPitchLow<StepPitchHigh&&StepPitchLow>.5f&&StepPitchHigh<1.2f,"steps are quiet, and the pitch range is sane");
    // ---- 0.24.2: walk / run / strafe / backpedal sets, quieter, natural pitch, landings ----
    Check(SetFor(250.f,1.f)==SetWalk&&SetFor(600.f,1.f)==SetRun&&SetFor(600.f,0.f)==SetStrafe&&SetFor(250.f,.1f)==SetStrafe&&SetFor(600.f,-1.f)==SetBack&&SetFor(250.f,-1.f)==SetWalk,"the set follows speed and the motion against the facing");
    Check(SetFor(Nan,1.f)==SetRun&&SetFor(600.f,Nan)==SetRun,"bad inputs fall back to the running set");
    Check(StepVolume<=.12f&&WalkVolume<StepVolume&&StepPitchLow>=.9f&&StepPitchHigh<=1.1f,"0.24.2: quieter than before (.16), walking quieter still, pitched as recorded");
    for(int S=0;S<SetCount;++S) {
        Check(SetVariants[S]>=2&&SetPrefix[S][0]=='S',"every set has clips and a name");
        int Last=-1;std::uint32_t R=99u+unsigned(S);
        for(int I=0;I<4000;++I) {R=R*1664525u+1013904223u;const int V=NextVariantOf(SetVariants[S],Last,R>>8);Check(V>=0&&V<SetVariants[S]&&V!=Last,"a set's variants rotate without repeats");Last=V;}
    }
    Check(LandGain(100.f)==0.f&&LandGain(LandMinFall)>0.f&&LandGain(2000.f)==1.f&&LandGain(900.f)>LandGain(500.f)&&LandGain(Nan)==0.f,"landings are silent when soft, louder from higher, capped");
    // ---- 0.26 G6: steps from the feet. A foot bone's height above the capsule bottom: a standing foot is 6-9 cm up, a stride lifts it to 20-35 cm. ----
    Check(LiftHeight>9.f&&StrikeHeight>=9.f&&StrikeHeight<LiftHeight&&LiftHeight<20.f&&FootCooldown>0.f&&FootCooldown<.4f&&LandingQuiet>0.f,"the strike thresholds sit between a standing foot and a stride's lift, with a hysteresis");
    {FootState Foot;int Steps=0;for(float Height:{7.f,8.f,6.f,9.f,7.f}) Steps+=FootStruck(Foot,Height,float(Steps));Check(Steps==0,"a foot that stays planted never steps (standing, idle shifts)");}
    {FootState Foot;Check(!FootStruck(Foot,20.f,1.f)&&!FootStruck(Foot,25.f,1.1f)&&FootStruck(Foot,8.f,1.4f),"a foot that lifts and comes down steps once, when it lands");
     Check(!FootStruck(Foot,8.f,1.5f)&&!FootStruck(Foot,9.f,1.6f),"and not again while it stays down");}
    {FootState Foot;FootStruck(Foot,20.f,1.f);Check(!FootStruck(Foot,12.f,1.1f)&&FootStruck(Foot,9.5f,1.2f),"it steps when the bone falls to the strike height, not before");}
    {FootState Foot;FootStruck(Foot,20.f,1.f);int Steps=0;for(int I=0;I<200;++I) {const float H=11.f+(I%2?1.5f:-1.5f);Steps+=FootStruck(Foot,H,1.f+I*.002f);}Check(Steps<=1,"jitter round the strike height is one step, not a burst");}
    {FootState Foot;FootStruck(Foot,20.f,1.f);Check(FootStruck(Foot,8.f,1.f),"the first step after a lift sounds");FootStruck(Foot,20.f,1.05f);Check(!FootStruck(Foot,8.f,1.05f+FootCooldown*.5f),"a second landing inside the cooldown is silent");FootStruck(Foot,20.f,1.5f);Check(FootStruck(Foot,8.f,1.5f+FootCooldown),"and sounds once the cooldown is over");}
    {FootState Foot;Check(!FootStruck(Foot,Nan,1.f)&&!FootStruck(Foot,Inf,1.f)&&!FootStruck(Foot,5.f,Nan)&&!FootStruck(Foot,-Inf,1.f),"bad heights and times make no step");}
    {   // a simulated run: each foot's bone height is a lifted arc once per stride (0.36 s), the feet half a stride apart; every stride gives each foot exactly one step
        FootState Left,Right;int StepsLeft=0,StepsRight=0;const float Stride=.36f,Dt=1.f/60.f;
        for(int I=0;I<60*20;++I) {
            const float T=I*Dt;const auto Arc=[&](float Phase) {const float P=std::fmod(Phase,Stride)/Stride;return P<.5f?7.f+30.f*std::sin(P*2.f*Pi):7.f;}; // up and back down in the first half stride, planted in the second
            StepsLeft+=FootStruck(Left,Arc(T),T);StepsRight+=FootStruck(Right,Arc(T+Stride*.5f),T);
        }
        Check(StepsLeft>=54&&StepsLeft<=56&&StepsRight>=54&&StepsRight<=56,"over twenty seconds of running each foot steps once per stride (55 of 55.5)");
    }
    std::cout<<"PASS: "<<Checks<<" footstep checks; one step per half stride, audibility, variant rotation, distance gain, walk/run/strafe/backpedal sets, landings.\nShared rules only; the imported sounds, their level and how they sit in the mix are unverified.\n";
}
