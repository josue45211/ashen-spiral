#include "../Source/AshenSpiral/Public/ASRules.h"
#include "../Source/AshenSpiral/Public/ASCareerRules.h"
#include "../Source/AshenSpiral/Public/ASNavigationRules.h"
#include "../Source/AshenSpiral/Public/ASAnimationRules.h"
#include "../Source/AshenSpiral/Public/ASAdventureRules.h"
#include <iostream>
#include <climits>
#include <cstdlib>
static int Checks=0;
static void Check(bool B,const char* M) {++Checks;if(!B){std::cerr<<"FAILED: "<<M<<'\n';std::exit(1);}}
static bool Near(double A,double B,double Tol=.001) {return std::abs(A-B)<Tol;}
static float Edge(const ASRules::NavGrid& G,const std::vector<ASNavigation::Link>& L,int A,int B) {
    float C=G.CanStep(A,B)?1.f:1e9f;
    for(auto X:L) if((X.A==A&&X.B==B)||(X.A==B&&X.B==A)) if(X.Cost>0&&std::isfinite(X.Cost)) C=std::min(C,X.Cost);
    return C;
}
static void OracleCheck(const ASRules::NavGrid& G,const std::vector<ASNavigation::Link>& Links,const std::vector<ASNavigation::Endpoint>& S,const std::vector<ASNavigation::Endpoint>& T) {
    const int N=int(G.Open.size());std::vector<float> D(N*N,1e9f);
    for(int A=0;A<N;++A) for(int B=0;B<N;++B) if(G.Valid(A)&&G.Valid(B)) D[A*N+B]=A==B?0:Edge(G,Links,A,B);
    for(int K=0;K<N;++K) for(int A=0;A<N;++A) for(int B=0;B<N;++B) D[A*N+B]=std::min(D[A*N+B],D[A*N+K]+D[K*N+B]);
    float Want=1e9f;for(auto A:S) for(auto B:T) if(G.Valid(A.Node)&&G.Valid(B.Node)&&A.Cost>=0&&B.Cost>=0) Want=std::min(Want,A.Cost+D[A.Node*N+B.Node]+B.Cost);
    const auto Path=ASNavigation::GuidePath(G,S,T,Links);
    if(Want>=1e8f) {Check(Path.empty(),"unreachable goal ring yields no false success");return;}
    Check(!Path.empty(),"reachable goal ring yields a route");float Entry=1e9f,Exit=1e9f,Cost=0;
    for(auto A:S) if(A.Node==Path.front()) Entry=std::min(Entry,A.Cost);
    for(auto B:T) if(B.Node==Path.back()) Exit=std::min(Exit,B.Cost);
    for(std::size_t I=1;I<Path.size();++I) Cost+=Edge(G,Links,Path[I-1],Path[I]);
    Check(Near(Cost+Entry+Exit,Want),"multi-endpoint weighted route matches independent all-pairs oracle");
}
static void Seed6767() {
    // Exact deterministic layout, abstract floor/corridor support. This does not
    // emulate Unreal mesh collision, stalactites, props or live player movement.
    const auto L=ASRules::Generate(6767u+22u*104729u);
    Check(L.Shape==3&&L.Exit==5&&L.Cells.size()==8,"seed 6767 chamber 23 topology fingerprint");
    const auto Floor=[&](float X,float Y) {
        for(auto C:L.Cells) if(std::abs(X-C.WorldX)<=C.HalfX-38&&std::abs(Y-C.WorldY)<=C.HalfY-38) return true;
        for(auto E:L.Edges) if(!E.Gap) {
            auto A=L.Cells[E.A],B=L.Cells[E.B];
            if(A.WorldY==B.WorldY&&std::abs(Y-A.WorldY)<=182&&X>=std::min(A.WorldX,B.WorldX)&&X<=std::max(A.WorldX,B.WorldX)) return true;
            if(A.WorldX==B.WorldX&&std::abs(X-A.WorldX)<=182&&Y>=std::min(A.WorldY,B.WorldY)&&Y<=std::max(A.WorldY,B.WorldY)) return true;
        }
        return false;
    };
    ASRules::NavGrid G;G.Width=100;G.Height=100;G.Open.resize(10000);
    const auto XY=[](int I){return std::pair<float,float>{float((I%100)*120-1800),float((I/100)*120-1800)};};
    for(int I=0;I<10000;++I) {auto P=XY(I);G.Open[I]=Floor(P.first,P.second);}
    const float GoalX=L.Cells[L.Exit].WorldX-220.f,GoalY=float(L.Cells[L.Exit].WorldY);
    std::vector<ASNavigation::Endpoint> Goals;
    for(int I=0;I<10000;++I) {auto P=XY(I);if(G.Valid(I)&&std::hypot(P.first-GoalX,P.second-GoalY)<=280) Goals.push_back({I,0});}
    for(auto C:L.Cells) {
        std::vector<ASNavigation::Endpoint> Starts;
        for(int I=0;I<10000;++I) {auto P=XY(I);if(G.Valid(I)&&std::hypot(P.first-C.WorldX,P.second-C.WorldY)<180) Starts.push_back({I,float(std::hypot(P.first-C.WorldX,P.second-C.WorldY)/120)});}
        auto P=ASNavigation::GuidePath(G,Starts,Goals,{});Check(!P.empty(),"seed 6767 chest approach reachable from every room in floor model");
        auto End=XY(P.back());Check(std::hypot(End.first-GoalX,End.second-GoalY)<=280,"seed route ends inside chest interaction reach");
        for(std::size_t I=1;I<P.size();++I) Check(G.CanStep(P[I-1],P[I]),"seed route stays on supported room/corridor floor");
    }
}
int main() {
    for(int Mask:{0,1<<9,4095}) for(int Points:{0,4,1000000,INT_MAX}) {
        int M=Mask,P=Points,S=17,C=29,V=41;ASRules::RetireUnusedProgress(M,P,S,C,V);
        Check(!(M&(1<<9))&&S==87&&C==0&&V==0,"old shard/fiber balances convert 1:1 and retired talent clears");
        const int Paid=P;ASRules::RetireUnusedProgress(M,P,S,C,V);Check(P==Paid&&S==87,"repeat sanitation cannot refund or convert twice");
        Check((M&~(1<<9))==(Mask&~(1<<9)),"all unrelated owned talents retain their stable bits");
    }
    {int M=1<<9,P=INT_MAX,S=INT_MAX,C=INT_MAX,V=INT_MAX;ASRules::RetireUnusedProgress(M,P,S,C,V);Check(P==1000000&&S==ASRules::MaxCurrency,"corrupt large saves saturate without integer overflow");}
    for(int Ward=0;Ward<4;++Ward) Check(ASRules::MaterialForWard(Ward)==(Ward==1?1:0),"drop tables never pay removed material slots");
    for(float Shield:{0.f,55.f,60.f,80.f,150.f}) for(float Gain:{0.f,10.f,12.f,22.f}) {
        float After=ASRules::GrantShield(Shield,Gain,60.f);Check(After>=Shield&&After<=Shield+Gain,"capped grants never subtract existing shield");
    }
    int PaidLevel=1,Embers=0,Tablets=0;
    for(int Level=1;Level<=100;++Level) {auto R=ASCareer::ClaimLevelReward(Level,PaidLevel);Embers+=R.Embers;Tablets+=R.Tablets;auto Again=ASCareer::ClaimLevelReward(Level,PaidLevel);Check(Again.Embers==0&&Again.Tablets==0,"replayed level reward does not duplicate payment");}
    Check(Embers==20&&Tablets==10,"levels 1 to 100 pay only intended small milestone rewards");
    int OldPaid=1;auto Retro=ASCareer::ClaimLevelReward(100,OldPaid);Check(Retro.Embers==Embers&&Retro.Tablets==Tablets,"old-profile catch-up equals gradual level progression");
    for(int I=0;I<ASCareer::Count;++I) {
        int Paid=0,E=0,T=0;for(auto Goal:ASCareer::Achievements[I].Goals) {auto R=ASCareer::ClaimTierReward(Goal,I,Paid);E+=R.Embers;T+=R.Tablets;auto A=ASCareer::ClaimTierReward(Goal,I,Paid);Check(A.Embers==0&&A.Tablets==0,"duplicate tier claim pays nothing");}
        Check(E==3&&T==1,"each complete achievement pays three embers and one tablet");
        Paid=0;auto R=ASCareer::ClaimTierReward(ASCareer::Achievements[I].Goals[2],I,Paid);Check(R.Embers==3&&R.Tablets==1,"tier jumps catch up all skipped rewards exactly once");
    }
    for(int I=0;I<20;++I) Check(ASRules::SideTrapKind(I)==(I%2?5:4),"sequential side-trap spawns include rotating blades");
    for(float A:{25.f,38.f,42.f}) for(float B:{30.f,38.f,42.f}) for(int I=0;I<=1000;++I) {
        const auto K=ASAnimation::TwoBone(I*.15f,A,B);
        Check(Near(std::hypot(K.Along,K.Out),A,.003)&&Near(std::hypot(K.Reach-K.Along,K.Out),B,.003),"two-bone solve preserves both segment lengths across unreachable targets");
    }
    for(int I=0;I<500;++I) {const auto F=ASAnimation::Foot(I*.03f,1);Check(F.Lift>=0&&F.Lift<=24.001&&std::abs(F.Forward)<=34.001,"gait keeps stance planted and swing within anatomical reach");}
    Check(Near(std::pow(1-ASAnimation::Blend(1.f/30,12),30),std::pow(1-ASAnimation::Blend(1.f/120,12),120),1e-5),"animation blending is frame-rate independent");
    Check(ASAdventure::NovaAreaRadius(330,true)==630&&ASAdventure::NovaAreaRadius(320,false)==320,"Nova expands existing area skills without changing ordinary casts");
    ASRules::Random R(200020);
    for(int Trial=0;Trial<400;++Trial) {
        ASRules::NavGrid G;G.Width=5;G.Height=5;G.Open.resize(25);for(auto& O:G.Open) O=R.Range(4)>0;
        std::vector<ASNavigation::Link> Links;for(int I=0;I<4;++I) Links.push_back({R.Range(25),R.Range(25),.5f+R.Range(10)});
        OracleCheck(G,Links,{{R.Range(25),float(R.Range(4))},{R.Range(25),float(R.Range(4))}},{{R.Range(25),float(R.Range(4))},{R.Range(25),float(R.Range(4))}});
    }
    Seed6767();
    std::cout<<"PASS: "<<Checks<<" 0.20 regression checks; migrations, one-time milestone rewards, shield grants, traps, joint lengths, 400 multi-endpoint route oracles and seed 6767 chamber 23 floor model.\nUnreal save serialization, mesh collision, Slate, animation rendering and networking are not executed.\n";
}
