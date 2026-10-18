#pragma once
#include <algorithm>
#include <cmath>
// Pure, shared pose/timing rules. UE and the offline geometry tests use these.
namespace ASHammer {
constexpr float Cooldown=7.5f,Startup=.16f,Flight=1.04f,Recovery=.22f,LandingDamage=100.f,LandingRadius=370.f;
constexpr float Height=208.35f; // 1.5 x the 660 cm/s jump apex at gravity 1568 cm/s^2.
struct Pose {float X,Y,Z,Pitch,Yaw,Roll,BodyYaw,BodyPitch;};
struct V {float X,Y,Z;};
inline V Add(V A,V B) {return {A.X+B.X,A.Y+B.Y,A.Z+B.Z};}
inline V Sub(V A,V B) {return {A.X-B.X,A.Y-B.Y,A.Z-B.Z};}
inline V Mul(V A,float S) {return {A.X*S,A.Y*S,A.Z*S};}
inline float Dot(V A,V B) {return A.X*B.X+A.Y*B.Y+A.Z*B.Z;}
inline V Cross(V A,V B) {return {A.Y*B.Z-A.Z*B.Y,A.Z*B.X-A.X*B.Z,A.X*B.Y-A.Y*B.X};}
inline V Unit(V A) {return Mul(A,1.f/std::max(.00001f,std::sqrt(Dot(A,A))));}
inline float Smooth(float T) {T=std::clamp(T,0.f,1.f);return T*T*(3.f-2.f*T);}
inline Pose Blend(Pose A,Pose B,float T) {
    T=Smooth(T);return {A.X+(B.X-A.X)*T,A.Y+(B.Y-A.Y)*T,A.Z+(B.Z-A.Z)*T,
    A.Pitch+(B.Pitch-A.Pitch)*T,A.Yaw+(B.Yaw-A.Yaw)*T,A.Roll+(B.Roll-A.Roll)*T,
    A.BodyYaw+(B.BodyYaw-A.BodyYaw)*T,A.BodyPitch+(B.BodyPitch-A.BodyPitch)*T};
}
inline Pose Hold() {return {43,6,104,70,-35,0,-6,4};}
inline float Impact(int Step) {return Step==2?.76f:.56f;}
inline float Hitstop(int Step) {return Step==2?.14f:.06f;}
inline float Cycle(int Step) {return Step==2?1.188f:.72f;}
inline float ComboWindow(float BaseInterval) {return 1.2f*BaseInterval/.72f;}
// TUNABLE: the finisher slows movement by 45% (was 80%) instead of rooting the player.
constexpr float FinisherSlow=.45f;
inline float FinisherSpeedScale() {return 1.f-FinisherSlow;}
inline bool Frontal(float DotForward) {return DotForward>0.f;}
inline bool Committed(int Weapon,int Step,float Now,float Start,float Duration) {return Weapon==1&&Step==2&&Start>=0&&Now>=Start&&Now<Start+Duration;}
inline bool LeapProtected(int State,float Now,float LandedAt) {return State==2||(State==3&&Now<LandedAt+Recovery);}
inline float LeapDistance(float DashSeconds) {return 2350.f*DashSeconds*1.15f;}
// The contact hold lives inside the full cycle: haste and DPS stay proportional.
inline float PosePhase(int Step,float Phase,bool Contact) {
    if(!Contact) return std::clamp(Phase,0.f,1.f);
    const float At=Impact(Step),Hold=Hitstop(Step)/Cycle(Step);
    if(Phase<=At) return Phase;
    if(Phase<=At+Hold) return At;
    return At+(1.f-At)*(Phase-At-Hold)/(1.f-At-Hold);
}
inline Pose Swing(int Step,float Phase) {
    const float P=std::clamp(Phase,0.f,1.f);const Pose Rest=Hold();
    Pose Wind,Strike,Follow;float WindEnd;
    if(Step==2) { // Reference 6-7: shoulder/hip coil; 8: planted strike; 9-10: settle.
        Wind={35,0,155,-5,0,-90,-18,-12};Strike={42,0,112,140,0,-90,12,16};
        Follow={40,0,113,138,0,-90,15,13};WindEnd=.48f;
    } else { // References 1-2 / 3-5. Local +Y is right: first stroke travels left to right.
        const float Side=Step==0?1.f:-1.f;
        Wind={40,0,114,85,-Side*65,0,-Side*26,-4};
        Strike={42,0,108,85,Side*20,0,Side*13,7};
        Follow={40,0,112,85,Side*65,0,Side*28,5};WindEnd=.25f;
    }
    const float At=Impact(Step),FollowEnd=Step==2?.88f:.78f;
    if(P<WindEnd) return Blend(Rest,Wind,P/WindEnd);
    if(P<At) return Blend(Wind,Strike,(P-WindEnd)/(At-WindEnd));
    if(P<FollowEnd) return Blend(Strike,Follow,(P-At)/(FollowEnd-At));
    return Blend(Follow,Rest,(P-FollowEnd)/(1.f-FollowEnd));
}
inline Pose Leap(int State,float Phase) {
    const Pose Crouch={43,0,98,85,-25,0,-12,17},Raised={35,0,155,-5,0,-90,-12,-12};
    const Pose Strike={42,0,112,140,0,-90,12,16};
    if(State==1) return Blend(Hold(),Crouch,Phase);
    if(State==2) {
        if(Phase<.28f) return Blend(Crouch,Raised,Phase/.28f);
        if(Phase<.65f) return Raised;
        return Blend(Raised,Strike,(Phase-.65f)/.35f);
    }
    return Blend(Strike,Hold(),Phase);
}
inline V Shaft(Pose P) {const float R=.0174532925199433f,A=P.Pitch*R,B=P.Yaw*R;return {std::sin(A)*std::cos(B),std::sin(A)*std::sin(B),std::cos(A)};}
inline V BodyPoint(Pose P,V Point) {
    const float A=P.BodyPitch*.0174532925199433f,B=P.BodyYaw*.0174532925199433f;
    Point.Z-=111.f;const float X=Point.X*std::cos(A)+Point.Z*std::sin(A),Z=-Point.X*std::sin(A)+Point.Z*std::cos(A);
    return {X*std::cos(B)-Point.Y*std::sin(B),X*std::sin(B)+Point.Y*std::cos(B),Z+111.f};
}
inline V HeadCenter(Pose P) {return Add({P.X,P.Y,P.Z},Mul(Shaft(P),104.f));}
struct Frame {V X,Y,Z;};
inline Frame Orientation(Pose P,Pose Before,Pose After) {
    const V Z=Shaft(P);V T=Sub(HeadCenter(After),HeadCenter(Before));T=Sub(T,Mul(Z,Dot(T,Z)));
    const float R=.0174532925199433f,A=P.Pitch*R,B=P.Yaw*R,C=P.Roll*R;
    const V BaseX={std::cos(A)*std::cos(B),std::cos(A)*std::sin(B),-std::sin(A)},BaseY={-std::sin(B),std::cos(B),0};
    const V Reference=Add(Mul(BaseX,-std::sin(C)),Mul(BaseY,std::cos(C)));
    // The squared head's long axis follows the tangent, not a fixed world axis.
    V Y=Dot(T,T)>.000001f?Unit(T):Reference;if(Dot(Y,Reference)<0) Y=Mul(Y,-1);
    return {Unit(Cross(Y,Z)),Y,Z};
}
inline Frame SwingFrame(int Step,float P) {return Orientation(Swing(Step,P),Swing(Step,std::max(0.f,P-.001f)),Swing(Step,std::min(1.f,P+.001f)));}
inline Frame LeapFrame(int State,float P) {return Orientation(Leap(State,P),Leap(State,std::max(0.f,P-.001f)),Leap(State,std::min(1.f,P+.001f)));}
inline V Transform(Frame F,V P) {return Add(Add(Mul(F.X,P.X),Mul(F.Y,P.Y)),Mul(F.Z,P.Z));}
}
