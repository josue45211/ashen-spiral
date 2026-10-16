#pragma once
#include <algorithm>
#include <cmath>

namespace ASPresentation {
inline float HUDScale(float Value) {return std::isfinite(Value)?float(std::clamp(int(std::lround(std::clamp(Value,.25f,1.f)*4.f)),1,4))*.25f:1.f;}
constexpr float NoticeLife=9.f,NoticeHold=4.5f;
inline float NoticeAlpha(float Age) {return Age<0?0.f:std::clamp((NoticeLife-Age)/(NoticeLife-NoticeHold),0.f,1.f);}
inline float NumberScale(float Value) {
    if(!std::isfinite(Value)) return 1.2f;
    return .8f+.2f*std::clamp(int(std::lround((std::clamp(Value,.8f,1.6f)-.8f)/.2f)),0,4);
}
inline float CrosshairScale(float Value) {return std::isfinite(Value)?std::clamp(Value,.5f,2.f):1.f;}
// Includes the larger critical-hit flash, so text never covers the aim point.
inline float CrosshairClearance(float Scale) {return 24.f*CrosshairScale(Scale)+16.f;}
inline float DamageTop(float CenterY,float Scale,float TextHeight,float Age,int Lane,bool Incoming) {
    return CenterY-CrosshairClearance(Scale)-std::max(0.f,TextHeight)-std::max(0.f,Age)*55.f
        -float(std::clamp(Lane,0,2))*24.f-(Incoming?76.f:0.f);
}
}
