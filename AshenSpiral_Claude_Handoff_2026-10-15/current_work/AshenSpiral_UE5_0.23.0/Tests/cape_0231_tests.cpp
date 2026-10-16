// Native structural tests for the 0.23.1 heavy cape chain. These check numbers, not how it looks.
#include "../Source/AshenSpiral/Public/ASRules.h"
#include "../Source/AshenSpiral/Public/ASAnimationRules.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
static int Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
using namespace ASAnimation;
static float Rest(int I) {const auto A=CapePoint(I-1,0,0,0,0),B=CapePoint(I,0,0,0,0);return std::sqrt((A.X-B.X)*(A.X-B.X)+(A.Y-B.Y)*(A.Y-B.Y)+(A.Z-B.Z)*(A.Z-B.Z));}
static void Invariants(const CapeChain& C) {
    for(int I=0;I<CapeNodes;++I) for(int A=0;A<3;++A) Check(std::isfinite(C.P[I][A])&&std::abs(C.P[I][A])<500.f,"cape stays finite and bounded");
    for(int I=1;I<CapeNodes;++I) {
        const float T=float(I)/6.f;
        Check(C.P[I][0]<=-27.f-3.f*T+.01f,"cape never enters the body: always behind the back plate");
        Check(C.P[I][2]>=8.f-.01f,"cape never goes through the floor");
        Check(std::abs(C.P[I][1])<=CapeWidth(T)+75.f+.01f,"cape reach is bounded sideways");
        const float D=std::sqrt(std::pow(C.P[I][0]-C.P[I-1][0],2)+std::pow(C.P[I][1]-C.P[I-1][1],2)+std::pow(C.P[I][2]-C.P[I-1][2],2));
        Check(std::abs(D-Rest(I))<Rest(I)*.12f,"cloth length is preserved within 12% (no visible stretching)");
    }
}
static float Settle(CapeChain& C,const CapeDrive& D,int Node,int Axis,float Final,float Fraction,float Limit) {
    // Seconds until the node is within Fraction of the way to Final (from where it started).
    const float Start=C.P[Node][Axis];float Time=0;
    while(Time<Limit) {C.Step(1.f/60.f,D);Time+=1.f/60.f;if(std::abs(C.P[Node][Axis]-Final)<=std::abs(Final-Start)*Fraction) return Time;}
    return Limit;
}
int main() {
    // Stability across random drives, frame rates and a long soak.
    for(float Dt:{1.f/240.f,1.f/60.f,1.f/20.f,1.f/10.f}) {
        ASRules::Random R(77);CapeChain C;CapeDrive D;C.Reset(D);
        for(int Step=0;Step<6000;++Step) {
            if(Step%40==0) {
                D.Speed=R.Range(150)/100.f;D.Turn=(R.Range(201)-100)/100.f;D.Flare=R.Range(101)/100.f;D.Lift=R.Range(101)/100.f;D.Wind=R.Range(101)/100.f;
                D.AccelX=float(R.Range(8001))-4000.f;D.AccelY=float(R.Range(8001))-4000.f;D.AccelZ=float(R.Range(8001))-4000.f;
            }
            D.Time+=Dt;C.Step(Dt,D);Invariants(C);
        }
    }
    // Heavy: lags a sudden sprint and settles with a beat of delay.
    {
        CapeChain C;CapeDrive Idle;Idle.Wind=0;C.Reset(Idle);for(int I=0;I<400;++I) C.Step(1.f/60.f,Idle);
        CapeDrive Sprint=Idle;Sprint.Speed=1.f;const float Final=CapeTarget(6,Sprint).X;
        const float Reach63=Settle(C,Sprint,6,0,Final,.37f,6.f);
        Check(Reach63>=.20f,"cape lags behind a sudden sprint (is not a rigid plane)");
        Check(Reach63<=2.5f,"cape does catch up while sprinting");
        const float Back=Settle(C,Idle,6,0,CapeTarget(6,Idle).X,.1f,8.f);
        Check(Back>=.45f&&Back<=5.f,"cape resettles slowly after a sprint, but does settle");
    }
    // Direction change: the hem catches and swings late.
    {
        CapeChain C;CapeDrive Idle;Idle.Wind=0;C.Reset(Idle);for(int I=0;I<300;++I) C.Step(1.f/60.f,Idle);
        CapeDrive Turn=Idle;Turn.Turn=1.f;const float Final=CapeTarget(6,Turn).Y,Start=C.P[6][1];
        for(int I=0;I<6;++I) C.Step(1.f/60.f,Turn);
        Check(std::abs(C.P[6][1]-Start)<std::abs(Final-Start)*.6f,"cape catches on a direction change instead of snapping");
    }
    // Inertia: forward acceleration drags the hem backward relative to a still character.
    {
        CapeChain A,B;CapeDrive D;D.Speed=.5f;A.Reset(D);B.Reset(D);CapeDrive Pushed=D;Pushed.AccelX=2600.f;
        for(int I=0;I<20;++I) {A.Step(1.f/60.f,D);B.Step(1.f/60.f,Pushed);}
        Check(B.P[6][0]<A.P[6][0]-3.f,"acceleration pushes the hem back (inertia)");
    }
    // Combat responses: dash/swing flare spreads the cape out; the leap lifts it; both stay clear of the body.
    {
        CapeDrive Calm;Calm.Speed=.2f;CapeChain Base,Flared,Lifted;Base.Reset(Calm);
        CapeDrive F=Calm;F.Flare=1.f;CapeDrive L=Calm;L.Lift=1.f;Flared.Reset(Calm);Lifted.Reset(Calm);
        for(int I=0;I<120;++I) {Base.Step(1.f/60.f,Calm);Flared.Step(1.f/60.f,F);Lifted.Step(1.f/60.f,L);Invariants(Flared);Invariants(Lifted);}
        Check(Flared.P[6][0]<Base.P[6][0]-10.f||Flared.P[6][2]>Base.P[6][2]+10.f,"dash and swings flare the cape");
        Check(Lifted.P[6][2]>Base.P[6][2]+30.f,"the leap lifts the hem");
    }
    // Idle wind gives a broad billow that is visibly different from dead calm.
    {
        CapeChain Calm,Windy;CapeDrive A,B;B.Wind=1.f;Calm.Reset(A);Windy.Reset(B);
        for(int I=0;I<200;++I) {A.Time+=1.f/60.f;B.Time+=1.f/60.f;Calm.Step(1.f/60.f,A);Windy.Step(1.f/60.f,B);}
        Check(Windy.P[6][0]<Calm.P[6][0]-25.f,"idle wind billows the hem out");
    }
    std::cout<<"PASS: "<<Checks<<" heavy cape chain checks.\n";
}
