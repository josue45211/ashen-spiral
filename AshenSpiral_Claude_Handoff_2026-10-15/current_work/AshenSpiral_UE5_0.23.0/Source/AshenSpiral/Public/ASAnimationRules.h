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
// ---- Heavy cape (0.23.1 A3): a damped, length-constrained chain in figure-local space.
// Heavy cloth: soft springs, real inertia, delayed turn/dash/leap/swing response, broad idle billow and a slow resettle.
// Node 0 is pinned to the shoulders. Nodes never enter the body (X stays behind the back plate) or the floor.
struct CapeDrive {float Time=0,Speed=0,Turn=0,Flare=0,Lift=0,Wind=0,AccelX=0,AccelY=0,AccelZ=0;};
constexpr int CapeNodes=7;
inline ClothPoint CapeTarget(int Node,const CapeDrive& D) {
    ClothPoint B=CapePoint(Node,D.Time,D.Speed,D.Turn,D.Flare);
    const float T=std::clamp(float(Node)/6.f,0.f,1.f),W=std::clamp(D.Wind,0.f,1.f),L=std::clamp(D.Lift,0.f,1.f);
    // Broad, slow billow (knight-in-the-wind reference): the hem lifts out and back, then falls.
    B.X-=62.f*W*T*T+22.f*L*T;B.Z+=34.f*W*T*T*T+76.f*L*T*T;
    B.Y+=std::sin(D.Time*.85f-T*2.1f)*46.f*W*T*T;
    return B;
}
struct CapeChain {
    float P[CapeNodes][3]={},V[CapeNodes][3]={};bool Ready=false;
    void Reset(const CapeDrive& D) {
        for(int I=0;I<CapeNodes;++I) {const ClothPoint C=CapeTarget(I,D);P[I][0]=C.X;P[I][1]=C.Y;P[I][2]=C.Z;V[I][0]=V[I][1]=V[I][2]=0.f;}
        Ready=true;
    }
    void Step(float Dt,const CapeDrive& D) {
        if(!Ready) Reset(D);
        Dt=std::clamp(Dt,0.f,.1f);const int Sub=std::max(1,int(std::ceil(Dt/(1.f/90.f))));const float H=Dt/float(Sub);
        static float Rest[CapeNodes]={};static bool RestReady=false;
        if(!RestReady) {
            CapeDrive Idle;for(int I=1;I<CapeNodes;++I) {const ClothPoint A=CapePoint(I-1,0,0,0,0),B=CapePoint(I,0,0,0,0);Rest[I]=std::sqrt((A.X-B.X)*(A.X-B.X)+(A.Y-B.Y)*(A.Y-B.Y)+(A.Z-B.Z)*(A.Z-B.Z));}
            (void)Idle;RestReady=true;
        }
        for(int S=0;S<Sub;++S) {
            const ClothPoint Pin=CapeTarget(0,D);P[0][0]=Pin.X;P[0][1]=Pin.Y;P[0][2]=Pin.Z;V[0][0]=V[0][1]=V[0][2]=0.f;
            for(int I=1;I<CapeNodes;++I) {
                const float T=float(I)/6.f,K=36.f-20.f*T,C=2.f*.6f*std::sqrt(K);
                const ClothPoint G=CapeTarget(I,D);const float Tg[3]={G.X,G.Y,G.Z},Ac[3]={D.AccelX,D.AccelY,D.AccelZ};
                for(int A=0;A<3;++A) {
                    const float Acc=K*(Tg[A]-P[I][A])-C*V[I][A]-.55f*T*std::clamp(Ac[A],-4000.f,4000.f);
                    V[I][A]=std::clamp(V[I][A]+Acc*H,-1400.f,1400.f);P[I][A]+=V[I][A]*H;
                }
            }
            // Alternate the length and clearance constraints; clearance (body/floor) always has the last word.
            for(int Pass=0;Pass<4;++Pass) {
                for(int I=1;I<CapeNodes;++I) {
                    float Dx=P[I][0]-P[I-1][0],Dy=P[I][1]-P[I-1][1],Dz=P[I][2]-P[I-1][2];const float Len=std::sqrt(Dx*Dx+Dy*Dy+Dz*Dz);
                    if(Len>.001f) {const float F=Rest[I]/Len;P[I][0]=P[I-1][0]+Dx*F;P[I][1]=P[I-1][1]+Dy*F;P[I][2]=P[I-1][2]+Dz*F;}
                }
                for(int I=1;I<CapeNodes;++I) {
                    const float T=float(I)/6.f;
                    P[I][0]=std::min(P[I][0],-27.f-3.f*T);          // always behind the back plate and legs
                    P[I][2]=std::max(P[I][2],8.f);                   // never through the floor
                    const float Reach=CapeWidth(T)+75.f;P[I][1]=std::clamp(P[I][1],-Reach,Reach);
                }
            }
        }
    }
};

}
