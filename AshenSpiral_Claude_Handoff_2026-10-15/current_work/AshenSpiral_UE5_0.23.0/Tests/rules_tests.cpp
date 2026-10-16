#include "../Source/AshenSpiral/Public/ASRules.h"
#include <iostream>
#include <limits>
#include <cstdlib>
#include <set>
#include <string>
static std::uint64_t Checks=0;
static void Check(bool Value,const char* Message) {++Checks;if(!Value) {std::cerr<<"FAILED: "<<Message<<"\n";std::exit(1);}}
static std::string Fingerprint(const ASRules::Layout& L) {
    std::string S=std::to_string(L.Shape)+":"+std::to_string(L.SpacingX)+":"+std::to_string(L.SpacingY)+":";for(auto C:L.Cells) {S+=std::to_string(C.X)+","+std::to_string(C.Y)+","+std::to_string(C.HalfX)+","+std::to_string(C.HalfY)+";";}
    for(auto E:L.Edges) S+=std::to_string(E.A)+","+std::to_string(E.B)+","+std::to_string(E.Gap)+";";
    return S+std::to_string(L.Exit);
}
int main() {
    using namespace ASRules;
    std::set<std::string> Layouts;std::set<int> Counts,Shapes,Widths,Heights,Footprints;
    int LayoutCount=0,TotalGaps=0;
    for(std::uint32_t Seed=0;Seed<10000;++Seed) {
        const auto L=Generate(Seed);++LayoutCount;
        const int Count=int(L.Cells.size());Counts.insert(Count);Shapes.insert(L.Shape);Widths.insert(L.Width);Heights.insert(L.Height);
        Check(Count>=MinCells&&Count<=MaxCells,"variable clearing count remains bounded");
        Check(Fingerprint(L)==Fingerprint(Generate(Seed)),"deterministic seed");
        if(Seed<200) Layouts.insert(Fingerprint(L));
        std::set<int> Cells;int Gaps=0;
        for(auto C:L.Cells) {
            Check(C.X>=0&&C.Y>=0&&C.X<Grid&&C.Y<Grid,"cell inside grid");
            Check(Cells.insert(C.Y*Grid+C.X).second,"unique cell positions");
            Check((C.SpanX==1&&C.SpanY==1)||(C.SpanX>=2&&C.SpanX<=3&&C.SpanY>=2&&C.SpanY<=3),"only 1x1, 2x2, 2x3, 3x2 or 3x3 room footprints");
            Check(C.HalfX==600*C.SpanX&&C.HalfY==600*C.SpanY,"room floor area equals the requested number of base grid units");
            Footprints.insert(C.SpanX*10+C.SpanY);
        }
        Check(L.Cells[0].SpanX*L.Cells[0].SpanY>=4,"each map has a spacious entry court");
        for(int A=0;A<Count;++A) for(int B=A+1;B<Count;++B) {
            const auto& P=L.Cells[A];const auto& Q=L.Cells[B];
            Check(std::abs(P.WorldX-Q.WorldX)>=P.HalfX+Q.HalfX+600||std::abs(P.WorldY-Q.WorldY)>=P.HalfY+Q.HalfY+600,"room rectangles never overlap and have corridor clearance");
        }
        for(auto E:L.Edges) {
            Check(E.A>=0&&E.B>=0&&E.A<Count&&E.B<Count,"edge indices in bounds");
            Check(std::abs(L.Cells[E.A].X-L.Cells[E.B].X)+std::abs(L.Cells[E.A].Y-L.Cells[E.B].Y)==1,"edges join adjacent clearings");
            Gaps+=E.Gap?1:0;
            const auto& A=L.Cells[E.A];const auto& B=L.Cells[E.B];const bool AlongX=A.Y==B.Y;
            const int LowX=AlongX?std::min(A.WorldX,B.WorldX):A.WorldX-220;
            const int HighX=AlongX?std::max(A.WorldX,B.WorldX):A.WorldX+220;
            const int LowY=AlongX?A.WorldY-220:std::min(A.WorldY,B.WorldY);
            const int HighY=AlongX?A.WorldY+220:std::max(A.WorldY,B.WorldY);
            for(int Other=0;Other<Count;++Other) if(Other!=E.A&&Other!=E.B) {
                const auto& C=L.Cells[Other];
                Check(HighX<=C.WorldX-C.HalfX||LowX>=C.WorldX+C.HalfX||HighY<=C.WorldY-C.HalfY||LowY>=C.WorldY+C.HalfY,"a corridor never crosses an unrelated room");
            }
        }
        TotalGaps+=Gaps;Check(Gaps<=2,"no more than two optional dash gaps");
        Check(L.Exit>0&&L.Exit<Count,"exit is not spawn and is in bounds");
        // Independent all-pairs oracle verifies weighted exit selection.
        std::vector<std::vector<int>> Dist(Count,std::vector<int>(Count,100000000));
        for(int A=0;A<Count;++A) Dist[A][A]=0;
        for(auto E:L.Edges) if(!E.Gap) {const auto& A=L.Cells[E.A];const auto& B=L.Cells[E.B];Dist[E.A][E.B]=Dist[E.B][E.A]=std::abs(A.WorldX-B.WorldX)+std::abs(A.WorldY-B.WorldY);}
        for(int K=0;K<Count;++K) for(int A=0;A<Count;++A) for(int B=0;B<Count;++B) Dist[A][B]=std::min(Dist[A][B],Dist[A][K]+Dist[K][B]);
        for(int A=0;A<Count;++A) Check(Dist[0][A]<=Dist[0][L.Exit]&&Dist[0][A]<100000000,"reward is farthest by shortest supported graph distance");
        for(int Destination=0;Destination<Count;++Destination) {
            int At=0,Hops=0;
            while(At!=Destination&&Hops++<Count) {
                const int Next=NextCell(L,At,Destination);Check(Next>=0&&Next<Count,"route next hop valid");
                bool Connected=false;for(auto E:L.Edges) if(!E.Gap&&((E.A==At&&E.B==Next)||(E.B==At&&E.A==Next))) Connected=true;
                Check(Connected,"route never requires an enemy to cross a gap");At=Next;
            }
            Check(At==Destination,"every clearing reachable without dashing");
        }
        Check(NextCell(L,-1,0)==-1&&NextCell(L,0,Count)==-1,"out of range route is rejected");
    }
    Check(Counts.size()==6&&Shapes.size()==5&&Widths.size()>=3&&Heights.size()>=3,"map counts, topology and proportions vary materially");
    Check(Layouts.size()>190,"seeds provide varied layouts");
    Check(Footprints==std::set<int>({11,22,23,32,33}),"all five room footprints appear across seeds");
    for(int R=0;R<32;++R) {
        Check(Ward(R)==R/8,"eight rooms per ward");Check(Boss(R)==(R%8==7),"boss room schedule");Check(Elite(R)==(R%8==3),"elite schedule");
        Check(Final(R)==(R==31),"only final room ends victorious");Check(WaveCount(R)==(Boss(R)?1:3),"boss and normal wave counts");
        for(int P=1;P<=2;++P) for(int W=-3;W<8;++W) {
            int N=WaveEnemies(R,W,P);Check(N>0&&N<=MaxEnemies,"bounded encounter size");
        }
    }
    Check(WaveEnemies(31,2,2)==42,"late jungle burst respects cap");
    Check(2350*.24>400+68,"dash displacement clears a 400cm gap plus capsule diameter");
    Check(Perfect(.70,0,false)&&Perfect(.92,0,false),"perfect timing endpoints inclusive");
    Check(!Perfect(.699,0,false)&&!Perfect(.921,0,false),"normal timing outside window");
    Check(Perfect(.55,3,true)&&Perfect(1.07,3,true),"rank and focus widen window");
    Check(!Perfect(std::numeric_limits<double>::quiet_NaN(),0,false),"NaN timing rejected");
    Check(!Perfect(std::numeric_limits<double>::infinity(),3,true),"infinite timing rejected");
    for(int Base:{8,10,12,20,25}) for(int Rank=0;Rank<6;++Rank) {
        const int C=Cost(Base,Rank,3);Check(C==(Rank<3?Base*(Rank+1):-1),"rank cost and maximum");
    }
    int Balance=20;Check(!Spend(Balance,-1)&&Balance==20,"max-rank sentinel cannot credit balance");
    Check(!Spend(Balance,21)&&Balance==20,"insufficient funds are unchanged");
    Check(Spend(Balance,20)&&Balance==0,"exact spend succeeds");
    Check(Credit(MaxCurrency,2147483647)==MaxCurrency,"credit cannot overflow");
    Check(Credit(5,-4)==5,"negative grant cannot debit balance");
    int Scrap=100,Bone=100,Ash=0;bool Bought=false;
    Check(Refinement(Scrap,Bone,Ash,Bought),"first refinement accepted");
    Check(Scrap==85&&Bone==92&&Ash==40&&Bought,"refinement grants exactly forty Ash");
    for(int I=0;I<100;++I) Check(!Refinement(Scrap,Bone,Ash,Bought),"refinement cannot be repeated");
    Check(Scrap==85&&Bone==92&&Ash==40,"rejected refinements leave currency unchanged");
    int Cores=9,Points=0;Bone=36;
    for(int I=0;I<3;++I) Check(Memory(Cores,Bone,Points),"repeatable memory craft succeeds");
    Check(Points==3&&Cores==0&&Bone==0,"every craft grants a permanent point and charges materials");
    Check(!Memory(Cores,Bone,Points)&&Points==3,"insufficient memory ingredients cannot grant points");
    Scrap=14;Bone=8;Ash=0;Bought=false;
    Check(!Refinement(Scrap,Bone,Ash,Bought)&&Scrap==14&&Bone==8&&Ash==0&&!Bought,"failed recipe is atomic");
    std::cout<<"PASS: "<<Checks<<" assertions; "<<LayoutCount<<" deterministic layouts; "<<TotalGaps<<" optional gap edges.\n";
    std::cout<<"Native shared-rule tests only. Unreal compilation, physics, rendering and networking were NOT exercised.\n";
}
