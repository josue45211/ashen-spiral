// Native tests for the 0.24 on-hit reward scaling (ASArsenal::OnHitScale / ScaleReward / ConduitHitsNeeded). Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASArsenalRules.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.01f) {return std::fabs(A-B)<E;}
int main() {
    using namespace ASArsenal;
    const float NaN=std::numeric_limits<float>::quiet_NaN(),Inf=std::numeric_limits<float>::infinity();
    // ---- Base interval and scale per weapon: slower weapons reward more per hit ----
    Check(Near(BaseInterval(0),.28f*(2.f+1.25f)/3.f)&&Near(BaseInterval(1),.72f*(2.f+1.65f)/3.f)&&Near(BaseInterval(5),.40f),"melee base intervals include the combo's average step length");
    Check(Near(BaseInterval(2),.30f)&&Near(BaseInterval(3),.48f)&&Near(BaseInterval(4),.34f)&&Near(BaseInterval(6),.72f)&&Near(BaseInterval(7),.105f)&&Near(BaseInterval(8),1.05f),"ranged base intervals are the weapon table's");
    const float Expect[9]={.758f,2.19f,.75f,1.2f,.85f,1.0f,1.8f,OnHitScaleMin,OnHitScaleMax};
    for(int W=0;W<9;++W) Check(Near(OnHitScale(W),Expect[W],.012f),"per-weapon on-hit scale: Blade .76, Hammer 2.19, Bow .75, Glaive 1.2, Staff .85, Scythe 1, Scattergun 1.8, Rotary floor, Mortar cap");
    Check(OnHitScale(1)>OnHitScale(0)&&OnHitScale(8)>OnHitScale(6)&&OnHitScale(6)>OnHitScale(3)&&OnHitScale(3)>OnHitScale(2)&&OnHitScale(2)>OnHitScale(7),"slower weapons always get the larger per-hit scale");
    Check(OnHitScale(-5)==OnHitScale(0)&&OnHitScale(99)==OnHitScale(8),"an out-of-range weapon id is clamped, never undefined");
    for(int W=0;W<9;++W) Check(OnHitScale(W)>=OnHitScaleMin&&OnHitScale(W)<=OnHitScaleMax,"scale is always inside its limits");
    // ---- Rewards per second are comparable across weapons (that is the point) ----
    float Low=1e9f,High=0.f;
    for(int W=0;W<9;++W) {
        const float PerSecond=OnHitScale(W)/BaseInterval(W); // base swings per second x per-hit scale
        Low=std::min(Low,PerSecond);High=std::max(High,PerSecond);
        const bool Clamped=OnHitScale(W)<=OnHitScaleMin+1e-6f||OnHitScale(W)>=OnHitScaleMax-1e-6f;
        if(!Clamped) Check(Near(PerSecond,1.f/OnHitReferenceInterval,.01f),"unclamped weapons give exactly the same reward rate per second");
    }
    Check(High/Low<1.2f,"even with the floor and cap, no weapon's reward rate is more than 1.2x another's");
    // Attack speed (a live rate) must not enter the scale: the scale takes only the weapon id, and more swings per second means proportionally more rewards.
    for(int W=0;W<9;++W) Check(Near(OnHitScale(W)*(1.5f/BaseInterval(W)),1.5f*OnHitScale(W)/BaseInterval(W),.001f),"a faster live rate multiplies the number of rewards, not their size");

    // ---- ScaleReward keeps the expected value exact ----
    {const auto R=ScaleReward(.18f,.5f);Check(Near(R.Chance,.09f,.0001f)&&R.Magnitude==1.f,"under 1: a smaller chance at full strength");}
    {const auto R=ScaleReward(1.f,.4f);Check(Near(R.Chance,.4f,.0001f)&&R.Magnitude==1.f,"a certain reward becomes a 40% chance at full strength on a fast weapon");}
    {const auto R=ScaleReward(1.f,2.f);Check(R.Chance==1.f&&Near(R.Magnitude,2.f,.0001f),"a certain reward on a slow weapon stays certain and doubles");}
    {const auto R=ScaleReward(.5f,4.f);Check(R.Chance==1.f&&Near(R.Magnitude,2.f,.0001f),"a probable reward past certainty turns the excess into strength");}
    {const auto R=ScaleReward(0.f,2.f);Check(R.Chance==0.f&&R.Magnitude==1.f,"a zero reward stays zero");}
    {const auto R=ScaleReward(NaN,NaN);Check(std::isfinite(R.Chance)&&std::isfinite(R.Magnitude),"non-finite inputs give finite outputs");}
    {const auto R=ScaleReward(Inf,-3.f);Check(R.Chance>=0.f&&R.Chance<=1.f&&R.Magnitude>=1.f,"absurd inputs are clamped");}
    unsigned Seed=24;auto Rand=[&Seed]() {Seed=Seed*1664525u+1013904223u;return float((Seed>>8)&0xFFFF)/65535.f;};
    for(int I=0;I<20000;++I) {
        const float P=Rand(),S=.2f+Rand()*3.f;const auto R=ScaleReward(P,S);
        Check(R.Chance>=0.f&&R.Chance<=1.f&&R.Magnitude>=1.f,"chance is a probability and magnitude never shrinks a reward");
        Check(Near(R.Chance*R.Magnitude,P*S,.0005f),"expected reward equals base chance x scale exactly");
    }
    // ---- Conduit: fewer hits needed for slow weapons ----
    Check(ConduitHitsNeeded(1.f)==6,"a scale of 1 keeps the old six hits");
    Check(ConduitHitsNeeded(OnHitScaleMin)>=15&&ConduitHitsNeeded(OnHitScaleMin)<=24,"the fastest weapon needs many more hits");
    Check(ConduitHitsNeeded(OnHitScaleMax)>=2&&ConduitHitsNeeded(OnHitScaleMax)<=3,"the slowest weapon needs only two or three");
    for(float S=.1f;S<5.f;S+=.05f) Check(ConduitHitsNeeded(S)>=1&&ConduitHitsNeeded(S)<=24&&ConduitHitsNeeded(S+.05f)<=ConduitHitsNeeded(S),"conduit hits are bounded and never increase with the scale");
    Check(ConduitHitsNeeded(NaN)==6&&ConduitHitsNeeded(0.f)==24,"bad scales fall back sensibly");
    std::printf("PASS: %lld on-hit scaling checks (per-weapon scale, equal reward rate, exact expected value over 20000 random rewards, conduit hits).\nShared rules only; Unreal combat, replication and gameplay are not executed.\n",Checks);
    return 0;
}
