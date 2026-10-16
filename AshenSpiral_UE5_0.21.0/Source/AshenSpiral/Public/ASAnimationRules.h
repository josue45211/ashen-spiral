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
struct Step {float Forward=0,Lift=0,Pitch=0;};
inline Step Foot(float Phase,float Weight) {
    float P=std::fmod(Phase/(2.f*Pi),1.f);if(P<0) P+=1.f;
    Weight=std::clamp(Weight,0.f,1.f);const float Stride=34.f*Weight;
    if(P<.58f) {const float T=P/.58f;return {Stride*(1.f-2.f*T),0.f,(-8.f+16.f*T)*Weight};}
    const float T=(P-.58f)/.42f,S=T*T*(3.f-2.f*T);
    return {-Stride+2.f*Stride*S,24.f*Weight*std::sin(T*Pi),-18.f*Weight*std::sin(T*Pi)};
}
}
