// Native tests for the engine-independent 0.24 enemy rules (ASEnemyRules.h) and the source meshes that must agree with them. Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASEnemyRules.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}

// ---- minimal OBJ reader (triangles only, as the source kit is) ----
struct Vec {double X=0,Y=0,Z=0;};
struct Obj {std::vector<Vec> V;std::vector<std::array<int,3>> F;};
static Obj Load(const std::string& Path) {
    Obj O;std::ifstream In(Path);std::string Line;
    while(std::getline(In,Line)) {
        std::istringstream S(Line);std::string Tag;S>>Tag;
        if(Tag=="v") {Vec P;S>>P.X>>P.Y>>P.Z;O.V.push_back(P);}
        else if(Tag=="f") {std::array<int,3> Tri{};for(int I=0;I<3;++I) {std::string Item;S>>Item;Tri[static_cast<size_t>(I)]=std::atoi(Item.c_str())-1;}O.F.push_back(Tri);}
    }
    return O;
}
static bool InTriangle(double Px,double Py,const Vec& A,const Vec& B,const Vec& C) {
    const auto Side=[](double X1,double Y1,double X2,double Y2,double X3,double Y3) {return (X1-X3)*(Y2-Y3)-(X2-X3)*(Y1-Y3);};
    const double D1=Side(Px,Py,A.X,A.Y,B.X,B.Y),D2=Side(Px,Py,B.X,B.Y,C.X,C.Y),D3=Side(Px,Py,C.X,C.Y,A.X,A.Y);
    const bool Neg=D1<0||D2<0||D3<0,Pos=D1>0||D2>0||D3>0;
    return !(Neg&&Pos);
}
// True when (X,Y) lies under any upward-facing top triangle of the mesh (all three corners at the top height).
static bool UnderTop(const Obj& O,double X,double Y,double TopZ) {
    for(const auto& Tri:O.F) {
        const Vec &A=O.V[static_cast<size_t>(Tri[0])],&B=O.V[static_cast<size_t>(Tri[1])],&C=O.V[static_cast<size_t>(Tri[2])];
        if(A.Z>TopZ-1e-3&&B.Z>TopZ-1e-3&&C.Z>TopZ-1e-3&&InTriangle(X,Y,A,B,C)) return true;
    }
    return false;
}

int main(int argc,char** argv) {
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
    {float Last=-100.f;bool Spaced=true;float NextSummon=0.f; // the cooldown alone spaces summons at least BroodSummonCooldown apart
        for(float Now=0.f;Now<600.f;Now+=.1f) if(BroodShouldSummon(0,BroodMinionCap,Now,NextSummon)) {if(Now-Last<BroodSummonCooldown-.001f) Spaced=false;Last=Now;NextSummon=Now+BroodSummonCooldown;}
        Check(Spaced,"summons are spaced by at least the cooldown");}

    // ---- Melee strike shape: the ground warning and the damage test are one rule ----
    Check(StrikeRadius(false)==210.f&&StrikeRadius(true)==340.f&&StrikeDamage(false)==12.f&&StrikeDamage(true)==24.f,"melee strike radius/damage keep the long-standing 210/340 and 12/24");
    Check(StrikeHits(100.f,.5f,210.f)&&StrikeHits(209.9f,.001f,210.f),"a hero in front and inside the radius is hit");
    Check(!StrikeHits(210.f,.5f,210.f)&&!StrikeHits(300.f,.9f,210.f),"a hero at or beyond the radius is not hit");
    Check(!StrikeHits(100.f,0.f,210.f)&&!StrikeHits(100.f,-.2f,210.f),"a hero beside or behind the enemy is not hit (the warning shows only the front half)");
    Check(!StrikeHits(NaN,.5f,210.f)&&!StrikeHits(100.f,NaN,210.f)&&!StrikeHits(100.f,.5f,NaN)&&!StrikeHits(Inf,.5f,210.f),"non-finite inputs never hit");
    Check(StrikeHits(339.f,.3f,StrikeRadius(true))&&!StrikeHits(339.f,.3f,StrikeRadius(false)),"the elite swing reaches further than the plain one");

    // ---- Source meshes must agree with the rules they illustrate ----
    if(argc>1) {
        const std::string Root=argv[1];
        const Obj Sector=Load(Root+"/SourceArt/Meshes/SM_StrikeSector.obj"),Arc=Load(Root+"/SourceArt/Meshes/SM_SwingArc.obj");
        Check(!Sector.V.empty()&&!Sector.F.empty()&&!Arc.V.empty()&&!Arc.F.empty(),"strike sector and swing arc meshes are present and non-empty");
        // SM_StrikeSector: radius 50 cm, never behind the enemy, and its top footprint equals the region StrikeHits accepts (checked on a grid, away from the edges).
        double MaxR=0,MinX=1e9,MaxY=-1e9,MinY=1e9;
        for(const auto& P:Sector.V) {MaxR=std::max(MaxR,std::hypot(P.X,P.Y));MinX=std::min(MinX,P.X);MaxY=std::max(MaxY,P.Y);MinY=std::min(MinY,P.Y);}
        Check(std::abs(MaxR-50.0)<.01,"the strike sector's outer edge is 50 cm, so scale = radius / 50");
        Check(MinX>-1e-4,"the strike sector never extends behind the enemy");
        Check(MaxY>49.99&&MinY<-49.99,"the strike sector spans the full +-90 degrees");
        int Compared=0;
        for(double X=-60;X<=60;X+=1.0) for(double Y=-60;Y<=60;Y+=1.0) {
            const double R=std::hypot(X,Y);
            if(std::abs(R-50.0)<1.5||std::abs(X)<1.5||R<1.0) continue; // skip the polygon edge, the centre line and the origin
            const bool Rule=StrikeHits(static_cast<float>(R),static_cast<float>(X/R),50.f);
            Check(UnderTop(Sector,X,Y,0.6)==Rule,"strike sector footprint equals the StrikeHits region");
            ++Compared;
        }
        Check(Compared>10000,"the footprint comparison covered the whole grid");
        // SM_SwingArc: outer radius 50 cm, never wider than the swing's real hit angle (dot > -0.05 -> 92.87 degrees), and it covers the whole frontal half.
        double ArcR=0,ArcAngle=0;
        for(const auto& P:Arc.V) {ArcR=std::max(ArcR,std::hypot(P.X,P.Y));ArcAngle=std::max(ArcAngle,std::abs(std::atan2(P.Y,P.X))*180.0/3.14159265358979);}
        Check(std::abs(ArcR-50.0)<.01,"the swing arc's outer edge is 50 cm, so scale = reach / 50");
        Check(ArcAngle<=92.87&&ArcAngle>=92.0,"the swing arc spans the swing's hit half-plane and not a degree more");
        for(int Deg=-90;Deg<=90;Deg+=3) {
            const double A=Deg*3.14159265358979/180.0;
            Check(UnderTop(Arc,49.5*std::cos(A),49.5*std::sin(A),1.6),"the swing arc covers the whole frontal half just inside its outer edge");
        }
    }
    std::printf("PASS: %lld enemy-rule checks (broodcaller cap/cooldown, melee strike shape, and the strike-sector and swing-arc meshes against those rules).\nEngine-independent rules and source geometry only; Unreal actors, animation, replication and gameplay are not executed.\n",Checks);
    return 0;
}
