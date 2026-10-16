#pragma once
#include <algorithm>
#include <cmath>
namespace ASAnimation {
constexpr float Pi=3.14159265358979323846f;
inline float Blend(float Delta,float Response) {return 1.f-std::exp(-std::max(0.f,Delta)*std::max(0.f,Response));}
struct Bend {float Along=0,Out=0,Reach=0;};
inline Bend TwoBone(float Distance,float Upper,float Lower) {
    Upper=std::max(.01f,Upper);Lower=std::max(.01f,Lower);
    const float D=std::clamp(Distance,std::abs(Upper-Lower)+.001f,Upper+Lower-.001f);
    const float A=(Upper*Upper-Lower*Lower+D*D)/(2.f*D);
    return {A,std::sqrt(std::max(0.f,Upper*Upper-A*A)),D};
}
// Visual gait follows actual travel. Contact feet are held in world space by
// ASPose::Plant; neither this phase nor the spring alters movement velocity.
inline float UnitPhase(float Phase) {float P=std::fmod(Phase/(2.f*Pi),1.f);return P<0?P+1.f:P;}
inline float StanceFraction(float Weight) {return .58f-.24f*std::clamp(Weight,0.f,1.f);}
inline bool FootContact(float Phase,float Weight) {return UnitPhase(Phase)<StanceFraction(Weight);}
inline float StrideDistance(float Weight) {return 165.f+95.f*std::clamp(Weight,0.f,1.f);}
struct Step {float Forward=0,Lift=0,Pitch=0;};
inline Step Foot(float Phase,float Weight) {
    const float P=UnitPhase(Phase),Stance=StanceFraction(Weight);
    Weight=std::clamp(Weight,0.f,1.f);const float Stride=37.f*Weight;
    if(P<Stance) {const float T=P/Stance;return {Stride*(1.f-2.f*T),0.f,(-7.f+13.f*T)*Weight};}
    const float T=(P-Stance)/(1.f-Stance),S=T*T*(3.f-2.f*T);
    return {-Stride+2.f*Stride*S,(14.f+14.f*Weight)*Weight*std::sin(T*Pi),-24.f*Weight*std::sin(T*Pi)};
}
inline float Reaction(float Age,float Strength,bool Player=false) {
    const float Length=Player?.23f:(Strength>.7f?.48f:.23f);
    if(Age<0.f||Age>=Length) return 0.f;
    const float T=Age/Length;
    // Immediate readable recoil; no gameplay stun and no movement lock.
    return std::clamp(Strength,0.f,1.f)*std::sin(std::min(1.f,T/.16f)*Pi*.5f)*std::pow(1.f-T,2.f);
}
inline float ReleaseGesture(float Age) {
    if(Age<0.f||Age>=.42f) return 0.f;
    const float T=Age/.42f;return std::sin(std::min(T/.22f,1.f)*Pi*.5f)*std::pow(1.f-T,1.4f);
}
struct ClothPoint {float X,Y,Z;};
// Continuous six-link cloth centerline. X is always behind the torso/backplate.
// Independent traveling waves, inertia and attack lift replace the rigid board.
inline ClothPoint CapePoint(int Node,float Time,float Speed,float Turn,float Flare) {
    const float T=std::clamp(float(Node)/6.f,0.f,1.f);
    Speed=std::clamp(Speed,0.f,1.5f);Turn=std::clamp(Turn,-1.f,1.f);Flare=std::clamp(Flare,0.f,1.f);
    const float Stream=std::clamp(Speed*.68f+Flare*.36f,0.f,.95f);
    const float Ripple=std::sin(Time*5.5f-T*8.f)*(1.2f+Speed*4.f)*T;
    return {-27.f-T*(8.f+76.f*Stream)-std::abs(Ripple)*.7f,
        T*T*(-Turn*28.f)+std::sin(Time*3.7f-T*6.5f)*(1.f+Speed*3.f)*T,
        141.f-126.f*T+45.f*Stream*T*T+Ripple};
}
inline float CapeWidth(float T) {return 27.f+19.f*std::clamp(T,0.f,1.f);}
}
