// Native tests for 0.25 phase A: the maul slam lands where the head of the maul does (ASEnemyRules::SlamReach), checked against the real ground-pound pose track. Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASEnemyPose.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.001f) {return std::fabs(A-B)<E;}
using namespace ASEnemyPose;
namespace ER=ASEnemyRules;

// The maul head in the figure's own space at pose time U: the weapon is posed by the track; the head is MaulHeadMeshZ up the weapon mesh.
static V3 MaulHead(int Ability,float U) {
    float D[ChannelCount];ClearChannels(D);Evaluate(TrackFor(Ability),U,D);
    RigInput In;RigOut O;Solve(In,D,O);Xf W[SlotCount];ToFigureSpace(O,W);
    return Apply(W[Weapon],V3{0.f,0.f,ER::MaulHeadMeshZ});
}
int main() {
    // ---- Which abilities have a reach ----
    for(int Id=0;Id<ER::AbilityCount;++Id) {
        const float R=ER::SlamReach(Id,1.f);
        if(Id==ER::AbilityPound||Id==ER::AbilityGlacial) Check(R>100.f&&R<400.f,"a maul slam reaches a body length or two in front of the brute");
        else Check(R==0.f,"every other ability lands at the feet");
    }
    Check(ER::SlamReach(-5,1.f)==0.f&&ER::SlamReach(9999,1.f)==0.f,"an out-of-range ability id has no reach");
    // ---- Scale: the elite is 1.45, a plain brute .90 ----
    Check(Near(ER::SlamReach(ER::AbilityPound,1.45f),ER::MaulHeadFigureX*ER::MaulFigureScaleX*1.45f,.01f),"the elite's reach is the head distance times the figure squash times its body scale");
    Check(ER::SlamReach(ER::AbilityPound,.90f)<ER::SlamReach(ER::AbilityPound,1.45f),"a bigger brute reaches further");
    Check(ER::SlamReach(ER::AbilityGlacial,1.45f)==ER::SlamReach(ER::AbilityPound,1.45f),"the frostbound elite's slam lands exactly where the pound does");
    const float Nan=std::numeric_limits<float>::quiet_NaN(),Inf=std::numeric_limits<float>::infinity();
    Check(ER::SlamReach(ER::AbilityPound,Nan)==0.f&&ER::SlamReach(ER::AbilityPound,Inf)==0.f&&ER::SlamReach(ER::AbilityPound,0.f)==0.f&&ER::SlamReach(ER::AbilityPound,-2.f)==0.f,"a bad body scale gives no reach, never NaN");
    Check(ER::SlamReach(ER::AbilityPound,100.f)==ER::SlamReach(ER::AbilityPound,3.f),"a huge body scale is capped");
    for(float S=.1f;S<=3.5f;S+=.05f) {const float R=ER::SlamReach(ER::AbilityPound,S);Check(std::isfinite(R)&&R>=0.f&&R<=ER::MaulHeadFigureX*ER::MaulFigureScaleX*3.f+.01f,"reach is finite and bounded across body scales");}
    // ---- The pose: at the strike key the head of the maul is where the reach says (tune the pose and this fails, so the warning cannot drift from the picture) ----
    const float StrikeU=1.3f;
    for(int Id:{ER::AbilityPound,ER::AbilityGlacial}) {
        const V3 Tip=MaulHead(Id,StrikeU);
        Check(Near(Tip.X,ER::MaulHeadFigureX,6.f),"the maul head is MaulHeadFigureX in front of the figure at the strike key");
        Check(std::fabs(Tip.Y)<60.f,"... and within a body width of the figure's centre line");
        Check(Tip.Z>0.f&&Tip.Z<80.f,"... and low, near the floor");
    }
    Check(MaulHead(ER::AbilityPound,.9f).X<0.f&&MaulHead(ER::AbilityPound,.9f).Z>200.f,"raised for the wind-up: the head is overhead and behind");
    // ---- The sweep is finite everywhere ----
    for(float U=0.f;U<=3.f;U+=.05f) {const V3 H=MaulHead(ER::AbilityPound,U);Check(std::isfinite(H.X)&&std::isfinite(H.Y)&&std::isfinite(H.Z),"the head position is finite throughout the track");}
    std::cout<<"PASS: "<<Checks<<" maul slam checks; reach per ability and body scale, bad inputs, and the head of the maul against the ground-pound pose at its strike.\nShared rules only; the slam, its warning disc and the imported mesh are Unreal gameplay: UNVERIFIED.\n";
}
