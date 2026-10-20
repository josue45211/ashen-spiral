// Native tests for the engine-independent 0.23 expedition rules. Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASExpeditionRules.h"
#include <cstdlib>
#include <limits>
#include <iostream>
#include <string>
#include <cstdio>
static int Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.0005f) {return std::abs(A-B)<E;}
int main() {
    using namespace ASExpedition;
    Check(Near(EnemyHealthScale(0),1.f)&&Near(EnemyHealthScale(1),1.5f)&&Near(EnemyHealthScale(2),2.1f),"normal-run enemy health scale 1/1.5/2.1");
    Check(Near(EnemyDamageScale(0),1.f)&&Near(EnemyDamageScale(1),1.3f)&&Near(EnemyDamageScale(2),1.6f),"normal-run enemy damage scale 1/1.3/1.6");
    Check(Difficulty(-5)==0&&Difficulty(9)==2,"difficulty is clamped to 0..2");
    Check(Near(Quality(0),1.f)&&Near(Quality(1),1.25f)&&Near(Quality(2),1.5f),"drop quality 0/+25/+50%");
    Check(EnemyHealthScale(0,1)==1.f,"endless wave 1 has no scaling above baseline");
    Check(EnemyHealthScale(0,10,2)>EnemyHealthScale(0,10,1)&&EnemyHealthScale(0,10,1)>EnemyHealthScale(0,10,0),"boss tier scales faster than elite than normal");
    for(int W=2;W<200;++W) Check(EnemyHealthScale(0,W)>=EnemyHealthScale(0,W-1)&&EnemyDamageScale(0,W)>=EnemyDamageScale(0,W-1),"endless scaling is monotonic");
    Check(RewardWave(1)&&RewardWave(20)&&!RewardWave(21)&&RewardWave(22)&&RewardWave(50)&&!RewardWave(51)&&!RewardWave(52)&&RewardWave(53)&&RewardWave(56)&&RewardWave(59)&&RewardWave(62),"reward cadence 1-20, even to 50, then every third from 50");
    Check(BossWave(60)&&RewardWave(60)&&!RewardWave(61)&&BossWave(10),"boss waves always reward and recur every ten");
    Check(SkyStage(1)==0&&SkyStage(20)==0&&SkyStage(21)==1&&SkyStage(49)==1&&SkyStage(50)==2,"sky stages clear/gloom/red at 21 and 50");
    Check(AngerSpeed(49)==1.f&&AngerSpeed(50)>1.1f&&AngerSpeed(5000)<=1.3f,"anger speed starts after 49 and is capped");
    for(int W=0;W<500;++W) Check(EnemyCount(W,1)<=36&&EnemyCount(W,2)<=36&&EnemyCount(W,1)>=8,"active enemy count is bounded");
    Check(ChoiceCount(0)==3&&ChoiceCount(1)==4&&ChoiceCount(2)==5&&ChoiceCount(99)==5&&ChoiceCost(0)==150&&ChoiceCost(1)==750&&ChoiceCost(2)<0,"choice count and rank costs");
    Check(Near(TraitPower(3,3),3.f)&&TraitPower(8,3)<8.f&&TraitPower(9,3)>TraitPower(8,3),"trait power: full to cap, diminishing after");
    for(int P=1;P<400;++P) Check(TraitPower(P,3)>TraitPower(P-1,3),"trait power always increases with picks");
    Check(Near(Bleed(10,1,true),10.f)&&Near(Bleed(10,1,false),9.f)&&Bleed(.2f,1,false)==0.f,"bleed pauses while reviving and floors at zero");
    float Sum=0;for(unsigned R=0;R<11;++R) Sum+=ShuffleWaves(R);Check(Near(Sum/11.f,4.f),"shuffle wave interval averages four");
    for(unsigned R=0;R<11;++R) Check(ShuffleWaves(R)>=1&&ShuffleWaves(R)<=7,"shuffle interval within 1..7");
    Check(Near(ShuffleSeconds(0),120.f)&&Near(ShuffleSeconds(1),180.f)&&Near(ShuffleSeconds(9),180.f),"time shuffle 120..180s");
    Check(!ShopAppears(9,0.f)&&ShopAppears(10,.1f)&&!ShopAppears(10,.2f),"lucky shop never before wave ten, 15% roll");
    for(int Ash:{0,50,51,500,999,1000,5000}) {
        const auto O=Odds(Ash);Check(O.Loss+O.Refund+O.Double+O.Five+O.Ten==100,"gamble odds sum to 100");
        int Counts[11]={0};for(int R=0;R<100;++R) ++Counts[GambleMultiplier(R,Ash)];
        Check(Counts[0]==O.Loss&&Counts[1]==O.Refund&&Counts[2]==O.Double&&Counts[5]==O.Five&&Counts[10]==O.Ten,"gamble roll buckets match published odds");
    }
    // Expected return per stake: the middle must be the worst bet (U-shaped appetite).
    auto EV=[](int Ash){const auto O=Odds(Ash);return (O.Refund*1+O.Double*2+O.Five*5+O.Ten*10)/100.f;};
    Check(EV(500)<EV(10)&&EV(500)<EV(2000),"middle-wealth gamble has the lowest expected return");
    Check(Synergy(2,8)==0&&Synergy(8,2)==0&&Synergy(4,8)==1&&Synergy(2,3)==2&&Synergy(2,4)==-1&&Synergy(2,2)==-1&&Synergy(0,8)==-1,"synergy pairs; same element and physical never trigger");
    Check(SynergyDamage(0,1000,1,true)>SynergyDamage(0,1000,1,false),"synergy deals bonus boss-max-HP damage to bosses");
    Check(SynergyDamage(2,1000,0,false)==50.f,"synergy power floors at 1");
    Check(DifficultyUnlocked(0,0)&&!DifficultyUnlocked(1,0)&&!DifficultyUnlocked(2,0),"fresh character: only Normal is open");
    Check(DifficultyUnlocked(1,1)&&!DifficultyUnlocked(2,1),"beating Normal unlocks Hard only");
    Check(DifficultyUnlocked(2,3)&&DifficultyUnlocked(2,2),"beating Hard unlocks Very Hard");
    Check(!DifficultyUnlocked(2,4)&&!DifficultyUnlocked(1,6),"unlocks require the previous tier, not just any clear");
    Check(ClearedMask(1,0)==1&&ClearedMask(0,0)==0&&ClearedMask(0,0xFF)==7,"older saves with wins count as Normal-beaten; mask is sanitized");
    Check(EndlessUnlocked(1)&&!EndlessUnlocked(0)&&!EndlessUnlocked(6),"Endless needs a Normal clear");
    Check(GambleStake(9,0)==0&&GambleStake(0,2)==0,"below the minimum there is no stake");
    Check(GambleStake(10,0)==10&&GambleStake(40,0)==10&&GambleStake(100,0)==25&&GambleStake(100,1)==50&&GambleStake(100,2)==100,"stake is derived from the index: 25%/50%/all, never below the minimum");
    for(int C=0;C<3000;C+=7) for(int I=-2;I<6;++I) Check(GambleStake(C,I)>=0&&GambleStake(C,I)<=C,"a stake can never exceed the balance, even for out-of-range indices");
    Check(GambleBalance(100,25,0)==75&&GambleBalance(100,25,1)==100&&GambleBalance(100,25,2)==125&&GambleBalance(100,100,10)==1000,"balance after a spin: loss, refund, x2, x10");
    Check(GambleBalance(100,25,-3)==75&&GambleBalance(100,500,2)==200&&GambleBalance(0,0,5)==0,"negative multipliers cannot mint money");
    Check(CanBuyShopItem(0,60,0,0,false)&&!CanBuyShopItem(0,59,0,0,false),"shop item needs exact Scrap");
    Check(!CanBuyShopItem(0,500,0,1,false),"each shop item can be bought once per shop");
    Check(!CanBuyShopItem(3,999,30,0,false)&&CanBuyShopItem(3,0,30,0,true)&&!CanBuyShopItem(3,0,29,0,true),"rare Ash stock only when the shop rolled rare, at exact Ash");
    Check(!CanBuyShopItem(-1,999,999,0,true)&&!CanBuyShopItem(ShopCount,999,999,0,true),"out-of-range shop indices are refused");
    Check(ShopStock(ShopPickIndex).Scrap>=ShopStock(0).Scrap*2&&!ShopStock(ShopPickIndex).Rare,"Trader's Pick is the premium Scrap item (high price) and not rare-gated");
    for(int I=0;I<ShopCount;++I) Check((ShopStock(I).Scrap>0)!=(ShopStock(I).Ash>0),"every shop item costs exactly one currency");
    Check(BloodrootGain(0)==0.f&&BloodrootGain(-4)==0.f&&Near(BloodrootGain(2),.7f),"Bloodroot gain is .35 HP per effective rank and never negative");
    Check(KillScrap(false,false)==1&&KillScrap(true,false)==5&&KillScrap(true,true)==25&&CheckpointAsh(10)==10&&CheckpointAsh(7)==2&&CheckpointAsh(0)==0,"Endless earnings table");
    // 0.23.1 looks: sky stages darken then redden, the wave-50 step is fast, portal text matches the numbers.
    Check(SkyLookFor(0).SunIntensity==4.5f&&SkyLookFor(0).Vignette==.20f&&SkyLookFor(0).FogDensity==.018f,"clear stage equals the existing lighting rig");
    Check(SkyLookFor(1).SunIntensity<SkyLookFor(0).SunIntensity&&SkyLookFor(1).Saturation<SkyLookFor(0).Saturation&&SkyLookFor(1).FogDensity>SkyLookFor(0).FogDensity,"gloomy stage is darker, greyer and foggier than clear");
    Check(SkyLookFor(2).SunG<.25f&&SkyLookFor(2).FogR>SkyLookFor(2).FogG*5.f&&SkyLookFor(2).GainR>SkyLookFor(2).GainG*1.5f,"red stage is unmistakably red in sun, fog and grade");
    Check(SkyLookFor(-3).SunIntensity==SkyLookFor(0).SunIntensity&&SkyLookFor(9).SunIntensity==SkyLookFor(2).SunIntensity,"sky stage index is clamped");
    // 0.24.2 aesthetics: the clouds thicken from clear to gloom, storms only from the gloom on, the red stage burns hardest
    Check(SkyLookFor(0).Storm==0.f&&SkyLookFor(1).Storm>0.f&&SkyLookFor(2).Storm>=SkyLookFor(1).Storm,"storm clouds only after the clear stage, strongest in the red");
    Check(SkyLookFor(1).CloudCover>SkyLookFor(0).CloudCover&&SkyLookFor(2).CloudCover>SkyLookFor(0).CloudCover,"the later stages are more overcast than the clear sky");
    for(int Stage=0;Stage<3;++Stage) Check(SkyLookFor(Stage).CloudDensity>0.f&&SkyLookFor(Stage).CloudDensity<.05f&&SkyLookFor(Stage).CloudCover>-1.f&&SkyLookFor(Stage).CloudCover<1.f,"cloud values stay in the material's range");
    {const auto Mid=LerpSky(SkyLookFor(0),SkyLookFor(2),.5f);Check(Near(Mid.Storm,.5f)&&Near(Mid.CloudCover,0.f)&&Near(Mid.GainB,(1.f+.58f)*.5f),"the cloud fields (the last members) interpolate like the rest");}
    Check(EndlessSkyStage(true,true,20)==0&&EndlessSkyStage(true,true,21)==1&&EndlessSkyStage(true,true,49)==1&&EndlessSkyStage(true,true,50)==2&&EndlessSkyStage(true,true,90)==2,"stage thresholds 21 and 50");
    Check(EndlessSkyStage(false,true,90)==0&&EndlessSkyStage(true,false,90)==0,"normal runs and the hub always keep the clear sky");
    Check(SkyBlendRate(1,2)>SkyBlendRate(0,1)*3.f&&SkyBlendRate(0,1)==SkyBlendRate(1,0),"the step into red is fast; other changes drift slowly");
    {const auto Mid=LerpSky(SkyLookFor(0),SkyLookFor(2),.5f);Check(Near(Mid.SunIntensity,(4.5f+2.4f)*.5f)&&Near(LerpSky(SkyLookFor(0),SkyLookFor(2),-1).SunIntensity,4.5f)&&Near(LerpSky(SkyLookFor(0),SkyLookFor(2),2).SunIntensity,2.4f),"sky look interpolation is clamped and linear");}
    for(int D=0;D<3;++D) {
        char Buffer[96];std::snprintf(Buffer,sizeof Buffer,"Enemy health x%.1f, damage x%.1f, reward quality +%d%%",EnemyHealthScale(D),EnemyDamageScale(D),int(std::lround((Quality(D)-1.f)*100.f)));
        Check(std::string(PortalModifiers(D))==Buffer,"portal tooltip text matches the live scaling numbers");
    }
    Check(std::string(PortalName(0))=="EMBER"&&std::string(PortalName(1))=="ASHEN"&&std::string(PortalName(2))=="BLOOD","portals are Ember, Ashen, Blood");
    Check(std::string(UnlockCondition(0)).empty()&&!std::string(UnlockCondition(1)).empty()&&!std::string(UnlockCondition(2)).empty(),"locked portals state their unlock condition; Ember has none");
    // 0.23.2 run reward log: every shop purchase and chest reward has a code, a name and a line; Heart of the Spiral stacks.
    for(int I=0;I<ShopCount;++I) Check(RunGainValid(I)&&std::string(RunGainName(I))==ShopStock(I).Name,"every shop item maps to a run-gain code with its shop name");
    Check(RunGainValid(RunGainChestHeal)&&RunGainValid(RunGainChestKit)&&RunGainValid(RunGainRelic)&&RunGainValid(RunGainStyle)&&RunGainValid(RunGainCinders)&&RunGainValid(RunGainAsh),"chest and checkpoint rewards have run-gain codes");
    Check(!RunGainValid(-1)&&!RunGainValid(6)&&!RunGainValid(7)&&!RunGainValid(RunGainCount),"invalid run-gain codes are rejected");
    Check(RunGainLine(4,1)=="Heart of the Spiral  x1"&&RunGainLine(4,3)=="Heart of the Spiral  x3"&&RunGainDetail(4,3)=="+75 maximum health this run (25 each).","Heart of the Spiral stacks and the detail shows the total");
    Check(RunGainLine(0,0).empty()&&RunGainLine(99,2).empty()&&RunGainLine(0,2)=="Field Dressing  x2"&&RunGainDetail(0,0).empty()&&RunGainDetail(99,1).empty(),"run-gain lines skip empty and invalid entries");
    for(int I=0;I<RunGainCount;++I) if(RunGainValid(I)) Check(RunGainLine(I,1)==AcquisitionTitle(RunGainName(I),1)&&!RunGainDetail(I,1).empty(),"every run gain uses the uniform title plus a detail line");
    Check(AcquisitionTitle("Attack Speed",2)=="Attack Speed  x2"&&AcquisitionTitle("X",0)=="X  x1","uniform acquisition title format");
    // 0.23.5 arena shift v3 (slower): 1s banner only, then ShiftSinkSeconds of sinking, swap, then ShiftRiseSeconds of rising; ease-in (quadratic).
    Check(ShiftQuietSeconds==1.f&&ShiftWarningSeconds==ShiftQuietSeconds+ShiftSinkSeconds&&ShiftBannerSeconds==ShiftWarningSeconds+ShiftRiseSeconds&&ShiftSinkSeconds>=1.5f&&ShiftRiseSeconds>=1.5f,"timeline: 1s quiet, then slow sink, swap, slow rise (was 1s each)");
    Check(ShiftCoverSunk(0.f,0.f)==0.f&&ShiftCoverSunk(5.f,-100.f)==0.f&&ShiftCoverSunk(1.f,100.f)==0.f&&ShiftCoverSunk(100.f,96.5f)==0.f,"no scheduled shift or far from it: cover stays up");
    Check(ShiftCoverSunk(100.f-ShiftSinkSeconds-.5f,100.f)==0.f&&ShiftCoverSunk(100.f-ShiftSinkSeconds,100.f)<.001f&&Near(ShiftCoverSunk(100.f-ShiftSinkSeconds*.5f,100.f),.25f)&&Near(ShiftCoverSunk(100.f,100.f),1.f),"sinking happens only in the sink window before the swap and ends fully sunk");
    Check(ShiftCoverSunk(100.f-ShiftSinkSeconds*.9f,100.f)<.02f+.001f&&ShiftCoverSunk(100.f-ShiftSinkSeconds*.1f,100.f)>.8f,"sink starts slowly and is fast at the end (slam flush)");
    Check(ShiftCoverSunk(100.f+ShiftRiseSeconds*.1f,100.f)>.98f&&ShiftCoverSunk(100.f+ShiftRiseSeconds*.9f,100.f)<.2f&&Near(ShiftCoverSunk(100.f+ShiftRiseSeconds,100.f),0.f)&&ShiftCoverSunk(150.f,100.f)==0.f,"rise starts slowly and is fast at the end (slam into place), then stays up");
    {const float D=ShiftSinkSeconds*.1f;Check((ShiftCoverSunk(100.f-.1f*D,100.f)-ShiftCoverSunk(100.f-1.1f*D,100.f))>(ShiftCoverSunk(100.f-ShiftSinkSeconds+1.1f*D,100.f)-ShiftCoverSunk(100.f-ShiftSinkSeconds+.1f*D,100.f))*3.f,"sinking accelerates (the last tenth moves at least 3x the first)");
     const float R=ShiftRiseSeconds*.1f;Check((ShiftCoverSunk(100.f+.1f*R,100.f)-ShiftCoverSunk(100.f+1.1f*R,100.f))<(ShiftCoverSunk(100.f+ShiftRiseSeconds-1.1f*R,100.f)-ShiftCoverSunk(100.f+ShiftRiseSeconds-.1f*R,100.f))/3.f,"rising accelerates (the last tenth moves at least 3x the first)");}
    Check(Near(ShiftCoverSunk(99.9999f,100.f),ShiftCoverSunk(100.0001f,100.f),.001f),"the swap from old to new cover has no jump");
    for(float T=-4.f;T<4.f;T+=.01f) Check(ShiftCoverSunk(100.f+T,100.f)>=0.f&&ShiftCoverSunk(100.f+T,100.f)<=1.f,"sunk fraction stays within 0..1");
    Check(ShiftCoverSolid(95.f,100.f)&&ShiftCoverSolid(100.f-ShiftSinkSeconds,100.f)&&!ShiftCoverSolid(99.9f,100.f)&&!ShiftCoverSolid(100.f,100.f)&&!ShiftCoverSolid(100.f+ShiftRiseSeconds*.5f,100.f)&&ShiftCoverSolid(100.f+ShiftRiseSeconds*.95f,100.f)&&ShiftCoverSolid(120.f,100.f)&&ShiftCoverSolid(5.f,-100.f),"collision is off while the blocks are sunk or half-risen and on once they are mostly up");
    // Collision may turn on only when at least ~70% of the wall is above ground (the tallest wall is 380 and the drop is 450).
    Check(ShiftCoverDrop*ShiftSolidBelow<=380.f*.3f,"collision turns on only when the wall is mostly above ground");
    Check(CircleHitsFootprint(0,0,50,0,0,400,130,0)&&!CircleHitsFootprint(400,0,50,0,0,400,130,0)&&CircleHitsFootprint(0,100,50,0,0,400,130,0)&&!CircleHitsFootprint(0,100,30,0,0,400,130,0)&&CircleHitsFootprint(0,150,50,0,0,400,130,90)&&!CircleHitsFootprint(300,0,50,0,0,400,130,90)&&CircleHitsFootprint(100,0,50,0,0,400,130,90)&&!CircleHitsFootprint(std::numeric_limits<float>::quiet_NaN(),0,50,0,0,400,130,0),"footprint overlap test: inside, beside, rotated, and bad input");
    Check(ShiftCoverSunk(std::numeric_limits<float>::quiet_NaN(),10.f)==0.f&&ShiftCoverDrop>=400.f,"bad times do nothing; the drop clears the tallest block");
    // 0.23.4 lightning strikes (Endless wave 50+)
    Check(!LightningActive(false,80)&&!LightningActive(true,49)&&LightningActive(true,50)&&LightningActive(true,200),"lightning only in Endless from wave 50");
    Check(LightningInterval(50,0.f)>=4.5f-.01f&&LightningInterval(50,1.f)<=7.5f+.01f&&LightningInterval(500,0.f)>=1.8f*.75f-.001f,"volley cadence: about 4.5-7.5s at wave 50, never faster than the floor");
    for(int W=50;W<400;++W) Check(LightningInterval(W+1,.5f)<=LightningInterval(W,.5f)+.0001f&&LightningInterval(W,std::numeric_limits<float>::quiet_NaN())>0.f,"strikes never get slower with the wave and survive a bad roll");
    Check(LightningVolley(50)==1&&LightningVolley(65)==2&&LightningVolley(80)==3&&LightningVolley(95)==4&&LightningVolley(500)==6&&LightningVolley(0)==1,"0.23.5: strikes per volley grow with the wave to six");
    for(int W=50;W<400;++W) Check(LightningVolley(W+1)>=LightningVolley(W)&&LightningMaxAtOnce(W)>=LightningVolley(W),"volleys never shrink as the wave climbs and the concurrency cap fits a whole volley");
    Check(Near(LightningPlayerDamage(100.f),40.f)&&Near(LightningPlayerDamage(500.f),100.f)&&LightningPlayerDamage(std::numeric_limits<float>::quiet_NaN())==LightningPlayerMinimum,"player damage: 20% of max health with a floor");
    Check(Near(LightningEnemyDamage(100.f,false),20.f)&&Near(LightningEnemyDamage(1000.f,true),30.f)&&LightningEnemyDamage(std::numeric_limits<float>::infinity(),false)==0.f,"enemy damage: 20% of max health (same as players), 3% for bosses");
    Check(LightningClusterWeight(0)==1.f&&LightningClusterWeight(5)==36.f&&LightningClusterWeight(-3)==1.f,"clusters attract strikes quadratically");
    {const std::vector<float> Weights{1.f,9.f};Check(LightningPickIndex(Weights,.05f)==0&&LightningPickIndex(Weights,.5f)==1&&LightningPickIndex(Weights,1.f)==1&&LightningPickIndex({},.5f)==-1&&LightningPickIndex({0.f,0.f},.7f)==0,"weighted pick follows the weights and handles edge cases");
     int Hits=0;for(int I=0;I<1000;++I) if(LightningPickIndex(Weights,float(I)/1000.f)==1) ++Hits;Check(Hits>=890&&Hits<=910,"a weight of 9 against 1 is picked about 90% of the time");}
    Check(LightningTelegraph==.75f&&LightningRadius>=200.f&&LightningMinSeparation>LightningRadius*1.5f,"three-quarter-second dodge window and a readable radius");
    Check(FlyingBossWave(30)&&FlyingBossWave(60)&&FlyingBossWave(90)&&!FlyingBossWave(10)&&!FlyingBossWave(20)&&!FlyingBossWave(40)&&!FlyingBossWave(50)&&!FlyingBossWave(33)&&!FlyingBossWave(0)&&!FlyingBossWave(-30),"0.23.5 flying boss: every third boss wave only (30, 60, 90, ...)");
    Check(FlyingBossWave(30)==BossWave(30)&&FlyingBossHover>FlierHover&&FlyingBossOrbitRadius>FlierOrbitRadius&&FlyingBossContactDamage>FlierContactDamage,"the flying boss is a boss wave and is bigger than the Gloomwing in every number");
    // 0.23.4 flying monster
    Check(!SpawnsFlier(true,7,0.f)&&SpawnsFlier(true,8,.05f)&&!SpawnsFlier(true,8,.2f)&&!SpawnsFlier(false,3,0.f)&&SpawnsFlier(false,4,.1f)&&!SpawnsFlier(false,4,.2f),"fliers appear from Endless wave 8 and story room 4, at a modest rate");
    Check(SpawnsFlier(true,1000,.27f)&&!SpawnsFlier(true,1000,.29f)&&SpawnsFlier(false,500,.24f)&&!SpawnsFlier(false,500,.26f)&&!SpawnsFlier(true,50,std::numeric_limits<float>::quiet_NaN()),"flier rate is capped and ignores a bad roll");
    Check(FlierKind==10&&FlierHealthScale<1.f&&FlierHover-88.f<230.f,"a flier hovers within melee's vertical reach (230 cm) of a standing player");
    Check(FlierAttackDelay(0.f)==4.f&&FlierAttackDelay(1.f)==6.f&&FlierAttackDelay(std::numeric_limits<float>::quiet_NaN())==5.f,"dives come every 4-6 seconds");
    Check(FlierDiveSpeed*FlierDiveSeconds>=700.f&&FlierWindup>=.5f&&FlierContactDamage>0.f,"a dive covers enough ground and is telegraphed for at least half a second");
    std::cout<<"PASS: "<<Checks<<" expedition rule checks.\n";
}
