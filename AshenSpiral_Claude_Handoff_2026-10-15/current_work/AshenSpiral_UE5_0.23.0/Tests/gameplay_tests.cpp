#include "../Source/AshenSpiral/Public/ASRules.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>

static int Checks=0;
static void Check(bool OK,const char* What) {
    ++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}
}
static bool Near(float A,float B) {return std::abs(A-B)<.0001f;}
struct Point {double X,Y,Z;};
static std::vector<Point> Mesh(const std::string& Path,const std::string& Wanted="") {
    std::ifstream Input(Path);Check(bool(Input),"weapon OBJ opens");std::vector<Point> Points;std::string Line;
    std::string Group;
    while(std::getline(Input,Line)) {
        if(Line.rfind("g ",0)==0) Group=Line.substr(2);
        if(Line.rfind("v ",0)==0&&(Wanted.empty()||Group==Wanted)) {
            std::istringstream Row(Line.substr(2));Point P{};Row>>P.X>>P.Y>>P.Z;Check(bool(Row),"valid OBJ vertex");Points.push_back(P);
        }
    }
    return Points;
}

static void ExpansionChecks() {
    using namespace ASRules;
    // Compare A* to a separate breadth-first distance oracle, including narrow
    // barriers between otherwise clear endpoints and disconnected spawn islands.
    Random RNG(91307);
    for(int Sample=0;Sample<160;++Sample) {
        NavGrid G;G.Width=1+RNG.Range(22);G.Height=1+RNG.Range(22);
        const int N=G.Width*G.Height;G.Open.assign(N,1);
        std::set<std::pair<int,int>> Walls;
        for(int I=1;I<N;++I) G.Open[I]=RNG.Range(5)!=0;
        for(int I=0;I<N;++I) for(int J:{I+1,I+G.Width}) {
            if(J>=N||!G.Open[I]||!G.Open[J]) continue;
            if(std::abs(J%G.Width-I%G.Width)+std::abs(J/G.Width-I/G.Width)!=1) continue;
            if(RNG.Range(8)==0) {Walls.insert({I,J});G.BlockEdge(I,J);}
        }
        auto Oracle=[&](int Start) {
            std::vector<int> Dist(N,-1),Queue{Start};Dist[Start]=0;
            for(std::size_t K=0;K<Queue.size();++K) {
                const int A=Queue[K];
                for(int D=0;D<4;++D) {
                    const int X=A%G.Width+(D==0?1:(D==1?-1:0));
                    const int Y=A/G.Width+(D==2?1:(D==3?-1:0));
                    if(X<0||Y<0||X>=G.Width||Y>=G.Height) continue;
                    const int B=Y*G.Width+X;
                    if(!G.Open[B]||Dist[B]>=0||Walls.count({std::min(A,B),std::max(A,B)})) continue;
                    Dist[B]=Dist[A]+1;Queue.push_back(B);
                }
            }
            return Dist;
        };
        const auto Dist=Oracle(0);
        for(int End=0;End<N;++End) {
            const auto P=G.Path(0,End);
            Check(P.empty()==(Dist[End]<0),"path search agrees with independent reachability oracle");
            if(P.empty()) continue;
            Check(P.front()==0&&P.back()==End&&int(P.size())==Dist[End]+1,"route is shortest and ends at the requested cell");
            for(std::size_t I=1;I<P.size();++I) {
                const int A=P[I-1],B=P[I];
                Check(G.Open[B]&&std::abs(A%G.Width-B%G.Width)+std::abs(A/G.Width-B/G.Width)==1,"route never cuts diagonally or wraps grid rows");
                Check(!Walls.count({std::min(A,B),std::max(A,B)}),"route never crosses a swept obstruction");
            }
        }
        G.KeepConnected(0);
        for(int I=0;I<N;++I) Check(bool(G.Open[I])==(Dist[I]>=0),"spawn filtering retains exactly the player-connected region");
    }
    // Long routes must continue to the player, even when an initial detour moves
    // away from their straight-line direction around a distant wall.
    NavGrid Long;Long.Width=160;Long.Height=96;Long.Open.assign(Long.Width*Long.Height,1);
    for(int X=16;X<160;X+=16) for(int Y=0;Y<96;++Y) {
        const int Door=(X/16)%2?94:1;if(Y!=Door) Long.Open[Y*160+X]=0;
    }
    const auto Far=Long.Path(0,159);
    Check(!Far.empty()&&Far.front()==0&&Far.back()==159&&Far.size()>900,"large serpentine map reaches distant player without range cutoff");
    for(std::size_t I=1;I<Far.size();++I) Check(Long.CanStep(Far[I-1],Far[I]),"long pursuit respects every wall");
    Long.Open[94*160+16]=0;Check(Long.Path(0,159).empty(),"sealed route stays unavailable instead of passing through wall");
    NavGrid Invalid;Invalid.Open={1,1};
    Check(Invalid.Path(0,1).empty(),"malformed navigation dimensions are rejected");
    Invalid.KeepConnected(0);Check(Invalid.Open==std::vector<std::uint8_t>({0,0}),"invalid navigation cannot expose spawn positions");

    for(int Phase=-1;Phase<=6;++Phase) {
        Check(CanUseWeapons(Phase)==((Phase>=0&&Phase<=3)||Phase==5),"hub, inter-wave and victory phases allow weapons/dash; defeat does not");
        Check(CanGainCombatBenefits(Phase)==(Phase==2),"on-cast and on-hit benefits require combat");
        for(bool Practice:{false,true}) {
            Check(!CanDamageTarget(Phase,Practice,true),"cosmetic casts cannot acquire damage eligibility in a later phase");
            const bool Expected=Practice?Phase==0:Phase==2;
            Check(CanDamageTarget(Phase,Practice,false)==Expected,"practice effects and combat effects have separate targets and lifetimes");
        }
    }
    for(bool Used:{false,true}) {
        Check(CanAirDash(true,false,Used),"landing permits another dash, subject to the separate cooldown");
        Check(CanAirDash(false,true,Used)==!Used,"one air dash is allowed per airborne period");
        Check(!CanAirDash(false,false,Used),"custom movement cannot restart a dash mid-dash");
    }
    Check(Near(HealthBarAlpha(0),1)&&Near(HealthBarAlpha(3),1),"enemy bars hold full opacity through three seconds");
    Check(Near(HealthBarAlpha(4),.5f)&&Near(HealthBarAlpha(5),0)&&Near(HealthBarAlpha(20),0),"enemy bars fade between seconds three and five");
    Check(Near(HealthBarAlpha(std::numeric_limits<float>::quiet_NaN()),0),"invalid bar age is hidden");

    for(int FPS:{30,60,120}) {
        ElementState Burn,Poison;
        for(int I=0;I<20;++I) {Burn.Apply(2,1,false);Poison.Apply(3,1,false);}
        Check(Poison.PoisonStacks==3,"poison caps at three stacks even after repeated hits");
        float FireTotal=0,PoisonTotal=0;
        for(int I=0;I<FPS*6;++I) {FireTotal+=Burn.Tick(1.f/FPS).first;PoisonTotal+=Poison.Tick(1.f/FPS).second;}
        Check(std::abs(FireTotal-18)<.002f,"burn does not stack and expires after eighteen base damage");
        Check(std::abs(PoisonTotal-36)<.002f,"three poison stacks deal thirty-six total base damage");
        Check(Burn.BurnDPS==0&&Poison.PoisonDPS==0&&Poison.PoisonStacks==0,"expired DOT clears its damage and stack state");
    }
    ElementState Burst;Burst.Apply(2,2,false);Burst.Apply(3,2,false);
    auto D=Burst.Tick(30);
    Check(Near(D.first,36)&&Near(D.second,24),"a long frame never deals damage beyond the remaining DOT duration");
    ElementState Refresh;Refresh.Apply(2,1,false);Refresh.Tick(2);Refresh.Apply(2,1,false);
    Check(Near(Refresh.Burn,3)&&Near(Refresh.BurnDPS,6),"burn refresh extends duration without multiplying DPS");
    Refresh.Apply(2,2,false);Refresh.Apply(2,1,false);
    Check(Near(Refresh.BurnDPS,12),"a weaker refresh does not erase a stronger burn");
    Check(Near(Refresh.Tick(-1).first,0)&&Near(Refresh.Burn,3),"invalid time cannot advance a DOT");
    ElementState Frost;for(int I=0;I<3;++I) Frost.Apply(4,1,false);
    Check(Near(Frost.Frozen,.8f)&&Near(Frost.Immune,3.8f),"normal enemies receive a short freeze and a longer immunity window");
    Frost.Tick(.8f);for(int I=0;I<30;++I) Frost.Apply(4,1,false);
    Check(Frost.Frozen==0&&Frost.Immune>0,"repeated frost hits cannot chain-freeze through immunity");
    Frost.Tick(3.1f);Frost.Apply(4,1,false);Frost.Apply(4,1,false);
    Check(Frost.Frozen>0,"freeze can trigger again after immunity expires");
    ElementState Boss;for(int I=0;I<100;++I) Boss.Apply(4,1,true);
    Check(Boss.Frozen==0&&Boss.Frost>0,"bosses can be chilled but cannot be hard-frozen");
    Boss.Tick(10);Check(Boss.Frost==0&&Boss.Immune==0,"frost buildup and immunity decay completely");
}

int main(int Argc,char** Argv) {
    using namespace ASRules;
    Check(Argc==2,"pass project root for real source-geometry tests");
    ExpansionChecks();
    Check(Near(SpiralCooldown(0,0),2.2f*1.5f),"base ability cooldown is exactly 50 percent longer");
    Check(Near(SpiralCooldown(1,0),2.97f),"first permanent cooldown upgrade has its advertised effect");
    Check(Near(SpiralCooldown(0,1),3.036f),"first run cooldown upgrade has its advertised effect");
    Check(Near(SpiralCooldown(5,5),1.155f),"combined cooldown reduction respects 65 percent cap");
    Check(Near(AttackSpeed(1,1),1.20f),"permanent and run attack speed stack");
    Check(Near(DamageMultiplier(1,1),1.25f),"permanent and run damage stack");
    Check(Near(StartingHealth(3,2),164.f),"health rank migration and run health combine");
    Check(Near(MoveSpeed(1,1),704.f),"movement upgrades apply their stated percentages");
    Check(Near(CriticalChance(1,1),.14f)&&Near(CriticalChance(5,5),.50f),"critical chance starts at five percent and caps at fifty");
    for(int P=-2;P<=7;++P) for(int R=-2;R<=7;++R) {
        const float CD=SpiralCooldown(P,R);
        Check(CD>=1.1549f&&CD<=3.3001f,"cooldown stays bounded for invalid and valid ranks");
        Check(SpiralCooldown(P,R+1)<=CD+.0001f,"more run cooldown ranks never slow the ability");
        for(float Rate:{.32f,.65f,.30f,.48f,.34f,.40f,.72f,.105f,1.05f}) {
            const float Interval=AttackInterval(Rate,P,R);
            Check(Interval>0&&Interval<=Rate+.0001f,"all weapon attack intervals stay positive and accelerate");
            Check(AttackInterval(Rate,P,R+1)<=Interval+.0001f,"all weapons benefit monotonically from haste");
            Check(Near(AttackInterval(Rate,P,R,true)*1.25f,Interval),"party attunement composes with attack-speed upgrades");
        }
    }
    std::array<int,RunUpgradeCount> Ranks{};std::set<int> ThirdChoices;
    for(unsigned Seed=1;Seed<=64;++Seed) {
        const auto Offers=RunUpgradeOffers(Ranks,Seed);
        Check(Offers.size()==3,"fresh runs get three stat alternatives");
        Check(Offers[0]==100&&Offers[1]==101,"cooldown and attack speed are discoverable immediately");
        Check(std::set<int>(Offers.begin(),Offers.end()).size()==Offers.size(),"no duplicate stat options in a chest");
        ThirdChoices.insert(Offers[2]);
        Check(Offers==RunUpgradeOffers(Ranks,Seed),"identical offer seeds reproduce the same choices");
    }
    Check(ThirdChoices.size()==4,"health, damage, movement and critical all appear as alternatives");
    Ranks[0]=2;
    Check(!CanUpgradeRunStat(0,Ranks[0],5),"do not offer cooldown ranks once the combined effect is capped");
    const auto Capped=RunUpgradeOffers(Ranks,11,5,0);
    Check(std::find(Capped.begin(),Capped.end(),100)==Capped.end(),"capped cooldown option is replaced by another stat");
    Ranks.fill(5);Check(RunUpgradeOffers(Ranks,11).empty(),"fully upgraded stats leave room for relic/style alternatives");
    Check(!CanUpgradeRunStat(-1,0)&&!CanUpgradeRunStat(6,0),"invalid upgrade ids cannot be purchased");
    Check(!IsRunUpgradeOffer(24)&&!IsRunUpgradeOffer(99)&&IsRunUpgradeOffer(100)&&IsRunUpgradeOffer(105)&&!IsRunUpgradeOffer(106),"stat ids never alias relic/style ids");

    Check(ReviveInReach(300.0*300,150)&&ReviveInReach(0,-150),"revive distance/height edges are inclusive");
    Check(!ReviveInReach(300.01*300.01,0)&&!ReviveInReach(0,150.01),"out-of-reach partners cannot revive");
    Check(!ReviveInReach(-1,0)&&!ReviveInReach(std::numeric_limits<double>::quiet_NaN(),0),"invalid positions cannot advance a revive");
    for(int FPS:{30,60,120}) {
        float Progress=0;
        for(int Frame=0;Frame<int(FPS*1.8);++Frame) Progress=AdvanceRevive(Progress,1.f/FPS,true);
        Check(Progress<ReviveSeconds,"revive cannot complete early at common frame rates");
        for(int Frame=0;Frame<FPS/3;++Frame) Progress=AdvanceRevive(Progress,1.f/FPS,true);
        Check(Near(Progress,ReviveSeconds),"continuous hold completes a two-second revive at common frame rates");
        Progress=AdvanceRevive(1.5f,1.f/FPS,false);
        Check(Progress==0,"release, obstruction, range loss, or an invalid helper cancels prior progress");
        for(int Frame=0;Frame<FPS;++Frame) Progress=AdvanceRevive(Progress,1.f/FPS,true);
        Check(Progress<ReviveSeconds,"a cancelled revive must be channeled again");
    }
    Check(Near(AdvanceRevive(1.f,-1.f,true),1.f),"negative frame time does not undo or complete revive");
    Check(Near(AdvanceRevive(1.f,std::numeric_limits<float>::quiet_NaN(),true),1.f),"invalid frame time does not corrupt revive");

    int VerticesChecked=0;
    for(int Weapon=0;Weapon<WeaponCount;++Weapon) {
        const auto Points=Mesh(std::string(Argv[1])+"/SourceArt/Meshes/SM_Weapon"+std::to_string(Weapon)+".obj");
        const auto Grip=Mesh(std::string(Argv[1])+"/SourceArt/Meshes/SM_Weapon"+std::to_string(Weapon)+".obj","grip");
        Check(!Grip.empty(),"weapon has hand-sized grip geometry");
        double Low=1000,High=-1000;
        for(auto P:Grip) {Low=std::min(Low,P.Z);High=std::max(High,P.Z);}
        Check(std::abs(Low+13)<.001&&std::abs(High-13)<.001,"every grip is centered at hand origin");
        if(Weapon<2) {
            const auto Head=Mesh(std::string(Argv[1])+"/SourceArt/Meshes/SM_Weapon"+std::to_string(Weapon)+".obj",Weapon==0?"blade":"hammer_head");
            Check(!Head.empty(),"blade or hammer head exists");
            for(auto P:Head) Check(P.Z>=18.99,"blade/head extends above the hand");
        }
        if(Weapon==0||Weapon==1||Weapon==5) continue; // Full melee cycles are verified in melee_021_tests.
        for(float Walk:{-1.f,0.f,1.f}) for(int Frame=0;Frame<=40;++Frame) {
            const auto Pose=WeaponGripPose(.28f*Frame/40.f,.28f,Walk,Weapon);
            Check(Pose.ForwardAngle>=(Weapon>=6?-.041f:0.f)&&Pose.ForwardAngle<1.2f,"weapon stays upright or strikes forward");
            for(const auto& P:Points) {
                // Same local +X-forward mesh convention and forward pitch used by ASCharacter.
                const double X=Pose.X+P.X*std::cos(Pose.ForwardAngle)+P.Z*std::sin(Pose.ForwardAngle);
                const double Y=Pose.Y+P.Y;
                const double Z=Pose.Z-P.X*std::sin(Pose.ForwardAngle)+P.Z*std::cos(Pose.ForwardAngle);
                const bool Torso=X>=-24&&X<=28&&std::abs(Y)<=33&&Z>=75&&Z<=147;
                const bool Head=X>=-35&&X<=35&&std::abs(Y)<=31&&Z>=130&&Z<=200;
                Check(std::isfinite(X)&&std::isfinite(Z)&&!Torso&&!Head,"weapon vertices stay outside torso/head throughout the sampled poses");
                ++VerticesChecked;
            }
        }
    }
    std::cout<<"PASS: "<<Checks<<" gameplay logic/geometry checks; "<<VerticesChecked<<" sampled weapon vertices.\n";
    std::cout<<"Uses the game's shared rules and actual OBJ geometry. Does not run Unreal networking, input, collision, or rendering.\n";
}
