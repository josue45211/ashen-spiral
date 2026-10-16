// Native structural tests for the 0.23 boss arena layouts. Not Unreal navigation/collision tests.
#include "../Source/AshenSpiral/Public/ASRules.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
static int Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
struct P2 {float X,Y;};
// Sample the rotated rectangle's boundary and interior corners for conservative distance checks.
static std::vector<P2> Outline(const ASRules::BossProp& B) {
    const float A=B.Yaw*ASRules::Pi180(),C=std::cos(A),S=std::sin(A);std::vector<P2> Pts;
    for(int I=0;I<=8;++I) for(int J=0;J<=8;++J) if(I==0||I==8||J==0||J==8) {
        const float LX=(I/8.f-.5f)*B.Length,LY=(J/8.f-.5f)*B.Thickness;
        Pts.push_back({B.X+LX*C-LY*S,B.Y+LX*S+LY*C});
    }
    return Pts;
}
static float MinDistance(const ASRules::BossProp& A,const ASRules::BossProp& B) {
    float Best=1e9f;const auto PA=Outline(A),PB=Outline(B);
    for(auto& U:PA) for(auto& V:PB) Best=std::min(Best,std::hypot(U.X-V.X,U.Y-V.Y));
    return Best;
}
int main() {
    using namespace ASRules;
    for(int W=0;W<WardCount;++W) {
        const auto L=GenerateBoss(W);
        Check(L.Cells.size()==2&&L.Edges.size()==1&&L.Exit==1&&L.Shape==BossShapeBase+W,"boss layout: arena + reward alcove, one door");
        Check(L.Cells[0].HalfX==BossArenaHalf&&L.Cells[1].WorldX-L.Cells[0].WorldX>BossArenaHalf,"alcove sits east of the arena");
        Check(NextCell(L,0,1)==1,"the reward alcove is reachable from the arena");
        const auto Props=BossProps(W);bool AnyCover=false;
        for(const auto& P:Props) {
            if(P.Kind!=0) continue;AnyCover=true;
            for(auto Pt:Outline(P)) {
                Check(std::abs(Pt.X)<=BossArenaHalf-290&&std::abs(Pt.Y)<=BossArenaHalf-290,"cover keeps 290 cm clear of the arena walls");
                Check(std::hypot(Pt.X,Pt.Y)>=320.f,"cover never sits on the boss spawn");
                Check(std::hypot(Pt.X-(-BossArenaHalf+450.f),Pt.Y-150.f)>=450.f&&std::hypot(Pt.X-(-BossArenaHalf+450.f),Pt.Y+150.f)>=450.f,"cover keeps 450 cm clear of both player spawn points");
                Check(!(Pt.X>1300.f&&std::abs(Pt.Y)<450.f),"cover keeps the east door approach clear");
            }
        }
        Check(AnyCover,"every ward has real cover");
        for(std::size_t A=0;A<Props.size();++A) for(std::size_t B=A+1;B<Props.size();++B) {
            if(Props[A].Kind!=0||Props[B].Kind!=0||Props[A].Group==Props[B].Group) continue;
            Check(MinDistance(Props[A],Props[B])>=250.f,"cover pieces leave at least 250 cm for movement and navigation");
        }
    }
    // Ward 0: pillars must sit on the radial-shot rays of both phases (12-way = 30 deg, 18-way = 20 deg).
    int Pillars=0;
    for(const auto& P:BossProps(0)) if(P.Kind==0) {
        ++Pillars;const float Angle=std::atan2(P.Y,P.X)/Pi180(),R=std::hypot(P.X,P.Y);
        const float A=std::fmod(std::fmod(Angle,360.f)+360.f,360.f);
        Check(std::abs(R-800.f)<1.f&&(std::fmod(A+.01f,30.f)<.05f)&&(std::fmod(A+.01f,20.f)<.05f),"ward 0 pillars lie on both radial-shot ray sets");
    }
    Check(Pillars==6,"ward 0 has six pillars");
    // Ward 1: walls alternate sides so no straight lane crosses the arena unobstructed.
    int Above=0,Below=0;for(const auto& P:BossProps(1)) if(P.Kind==0) (P.Y>0?Above:Below)++;
    Check(Above==2&&Below==2,"ward 1 walls stagger across both sides");
    // Ward 3: four ring pieces leave four gates.
    int Ring=0;for(const auto& P:BossProps(3)) if(P.Kind==0&&std::hypot(P.X,P.Y)<800.f) ++Ring;
    Check(Ring==4,"ward 3 ring has four pieces, leaving four gates");
    Check(Boss(7)&&Boss(15)&&Boss(23)&&Boss(31)&&Ward(7)==0&&Ward(31)==3,"boss rooms map to wards 0..3");
    // Endless arena: large, enclosed, centre clear, deterministic, shuffles never bury players.
    {
        const auto L=GenerateEndless();
        Check(L.Cells.size()==1&&L.Exit==0&&L.Shape==EndlessShape,"endless arena is one enclosed cell");
        const float EndlessArea=float(2*EndlessHalf)*float(2*EndlessHalf),BossArea=float(2*BossArenaHalf)*float(2*BossArenaHalf);
        Check(EndlessArea/BossArea>=2.7f&&EndlessArea/BossArea<=3.3f,"endless arena is about three times a boss arena by area");
        std::array<KeepOut,2> None;
        for(std::uint32_t Seed=1;Seed<40;++Seed) {
            const auto A=EndlessProps(Seed,None),B=EndlessProps(Seed,None);
            Check(A.size()==B.size(),"endless props are deterministic");
            Check(A.size()>=40,"endless arena is full of scattered obstacles");
            for(std::size_t I=0;I<A.size();++I) {
                Check(A[I].X==B[I].X&&A[I].Y==B[I].Y&&A[I].Yaw==B[I].Yaw,"endless props are identical on server and client");
                Check(std::hypot(A[I].X,A[I].Y)>=EndlessCentreClear,"endless centre (chest and spawn) stays clear");
                Check(std::abs(A[I].X)<=EndlessHalf-300&&std::abs(A[I].Y)<=EndlessHalf-300,"endless obstacles stay inside the walls");
                for(std::size_t J=I+1;J<A.size();++J) {
                    const float Ri=.5f*std::hypot(A[I].Length,A[I].Thickness),Rj=.5f*std::hypot(A[J].Length,A[J].Thickness);
                    Check(std::hypot(A[I].X-A[J].X,A[I].Y-A[J].Y)>=Ri+Rj+EndlessGap-.01f,"endless obstacles leave a navigable gap");
                }
            }
            std::array<KeepOut,2> Keep;Keep[0]={1200.f,-900.f,true};Keep[1]={-2000.f,1500.f,true};
            for(const auto& O:EndlessProps(Seed,Keep)) for(const auto& K:Keep)
                Check(std::hypot(O.X-K.X,O.Y-K.Y)>=EndlessKeepRadius,"a shuffle never places cover on a player");
        }
        Check(EndlessProps(1,None).size()!=0&&(EndlessProps(1,None)[0].X!=EndlessProps(2,None)[0].X||EndlessProps(1,None)[0].Y!=EndlessProps(2,None)[0].Y),"different seeds give different layouts");
    }
    std::cout<<"PASS: "<<Checks<<" boss arena structure checks.\n";
}
