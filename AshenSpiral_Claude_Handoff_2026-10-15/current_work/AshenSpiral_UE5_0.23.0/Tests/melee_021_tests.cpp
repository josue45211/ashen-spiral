#include "../Source/AshenSpiral/Public/ASMeleeRules.h"
#include "../Source/AshenSpiral/Public/ASArsenalRules.h"
#include "../Source/AshenSpiral/Public/ASBuildRules.h"
#include "../Source/AshenSpiral/Public/ASArchiveRules.h"
#include <iostream>
#include <limits>
#include <fstream>
#include <sstream>
#include <set>
#include <cstdlib>
static int Checks=0;
static void Check(bool OK,const char* Why) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<Why<<'\n';std::exit(1);}}
static bool Near(double A,double B) {return std::abs(A-B)<.001;}
struct V {double X,Y,Z;};
static V Rotate(V P,double Pitch,double Yaw) {
    const double A=Pitch*3.141592653589793/180,B=Yaw*3.141592653589793/180;
    const double X=P.X*std::cos(A)+P.Z*std::sin(A),Z=-P.X*std::sin(A)+P.Z*std::cos(A);
    return {X*std::cos(B)-P.Y*std::sin(B),X*std::sin(B)+P.Y*std::cos(B),Z};
}
static std::vector<V> Mesh(const std::string& Root,const std::string& Name) {
    std::ifstream F(Root+"/SourceArt/Meshes/"+Name+".obj");Check(bool(F),"source mesh opens");
    std::vector<V> Out;std::string S;
    while(std::getline(F,S)) if(S.rfind("v ",0)==0) {std::istringstream Row(S.substr(2));V P{};Row>>P.X>>P.Y>>P.Z;Out.push_back(P);}
    return Out;
}
int main(int Argc,char** Argv) {
    // 0.23.4: the edge-of-reach cue only fires in the outer band of the real reach.
    Check(ASMelee::TargetInReach(300.f,280.f,38.f,0.f,.9f,.3f)&&!ASMelee::TargetInReach(330.f,280.f,38.f,0.f,.9f,.3f)&&!ASMelee::TargetInReach(100.f,280.f,38.f,0.f,-.5f,.3f)&&!ASMelee::TargetInReach(100.f,280.f,38.f,300.f,.9f,.3f),"0.23.5 in-reach cue: inside reach plus the target radius, in front, and at a similar height");
    Check(!ASMelee::TargetInReach(std::numeric_limits<float>::quiet_NaN(),280.f,38.f,0.f,1.f,.3f)&&!ASMelee::TargetInReach(100.f,0.f,38.f,0.f,1.f,.3f)&&ASMelee::TargetInReach(560.f,560.f,0.f,0.f,1.f,.3f)==false&&ASMelee::TargetInReach(500.f,280.f*2.f,38.f,0.f,1.f,.3f),"in-reach cue scales with the effective reach (Wide Arc) and survives bad numbers");
    // 0.23.3: a swing survives an Endless arena shuffle (same phase and epoch) but not a room or phase change.
    Check(ASMelee::ContextValid(2,2,7,7),"pending swing stays valid when nothing changed (including across a shuffle, which keeps the epoch)");
    Check(!ASMelee::ContextValid(2,2,7,8)&&!ASMelee::ContextValid(2,3,7,7)&&!ASMelee::ContextValid(0,2,7,7),"pending swing is cancelled by a new room epoch or a phase change");
    Check(ASMelee::ContextValid(1,2,7,7)&&!ASMelee::ContextValid(2,1,7,7)&&!ASMelee::ContextValid(1,2,7,8),"0.23.5: a swing or leap begun during the countdown survives enemies spawning (Countdown->Combat), but not an epoch change");
    Check(Argc==2,"project path supplied");const std::string Root=Argv[1];
    for(int Capacity=1;Capacity<=3;++Capacity) {
        int Used=0,Available=Capacity;
        for(int N=0;N<Capacity;++N) {
            Check(ASRules::CanAirDash(false,true,Used,Capacity,Available),"every purchased airborne charge can be spent");
            ++Used;--Available;
            // Replaying a saved move uses the numeric spent count, not bool(Used).
            const int SavedUsed=Used,SavedAvailable=Available;
            Check(ASRules::CanAirDash(false,true,SavedUsed,Capacity,SavedAvailable)==(N+1<Capacity),"saved air count preserves second/third dash eligibility");
        }
        Available=Capacity;Check(!ASRules::CanAirDash(false,true,Used,Capacity,Available),"airborne recharge cannot exceed the per-landing budget");
        Used=0;Check(ASRules::CanAirDash(true,false,Used,Capacity,Available),"landing resets the air budget");
        Check(!ASRules::CanAirDash(true,false,Used,Capacity,0),"landing never invents an available charge");
        Check(!ASRules::CanAirDash(false,false,Used,Capacity,Available),"custom dash movement cannot restart itself");
    }
    const int RangedCaps[]={10,7,14,6,40,3};int RangedIndex=0;
    for(int W=0;W<9;++W) {
        if(!ASMelee::IsMelee(W)) {Check(ASArsenal::BaseCapacity(W)==RangedCaps[RangedIndex++],"ranged magazine capacity unchanged");continue;}
        ASBuild::Context C;C.Weapon=W;
        Check(!ASArsenal::UsesMagazine(W)&&ASArsenal::Capacity(W,5)==0&&ASArsenal::ReloadSeconds(W,5)==0,"every melee weapon has no magazine/reload");
        for(int Seed=1;Seed<=1000;++Seed) for(int Pick:ASBuild::Offers(C,Seed)) Check(Pick!=263&&Pick!=264,"actual melee drafts exclude both reload traits");
        const auto Vertices=Mesh(Root,"SM_Weapon"+std::to_string(W));
        for(int Step=0;Step<ASMelee::ComboLength(W);++Step) {
            const float Base=ASArsenal::Rate(W)*ASMelee::TimeScale(W,Step);
            Check(Near(ASMelee::BaseSeconds(W),ASArsenal::Rate(W)),"pose timing and weapon rate have one matching base");
            for(int Rank=0;Rank<=5;++Rank) {
                const float Cycle=ASRules::AttackInterval(ASArsenal::Rate(W),Rank,Rank,true)*ASMelee::TimeScale(W,Step);
                Check(Near(Cycle/Base,1/((1+.08*Rank+.12*Rank)*1.25)),"attack speed scales the whole cycle, including finisher recovery");
                for(int FPS:{15,30,60,144}) {
                    bool Pending=true;int Hits=0;
                    for(int Frame=0;Frame<int(Cycle*FPS)+3;++Frame) if(ASMelee::ImpactDue(float(Frame)/FPS,Cycle*ASMelee::ImpactFraction(W,Step),Pending)) {++Hits;Pending=false;}
                    Check(Hits==1,"each swing lands exactly once even when a frame skips the hit deadline");
                }
            }
            for(int Frame=0;Frame<=240;++Frame) {
                const auto P=ASMelee::Swing(W,Step,Frame/240.f);
                const double Right=std::sqrt(P.X*P.X+(P.Y-34)*(P.Y-34)+(P.Z-133)*(P.Z-133));
                if(Right>=83.99) std::cerr<<"right grip W "<<W<<" step "<<Step<<" frame "<<Frame<<" length "<<Right<<'\n';
                Check(Right<83.99,"right grip remains reachable without a stretched or detached arm");
                if(W!=0) {
                    const V Offset=Rotate({0,0,ASMelee::SupportHeight(W)},P.Pitch,P.Yaw);
                    const double X=P.X+Offset.X,Y=P.Y+Offset.Y+34,Z=P.Z+Offset.Z-133;
                    const double Left=std::sqrt(X*X+Y*Y+Z*Z);
                    if(Left>=83.99) std::cerr<<"left grip W "<<W<<" step "<<Step<<" frame "<<Frame<<" length "<<Left<<'\n';
                    Check(Left<83.99,"support grip stays on the shaft through the complete two-hand swing");
                }
                for(V Q:Vertices) {
                    if(W==1) {auto V=ASHammer::Transform(ASHammer::SwingFrame(Step,Frame/240.f),{float(Q.X),float(Q.Y),float(Q.Z)});Q={V.X,V.Y,V.Z};} else Q=Rotate(Q,P.Pitch,P.Yaw);Q={Q.X+P.X,Q.Y+P.Y,Q.Z+P.Z};
                    // Torso inverse transform follows the runtime upper-body lean/twist.
                    V Body=Rotate({Q.X,Q.Y,Q.Z-111},0,-P.BodyYaw);Body=Rotate(Body,-P.BodyPitch,0);
                    const bool Torso=Body.X>-17&&Body.X<20&&std::abs(Body.Y)<27&&Body.Z>-29&&Body.Z<37;
                    const bool Head=Q.X>-13&&Q.X<16&&std::abs(Q.Y)<16&&Q.Z>152&&Q.Z<180;
                    if(Torso||Head) std::cerr<<"penetration W "<<W<<" step "<<Step<<" frame "<<Frame<<" point "<<Q.X<<','<<Q.Y<<','<<Q.Z<<'\n';
                    Check(!Torso&&!Head,"actual weapon geometry avoids deep torso/helmet penetration");
                    Check(Q.Z>-1,"weapon geometry stays above the floor in standing melee poses");
                }
            }
            const auto Start=ASMelee::Swing(W,Step,0),End=ASMelee::Swing(W,Step,1);
            Check(Near(Start.X,End.X)&&Near(Start.Y,End.Y)&&Near(Start.Z,End.Z)&&Near(Start.Pitch,End.Pitch),"complete swing returns continuously to the next combo's grip");
        }
    }
    Check(ASMelee::DamageScale(1,2)>2.5f&&ASMelee::TimeScale(1,2)>1.5f,"hammer trades a longer charged finisher for much higher damage");
    Check(ASMelee::TimeScale(0,2)<ASMelee::TimeScale(1,2)&&ASArsenal::Rate(0)<ASArsenal::Rate(1),"blade retains the faster three-hit identity");
    for(bool Boss:{false,true}) for(float Range:{180.f,700.f,1100.f}) {
        float Previous=1;
        for(float D=0;D<5000;D+=10) {
            const float Scale=ASMelee::ApproachScale(D,Range,Boss);
            Check(Scale>=Previous-.0001f&&Scale<=1.f+(Boss?.1501f:.3501f),"approach boost is smooth, monotonic with distance and capped");
            if(D<=Range+200) Check(Near(Scale,1),"enemy is at normal speed before attack range");
            Previous=Scale;
        }
    }
    const char* Names[]={"SM_ArchiveShelf","SM_ArchiveShelfBroken","SM_ArchiveDesk","SM_ArchiveDeskBroken","SM_Pillar","SM_ArchiveDebris"};
    std::vector<V> Shapes[6];for(int I=0;I<6;++I) Shapes[I]=Mesh(Root,Names[I]);
    std::set<int> Motifs;int Rooms=0;
    for(int Seed:{1,42,1103,6767,210021}) for(int Room=0;Room<10;++Room) for(int HX:{600,1200,1800}) for(int HY:{600,1200,1800}) {
        const auto A=ASArchive::Dress(Seed,Room,float(HX),float(HY)),B=ASArchive::Dress(Seed,Room,float(HX),float(HY));
        ++Rooms;Motifs.insert(A.Motif);Check(A.Props.size()==B.Props.size()&&!A.Props.empty(),"room dressing is deterministic and nonempty at every supported size");
        for(std::size_t I=0;I<A.Props.size();++I) {
            const auto& P=A.Props[I];Check(P.X==B.Props[I].X&&P.Y==B.Props[I].Y&&P.Type==B.Props[I].Type&&P.Yaw==B.Props[I].Yaw,"both peers generate identical prop placement");
            const double Angle=P.Yaw*3.141592653589793/180;
            if(P.Type==ASArchive::Shelf||P.Type==ASArchive::BrokenShelf) Check(-P.X*std::cos(Angle)-P.Y*std::sin(Angle)>0,"shelf open face points toward the room's usable space");
            for(V Q:Shapes[P.Type]) {
                Q=Rotate({Q.X*P.Scale,Q.Y*P.Scale,Q.Z*P.ZScale},0,P.Yaw);Q.X+=P.X;Q.Y+=P.Y;
                Check(std::abs(Q.X)<HX-24&&std::abs(Q.Y)<HY-24,"actual prop vertices stay inside room walls");
                Check(std::abs(Q.X)>350&&std::abs(Q.Y)>350,"actual props leave the central cross, doors and objectives unobstructed");
            }
        }
    }
    Check(Motifs.size()==4,"all four Archives dressing motifs appear");
    std::cout<<"PASS: "<<Checks<<" melee/0.21 checks; 1687 full-cycle pose samples; "<<Rooms<<" Archives rooms; 3000 melee reward drafts.\n";
    std::cout<<"Shared rules and source geometry only. Unreal movement, collision, RPC timing and rendering are not executed.\n";
}
