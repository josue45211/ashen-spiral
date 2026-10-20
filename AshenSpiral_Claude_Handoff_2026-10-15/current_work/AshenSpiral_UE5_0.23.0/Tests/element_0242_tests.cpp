// Native tests for the 0.24.2 element rework (F4; playtest: "the hammer appears to get so little - buff elemental across the board to equal effective DPS to other meta builds").
// An element proc from a hit now carries a pool of damage proportional to the hit (ASRules::ElementShare), drained evenly over the element's time, and frost makes a body brittle.
// These tests hold the pool to exact delivery, keep the old flat parts unchanged, and check that an element build keeps pace with Chain Lightning and Impact Bloom on EVERY weapon,
// using the game's own on-hit scaling (ASArsenal) and proc chances (ASBuild). Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASArsenalRules.h"
#include "../Source/AshenSpiral/Public/ASBuildRules.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float Relative) {return std::fabs(A-B)<=Relative*std::max(1.f,std::fabs(B));}
static float Delivered(ASRules::ElementState& E,int Kind,float Seconds,float Dt) {
    float Total=0.f;
    for(float T=0.f;T<Seconds;T+=Dt) {const auto D=E.Tick(Dt);Total+=Kind==2?D.first:(Kind==3?D.second:(Kind==7?E.BleedDamage:E.FrostDamage));}
    return Total;
}
int main() {
    using namespace ASRules;
    const float NaN=std::numeric_limits<float>::quiet_NaN(),Inf=std::numeric_limits<float>::infinity();
    const int Kinds[]={2,3,7,4};
    // ---- the pool is delivered exactly, at any frame rate, through refreshes ----
    for(int Kind:Kinds) {
        for(float Dt:{1.f/30.f,1.f/60.f,1.f/144.f,.1f,.37f}) {
            ElementState E;E.Apply(Kind,0.f,false,1.f,1.f,500.f);
            Check(Near(Delivered(E,Kind,12.f,Dt),500.f,.002f),"a pool is delivered in full, whatever the frame time");
        }
        ElementState Twice;Twice.Apply(Kind,0.f,false,1.f,1.f,300.f);float Got=Delivered(Twice,Kind,1.f,1.f/60.f);Twice.Apply(Kind,0.f,false,1.f,1.f,200.f);Got+=Delivered(Twice,Kind,12.f,1.f/60.f);
        Check(Near(Got,500.f,.002f),"a second proc mid-way adds its pool; nothing is lost or delivered twice");
        ElementState Long;Long.Apply(Kind,0.f,false,1.4f,1.f,500.f);
        Check(Near(Delivered(Long,Kind,14.f,1.f/60.f),700.f,.002f),"Lingering Affliction stretches the time and grows the pool alike (damage per second is kept)");
        ElementState Bad;Bad.Apply(Kind,0.f,false,1.f,1.f,NaN);Bad.Apply(Kind,0.f,false,1.f,1.f,-50.f);Bad.Apply(Kind,0.f,false,1.f,1.f,Inf);
        const float BadTotal=Delivered(Bad,Kind,12.f,1.f/60.f);Check(std::isfinite(BadTotal)&&BadTotal<=1.e6f*1.8f+1.f,"bad pools deliver nothing absurd and never NaN");
        ElementState Rate;Rate.Apply(Kind,0.f,false,1.f,1.f,600.f);const float First=Delivered(Rate,Kind,.5f,1.f/60.f);
        Check(First>0.f&&First<600.f*.5f,"a pool is spread over the element's time, not dumped at once");
    }
    // ---- the old flat parts (fields, wells, auras) are untouched when no pool is passed ----
    {ElementState B;B.Apply(2,1.f,false);Check(Near(Delivered(B,2,5.f,1.f/60.f),18.f,.01f),"a plain burn is still 6 damage a second for 3 seconds");}
    {ElementState P;P.Apply(3,1.f,false);P.Apply(3,1.f,false);P.Apply(3,1.f,false);Check(Near(Delivered(P,3,6.f,1.f/60.f),36.f,.01f),"three plain poison stacks are still 3 x 3 a second for 4 seconds");}
    {ElementState F;F.Apply(4,1.f,false);F.Apply(4,1.f,false);F.Apply(4,1.f,false);Check(F.Frozen>0.f&&F.FrostPool==0.f&&F.Chill==0.f,"plain frost still freezes at the third application and carries no frostbite");}
    // ---- frost makes a body brittle ----
    Check(FrostVulnerability(0.f,0.f)==1.f&&Near(FrostVulnerability(30.f,0.f),1.f+ChillVulnerability,.0001f)&&Near(FrostVulnerability(70.f,.5f),1.f+FrozenVulnerability,.0001f),"chilled bodies take more damage, frozen ones more again");
    Check(FrostVulnerability(NaN,NaN)==1.f&&FrostVulnerability(-5.f,-1.f)==1.f&&FrozenVulnerability>ChillVulnerability,"bad numbers change nothing");
    {ElementState Boss;for(int I=0;I<40;++I) Boss.Apply(4,1.f,true,1.f,2.f);Check(Boss.Frozen==0.f&&FrostVulnerability(Boss.Frost,Boss.Frozen)==1.f+ChillVulnerability,"a boss never freezes, so it is only ever chilled");}
    // ---- parity: per hit, what a build adds as a fraction of the hit, on every weapon (ASArsenal's on-hit scale applied the way ASCharacter::HitProcs applies it) ----
    // Element trait rank r: chance ProcChance(r, .18) scaled per weapon (excess becomes magnitude), the pool is Share x the hit x magnitude.
    // Chain Lightning rank r with enough enemies near: r links at 30% x scale of the hit. Impact Bloom rank r with two neighbours: 2 x 15% x r x scale.
    for(int W=0;W<=8;++W) {
        const float Scale=ASArsenal::OnHitScale(W);
        for(int Rank=1;Rank<=3;++Rank) {
            const auto R=ASArsenal::ScaleReward(ASBuild::ProcChance(Rank,.18f),Scale);
            const float Chain=float(Rank)*.30f*Scale,Bloom=2.f*.15f*float(Rank)*Scale;
            for(int Kind:Kinds) {
                const float Element=R.Chance*R.Magnitude*ElementShare(Kind);
                Check(Element>=Chain*.8f&&Element<=Chain*1.35f,"an element trait adds about what Chain Lightning adds at the same rank, on every weapon");
                Check(Element>=Bloom*.8f&&Element<=Bloom*1.6f,"... and keeps pace with Impact Bloom among two neighbours");
                const float Old=Kind==4?0.f:R.Chance*(Kind==2?18.f:(Kind==3?12.f:12.f))/std::max(1.f,ASArsenal::Damage(W)); // the old flat DOT per proc, as a fraction of the hit
                Check(Element>=Old,"no element is worth less per proc than it was, even on the fastest weapon (whose tiny hits made the old flat DOT relatively big)");
                if(W==1&&Kind!=4) Check(Element>=Old*5.f,"on the Hoarfrost Maul (the playtest's complaint) an element proc is worth at least five times what it was");
            }
        }
        // The weapon's own element on every hit: its share of the hit, plus (frost) brittleness. The Hoarfrost Maul's frost used to add nothing.
        const float Own=OwnElementShare*Scale;Check(Own>0.f&&Own<=OwnElementShare*ASArsenal::OnHitScaleMax+1e-4f,"the weapon's own element adds a share of every hit");
    }
    {const float Maul=OwnElementShare*ASArsenal::OnHitScale(1)+ChillVulnerability;Check(Maul>=.4f,"the Hoarfrost Maul's own frost now adds at least 40% to its hits (frostbite plus brittleness)");}
    // ---- a simulated minute: the maul with Kindling Touch x3 against one target, the real ElementState ticking at 60 Hz, seeded procs ----
    for(int W:{0,1,7,8}) {
        std::mt19937 Rng(1234u+unsigned(W));std::uniform_real_distribution<float> U(0.f,1.f);
        const float Interval=ASArsenal::BaseInterval(W),Hit=ASArsenal::Damage(W),Scale=ASArsenal::OnHitScale(W);
        const auto R=ASArsenal::ScaleReward(ASBuild::ProcChance(3,.18f),Scale);
        ElementState E;float Direct=0.f,Dot=0.f,NextHit=0.f;
        for(float T=0.f;T<60.f;T+=1.f/60.f) {
            if(T>=NextHit) {NextHit+=Interval;Direct+=Hit;if(U(Rng)<R.Chance) E.Apply(2,1.f,false,1.f,R.Magnitude,Hit*BurnShare*R.Magnitude);}
            Dot+=E.Tick(1.f/60.f).first;
        }
        const float Expected=Direct*R.Chance*R.Magnitude*BurnShare;
        Check(Near(Dot-6.f*60.f,Expected,.2f)||Near(Dot,Expected,.2f),"a minute of burning procs delivers what the per-hit numbers promise (within 20% of chance noise)");
        Check(Dot>=Direct*.25f,"Kindling Touch x3 adds at least a quarter of the weapon's own damage on every weapon tested");
    }
    std::cout<<"PASS: "<<Checks<<" element checks; pools delivered exactly through refreshes and frame rates, flat parts unchanged, frost brittleness, element parity with Chain Lightning and Impact Bloom on all nine weapons, a simulated minute.\n";
    std::cout<<"Shared rules only; Unreal actors, replication and feel were not run.\n";
    return 0;
}
