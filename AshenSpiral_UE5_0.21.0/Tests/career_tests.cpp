#include "../Source/AshenSpiral/Public/ASCareerRules.h"
#include <cstdlib>
#include <iostream>
#include <limits>
static int Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
int main() {
    using namespace ASCareer;
    Check(Level(0)==1&&Progress(0)==0,"new and migrated profiles start at level one");
    Check(Level(-1)==1&&Progress(-1)==0,"negative experience is bounded");
    Check(Threshold(std::numeric_limits<int>::min())==0,"invalid low level has no integer underflow");
    std::int64_t Expected=0;
    for(int L=1;L<=10000;++L) {
        Check(Threshold(L)==Expected,"cumulative threshold agrees with summed linear costs");
        Check(Cost(L)==100+(L-1)*25,"each level takes twenty-five more XP than the previous");
        if(Expected<=MaxValue) {
            const int XP=int(Expected);
            Check(Level(XP)==L&&Progress(XP)==0,"exact threshold advances level once");
            if(L>1) Check(Level(XP-1)==L-1&&Progress(XP-1)==Cost(L-1)-1,"one XP below threshold remains previous level");
            Check(Level(XP+1)==L&&Progress(XP+1)==1,"first XP after threshold preserved");
        }
        Expected+=Cost(L);
    }
    Check(Add(MaxValue-3,10)==MaxValue,"experience saturates safely");
    Check(Add(std::numeric_limits<int>::max(),std::numeric_limits<int>::max())==MaxValue,"large awards cannot overflow");
    Check(Add(-1,-1)==0&&Add(25,-50)==25,"invalid awards never subtract progress");
    Check(Level(std::numeric_limits<int>::max())==Level(MaxValue),"loaded experience cap matches live awards");
    Check(Progress(MaxValue)>=0&&Progress(MaxValue)<Cost(Level(MaxValue)),"maximum supported experience has valid progress");
    for(int I=0;I<Count;++I) {
        const auto& A=Achievements[I];Check(int(A.Stat)==I,"achievement counter has stable serialized index");
        Check(A.Goals[0]>0&&A.Goals[0]<A.Goals[1]&&A.Goals[1]<A.Goals[2],"achievement goals become progressively harder");
        Check(Tier(0,I)==0,"new counters unlock nothing");
        for(int T=0;T<3;++T) {
            Check(Tier(A.Goals[T]-1,I)==T,"tier remains locked before threshold");
            Check(Tier(A.Goals[T],I)==T+1,"tier unlocks on threshold");
            Check(Tier(A.Goals[T]+1,I)==T+1,"progress past threshold retains tier");
        }
        Check(Tier(MaxValue,I)==3,"achievements cap at tier three");
    }
    Check(Count*3==36&&Tier(100,-1)==0&&Tier(100,Count)==0,"thirty-six milestones and safe unknown ids");
    for(int C=-50;C<=10000;++C) {
        const int Ash=Cashout(C),Spent=CashoutCost(C);
        Check(Ash>=0&&Ash<=3,"conversion is capped at three permanent Ash");
        Check(Spent==Ash*10&&Spent<=std::max(C,0)/10,"conversion consumes no more than ten percent of balance");
        Check(Spent<=std::max(C,0),"conversion cannot overdraw Cinders");
        if(C>=0&&C<300) Check(Spent==C/100*10,"fractional Ash is rounded down in ten-Cinder units");
    }
    Check(Cashout(99)==0&&Cashout(100)==1&&Cashout(250)==2&&Cashout(300)==3,"cashout boundary examples");
    Check(Cashout(std::numeric_limits<int>::min())==0&&Cashout(std::numeric_limits<int>::max())==3,"invalid balances safely bounded");
    Check(!MultiStatus(0)&&!MultiStatus(1)&&!MultiStatus(4|8),"freeze plus its frost buildup is only one status");
    Check(MultiStatus(1|2)&&MultiStatus(2|4)&&MultiStatus(1|16),"distinct elemental and bleed combinations count");
    std::cout<<"PASS: "<<Checks<<" career checks; linear XP thresholds, 36 achievement milestones, bounded Cinder conversion and status combinations.\n";
}
