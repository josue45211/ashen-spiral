#include "../Source/AshenSpiral/Public/ASArsenalRules.h"
#include "../Source/AshenSpiral/Public/ASBuildRules.h"
#include "../Source/AshenSpiral/Public/ASNavigationRules.h"
#include "../Source/AshenSpiral/Public/ASEffectArt.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
static int Checks=0;
static void Check(bool OK,const char* Why) {++Checks;if(!OK){std::cerr<<"FAILED: "<<Why<<'\n';std::exit(1);}}
static bool Near(float A,float B) {return std::abs(A-B)<.001f;}
static float EdgeCost(const ASRules::NavGrid& G,const std::vector<ASNavigation::Link>& Links,int A,int B) {
    float Cost=G.CanStep(A,B)?1.f:1e9f;
    for(const auto& L:Links) if(G.Valid(A)&&G.Valid(B)&&std::isfinite(L.Cost)&&L.Cost>0&&((L.A==A&&L.B==B)||(L.B==A&&L.A==B))) Cost=std::min(Cost,L.Cost);
    return Cost;
}
// Independent all-pairs Floyd-Warshall oracle; no priority queue or path parents.
static float Oracle(const ASRules::NavGrid& G,const std::vector<ASNavigation::Link>& Links,int Start,int Goal) {
    if(!G.Valid(Start)||!G.Valid(Goal)) return 1e9f;
    const int N=int(G.Open.size());std::vector<float> D(N*N,1e9f);
    for(int A=0;A<N;++A) for(int B=0;B<N;++B) D[A*N+B]=A==B?0:EdgeCost(G,Links,A,B);
    for(int K=0;K<N;++K) for(int A=0;A<N;++A) for(int B=0;B<N;++B) D[A*N+B]=std::min(D[A*N+B],D[A*N+K]+D[K*N+B]);
    return D[Start*N+Goal];
}
static void VerifyRoute(const ASRules::NavGrid& G,const std::vector<ASNavigation::Link>& Links,int A,int B) {
    const float Want=Oracle(G,Links,A,B);const auto Path=ASNavigation::GuidePath(G,A,B,Links);
    if(Want>=1e8f) {Check(Path.empty(),"unreachable destination returns no misleading route");return;}
    Check(!Path.empty()&&Path.front()==A&&Path.back()==B,"route reaches exact endpoints");float Actual=0;
    for(std::size_t I=1;I<Path.size();++I) {const float C=EdgeCost(G,Links,Path[I-1],Path[I]);Check(C<1e8f,"every segment is a legal walk or validated jump");Actual+=C;}
    Check(Near(Actual,Want),"jump-aware route has minimum distance, not fewest segments");
}
int main() {
    using namespace ASArsenal;
    for(int W=0;W<9;++W) {
        const int Base=BaseCapacity(W);int Ammo=Base;
        if(!UsesMagazine(W)) {
            Check(Base==0&&Capacity(W,5)==0&&ReloadSeconds(W,5)==0,"melee has no magazine or reload timer at any rank");
            Check(!CanReload(0,Base,false,true),"melee rejects reload");continue;
        }
        Check(!CanReload(Ammo,Base,false,true),"full magazines cannot restart reloads");
        for(int I=0;I<Base;++I) Check(Consume(Ammo,false),"exactly one full magazine can fire");
        Check(Ammo==0&&!Consume(Ammo,false)&&Ammo==0,"empty attacks never create ammunition or go negative");
        Check(CanReload(Ammo,Base,false,true),"empty magazine can reload");
        Check(!CanReload(Ammo,Base,true,true)&&!CanReload(Ammo,Base,false,false),"reload spam and blocked phases are rejected");
        Ammo=Base-1;Check(CanReload(Ammo,Base,false,true),"manual partial reload permitted");
        Check(!Consume(Ammo,true)&&Ammo==Base-1,"reloading suppresses primary attacks without losing ammunition");
        for(int Rank=0;Rank<5;++Rank) {
            Check(Capacity(W,Rank+1)>Capacity(W,Rank),"capacity improves at every pick, including small magazines");
            Check(ReloadSeconds(W,Rank+1)<ReloadSeconds(W,Rank),"reload-speed picks always reduce downtime");
        }
        Check(Capacity(W,5)>=Base*2&&Capacity(W,5)<=Base*3&&Capacity(W,999)==Capacity(W,5),"capacity stays bounded and stops improving after five picks");
        Check(Capacity(W,-1)==Base&&Near(ReloadSeconds(W,999),ReloadSeconds(W,5)),"corrupt ranks clamp safely");
        const float Sustain=Damage(W)*Pellets(W)*Base/(Base*Rate(W)+ReloadSeconds(W));
        Check(Sustain>45&&Sustain<105,"unmodified sustained damage stays in initial balance envelope before unique effects");
        const float End=10+ReloadSeconds(W);Check(!ReloadComplete(End,End-.01f)&&ReloadComplete(End,End),"no ammunition refill before deadline");
        Check(Near(ReloadProgress(10,End,10),0)&&Near(ReloadProgress(10,End,(10+End)*.5f),.5f)&&Near(ReloadProgress(10,End,End+4),1),"reload display is normalized and bounded");
    }
    Check(Capacity(8,5)==8&&Capacity(1,5)==0&&Capacity(7,5)==80,"melee remains magazine-free and rotary gains eight");
    Check(!ReloadComplete(0,100),"inactive timer does not silently refill weapons");
    for(int R=0;R<5;++R) Check(SpreadScale(R+1)<SpreadScale(R),"all shared spread patterns tighten at every rank");
    Check(Near(24*SpreadScale(5),6)&&Near(5*SpreadScale(5),1.25f)&&Near(SpreadScale(20),.25f),"split and echo offsets use same capped 75 percent reduction");
    Check(Near(EnemySpeedScale(false),1.25f)&&Near(EnemySpeedScale(true),1.35f),"normal and elite/boss base speeds have requested multipliers");
    ASBuild::Context C;C.Hurt=true;C.Curse=1;C.Room=8;
    using ASBuild::Trait;
    for(int W=0;W<9;++W) {C.Weapon=W;
        Check(!ASBuild::CanChoose(C,255),"retired Vault Sense never offered");
        Check(ASBuild::CanChoose(C,263)==UsesMagazine(W)&&ASBuild::CanChoose(C,264)==UsesMagazine(W),"reload traits only appear for ranged weapons");
        Check(ASBuild::CanChoose(C,265)==((476&(1<<W))!=0),"precision only offered to projectile weapons");
    }
    for(int I=63;I<66;++I) {C.Traits[I]=1;Check(!ASBuild::CanEvolve(C,I)&&!ASBuild::Evolved(~std::uint64_t(0),I),"utility ranks cannot overflow legacy evolution bits");}
    C.Traits.fill(0);C.Weapon=7;
    for(int I=0;I<ASBuild::Count;++I) {
        Check(std::strlen(ASBuild::Briefs[I])<=44,"default card descriptions remain concise");
        C.Challenge=0;const int Base=ASBuild::Weight(C,200+I);
        for(int Tier=1;Tier<=3;++Tier) {C.Challenge=Tier;const int Now=ASBuild::Weight(C,200+I);Check(ASBuild::Catalog[I].Rarity?Now>Base:Now==Base,"earned stakes only increases rare and legendary weights");}
    }
    int Rare[2]={},Legendary[2]={};
    for(int Level=0;Level<2;++Level) {C.Challenge=Level?3:0;
        for(int Seed=1;Seed<=20000;++Seed) for(int Offer:ASBuild::Offers(C,Seed)) if(ASBuild::IsTrait(Offer)) {
            const int R=ASBuild::Catalog[Offer-200].Rarity;Rare[Level]+=R==1;Legendary[Level]+=R==2;Check(Offer!=255,"large reward sample never offers Vault Sense");
        }
    }
    Check(Rare[1]>Rare[0]&&Legendary[1]>Legendary[0],"actual reward drafts improve both rare and legendary frequency after stakes clears");
    ASRules::NavGrid Gap;Gap.Width=7;Gap.Height=1;Gap.Open={1,1,1,0,1,1,1};
    VerifyRoute(Gap,{},0,6);VerifyRoute(Gap,{{2,4,2}},0,6);VerifyRoute(Gap,{{2,4,2}},6,0);
    ASRules::NavGrid Walk;Walk.Width=5;Walk.Height=2;Walk.Open.assign(10,1);
    VerifyRoute(Walk,{{0,4,10}},0,4);VerifyRoute(Walk,{{0,4,2}},0,4);
    VerifyRoute(Walk,{{0,4,-2},{0,4,std::numeric_limits<float>::quiet_NaN()},{0,100,1}},0,4);
    ASRules::Random Rand(190019);
    for(int Trial=0;Trial<1200;++Trial) {
        ASRules::NavGrid G;G.Width=5;G.Height=5;G.Open.assign(25,1);
        for(auto& O:G.Open) O=Rand.Range(5)>0;
        for(int A=0;A<25;++A) if(Rand.Range(4)==0) G.BlockEdge(A,A+1);
        std::vector<ASNavigation::Link> Links;for(int I=0;I<4;++I) Links.push_back({Rand.Range(25),Rand.Range(25),.5f+Rand.Range(20)*.5f});
        VerifyRoute(G,Links,Rand.Range(25),Rand.Range(25));
    }
    for(int Kind=0;Kind<16;++Kind) for(const auto& Stroke:ASEffectArt::Draw(Kind)) for(const auto& P:Stroke.Points) Check(std::isfinite(P.X)&&std::isfinite(P.Y)&&P.X>=0&&P.X<=100&&P.Y>=0&&P.Y<=100,"effect illustrations remain inside their card viewport");
    std::cout<<"PASS: "<<Checks<<" reload/navigation checks; 1200 route-oracle comparisons; 40000 reward drafts.\n";
    std::cout<<"Stakes tier 0 -> 3: rare cards "<<Rare[0]<<" -> "<<Rare[1]<<", legendary cards "<<Legendary[0]<<" -> "<<Legendary[1]<<" across 60000 choices per tier.\nShared rules only; Unreal actors, audio, physics, Slate and replication not executed.\n";
}
