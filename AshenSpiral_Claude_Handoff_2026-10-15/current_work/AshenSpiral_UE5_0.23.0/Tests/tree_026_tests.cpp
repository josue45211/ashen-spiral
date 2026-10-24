// Native tests for the 0.26 skill tree (ASTreeRules.h, and its storage in ASRules.h). Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASTreeRules.h"
#include "../Source/AshenSpiral/Public/ASRules.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.001f) {return std::fabs(A-B)<E;}
int main() {
    using namespace ASTree;
    // ---- the shape: three lanes of eight, a capstone last, ids that map back ----
    Check(Lanes==3&&PerLane==8&&Nodes==24,"three lanes of eight nodes");
    Check(FirstRank==20&&ASRules::HearthCount==FirstRank+Nodes,"the tree is appended after the 20 hearth upgrades: the hearth's rank list is exactly as long as both");
    for(int N=0;N<Nodes;++N) {
        Check(Valid(N)&&LaneOf(N)*PerLane+SlotOf(N)==N,"a node id is lane * 8 + slot");
        Check(IsCapstone(N)==(N%8==7),"the last slot of a lane is its capstone");
        Check(Tier(N)>=1&&Tier(N)<=4&&(Tier(N)==4)==IsCapstone(N),"tiers run one to four and only the capstone is four");
        Check(Cost(N)>0&&(!IsCapstone(N)||Cost(N)>Cost(N-1)),"every node costs Ash and a capstone costs more than anything before it in its lane");
        Check(Required[SlotOf(N)]<=SlotOf(N),"a node never needs more nodes than come before it");
    }
    Check(!Valid(-1)&&!Valid(Nodes)&&Cost(-1)==0&&Cost(99)==0&&!IsCapstone(-3),"an out-of-range id is nothing");
    Check(Near(float(Cost(0)),15.f)&&Cost(2)==30&&Cost(5)==50&&Cost(7)==120,"openers 15, middle 30, late 50, capstone 120 Ash");
    Check(LaneCost(0)==340&&TotalCost()==1020,"a lane costs 340 Ash, the whole tree 1020 (about two thirds of every hearth upgrade together: real, and a long chase)");
    Check(Required[0]==0&&Required[1]==0&&Required[7]==7,"the openers need nothing; the capstone needs the other seven");
    // ---- walking a lane: openers, then the middle, then the late nodes, then the capstone ----
    for(int Lane=0;Lane<Lanes;++Lane) {
        Owned M=0;int Ash=LaneCost(Lane)+5;
        for(int S=0;S<PerLane;++S) {
            const int N=Lane*PerLane+S;
            Check(Available(M,N),"the next node of a lane in order is available");
            if(S<PerLane-1) Check(!Available(M,Lane*PerLane+PerLane-1)||CountInLane(M,Lane)>=7,"the capstone is not available before the other seven");
            Ash=Buy(M,Ash,N);Check(Ash>=0,"a lane can be bought in order with its full price");
        }
        Check(LaneComplete(M,Lane)&&CountInLane(M,Lane)==8&&Ash==5,"a finished lane holds eight nodes and cost exactly its price");
        for(int N=0;N<Nodes;++N) if(LaneOf(N)!=Lane) Check(!Owns(M,N),"finishing one lane touches no other lane");
    }
    // ---- the rules of buying ----
    {
        Owned M=0;
        Check(Available(M,0)&&Available(M,1)&&!Available(M,2)&&!Available(M,5)&&!Available(M,7),"at first only a lane's two openers are open");
        Check(Available(M,8)&&Available(M,9)&&!Available(M,10)&&Available(M,16)&&Available(M,17),"in every lane");
        int Ash=100;Check(Buy(M,Ash,2)==-1&&M==0,"a middle node cannot be bought first, and a refused buy changes nothing");
        Ash=Buy(M,Ash,0);Check(Ash==85&&Owns(M,0),"an opener is bought for 15 Ash");
        Check(Buy(M,Ash,0)==-1,"a node cannot be bought twice");
        Check(!Available(M,2),"one opener is not two");
        Ash=Buy(M,Ash,1);Check(Ash==70&&Available(M,2)&&Available(M,3)&&Available(M,4)&&!Available(M,5),"two openers open the middle nodes, not the late ones");
        Check(Buy(M,29,2)==-1&&Buy(M,30,2)==0,"a middle node needs its full 30 Ash");
        Check(!Available(M,10)&&!Available(M,18),"another lane's middle nodes stay shut: lanes are not locks, but they are separate");
        Check(Buy(M,0,8)==-1&&Buy(M,-5,8)==-1,"no Ash, no node");
    }
    {
        // spreading: two openers in all three lanes is six nodes and no middle node anywhere
        Owned M=0;int Ash=1000;for(int Lane=0;Lane<Lanes;++Lane) for(int S=0;S<2;++S) {Ash=Buy(M,Ash,Lane*PerLane+S);Check(Ash>=0,"openers can be spread over every lane");}
        Check(Ash==1000-6*15,"six openers cost 90");
        for(int Lane=0;Lane<Lanes;++Lane) Check(CountInLane(M,Lane)==2&&!LaneComplete(M,Lane)&&Available(M,Lane*PerLane+2),"each spread lane stands at two and its middle opens");
    }
    // ---- the profile's rank list ----
    {
        std::vector<int> Old(20,0);Old[3]=2;
        Check(FromRanks(Old)==0,"an old profile (20 ranks) owns no node");
        std::vector<int> Ranks(ASRules::HearthCount,0);Ranks[FirstRank+0]=1;Ranks[FirstRank+7]=1;Ranks[FirstRank+23]=1;Ranks[2]=5;
        const Owned M=FromRanks(Ranks);Check(Owns(M,0)&&Owns(M,7)&&Owns(M,23)&&!Owns(M,1)&&CountInLane(M,0)==2&&CountInLane(M,2)==1,"ranks past the hearth are nodes; the hearth's own ranks are not");
        Ranks[FirstRank+4]=-3;Check(!Owns(FromRanks(Ranks),4),"a bad rank owns nothing");
        std::vector<int> Short(FirstRank+5,1);Check(CountInLane(FromRanks(Short),0)==5&&CountInLane(FromRanks(Short),1)==0,"a short list reads only what it has");
        Check(FromRanks({})==0,"an empty list owns nothing");
    }
    // ---- the effects: small, capped, never stacking with themselves ----
    for(float V:{HonedEdgeDamage,LongReachBonus,QuickHandsSpeed,IronSkinGuard,LeapMasterCooldown,SteadyHandDamage,DeepMagazineBonus,QuickReloadSpeed,EagleEyeCritical,FleetAimSpeed,LongShotSpeed,MarksmanBonus,RhythmCooldown,SteadfastRevive,BrandHunterNeed,BrandHunterBurst,TetherWardenBonus,ConvergenceDamage})
        Check(V>0.f&&V<=.30f,"every percentage a node gives is between nothing and thirty percent");
    Check(ExecutionerBonus<=.30f&&ExecutionerBelow<=.30f&&EchoScale<=.5f&&EchoScale>0.f&&BloodRushHeal<=5.f,"the conditional and flat bonuses are small too");
    Check(PredatorLockout>=1.f&&PierceRounds>=1&&PierceRounds<=3&&PierceSeconds>=3.f&&PierceSeconds<=8.f&&EchoDelay>=.4f&&EchoDelay<=1.5f,"the capstones' limits are kept");
    {
        const Owned All=(Owned(1)<<Nodes)-1;
        Check(Near(MeleeDamageBonus(All),.05f)&&Near(RangedDamageBonus(All),.05f)&&MeleeDamageBonus(0)==0.f&&RangedDamageBonus(0)==0.f,"the damage nodes give 5%, or nothing without them");
        Check(MagazineWith(0,30)==30&&MagazineWith(All,30)==35&&MagazineWith(All,6)==7&&MagazineWith(All,3)==4&&MagazineWith(All,0)==0&&MagazineWith(All,1)==2,"a bigger magazine is +15% and at least one round, and a weapon with no magazine has none");
        Check(Near(ReloadScale(All),.88f)&&ReloadScale(0)==1.f,"reloads are 12% faster");
        Check(Near(CriticalWith(All,.05f),.09f)&&Near(CriticalWith(All,.48f),.50f)&&Near(CriticalWith(All,.50f),.50f)&&CriticalWith(0,.2f)==.2f,"+4 points of critical chance, never past the 50% ceiling");
        Check(Near(LeapScale(All),.85f)&&LeapScale(0)==1.f&&Near(TeamCooldownScale(All),.88f)&&TeamCooldownScale(0)==1.f,"leaps recover 15% faster, team skills 12%");
        Check(Near(BrandNeedScale(All),.85f)&&Near(BrandBurstScale(All),1.15f)&&BrandNeedScale(0)==1.f&&BrandBurstScale(0)==1.f,"the brand needs 15% less and bursts 15% harder");
        Check(Near(TetherSecondsFor(All,7.f),9.f)&&Near(TetherBonusFor(All,.15f),.20f)&&TetherSecondsFor(0,7.f)==7.f&&TetherBonusFor(0,.15f)==.15f,"the tether lasts two seconds longer and gives 20%");
        Check(Near(TwinWindowFor(All,1.5f),2.1f)&&Near(TwinDamageScale(All),1.2f)&&TwinWindowFor(0,1.5f)==1.5f&&TwinDamageScale(0)==1.f,"the call is .6 s longer, the collapse 20% harder");
        Check(Near(ExecutionerScale(All,29.f,100.f),1.25f)&&Near(ExecutionerScale(All,30.f,100.f),1.25f)&&ExecutionerScale(All,31.f,100.f)==1.f&&ExecutionerScale(0,1.f,100.f)==1.f&&ExecutionerScale(All,1.f,0.f)==1.f,"executioner: a quarter more against the last 30%, only with the node, a bad health never");
        Check(Near(MarksmanScale(All,true),1.15f)&&MarksmanScale(All,false)==1.f&&MarksmanScale(0,true)==1.f,"marksman: elites and bosses only");
        Check(PredatorReady(All,10.f,10.f)&&!PredatorReady(All,9.9f,10.f)&&!PredatorReady(0,99.f,0.f),"the capstone refunds at most once per lockout, and only with the node");
        // the rest of the nodes, each from its own bit
        Check(Near(MeleeIntervalScale(All),1.f/1.06f)&&MeleeIntervalScale(0)==1.f&&MeleeIntervalScale(All)<1.f,"quick hands shorten the swing interval by exactly the speed it gives");
        Check(Near(MeleeReachScale(All),1.08f)&&MeleeReachScale(0)==1.f&&Near(MeleeGuard(All),.06f)&&MeleeGuard(0)==0.f,"long reach +8%, iron skin 6%");
        Check(Near(KillHeal(All),3.f)&&KillHeal(0)==0.f&&Near(RunSpeedScale(All),1.05f)&&RunSpeedScale(0)==1.f&&Near(ProjectileSpeedScale(All),1.12f)&&ProjectileSpeedScale(0)==1.f,"blood rush 3 health, fleet aim +5%, long shot +12%");
        Check(Near(ReviveScale(All),1.3f)&&ReviveScale(0)==1.f&&Near(FocusBonus(All),15.f)&&FocusBonus(0)==0.f,"steadfast hands lift 30% faster, wellspring +15 Focus");
        Check(Near(WardShield(All,false),12.f)&&Near(WardShield(All,true),24.f)&&WardShield(0,true)==0.f,"shared ward: 12 shield, 24 with the partner up, none without the node");
        Check(PierceFor(All,true)==2&&PierceFor(All,false)==0&&PierceFor(0,true)==0,"piercing rounds: two enemies, only inside the window, only with the capstone");
        Check(Near(EchoDamage(100.f),50.f)&&EchoDamage(0.f)==0.f&&EchoDamage(-5.f)==0.f&&EchoDamage(std::numeric_limits<float>::quiet_NaN())==0.f&&EchoDamage(std::numeric_limits<float>::infinity())==0.f,"an echo is half the burst, and a bad number is no echo");
        Check(EchoOwed(All,0)&&EchoOwed(0,All)&&EchoOwed(All,All)&&!EchoOwed(0,0)&&!EchoOwed(Bit(HonedEdge),Bit(Wellspring)),"either hero of a burst holding the capstone owes the echo, and no node but the capstone does");
        const Owned Others=All&~Bit(SynergyEcho);Check(!EchoOwed(Others,Others),"the other twenty-three nodes make no echo");
    }
    {
        // reading the rank list without a copy (the character's TArray): the same answer as the vector form, and safe for null and short data
        std::vector<int> Ranks(ASRules::HearthCount,0);Ranks[FirstRank+3]=1;Ranks[FirstRank+16]=2;
        Check(FromRankData(Ranks.data(),int(Ranks.size()))==FromRanks(Ranks)&&Owns(FromRankData(Ranks.data(),int(Ranks.size())),3)&&Owns(FromRankData(Ranks.data(),int(Ranks.size())),16),"the raw form reads what the vector form reads");
        Check(FromRankData(nullptr,44)==0&&FromRankData(Ranks.data(),0)==0&&FromRankData(Ranks.data(),FirstRank)==0&&FromRankData(Ranks.data(),-4)==0,"null, empty, hearth-only and negative counts own nothing");
        Check(FromRankData(Ranks.data(),FirstRank+4)==Bit(3),"a list that ends mid-tree owns only what it holds");
    }
    std::printf("PASS: %lld skill tree checks (the three lanes of eight and their prices, walking, spreading and refusing, the rank list's old and short forms, every node's bounded effect).\nShared rules only; the menu, the purchases in a profile and the effects in a fight are not executed here.\n",Checks);
    return 0;
}
