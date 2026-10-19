// Native tests for the engine-independent 0.24 enemy rules (ASEnemyRules.h). Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASEnemyRules.h"
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <limits>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
int main() {
    using namespace ASEnemyRules;
    // ---- Broodcaller summon rule ----
    Check(BroodMinionPair==2&&BroodMinionCap>=BroodMinionPair&&BroodMinionCapCoop>=BroodMinionCap,"brood pair/cap constants are consistent");
    Check(BroodShouldSummon(0,BroodMinionCap,10.f,0.f),"broodcaller summons when it has no brood and the cooldown is over");
    Check(!BroodShouldSummon(0,BroodMinionCap,5.f,6.f),"no summon while the cooldown is running");
    Check(BroodShouldSummon(0,BroodMinionCap,6.f,6.f),"cooldown boundary: summon is allowed exactly when it ends");
    Check(BroodShouldSummon(BroodMinionCap-BroodMinionPair,BroodMinionCap,99.f,0.f),"a pair fits when exactly two slots remain");
    Check(!BroodShouldSummon(BroodMinionCap-BroodMinionPair+1,BroodMinionCap,99.f,0.f),"a pair does not fit with one slot left");
    Check(!BroodShouldSummon(BroodMinionCap,BroodMinionCap,99.f,0.f),"no summon at the cap");
    Check(BroodShouldSummon(4,BroodMinionCapCoop,99.f,0.f)&&!BroodShouldSummon(5,BroodMinionCapCoop,99.f,0.f),"co-op cap of six admits one more pair at four");
    Check(BroodShouldSummon(0,0,99.f,0.f)&&!BroodShouldSummon(1,0,99.f,0.f),"a nonsensical cap still allows one pair when empty");
    Check(BroodShouldSummon(-3,BroodMinionCap,99.f,0.f),"a negative live count is treated as zero");
    const float NaN=std::numeric_limits<float>::quiet_NaN(),Inf=std::numeric_limits<float>::infinity();
    Check(!BroodShouldSummon(0,BroodMinionCap,NaN,0.f)&&!BroodShouldSummon(0,BroodMinionCap,10.f,NaN)&&!BroodShouldSummon(0,BroodMinionCap,Inf,0.f),"non-finite times never summon");
    // Regression for the 0.23 bug: the Broodcaller stopped summoning after its first two attacks. Simulate three minutes of fighting where players kill
    // one minion every 2.5 s; summoning must keep going for the whole fight and the live count must never exceed the cap.
    for(int Cap:{BroodMinionCap,BroodMinionCapCoop}) {
        int Live=0,Summons=0,MaxLive=0;float NextSummon=0.f,NextKill=2.5f;
        for(float Now=0.f;Now<180.f;Now+=1.9f) { // one attack every 1.9 s, as in ASEnemy::Tick
            while(NextKill<=Now) {if(Live>0) --Live;NextKill+=2.5f;}
            if(BroodShouldSummon(Live,Cap,Now,NextSummon)) {Live+=BroodMinionPair;++Summons;NextSummon=Now+BroodSummonCooldown;}
            if(Live>MaxLive) MaxLive=Live;
        }
        Check(Summons>=10,"the broodcaller keeps summoning through a long fight (the old rule stopped after two)");
        Check(MaxLive<=std::max(Cap,BroodMinionPair),"the living brood never exceeds the cap");
    }
    // The cooldown alone spaces summons at least BroodSummonCooldown apart.
    {float Last=-100.f;bool Spaced=true;float NextSummon=0.f;
        for(float Now=0.f;Now<600.f;Now+=.1f) if(BroodShouldSummon(0,BroodMinionCap,Now,NextSummon)) {if(Now-Last<BroodSummonCooldown-.001f) Spaced=false;Last=Now;NextSummon=Now+BroodSummonCooldown;}
        Check(Spaced,"summons are spaced by at least the cooldown");}
    std::printf("PASS: %lld enemy-rule checks (broodcaller summon cap, cooldown and long-fight regression).\nEngine-independent rules only; Unreal actors, animation, replication and gameplay are not executed.\n",Checks);
    return 0;
}
