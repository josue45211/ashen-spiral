// Native tests for the engine-independent 0.24 enemy rules (ASEnemyRules.h) and the source meshes that must agree with them. Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASEnemyRules.h"
#include "../Source/AshenSpiral/Public/ASExpeditionRules.h"
#include "../Source/AshenSpiral/Public/ASMeleeRules.h"
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

    // ---- Fliers: flight limits ----
    {
        const float BossHalf=187.5f*2.15f*.5f; // the flying boss's capsule half-height (ASEnemy::RefreshLook)
        const float Low=BossHalf+FlightFloorClearance;
        Check(ClampFlightHeight(0.f,0.f,BossHalf)==Low&&ClampFlightHeight(-5000.f,0.f,BossHalf)==Low,"a flier's centre is lifted until its capsule clears the floor");
        Check(ClampFlightHeight(400.f,0.f,BossHalf)==400.f,"a legal height is left alone");
        Check(ClampFlightHeight(9000.f,0.f,BossHalf)==FlightCeiling&&ClampFlightHeight(1.e9f,0.f,BossHalf)==FlightCeiling,"a flier cannot climb above the ceiling");
        Check(ClampFlightHeight(NaN,0.f,BossHalf)==Low&&ClampFlightHeight(Inf,0.f,BossHalf)==FlightCeiling,"non-finite heights land on a legal height");
        Check(ClampFlightHeight(300.f,100.f,BossHalf)==std::max(300.f,100.f+Low),"the band follows the floor height");
        for(float Z=-3000.f;Z<=6000.f;Z+=37.f) {const float C=ClampFlightHeight(Z,0.f,BossHalf);Check(C>=Low&&C<=FlightCeiling&&ClampFlightHeight(C,0.f,BossHalf)==C,"height clamp stays in the band and is idempotent");}
        float X=9000.f,Y=-9000.f;ClampToRect(-2840.f,2840.f,-2840.f,2840.f,X,Y);
        Check(X==2840.f&&Y==-2840.f,"a point far outside the arena lands on its nearest edge");
        X=100.f;Y=-300.f;ClampToRect(-2840.f,2840.f,-2840.f,2840.f,X,Y);Check(X==100.f&&Y==-300.f,"a point inside the arena is not moved");
        X=NaN;Y=Inf;ClampToRect(-10.f,30.f,-20.f,40.f,X,Y);Check(X==10.f&&Y==40.f,"non-finite coordinates land at the rectangle's centre or edge, never crash");
        X=0.f;Y=0.f;ClampToRect(50.f,-50.f,10.f,10.f,X,Y);Check(std::isfinite(X)&&std::isfinite(Y),"an inverted rectangle is normalised");
        // The Endless arena is +-3100 cm; a flier steering for a point past the edge is always brought back inside the margin.
        unsigned Seed=7;auto Rand=[&Seed]() {Seed=Seed*1664525u+1013904223u;return float((Seed>>8)&0xFFFF)/65535.f;};
        for(int I=0;I<5000;++I) {float Px=(Rand()-.5f)*20000.f,Py=(Rand()-.5f)*20000.f;ClampToRect(-3100.f+FlightEdgeMargin,3100.f-FlightEdgeMargin,-3100.f+FlightEdgeMargin,3100.f-FlightEdgeMargin,Px,Py);
            Check(std::fabs(Px)<=3100.f-FlightEdgeMargin+.01f&&std::fabs(Py)<=3100.f-FlightEdgeMargin+.01f,"a confined flier is always inside the Endless arena margin");}

        // ---- The flying boss must be reachable by melee ----
        Check(ASExpedition::FlyingBossHover>=Low,"the boss hover keeps its centre above the minimum height its own capsule needs (otherwise the floor clamp would fight the hover)");
        Check(ASExpedition::FlyingBossHover-88.f+18.f<ASMelee::FlierVerticalReach,"the boss hovers within the flier vertical melee reach of a standing player (bob included)");
        Check(ASMelee::VerticalReachFor(true)>ASMelee::VerticalReachFor(false)&&ASMelee::VerticalReachFor(false)==230.f,"fliers get extra vertical reach; everything else keeps 230 cm");
        Check(ASMelee::TargetInReach(100.f,280.f,40.f,229.f,1.f,.3f)&&!ASMelee::TargetInReach(100.f,280.f,40.f,231.f,1.f,.3f),"the default vertical limit is still 230 cm");
        Check(ASMelee::TargetInReach(100.f,280.f,40.f,300.f,1.f,.3f,ASMelee::VerticalReachFor(true))&&!ASMelee::TargetInReach(100.f,280.f,40.f,330.f,1.f,.3f,ASMelee::VerticalReachFor(true)),"a flier within 320 cm above is in reach, beyond it is not");
        Check(ASExpedition::FlyingBossOrbitMin<=280.f+80.f,"at the near end of its orbit the boss is inside a basic swing's reach plus its own radius");
        Check(ASExpedition::FlierOrbitMin<=280.f+40.f,"a Gloomwing's near orbit is inside a basic swing's reach plus its radius");
        Check(OrbitRadius(0.f,330.f,760.f,9.f)==330.f&&std::fabs(OrbitRadius(4.5f,330.f,760.f,9.f)-760.f)<.01f&&std::fabs(OrbitRadius(9.f,330.f,760.f,9.f)-330.f)<.01f,"the orbit radius starts near, peaks half a cycle later and returns");
        int Close=0,Total=0;for(float T=0.f;T<9.f;T+=.01f) {++Total;if(OrbitRadius(T,330.f,760.f,9.f)<=360.f) ++Close;}
        Check(Close>=Total/10,"the boss spends a real share of each cycle within melee reach");
        Check(OrbitRadius(NaN,330.f,760.f,9.f)==760.f&&OrbitRadius(1.f,330.f,760.f,0.f)==760.f,"a bad time or cycle length falls back to the far orbit");
    }

    // ---- Flying boss: the molt ----
    Check(MoltDue(false,50.f,100.f)&&MoltDue(false,1.f,100.f)&&!MoltDue(false,51.f,100.f),"the molt triggers at half health");
    Check(!MoltDue(true,10.f,100.f)&&!MoltDue(false,0.f,100.f)&&!MoltDue(false,-5.f,100.f),"the molt happens once and never on a dead boss");
    Check(!MoltDue(false,NaN,100.f)&&!MoltDue(false,50.f,NaN)&&!MoltDue(false,50.f,0.f)&&!MoltDue(false,Inf,100.f),"bad health values never trigger the molt");
    Check(MoltHatchlings(1)==4&&MoltHatchlings(2)==6&&MoltHatchlings(0)==4,"four hatchlings solo, six in co-op");
    Check(MoltWindupSeconds>1.f&&MoltWindupSeconds<3.f&&HatchlingHealthScale>0.f&&HatchlingHealthScale<1.f,"the molt scream is a readable beat and hatchlings are weaker than a full Gloomwing");
    Check(AbilityNone==0&&AbilityDive!=AbilityMolt&&AbilityDive>0&&AbilityMolt>0,"ability ids are distinct and non-zero");

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
    std::printf("PASS: %lld enemy-rule checks (broodcaller cap/cooldown, melee strike shape, flier limits, flying-boss reach and molt, and the strike-sector and swing-arc meshes against those rules).\nEngine-independent rules and source geometry only; Unreal actors, animation, replication and gameplay are not executed.\n",Checks);
    return 0;
}
