// Native tests for 0.25 phase C: the Endless arena's decor ring (ASDecorRules.h). Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASDecorRules.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
using namespace ASDecor;
int main() {
    const auto& Table=Kits();
    // ---- The kit table ----
    std::set<std::string> Names;int PerStage[StageCount]={0,0,0};
    for(const auto& K:Table) {
        Check(K.Mesh&&std::strncmp(K.Mesh,"SM_DC_",6)==0,"every kit names a Dragon Cave mesh (SM_DC_ prefix)");
        Check(Names.insert(K.Mesh).second,"no mesh is listed twice");
        Check(K.Stage>=0&&K.Stage<StageCount,"every kit belongs to a stage");
        Check(K.Height>=300.f&&K.Height<=1600.f,"decor stands from a few metres to a few storeys tall");
        Check(K.Radius>=80.f&&K.Radius<=450.f,"every kit has a believable footprint");
        Check(K.Weight>=1,"every kit can be picked");
        ++PerStage[K.Stage];
    }
    for(int S=0;S<StageCount;++S) Check(PerStage[S]>=6,"every stage has a real choice of scenery");
    // ---- The ring ----
    long long Instances=0;
    for(std::uint32_t Seed=0;Seed<400;++Seed) {
        const auto Ring1=Ring(Seed),Ring2=Ring(Seed);
        Check(Ring1.size()==size_t(StageCounts[0]+StageCounts[1]+StageCounts[2]),"the ring always fills every stage's count");
        Check(Ring1.size()==Ring2.size(),"the same seed gives the same count");
        for(size_t I=0;I<Ring1.size();++I) {
            const auto &A=Ring1[I],&B=Ring2[I];
            Check(A.Kit==B.Kit&&A.X==B.X&&A.Y==B.Y&&A.Yaw==B.Yaw&&A.Scale==B.Scale,"the same seed gives the identical placement on every machine");
            Check(A.Kit>=0&&A.Kit<int(Table.size()),"a placement names a real kit");
            Check(std::isfinite(A.X)&&std::isfinite(A.Y)&&std::isfinite(A.Yaw)&&std::isfinite(A.Scale),"placements are finite");
            const float Edge=std::max(std::fabs(A.X),std::fabs(A.Y));
            Check(Edge>=RingInner&&Edge<=RingOuter+.01f,"decor stands in the ring beyond the walls");
            Check(Edge>ArenaHalf+300.f,"and never inside, or against, the arena walls");
            Check(A.Scale>=ScaleMin-.001f&&A.Scale<=ScaleMax+.001f,"scale jitter stays in its range");
            Check(A.Yaw>=0.f&&A.Yaw<=360.f,"yaw is a full turn");
        }
        // stage order and counts: stage 0 first, then 1, then 2
        int Seen[StageCount]={0,0,0};int Last=0;
        for(const auto& P:Ring1) {const int St=Table[size_t(P.Kit)].Stage;Check(St>=Last,"placements are listed stage by stage");Last=St;++Seen[St];}
        for(int S=0;S<StageCount;++S) Check(Seen[S]==StageCounts[size_t(S)],"each stage gets exactly its count");
        // clear of each other
        for(size_t I=0;I<Ring1.size();++I) for(size_t J=I+1;J<Ring1.size();++J) {
            const auto &A=Ring1[I],&B=Ring1[J];
            const float Need=(Table[size_t(A.Kit)].Radius*A.Scale+Table[size_t(B.Kit)].Radius*B.Scale)*.6f;
            Check(std::hypot(A.X-B.X,A.Y-B.Y)>=Need-.01f,"no two pieces of scenery overlap");
        }
        Instances=std::max<long long>(Instances,(long long)Ring1.size());
    }
    Check(Instances<=600,"the ring stays inside the plan's 600-instance budget");
    // ---- Different arenas look different ----
    {const auto A=Ring(1),B=Ring(2);bool Differ=false;for(size_t I=0;I<A.size()&&I<B.size();++I) if(A[I].X!=B[I].X||A[I].Kit!=B[I].Kit) Differ=true;Check(Differ,"another seed gives another ring");}
    // ---- Which stage shows when ----
    for(int Sky=0;Sky<StageCount;++Sky) for(int Decor=0;Decor<StageCount;++Decor) Check(StageVisible(Decor,Sky)==(Sky>=Decor),"a stage of decor shows from its own sky stage on");
    Check(!StageVisible(-1,2)&&!StageVisible(StageCount,2)&&!StageVisible(0,-1),"out-of-range stages never show");
    std::cout<<"PASS: "<<Checks<<" decor checks; kit table, deterministic ring (400 seeds), ring bounds, spacing, stage counts and order, stage visibility.\nShared rules only; the imported meshes, their scale, and how the ring looks are Unreal gameplay: UNVERIFIED.\n";
}
