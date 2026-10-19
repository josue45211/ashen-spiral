// Native tests for the 0.24 Endless cover-collision rule (ASExpedition::CoverSolidDecision and the shift timeline it builds on). Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASExpeditionRules.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.002f) {return std::fabs(A-B)<E;}
int main() {
    using namespace ASExpedition;
    const float T0=100.f;
    // ---- The timeline: solid, then off from a little before the swap until the new cover is ~70% up, then solid again ----
    const float OffStart=-ShiftSinkSeconds*(1.f-std::sqrt(ShiftSolidBelow)); // seconds relative to the swap
    const float OnAt=ShiftRiseSeconds*std::sqrt(1.f-ShiftSolidBelow);
    Check(Near(OffStart,-.8f)&&Near(OnAt,1.3856f,.001f),"timeline constants: off from 0.8 s before the swap, on again 1.39 s after it");
    Check(ShiftCoverSolid(T0-5.f,T0)&&ShiftCoverSolid(T0+OffStart-.01f,T0),"cover is solid until just before it sinks");
    Check(!ShiftCoverSolid(T0+OffStart+.01f,T0)&&!ShiftCoverSolid(T0,T0)&&!ShiftCoverSolid(T0+OnAt-.01f,T0),"cover is passable from 0.8 s before the swap until it has risen");
    Check(ShiftCoverSolid(T0+OnAt+.01f,T0)&&ShiftCoverSolid(T0+ShiftRiseSeconds,T0)&&ShiftCoverSolid(T0+30.f,T0),"cover is solid again once it is about 70% risen");
    Check(ShiftCoverSolid(50.f,-100.f)&&ShiftCoverSolid(50.f,0.f),"no shift scheduled means solid");
    const float NaN=std::numeric_limits<float>::quiet_NaN();
    Check(ShiftCoverSolid(NaN,T0)&&ShiftCoverSolid(5.f,NaN),"non-finite times are treated as no shift (solid), never as a crash");

    // ---- The decision table ----
    // Timeline off: never solid, whatever else is true.
    Check(!CoverSolidDecision(T0,T0,true,false,0.f)&&!CoverSolidDecision(T0,T0,false,false,99.f)&&!CoverSolidDecision(T0+.5f,T0,true,true,0.f),"collision is off while the timeline says so, even if it was on before");
    // Timeline on, already solid or undecided: stays solid even with a hero beside a wall.
    Check(CoverSolidDecision(T0+5.f,T0,true,true,0.f)&&CoverSolidDecision(T0+5.f,T0,true,false,0.f),"collision that is already on never flips back on account of a hero");
    // Switching on: only when nobody is near, or after the hold-off cap.
    Check(CoverSolidDecision(T0+2.f,T0,false,false,0.f),"switches on at once when nobody is near a wall");
    Check(!CoverSolidDecision(T0+2.f,T0,false,true,0.f)&&!CoverSolidDecision(T0+2.f,T0,false,true,ShiftHoldOffMaxSeconds-.01f),"switching on waits while a hero is near a rising wall");
    Check(CoverSolidDecision(T0+2.f,T0,false,true,ShiftHoldOffMaxSeconds)&&CoverSolidDecision(T0+2.f,T0,false,true,99.f),"the hold-off ends after its cap, so one player cannot keep every wall passable");
    Check(!CoverSolidDecision(T0+2.f,T0,false,true,NaN),"a non-finite hold-off time never forces collision on");
    Check(ShiftHoldOffMaxSeconds>0.f&&ShiftHoldOffMaxSeconds<10.f&&ShiftHoldOffRadius>=100.f,"hold-off cap and radius are sane (radius larger than a hero plus network delay)");

    // ---- A hero parked beside a wall: collision comes on exactly when the cap runs out ----
    {bool Solid=false;float HoldSince=-1.f,OnTime=-1.f;const float Dt=1.f/60.f;
        for(float Now=T0-3.f;Now<T0+20.f;Now+=Dt) {
            const bool Timeline=ShiftCoverSolid(Now,T0);
            const bool Switching=Timeline&&!Solid&&Now>T0;       // after the swap, coming back on
            if(Switching) {if(HoldSince<0.f) HoldSince=Now;} else HoldSince=-1.f;
            const bool Was=Solid;
            Solid=CoverSolidDecision(Now,T0,Now<T0?true:Was,true,HoldSince<0.f?0.f:Now-HoldSince);
            if(!Was&&Solid&&Now>T0&&OnTime<0.f) OnTime=Now;
        }
        Check(OnTime>0.f&&Near(OnTime-(T0+OnAt),ShiftHoldOffMaxSeconds,.1f),"with a hero parked beside a wall, collision comes on ShiftHoldOffMaxSeconds after the timeline allows it");}

    // ---- Randomised invariant check over many shifts: collision never switches on near a hero before the cap, and only ever on, never flickers ----
    unsigned Seed=20240;auto Rand=[&Seed]() {Seed=Seed*1664525u+1013904223u;return float((Seed>>8)&0xFFFF)/65535.f;};
    for(int Trial=0;Trial<2000;++Trial) {
        const float Shift=50.f+Rand()*100.f;bool Solid=true;float HoldSince=-1.f;int Flips=0;bool Wrong=false;
        const float NearFrom=Shift-1.f+Rand()*3.f,NearUntil=NearFrom+Rand()*8.f; // a hero stands near a wall for a random stretch
        for(float Now=Shift-3.f;Now<Shift+15.f;Now+=.05f) {
            const bool Timeline=ShiftCoverSolid(Now,Shift),HeroNear=Now>=NearFrom&&Now<NearUntil;
            const bool Switching=Timeline&&!Solid;
            if(Switching) {if(HoldSince<0.f) HoldSince=Now;} else HoldSince=-1.f;
            const bool Next=CoverSolidDecision(Now,Shift,Solid,HeroNear,HoldSince<0.f?0.f:Now-HoldSince);
            if(Next!=Solid) ++Flips;
            if(!Timeline&&Next) Wrong=true;                                                  // never solid while the timeline is off
            if(!Solid&&Next&&HeroNear&&Now-HoldSince<ShiftHoldOffMaxSeconds-.001f) Wrong=true; // never switches on beside a hero before the cap
            Solid=Next;
        }
        Check(!Wrong,"randomised: collision never turns on against the rule");
        Check(Flips==2,"randomised: collision turns off once and on once per shift, with no flicker");
    }
    std::printf("PASS: %lld arena-shift collision checks (timeline windows, decision table, hold-off cap, 2000 randomised shifts).\nShared rules only; Unreal collision, replication and gameplay are not executed.\n",Checks);
    return 0;
}
