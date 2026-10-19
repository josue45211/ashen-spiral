// Native tests for the 0.24 enemy pose system (Tier 0 animation): Unreal-faithful math, track structure, the rig solver against the old hard-coded walking pose, and sweeps over
// every ability's track. Plain C++ over ASEnemyPose.h; Unreal rendering and the component hierarchy are not executed (the offline preview draws the same numbers).
#include "../Source/AshenSpiral/Public/ASEnemyPose.h"
#include "../Source/AshenSpiral/Public/ASEnemyDeathTracks.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.001f) {return std::fabs(A-B)<E;}
static bool NearV(ASEnemyPose::V3 A,ASEnemyPose::V3 B,float E=.002f) {return Near(A.X,B.X,E)&&Near(A.Y,B.Y,E)&&Near(A.Z,B.Z,E);}
static unsigned Seed=24u;
static float Rand() {Seed=Seed*1664525u+1013904223u;return float((Seed>>8)&0xFFFF)/65535.f;}
static float Range(float A,float B) {return A+(B-A)*Rand();}
using namespace ASEnemyPose;
namespace ER=ASEnemyRules;
// The rows of Unreal's FRotationMatrix for (pitch, yaw, roll) in degrees: where the X, Y and Z axes point after the rotation.
static void MatrixAxes(float PitchDeg,float YawDeg,float RollDeg,V3& X,V3& Y,V3& Z) {
    const float SP=std::sin(PitchDeg*DegToRad),CP=std::cos(PitchDeg*DegToRad),SY=std::sin(YawDeg*DegToRad),CY=std::cos(YawDeg*DegToRad),SR=std::sin(RollDeg*DegToRad),CR=std::cos(RollDeg*DegToRad);
    X={CP*CY,CP*SY,SP};
    Y={SR*SP*CY-CR*SY,SR*SP*SY+CR*CY,-SR*CP};
    Z={-(CR*SP*CY+SR*SY),CY*SR-CR*SP*SY,CR*CP};
}
static V3 RandomUnit() {V3 V;do {V={Range(-1,1),Range(-1,1),Range(-1,1)};} while(Length(V)<.2f||Length(V)>1.f);return Normalize(V);}
int main() {
    const float NaN=std::numeric_limits<float>::quiet_NaN();
    // ---- Math: Unreal's conventions ----
    Check(NearV(Rotate(FromRotator(0,90,0),V3{1,0,0}),V3{0,1,0})&&NearV(Rotate(FromRotator(90,0,0),V3{1,0,0}),V3{0,0,1})&&NearV(Rotate(FromRotator(0,0,90),V3{0,1,0}),V3{0,0,-1}),"yaw 90 turns forward to the right, pitch 90 turns forward up, roll 90 tips the right side down");
    for(int I=0;I<20000;++I) {
        const float P=Range(-180,180),Y=Range(-180,180),R=Range(-180,180);V3 X,Yv,Z;MatrixAxes(P,Y,R,X,Yv,Z);
        const Q4 Q=FromRotator(P,Y,R);
        Check(NearV(Rotate(Q,V3{1,0,0}),X,.003f)&&NearV(Rotate(Q,V3{0,1,0}),Yv,.003f)&&NearV(Rotate(Q,V3{0,0,1}),Z,.003f),"the rotator's quaternion rotates the axes exactly as Unreal's rotation matrix does");
        Check(Near(Q.X*Q.X+Q.Y*Q.Y+Q.Z*Q.Z+Q.W*Q.W,1.f,.0005f),"a rotator's quaternion is unit length");
    }
    for(int I=0;I<5000;++I) {
        const V3 A=RandomUnit(),B=RandomUnit();
        Check(NearV(Rotate(Between(A,B),A),B,.003f),"FindBetweenNormals rotates A onto B");
        const Q4 Q1=FromRotator(Range(-90,90),Range(-90,90),Range(-90,90)),Q2=FromRotator(Range(-90,90),Range(-90,90),Range(-90,90));const V3 V=RandomUnit();
        Check(NearV(Rotate(Mul(Q1,Q2),V),Rotate(Q1,Rotate(Q2,V)),.003f),"A * B applies B first, then A");
    }
    Check(NearV(Rotate(Between(V3{0,0,1},V3{0,0,-1}),V3{0,0,1}),V3{0,0,-1},.003f)&&NearV(Rotate(Between(V3{1,0,0},V3{-1,0,0}),V3{1,0,0}),V3{-1,0,0},.003f)&&NearV(Rotate(Between(V3{0,1,0},V3{0,-1,0}),V3{0,1,0}),V3{0,-1,0},.003f),"opposite vectors still rotate onto each other");
    Check(NearV(Rotate(Between(V3{0,0,1},V3{0,0,1}),V3{1,2,3}),V3{1,2,3}),"the same vector twice is the identity");
    for(int I=0;I<3000;++I) { // composition matches the nested application whenever the parent scale is uniform or the child is unrotated
        Xf P{V3{Range(-50,50),Range(-50,50),Range(-50,50)},FromRotator(Range(-90,90),Range(-90,90),Range(-90,90)),V3{1,1,1}*Range(.5f,2.f)};
        Xf C{V3{Range(-50,50),Range(-50,50),Range(-50,50)},FromRotator(Range(-90,90),Range(-90,90),Range(-90,90)),V3{Range(.5f,2.f),Range(.5f,2.f),Range(.5f,2.f)}};
        const V3 X{Range(-30,30),Range(-30,30),Range(-30,30)};
        Check(NearV(Apply(Compose(P,C),X),Apply(P,Apply(C,X)),.02f),"composing a uniformly scaled parent equals applying child then parent");
        Xf P2{P.Loc,P.Rot,V3{Range(.5f,2.f),Range(.5f,2.f),Range(.5f,2.f)}};Xf C2{C.Loc,Q4{},C.Scale};
        Check(NearV(Apply(Compose(P2,C2),X),Apply(P2,Apply(C2,X)),.02f),"... and so does an unrotated child under a non-uniformly scaled parent (the head and its visor)");
    }
    {   // Segments: the mesh's +Z points from the lower end to the upper end, centred, stretched to length.
        Xf S;Segment(S,V3{0,0,100},V3{0,0,40},42.f);
        Check(NearV(S.Loc,V3{0,0,70})&&NearV(Rotate(S.Rot,V3{0,0,1}),V3{0,0,1})&&Near(S.Scale.Z,60.f/42.f,.0005f),"a vertical segment is centred and stretched");
        Segment(S,V3{50,0,0},V3{0,0,0},42.f);
        Check(NearV(Rotate(S.Rot,V3{0,0,1}),V3{1,0,0})&&NearV(S.Loc,V3{25,0,0}),"a segment from the origin out along +X points the mesh's top along +X");
        Segment(S,V3{1,2,3},V3{1,2,3},42.f);
        Check(std::isfinite(S.Rot.W)&&Near(S.Scale.Z,0.f),"a zero-length segment is harmless");
    }

    // ---- Track structure ----
    for(int Ability=0;Ability<ER::AbilityCount;++Ability) {
        const Track& T=TrackFor(Ability);std::set<int> Channels;bool Ended=false;
        for(int I=0;I<MaxCurves;++I) {
            const Curve& C=T.Curves[I];
            if(C.Channel<0) {Ended=true;break;}
            Check(C.Channel<ChannelCount,"a curve drives a real channel");
            Check(Channels.insert(C.Channel).second,"a channel is driven by one curve per track");
            int N=0;while(N<MaxKeys&&C.Keys[N].T>=0.f) ++N;
            Check(N>=2&&N<=MaxKeys,"a curve has between two keys and the maximum");
            const bool Chained=Ability==ER::AbilityRiposte||Ability==ER::AbilityBlinkStrike; // these begin straight out of another ability's pose (the parry's guard, the blink's pop)
            Check(C.Keys[0].T==0.f&&(Chained||C.Keys[0].V==0.f),"every curve starts at neutral at the very start (a chained ability starts where the one before it ended)");
            Check(C.Keys[N-1].T==TrackEnd&&C.Keys[N-1].V==0.f,"... and ends at neutral exactly at the end of the recovery");
            for(int K=1;K<N;++K) Check(C.Keys[K].T>C.Keys[K-1].T&&C.Keys[K].T<=TrackEnd,"keys are strictly increasing inside the track");
            const float Limit=C.Channel==ChPlant?1.f:(C.Channel==ChBend||C.Channel==ChBendRoll?120.f:(C.Channel>=ChScaleX&&C.Channel<=ChScaleZ?.8f:(C.Channel==ChCrouch?30.f:(C.Channel==ChLeanX||C.Channel==ChLeanY?40.f:(C.Channel>=ChHandLX&&C.Channel<=ChHandRZ?140.f:(C.Channel==ChFootLX||C.Channel==ChFootRX?40.f:(C.Channel==ChStance?30.f:(C.Channel==ChGlow?3.f:120.f))))))));
            for(int K=0;K<N;++K) Check(std::fabs(C.Keys[K].V)<=Limit&&std::isfinite(C.Keys[K].V),"every key stays inside its channel's sane range");
            float Previous=EvalCurve(C,0.f);
            for(float U=.005f;U<=TrackEnd+.0001f;U+=.005f) {const float V=EvalCurve(C,U);Check(std::fabs(V-Previous)<=Limit*.35f,"a curve has no jumps");Previous=V;}
            Check(EvalCurve(C,-5.f)==C.Keys[0].V&&EvalCurve(C,50.f)==0.f&&EvalCurve(C,NaN)==0.f,"outside the track a curve holds its end values, and NaN gives neutral");
        }
        Check(Ended,"every track list ends with a sentinel");
    }
    {   // Evaluate fills every channel and zeroes the rest.
        float D[ChannelCount];for(float& V:D) V=99.f;
        Evaluate(TrackFor(ER::AbilityNone),1.5f,D);bool Zero=true;for(float V:D) Zero&=V==0.f;
        Check(Zero,"an empty track evaluates to the neutral pose");
        Evaluate(TrackFor(ER::AbilitySlash),1.f,D);
        Check(D[ChHandRZ]>100.f&&D[ChWeaponPitch]>50.f&&D[ChPlant]==1.f&&D[ChLeanY]==0.f,"the slash's peak is a raised sword in a planted stance, and undriven channels stay zero");
        Evaluate(TrackFor(ER::AbilitySlash),0.f,D);Zero=true;for(float V:D) Zero&=V==0.f;
        Check(Zero,"every track starts neutral");
        Evaluate(TrackFor(ER::AbilitySlash),TrackEnd,D);Zero=true;for(float V:D) Zero&=V==0.f;
        Check(Zero,"... and ends neutral");
    }

    {   // Shared tracks: every ability resolves to a real track, the mutations' abilities to the ones they share a body language with.
        for(int Id=0;Id<ER::AbilityCount;++Id) Check(TrackIndex(Id)>=0&&TrackIndex(Id)<OwnTracks&&(Id>=OwnTracks||TrackIndex(Id)==Id),"an ability with a track of its own plays it, every other one resolves to a real track");
        Check(&TrackFor(ER::AbilityGlacial)==&TrackFor(ER::AbilityPound)&&&TrackFor(ER::AbilityStorm)==&TrackFor(ER::AbilityMeteors)&&&TrackFor(ER::AbilityCinders)==&TrackFor(ER::AbilitySlash)&&&TrackFor(ER::AbilitySeeds)==&TrackFor(ER::AbilityBlast),"the elite mutations' abilities share the tracks of the abilities they look like");
        Check(&TrackFor(-4)==&TrackFor(0)&&&TrackFor(9999)==&TrackFor(TrackIndex(ER::AbilityCount-1)),"an out-of-range ability clamps to a real track");
    }
    // ---- Timeline ----
    {
        const ER::AbilitySpec Slash=ER::SpecFor(ER::AbilitySlash),Pounce=ER::SpecFor(ER::AbilityPounce);
        Check(AbilityU(Slash,true,10.f,10.f,10.85f,-100.f)==0.f&&Near(AbilityU(Slash,true,10.425f,10.f,10.85f,-100.f),.5f,.001f)&&AbilityU(Slash,true,20.f,10.f,10.85f,-100.f)==1.f,"the wind-up runs 0..1 and clamps");
        Check(AbilityU(Slash,false,50.f,10.f,10.85f,-100.f)==-1.f,"a long-ago release plays nothing");
        Check(AbilityU(Slash,false,10.f,10.f,10.85f,10.85f)==-1.f,"a release in the future is not a pose yet");
        Check(AbilityU(Slash,false,10.85f,10.f,10.85f,10.85f)==1.f&&Near(AbilityU(Slash,false,10.85f+StrikeSeconds*.5f,10.f,10.85f,10.85f),1.5f,.001f),"an instant ability's strike runs 1..2 over StrikeSeconds");
        Check(Near(AbilityU(Slash,false,10.85f+StrikeSeconds,10.f,10.85f,10.85f),2.f,.001f)&&Near(AbilityU(Slash,false,10.85f+StrikeSeconds+PoseRecoverySeconds(Slash)*.5f,10.f,10.85f,10.85f),2.5f,.01f),"... then the recovery 2..3 over what is left of the ability's Recover (at least MinRecoverySeconds)");
        Check(AbilityU(Slash,false,10.85f+StrikeSeconds+PoseRecoverySeconds(Slash)+.01f,10.f,10.85f,10.85f)==-1.f&&Near(StrikeSeconds+PoseRecoverySeconds(Slash),Slash.Recover,.001f),"... and nothing after it: the whole pose ends exactly when the Recover state does");
        Check(Near(AbilityU(Pounce,false,5.f+Pounce.Attack*.5f,0.f,1.f,5.f),1.5f,.001f),"a committed ability's strike lasts as long as its committed motion");
        float Last=-1.f;bool Monotone=true;
        for(float Now=10.f;Now<12.f;Now+=.005f) {const bool Winding=Now<10.85f;const float U=AbilityU(Slash,Winding,Now,10.f,10.85f,10.85f);if(U>=0.f) {if(U<Last-1e-4f) Monotone=false;Last=U;}}
        Check(Monotone&&Last>2.5f,"track time never runs backward through a whole attack");
        Check(AbilityU(Slash,true,NaN,0.f,1.f,0.f)==-1.f&&AbilityU(Slash,false,1.f,0.f,NaN,0.f)==-1.f,"non-finite times play nothing");
    }

    for(int Id=1;Id<ER::AbilityCount;++Id) {
        const ER::AbilitySpec& A=ER::SpecFor(Id);
        if(A.Attack>0.f&&A.Recover>0.f) Check(Near(PoseRecoverySeconds(A),std::max(A.Recover,MinRecoverySeconds),.001f),"a committed ability's pose recovery is its whole Recover state");
        else if(A.Recover>0.f&&Id!=ER::AbilityDive&&Id!=ER::AbilityMolt) Check(StrikeSeconds+PoseRecoverySeconds(A)<=A.Recover+.001f,"an instant ability's pose (strike plus recovery) fits inside its Recover state");
    }
    // ---- Solver: the neutral walking pose equals the old hard-coded one ----
    {
        float D[ChannelCount];ClearChannels(D);
        for(int I=0;I<3000;++I) {
            RigInput In;In.Weight=Range(0,1);In.GaitPhase=Range(0,6.2831853f);In.Step=std::sin(In.GaitPhase)*In.Weight;In.Seconds=Range(0,100);
            In.Speed=In.Weight;In.HeadingX=1.f;In.HeadingY=0.f;
            RigOut O;Solve(In,D,O);
            V3 X,Y,Z;
            MatrixAxes(-5.f-In.Weight*4.f,In.Step*3.f,0.f,X,Y,Z);
            Check(NearV(Rotate(O.Part[Torso].Rot,V3{1,0,0}),X,.003f)&&NearV(Rotate(O.Part[Torso].Rot,V3{0,0,1}),Z,.003f),"the neutral torso leans as the old gait set it");
            MatrixAxes(5.f,-In.Step*2.f,0.f,X,Y,Z);
            Check(NearV(Rotate(O.Part[Head].Rot,V3{1,0,0}),X,.003f),"... and so does the head");
            Check(NearV(O.Part[Torso].Loc,V3{0,0,111})&&NearV(O.Part[Head].Loc,V3{0,0,165})&&NearV(O.Part[Pelvis].Loc,V3{0,0,78})&&NearV(O.Part[PauldronL].Loc,V3{0,-34,135})&&NearV(O.Part[PauldronR].Loc,V3{0,34,135}),"the static parts sit where RefreshLook put them");
            Check(NearV(O.Part[HandL].Loc,V3{24.f-In.Step*12.f,-45.f,74.f})&&NearV(O.Part[HandR].Loc,V3{24.f+In.Step*12.f,45.f,74.f}),"the hands hang and swing exactly as before");
            for(int Side=0;Side<2;++Side) {
                const auto F=ASAnimation::Foot(In.GaitPhase+float(Side)*ASAnimation::Pi,In.Weight);
                const V3 Want{6.f+F.Forward,(Side==0?-16.f:16.f),12.f+F.Lift},Hip{0,(Side==0?-16.f:16.f),79.f};
                if(Length(Want-Hip)<75.f) Check(NearV(O.Part[Side==0?BootL:BootR].Loc,Want+V3{3,0,-2}),"a reachable foot lands where the old gait put it (boots are 3 forward and 2 down of it)");
            }
            Check(NearV(O.FigureOffset,V3{})&&O.FigureScale.X==1.f&&O.FigureScale.Y==1.f&&O.FigureScale.Z==1.f&&O.Glow==0.f,"no offset, scale or glow without a pose");
        }
        // Heading: sideways travel swings the feet sideways.
        RigInput In;In.Weight=1.f;In.GaitPhase=0.f;In.HeadingX=0.f;In.HeadingY=1.f;RigOut O;Solve(In,D,O);
        const auto F=ASAnimation::Foot(0.f,1.f);
        Check(Near(O.Part[BootL].Loc.Y,-16.f+F.Forward,.5f)&&Near(O.Part[BootL].Loc.X,9.f,.5f),"travelling sideways swings the feet along Y instead of X");
        // The sniper's rifle hold.
        RigInput Gun;Gun.Gun=true;RigOut G;Solve(Gun,D,G);
        Check(NearV(G.Part[HandL].Loc,V3{76,-5,133})&&NearV(G.Part[HandR].Loc,V3{54,25,119}),"the sniper holds its rifle at the old fixed hand positions");
    }

    // ---- Solver: the upper body hinges about the hips ----
    {
        float D[ChannelCount];ClearChannels(D);
        RigInput In;RigOut Flat,Back;Solve(In,D,Flat);
        D[ChBend]=90.f;Solve(In,D,Back);Xf WFlat[SlotCount],WBack[SlotCount];ToFigureSpace(Flat,WFlat);ToFigureSpace(Back,WBack);
        const V3 Pivot{0.f,0.f,79.f};
        Check(NearV(Back.Part[Torso].Loc,V3{-32.f,0.f,79.f},.01f),"leaning back 90 degrees swings the chest from above the hips to behind them");
        Check(NearV(Back.Part[Pelvis].Loc,Flat.Part[Pelvis].Loc)&&NearV(Back.Part[BootL].Loc,Flat.Part[BootL].Loc)&&NearV(Back.Part[BootR].Loc,Flat.Part[BootR].Loc),"the pelvis and the feet do not move when the body bends");
        for(int Slot:{int(Torso),int(Head),int(PauldronL),int(PauldronR),int(Cape)}) Check(Near(Length(Back.Part[Slot].Loc-Pivot),Length(Flat.Part[Slot].Loc-Pivot),.01f),"every upper-body part keeps its distance from the hip line (the bend is rigid)");
        Check(NearV(Rotate(Back.Part[Torso].Rot,V3{0,0,1}),V3{-std::cos(5.f*DegToRad),0.f,-std::sin(5.f*DegToRad)},.01f)||Rotate(Back.Part[Torso].Rot,V3{0,0,1}).X<-.9f,"the torso's top points back along the bend");
        // The hands hang from the carried shoulders: the arm roots move with the body, the hand offsets do not rotate.
        Check(NearV(WBack[UpperArmR].Loc-WFlat[UpperArmR].Loc,V3{},500.f)&&Length(Back.Part[HandR].Loc-WFlat[HandR].Loc)>20.f,"the shoulders and hands move when the body bends");
        D[ChBend]=0.f;D[ChBendRoll]=40.f;RigOut Right;Solve(In,D,Right);
        Check(Right.Part[Head].Loc.Y>40.f&&Right.Part[Head].Loc.Z<Flat.Part[Head].Loc.Z,"leaning right 40 degrees moves the head to the right and lowers it");
        // No bend and no crouch: the hands are exactly where the old absolute positions put them (the equivalence test above covers the walking swing).
        Check(NearV(Flat.Part[HandL].Loc,V3{24.f,-45.f,74.f})&&NearV(Flat.Part[HandR].Loc,V3{24.f,45.f,74.f}),"with no bend the hands hang where they always did");
    }
    // ---- Solver: sweeps over every ability ----
    for(int Ability=1;Ability<ER::AbilityCount;++Ability) {
        const Track& T=TrackFor(Ability);
        for(int Sample=0;Sample<400;++Sample) {
            const float U=Range(0.f,TrackEnd);float D[ChannelCount];Evaluate(T,U,D);
            RigInput In;In.Weight=Rand()<.5f?0.f:Range(0,1);In.GaitPhase=Range(0,6.2831853f);In.Step=std::sin(In.GaitPhase)*In.Weight;In.Seconds=Range(0,50);In.Gun=Rand()<.15f;
            const float Angle=Range(0,6.2831853f);In.HeadingX=std::cos(Angle);In.HeadingY=std::sin(Angle);
            In.Release=Rand()<.3f?Range(0,1):0.f;In.Recoil=Rand()<.3f?Range(0,1):0.f;In.PushX=Range(-1,1);In.PushY=Range(-1,1);
            RigOut O;Solve(In,D,O);Xf World[SlotCount];ToFigureSpace(O,World);
            for(int Slot=0;Slot<SlotCount;++Slot) {
                const Xf& X=O.Part[Slot];
                Check(std::isfinite(X.Loc.X)&&std::isfinite(X.Loc.Y)&&std::isfinite(X.Loc.Z)&&std::isfinite(X.Scale.X)&&std::isfinite(X.Scale.Y)&&std::isfinite(X.Scale.Z),"every part transform is finite");
                Check(Near(X.Rot.X*X.Rot.X+X.Rot.Y*X.Rot.Y+X.Rot.Z*X.Rot.Z+X.Rot.W*X.Rot.W,1.f,.002f),"every rotation is a unit quaternion");
                Check(X.Scale.X>.19f&&X.Scale.Y>.19f&&X.Scale.Z>.19f&&X.Scale.X<3.f&&X.Scale.Y<3.f&&X.Scale.Z<3.f,"no part is crushed or blown up");
                const V3 P=World[Slot].Loc;
                Check(P.X>-170.f&&P.X<260.f&&P.Y>-170.f&&P.Y<170.f&&P.Z>-12.f&&P.Z<340.f,"every part stays within the figure's reach envelope");
            }
            for(int Slot:{UpperArmL,ForearmL,UpperArmR,ForearmR,ThighL,GreaveL,ThighR,GreaveR}) Check(Near(O.Part[Slot].Scale.Z,1.f,.002f),"limb segments keep their bone length (no stretching)");
            Check(O.Part[BootL].Loc.Z>6.f&&O.Part[BootR].Loc.Z>6.f,"the boots stay on or above the floor");
            Check(World[HandL].Loc.Z>10.f&&World[HandR].Loc.Z>10.f,"the hands never scrape the floor");
            Check(World[Head].Loc.Z>World[Torso].Loc.Z&&World[Torso].Loc.Z>World[Pelvis].Loc.Z,"head over torso over pelvis");
            Check(Length(World[HandL].Loc-(V3{0,-34,133}+V3{D[ChLeanX],D[ChLeanY],-D[ChCrouch]}))<=84.f&&Length(World[HandR].Loc-(V3{0,34,133}+V3{D[ChLeanX],D[ChLeanY],-D[ChCrouch]}))<=84.f,"each hand is within its arm's reach of its shoulder");
            Check(O.FigureScale.X>.19f&&O.FigureScale.Z>.19f&&std::isfinite(O.Glow),"figure scale and glow are sane");
            BlobOut B;SolveBlob(D,B);
            Check(B.Scale.X>.19f&&B.Scale.Y>.19f&&B.Scale.Z>.19f&&Near(B.Rot.X*B.Rot.X+B.Rot.Y*B.Rot.Y+B.Rot.Z*B.Rot.Z+B.Rot.W*B.Rot.W,1.f,.002f),"the blob body keeps a sane scale and a unit rotation");
        }
    }
    // ---- Baked deaths (retargeted from the Mannequin's death animations, ASEnemyDeathTracks.h) ----
    {
        Check(DeathVariants==DeathVariantCount&&DeathTrackEnd==1.f&&DeathFronts+3==DeathVariants&&DeathBack==DeathFronts&&DeathLeft==DeathBack+1&&DeathRight==DeathLeft+1,"six death variants: three fronts, a back, a left and a right");
        for(int V=0;V<DeathVariants;++V) {
            const Track& T=DeathTrack(V);std::set<int> Channels;bool Ended=false;int Curves=0;
            for(int I=0;I<MaxCurves;++I) {
                const Curve& C=T.Curves[I];
                if(C.Channel<0) {Ended=true;break;}
                ++Curves;
                Check(C.Channel<ChannelCount&&Channels.insert(C.Channel).second,"a death curve drives a real channel, once");
                int N=0;while(N<MaxKeys&&C.Keys[N].T>=0.f) ++N;
                Check(N>=2&&N<=MaxKeys-1,"a death curve has two to eleven keys and an end sentinel");
                Check(C.Keys[0].T==0.f&&C.Keys[0].V==0.f,"every death curve starts at the walking pose");
                Check(C.Keys[N-1].T==DeathTrackEnd,"... and runs to the end of the death");
                const float Limit=C.Channel==ChPlant?1.2f:(C.Channel==ChBend||C.Channel==ChBendRoll||C.Channel==ChTorsoYaw||C.Channel==ChHeadPitch||C.Channel==ChHeadYaw?130.f:(C.Channel>=ChWeaponPitch&&C.Channel<=ChWeaponRoll?170.f:
                    (C.Channel>=ChHandLX&&C.Channel<=ChHandRZ?170.f:(C.Channel==ChCrouch||C.Channel==ChLeanX||C.Channel==ChLeanY?90.f:(C.Channel==ChFootLX||C.Channel==ChFootRX?45.f:(C.Channel==ChStance?35.f:150.f))))));
                for(int K=0;K<N;++K) {Check(std::isfinite(C.Keys[K].V)&&std::fabs(C.Keys[K].V)<=Limit,"every death key is inside its channel's range");if(K) Check(C.Keys[K].T>C.Keys[K-1].T&&C.Keys[K].T<=DeathTrackEnd,"death keys increase inside the track");}
                float Previous=EvalCurve(C,0.f);
                for(float U=.005f;U<=1.0001f;U+=.005f) {const float Value=EvalCurve(C,U);Check(std::fabs(Value-Previous)<=Limit*.6f,"a death curve has no jumps");Previous=Value;}
            }
            Check(Ended&&Curves>=12,"a death track is a sentinel-terminated list of at least a dozen curves");
            for(int Sample=0;Sample<=200;++Sample) {
                float D[ChannelCount];Evaluate(T,float(Sample)/200.f,D);
                RigInput In;In.Seconds=float(Sample)*.01f;In.Gun=(V&1)!=0;
                RigOut O;Solve(In,D,O);Xf World[SlotCount];ToFigureSpace(O,World);
                for(int Slot=0;Slot<SlotCount;++Slot) {
                    const Xf& X=O.Part[Slot];const V3 P=World[Slot].Loc;
                    Check(std::isfinite(X.Loc.X)&&std::isfinite(X.Loc.Y)&&std::isfinite(X.Loc.Z)&&std::isfinite(X.Scale.X)&&std::isfinite(X.Scale.Z)&&Near(X.Rot.X*X.Rot.X+X.Rot.Y*X.Rot.Y+X.Rot.Z*X.Rot.Z+X.Rot.W*X.Rot.W,1.f,.002f),"a dying pose is finite with unit rotations");
                    Check(P.X>-260.f&&P.X<320.f&&P.Y>-230.f&&P.Y<230.f&&P.Z>-16.f&&P.Z<350.f,"a dying part stays inside the figure's envelope and never sinks below the floor");
                }
                for(int Slot:{int(UpperArmL),int(ForearmL),int(UpperArmR),int(ForearmR),int(ThighL),int(GreaveL),int(ThighR),int(GreaveR)}) Check(Near(O.Part[Slot].Scale.Z,1.f,.002f),"limbs keep their length while dying");
                Check(O.Part[BootL].Loc.Z>6.f&&O.Part[BootR].Loc.Z>6.f,"a dying enemy's boots stay on the floor");
            }
        }
        // The first frame of every death is the walking pose exactly.
        for(int V=0;V<DeathVariants;++V) {float D[ChannelCount];Evaluate(DeathTrack(V),0.f,D);bool Zero=true;for(float X:D) Zero&=X==0.f;Check(Zero,"every death starts neutral");}
        Check(&DeathTrack(-3)==&DeathTrack(0)&&&DeathTrack(99)==&DeathTrack(DeathVariants-1),"an out-of-range death variant clamps");
        // Which collapse plays.
        Check(DeathVariantFor(1.f,0.f,0u)==DeathBack&&DeathVariantFor(1.f,.5f,7u)==DeathBack,"pushed forward (hit from behind) it slumps forward: the back variant");
        Check(DeathVariantFor(0.f,1.f,0u)==DeathLeft&&DeathVariantFor(-.2f,1.f,5u)==DeathLeft,"pushed to its right (hit from the left) it goes down to the right: the left variant");
        Check(DeathVariantFor(0.f,-1.f,0u)==DeathRight&&DeathVariantFor(.2f,-1.f,5u)==DeathRight,"pushed to its left: the right variant");
        std::set<int> Fronts;for(unsigned Id=0;Id<30u;++Id) {const int V=DeathVariantFor(-1.f,.2f,Id);Check(V>=0&&V<DeathFronts,"hit from the front: one of the three doubled-over variants");Fronts.insert(V);}
        Check(Fronts.size()==3,"a crowd of front deaths uses all three");
        Check(DeathVariantFor(NaN,0.f,4u)<DeathFronts&&DeathVariantFor(0.f,NaN,4u)<DeathFronts,"a non-finite direction still gives a valid front variant");
    }
    std::printf("PASS: %lld enemy pose checks (Unreal rotation/quaternion math against the engine's matrix, track structure, timeline, the solver's neutral pose equals the old gait, sweeps over every ability: finite, in reach, no stretching, on the floor).\nShared rules only; Unreal rendering and the component hierarchy are not executed (see the offline pose preview).\n",Checks);
    return 0;
}
