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
    std::cout<<"PASS: "<<Checks<<" footstep checks; one step per half stride, audibility, variant rotation, distance gain.\nShared rules only; the imported sounds, their level and how they sit in the mix are unverified.\n";
}
