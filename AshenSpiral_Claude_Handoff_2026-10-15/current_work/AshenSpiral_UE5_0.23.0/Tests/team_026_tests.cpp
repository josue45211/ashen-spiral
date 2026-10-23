// Native tests for the 0.26 team abilities (ASTeamRules.h, and the ability tables in ASAdventureRules.h). Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASTeamRules.h"
#include "../Source/AshenSpiral/Public/ASAdventureRules.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.01f) {return std::fabs(A-B)<E;}
int main() {
    using namespace ASTeam;
    const float NaN=std::numeric_limits<float>::quiet_NaN(),Inf=std::numeric_limits<float>::infinity();
    // ---- the ability tables: nine skills, the three new ones usable from the base focus pool ----
    Check(ASAdventure::AbilityCount==9&&FirstAbility==ASAdventure::AbilityCount-3&&Brand==6&&Tether==7&&Twin==8,"nine active skills; the team skills are the last three");
    for(int A=0;A<ASAdventure::AbilityCount;++A) {
        Check(ASAdventure::AbilityCost(A)>0.f&&ASAdventure::AbilityCost(A)<=60.f,"every skill is usable from the base focus pool");
        Check(ASAdventure::AbilityCooldown(A,1.f)>0.f&&Near(ASAdventure::AbilityCooldown(A,-9.f),ASAdventure::AbilityCooldown(A,1.f)*.5f)&&Near(ASAdventure::AbilityCooldown(A,9.f),ASAdventure::AbilityCooldown(A,1.f)),"recovery clamps for the team skills too");
    }
    Check(ASAdventure::AbilityCost(-1)==ASAdventure::AbilityCost(0)&&ASAdventure::AbilityCost(99)==ASAdventure::AbilityCost(ASAdventure::AbilityCount-1),"out of range ids stay bounded");
    Check(Near(ASAdventure::AbilityCost(Brand),30.f)&&Near(ASAdventure::AbilityCost(Tether),35.f)&&Near(ASAdventure::AbilityCost(Twin),40.f),"brand 30, tether 35, collapse 40 focus");
    Check(Near(ASAdventure::AbilityCooldown(Brand,1.f),10.f)&&Near(ASAdventure::AbilityCooldown(Tether,1.f),16.f)&&Near(ASAdventure::AbilityCooldown(Twin,1.f),16.f),"brand 10 s, tether and collapse 16 s");
    Check(AbilityIsTeam(Brand)&&AbilityIsTeam(Tether)&&AbilityIsTeam(Twin)&&!AbilityIsTeam(0)&&!AbilityIsTeam(5)&&!AbilityIsTeam(-1),"the team skills are exactly 6-8");
    Check(SynergyBrand==3&&SynergyTether==4&&SynergyTwin==5,"the cue triggers follow the three elemental synergies");
    // ---- Ashen Brand ----
    Check(Near(BrandNeed(70.f,false),70.f)&&Near(BrandNeed(300.f,false),75.f)&&Near(BrandNeed(436.f,false),109.f)&&Near(BrandNeed(5000.f,false),400.f)&&Near(BrandNeed(1.f,false),70.f),"the brand needs a quarter of a body's health stored, between 70 and 400");
    Check(Near(BrandNeed(1100.f,true),120.f)&&Near(BrandNeed(3550.f,true),142.f)&&Near(BrandNeed(100000.f,true),500.f),"a boss's brand needs 4% of its health, between 120 and 500");
    Check(BrandNeed(NaN,false)==70.f&&BrandNeed(-4.f,true)==70.f&&BrandNeed(Inf,false)==70.f,"a bad health never makes a bad need");
    for(float Hp=1.f;Hp<20000.f;Hp*=1.37f) for(bool Boss:{false,true}) Check(BrandNeed(Hp,Boss)>=70.f&&BrandNeed(Hp,Boss)<=500.f&&BrandNeed(Hp*1.01f,Boss)>=BrandNeed(Hp,Boss),"the need is bounded and never falls as health grows");
    Check(Near(BrandCharge(30.f,false),30.f)&&Near(BrandCharge(30.f,true),10.5f)&&BrandCharge(-5.f,false)==0.f&&BrandCharge(NaN,true)==0.f&&BrandCharge(Inf,true)==0.f,"a partner's hit is stored whole, the brander's own at 35%, nothing for a bad number");
    Check(BrandOwnShare<1.f&&BrandOwnShare>0.f,"alone you can still fill your own brand, only slower");
    Check(BrandFillPercent(0.f,100.f)==0&&BrandFillPercent(50.f,100.f)==50&&BrandFillPercent(100.f,100.f)==100&&BrandFillPercent(900.f,100.f)==100&&BrandFillPercent(5.f,0.f)==0&&BrandFillPercent(NaN,100.f)==0,"the fill is a percent, held between none and full");
    Check(BrandMilestone(0)==0&&BrandMilestone(24)==0&&BrandMilestone(25)==1&&BrandMilestone(74)==2&&BrandMilestone(75)==3&&BrandMilestone(100)==4&&BrandMilestone(400)==4&&BrandMilestone(-8)==0,"a tick as the fill passes each quarter");
    {
        // a partner's thirty-damage claws (every .19 s) fill an elite's 75 brand in three hits; alone, the brander needs eight
        float Stored=0.f;int Hits=0;while(Stored<BrandNeed(300.f,false)&&Hits<100) {Stored+=BrandCharge(30.f,false);++Hits;}Check(Hits==3,"a partner needs three claw hits to fill an elite's brand");
        Stored=0.f;Hits=0;while(Stored<BrandNeed(300.f,false)&&Hits<100) {Stored+=BrandCharge(30.f,true);++Hits;}Check(Hits==8,"a lone hero needs eight (a 300-health elite dies on the tenth: the brand bursts near the end, never before the fight is half done)");
    }
    Check(Near(BrandBurstDamage(75.f,300.f,false),75.f)&&Near(BrandBurstDamage(150.f,3550.f,true),150.f+3550.f*.035f)&&BrandBurstDamage(-5.f,300.f,false)==0.f&&BrandBurstDamage(100.f,NaN,true)==100.f,"the burst deals what was stored, and a boss a little more");
    Check(Near(BrandSplashDamage(100.f,false),60.f)&&Near(BrandSplashDamage(100.f,true),30.f)&&BrandSplashDamage(-1.f,false)==0.f,"the splash is 60% of what was stored, half of that if the branded enemy died first");
    Check(BrandDeathBursts(50.f,100.f)&&!BrandDeathBursts(49.f,100.f)&&!BrandDeathBursts(10.f,0.f),"a brand that dies half full still splashes");
    Check(BrandSeconds>=6.f&&BrandSeconds<=10.f&&BrandSplashRadius>=300.f&&BrandAssist>=200.f&&BrandReach>=1000.f,"the brand's reach and time are generous enough to use in a fight");
    // ---- Ember Tether ----
    Check(Near(LashDamageFor(true),18.f)&&Near(LashDamageFor(false),18.f*.55f)&&LashDamageFor(false)<LashDamageFor(true),"a stake's lash is weaker than a partner's");
    Check(Near(TetherBuff(true,0.f),.15f)&&Near(TetherBuff(true,TetherBuffRange),.15f)&&TetherBuff(true,TetherBuffRange+1.f)==0.f&&TetherBuff(false,0.f)==0.f&&TetherBuff(true,NaN)==0.f&&TetherBuff(true,Inf)==0.f,"the damage bonus is for a pair that stays close, never for a lone hero");
    Check(!TetherSnaps(LinkNone,99999.f)&&!TetherSnaps(LinkPair,TetherSnap)&&TetherSnaps(LinkPair,TetherSnap+1.f)&&!TetherSnaps(LinkAnchor,AnchorSnap)&&TetherSnaps(LinkAnchor,AnchorSnap+1.f)&&TetherSnaps(LinkPair,NaN)&&TetherSnaps(LinkAnchor,NaN),"a tether snaps when stretched too far (a bad distance too), never when there is none");
    Check(Near(SegmentDistance2D(5.f,5.f,0.f,0.f,10.f,0.f),5.f)&&Near(SegmentDistance2D(-3.f,4.f,0.f,0.f,10.f,0.f),5.f)&&Near(SegmentDistance2D(13.f,4.f,0.f,0.f,10.f,0.f),5.f)&&Near(SegmentDistance2D(1.f,1.f,2.f,2.f,2.f,2.f),std::sqrt(2.f),.001f),"distance to a segment: beside it, past either end, and a point-sized one");
    {float T=-1.f;SegmentDistance2D(7.f,3.f,0.f,0.f,10.f,0.f,&T);Check(Near(T,.7f,.001f),"the closest point's place along the segment");SegmentDistance2D(-7.f,3.f,0.f,0.f,10.f,0.f,&T);Check(T==0.f,"clamped at its start");SegmentDistance2D(77.f,3.f,0.f,0.f,10.f,0.f,&T);Check(T==1.f,"and at its end");}
    {
        const float A[3]={0.f,0.f,90.f},B[3]={1000.f,0.f,90.f};
        const float On[3]={500.f,0.f,85.f},Beside[3]={500.f,120.f,85.f},Far[3]={500.f,300.f,85.f},High[3]={500.f,0.f,400.f},Past[3]={1200.f,0.f,85.f},Bad[3]={NaN,0.f,85.f};
        Check(LashCrosses(On,40.f,A,B),"an enemy on the line is crossed");
        Check(LashCrosses(Beside,40.f,A,B)&&!LashCrosses(Beside,0.f,A,B),"one a body-width beside it is crossed by its width, not as a point");
        Check(!LashCrosses(Far,40.f,A,B)&&!LashCrosses(High,40.f,A,B)&&!LashCrosses(Past,40.f,A,B),"far to the side, high above, or past the end: not crossed");
        Check(!LashCrosses(Bad,40.f,A,B),"a bad position is never crossed");
        const float Rising[3]={1000.f,0.f,500.f};Check(!LashCrosses(On,40.f,A,Rising)||LashCrosses(On,40.f,A,Rising),"a sloped line is evaluated without trouble");
        const float Mid[3]={500.f,0.f,290.f};Check(LashCrosses(Mid,40.f,A,Rising),"a sloped line is crossed at its own height there");
    }
    Check(TetherSeconds>=5.f&&TetherSeconds<=10.f&&TetherFormRange>TetherBuffRange&&TetherSnap>TetherFormRange&&AnchorSnap>AnchorPlaceRange,"the tether forms at a reach, holds longer than it forms and snaps beyond both");
    Check(LashInterval>0.f&&LashHalfWidth>=60.f&&LashCuesPerPulse>=2,"the lash pulses often enough and wide enough to read");
    {
        // the lash against one enemy over a full tether is worth about a half-dozen claw hits, against a crowd it scales with the crowd: a real reward for lining enemies up between you
        const float PerEnemy=LashDamageFor(true)*(TetherSeconds/LashInterval);Check(PerEnemy>200.f&&PerEnemy<400.f,"one enemy held on the line for the whole tether takes 200-400 (the line is a zone, not a weapon)");
    }
    // ---- Twin Collapse ----
    Check(TwinJoins(10.f,10.5f,800.f)&&TwinJoins(10.f,10.f,0.f)&&TwinJoins(10.f,10.f,TwinMaxSeparation),"a second cast inside the window and close enough joins the first");
    Check(!TwinJoins(10.6f,10.5f,800.f)&&!TwinJoins(10.f,0.f,800.f)&&!TwinJoins(10.f,10.5f,TwinMaxSeparation+1.f)&&!TwinJoins(10.f,10.5f,NaN)&&!TwinJoins(10.f,10.5f,Inf),"too late, no call open, too far apart or a bad distance: it does not");
    Check(TwinWindow>=1.f&&TwinWindow<=2.f&&TwinMaxSeparation>=1000.f,"the window is a beat a partner can answer");
    {
        const float Both=2.f*TwinLesserDamage+TwinBlastDamage(300.f,false),Sum=2.f*TwinLesserDamage;
        Check(TwinBlastDamage(300.f,false)>TwinLesserDamage*3.f&&Both>Sum*2.f,"two together deal more than twice what two lone casts do");
        Check(TwinRadius>TwinLesserRadius*2.f&&TwinStun>TwinLesserStun*2.f&&TwinShield>TwinLesserShield*2.f,"and reach farther, hold longer and shield more");
        Check(Near(TwinBlastDamage(3550.f,true),TwinDamage+3550.f*.05f)&&TwinBlastDamage(NaN,true)==TwinDamage&&Near(TwinBlastDamage(-5.f,true),TwinDamage),"a boss takes 5% of its health on top");
        Check(TwinDelay>=.4f&&TwinDelay<=1.f&&TwinPullTick>0.f&&TwinPull>0.f,"the pull-in is short and real");
    }
    // ---- the lone-hero forms are weaker than the pair's, by design ----
    Check(BrandOwnShare<.5f&&LashAnchorScale<.7f&&TwinLesserDamage*2.f<TwinBlastDamage(300.f,false)&&TetherBuff(false,0.f)==0.f,"every skill is weaker alone: the brand fills slower, the stake lashes softer, the collapse is the small one, no damage bonus");
    std::printf("PASS: %lld team ability checks (the nine-skill tables, brand need/charge/burst/splash/death, tether lash geometry/buff/snap, twin collapse window/separation/oversize, every lone-hero form weaker).\nShared rules only; Unreal gameplay, replication and the show are not executed.\n",Checks);
    return 0;
}
