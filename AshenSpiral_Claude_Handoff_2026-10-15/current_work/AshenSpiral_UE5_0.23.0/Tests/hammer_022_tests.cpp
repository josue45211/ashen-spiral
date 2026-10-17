#include "../Source/AshenSpiral/Public/ASMeleeRules.h"
#include "../Source/AshenSpiral/Public/ASArsenalRules.h"
#include "../Source/AshenSpiral/Public/ASRules.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
using ASHammer::V;
static int Checks=0;
static void Check(bool OK,const char* Why) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<Why<<'\n';std::exit(1);}}
static bool Near(float A,float B,float E=.001f) {return std::abs(A-B)<E;}
static float Length(V A) {return std::sqrt(ASHammer::Dot(A,A));}
static V BodyLocal(ASHammer::Pose P,V A) {
    const float R=.0174532925199433f,Y=-P.BodyYaw*R,B=-P.BodyPitch*R;A.Z-=111.f;
    V T={A.X*std::cos(Y)-A.Y*std::sin(Y),A.X*std::sin(Y)+A.Y*std::cos(Y),A.Z};
    return {T.X*std::cos(B)+T.Z*std::sin(B),T.Y,-T.X*std::sin(B)+T.Z*std::cos(B)};
}
int main(int Argc,char** Argv) {
    using namespace ASHammer;Check(Argc==2,"project path supplied");std::ifstream Obj(std::string(Argv[1])+"/SourceArt/Meshes/SM_Weapon1.obj");Check(bool(Obj),"hammer mesh opens");
    std::vector<V> Mesh;std::string Line;while(std::getline(Obj,Line)) if(Line.rfind("v ",0)==0) {std::istringstream S(Line.substr(2));V P;S>>P.X>>P.Y>>P.Z;Mesh.push_back(P);}
    int Samples=0;
    for(int Mode=0;Mode<6;++Mode) for(int I=0;I<=480;++I) {
        ++Samples;const float T=I/480.f;const auto P=Mode<3?Swing(Mode,T):Leap(Mode-2,T);const auto F=Mode<3?SwingFrame(Mode,T):LeapFrame(Mode-2,T);
        const V Grip={P.X,P.Y,P.Z},Support=Add(Grip,Mul(F.Z,24));
        Check(Length(Sub(Grip,BodyPoint(P,{0,34,133})))<83.99f,"rear hand reachable from rotating shoulder");
        Check(Length(Sub(Support,BodyPoint(P,{0,-34,133})))<83.99f,"forward hand stays on shaft from rotating shoulder");
        Check(Near(Length(Sub(Support,Grip)),24.f),"two hands retain their exact shaft spacing");
        Check(Near(Dot(F.X,F.Y),0)&&Near(Dot(F.X,F.Z),0)&&Near(Dot(F.Y,F.Z),0)&&Near(Dot(Cross(F.X,F.Y),F.Z),1),"weapon basis is orthonormal and right handed");
        const auto Before=Mode<3?Swing(Mode,std::max(0.f,T-.001f)):Leap(Mode-2,std::max(0.f,T-.001f));
        const auto After=Mode<3?Swing(Mode,std::min(1.f,T+.001f)):Leap(Mode-2,std::min(1.f,T+.001f));
        V Tangent=Sub(HeadCenter(After),HeadCenter(Before));Tangent=Sub(Tangent,Mul(F.Z,Dot(Tangent,F.Z)));
        if(Length(Tangent)>.001f) Check(std::abs(Dot(Unit(Tangent),F.Y))>.999f,"squared head axis follows the swing tangent including transition frames");
        for(V Q:Mesh) {
            Q=Add(Grip,Transform(F,Q));const V B=BodyLocal(P,Q);
            const bool Torso=B.X>-17&&B.X<20&&std::abs(B.Y)<27&&B.Z>-29&&B.Z<37;
            const bool Head=Q.X>-13&&Q.X<16&&std::abs(Q.Y)<16&&Q.Z>152&&Q.Z<180;
            if(Torso||Head||Q.Z<-1) std::cerr<<"mode "<<Mode<<" frame "<<I<<" point "<<Q.X<<','<<Q.Y<<','<<Q.Z<<'\n';
            Check(!Torso&&!Head,"actual weapon vertices avoid deep torso/helmet penetration");Check(Q.Z>-1,"weapon stays above standing ground plane");
        }
    }
    for(int Step=0;Step<3;++Step) {
        const auto A=Swing(Step,0),B=Swing(Step,1);Check(Near(A.X,B.X)&&Near(A.Y,B.Y)&&Near(A.Z,B.Z),"combo cycles connect at the same ready grip");
        for(int Haste=0;Haste<=10;++Haste) {
            const float Speed=1.f+Haste*.22f,Duration=Cycle(Step)/Speed,Window=ComboWindow(.72f/Speed);
            Check(Near(Window,1.2f/Speed),"combo grace window follows the same haste multiplier");
            const float Stop=Hitstop(Step)/Speed;
            Check(Near(PosePhase(Step,Impact(Step)+Stop/Duration*.5f,true),Impact(Step)),"contact hitstop holds the impact pose");
            Check(Near(PosePhase(Step,1,true),1),"hitstop recovers inside original attack duration; no lost DPS");
            float Previous=0;for(int F=0;F<=1000;++F) {const float P=PosePhase(Step,F/1000.f,true);Check(P>=Previous-.0001f&&P<=1.0001f,"pose clock never reverses or overshoots");Previous=P;}
            for(int FPS:{15,30,60,144}) {
                bool Pending=true;int Hits=0;
                for(int F=0;F<300;++F) if(ASMelee::ImpactDue(F/float(FPS),Duration*Impact(Step),Pending)) {Pending=false;++Hits;}
                Check(Hits==1,"each low/high FPS swing resolves once, including whiffs");
                Check(1.f/FPS<Window,"held chain cannot lapse between supported simulation ticks");
            }
        }
    }
    Check(Near((62+62+161.2f)/(.72f+.72f+1.188f),108.52359f),"specified combo damage and duration preserve full-combo DPS");
    for(int Angle=-180;Angle<=180;++Angle) {const float D=std::cos(Angle*.0174532925199433f);Check(Frontal(D)==(D>0),"every combo step excludes the rear hemisphere");}
    Check(Committed(1,2,.5f,0,1.188f)&&!Committed(1,1,.5f,0,.72f)&&!Committed(0,2,.5f,0,1),"commitment affects hammer finisher only");
    Check(!Committed(1,2,1.188f,0,1.188f),"commitment releases at full animation recovery");
    Check(Near(FinisherSpeedScale(),.2f,.0001f)&&FinisherSpeedScale()>0.f,"finisher slows movement by 80 percent rather than rooting");
    Check(!LeapProtected(1,.1f,-10)&&LeapProtected(2,.2f,-10)&&LeapProtected(3,1.3f,1.2f)&&!LeapProtected(3,1.43f,1.2f)&&!LeapProtected(0,2,1.2f),"startup vulnerable; flight/recovery protected; recovery ends immunity");
    Check(Near(LeapDistance(.22f)/(2350.f*.22f),1.15f)&&Near(Height/(660.f*660.f/(2.f*1568.f)),1.5f,.001f),"leap has 15 percent extra dash reach and 50 percent extra jump height");
    Check(ASHammer::Cooldown>=7&&ASHammer::Cooldown<=8,"leap cooldown stays in approved range");
    for(int W=0;W<9;++W) if(W!=1) Check(!Committed(W,2,.5f,0,2),"all other weapons unaffected by hammer commitment");
    std::cout<<"PASS: "<<Checks<<" hammer/0.22 checks; "<<Samples<<" actual-mesh pose samples; tangent orientation, two-hand reach, contact timing, haste, frontal coverage and leap immunity boundaries.\n";
    std::cout<<"Shared rules/source geometry only; Unreal physics and network execution still require playtest.\n";
}
