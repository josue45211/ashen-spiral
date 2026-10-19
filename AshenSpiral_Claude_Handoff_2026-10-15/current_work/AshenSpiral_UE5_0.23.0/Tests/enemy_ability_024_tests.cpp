// Native tests for the 0.24 enemy ability rules (ASEnemyAbilities.h): shove, back-hop, parry, ground pound shockwave and enrage, hazards, guard break, meteors, blink, sniper
// tracking and relocation. Plain C++; Unreal combat, collision, replication and rendering are not executed.
#include "../Source/AshenSpiral/Public/ASEnemyAbilities.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.001f) {return std::fabs(A-B)<E;}
using namespace ASEnemyRules;
int main() {
    const float NaN=std::numeric_limits<float>::quiet_NaN(),Inf=std::numeric_limits<float>::infinity();
    // ---- Shove ----
    Check(ShoveRadius>MeleeRadius&&ShoveDamage<MeleeDamage&&ShoveKnockback>0.f&&ShoveLift>=0.f,"a shove reaches further than a slash, hurts less, and throws the hero");
    Check(StrikeHits(200.f,.9f,ShoveRadius)&&!StrikeHits(280.f,.9f,ShoveRadius)&&!StrikeHits(100.f,-.1f,ShoveRadius),"the shove hits inside exactly the half disc its warning draws");
    // ---- Back-hop ----
    {
        float X,Y;
        Check(BackHopDirection(1.f,0.f,[](float,float) {return true;},X,Y)&&Near(X,-1.f)&&Near(Y,0.f),"in the open the archer hops straight away from the target");
        Check(BackHopDirection(1.f,0.f,[](float Dx,float) {return Dx>-.5f;},X,Y)&&X>-.5f&&Near(X*X+Y*Y,1.f,.002f),"with a wall behind it hops along the nearest clear direction");
        Check(!BackHopDirection(1.f,0.f,[](float,float) {return false;},X,Y)&&X==0.f&&Y==0.f,"boxed in, it does not hop");
        Check(!BackHopDirection(0.f,0.f,[](float,float) {return true;},X,Y),"no target direction, no hop");
        Check(BackHopSpeed*BackHopSeconds>=BackHopDistance*.9f&&BackHopSpeed*BackHopSeconds<=BackHopDistance*1.1f,"the hop's speed and time cover its distance");
    }
    // ---- Parry ----
    Check(Near(ParryDamage(true,100.f),15.f)&&ParryDamage(false,100.f)==100.f&&ParryDamage(true,0.f)==0.f,"a parrying duelist takes 15% damage, an unguarded one all of it");
    Check(ParryStanceSeconds>=.6f&&ParryRiposteDelay<ParryStanceSeconds&&RiposteRadius>=MeleeRadius&&RiposteDamage>MeleeDamage,"the stance is readable, the counter comes quickly, reaches at least a slash and hurts more");
    // ---- Shockwave ----
    {
        Check(ShockwaveRadius(0.f,PoundStartRadius,PoundSpeed,PoundMaxRadius)==PoundStartRadius&&ShockwaveRadius(100.f,PoundStartRadius,PoundSpeed,PoundMaxRadius)==PoundMaxRadius&&ShockwaveRadius(-3.f,PoundStartRadius,PoundSpeed,PoundMaxRadius)==PoundStartRadius,"the ring starts at its start radius, grows, and stops at its maximum");
        Check(ShockwaveRadius(NaN,1.f,1.f,1.f)==0.f&&ShockwaveRadius(1.f,1.f,Inf,100.f)==0.f,"a non-finite ring is nothing");
        float Last=0.f;bool Monotone=true;for(float T=0.f;T<2.f;T+=.01f) {const float R=ShockwaveRadius(T,PoundStartRadius,PoundSpeed,PoundMaxRadius);if(R<Last) Monotone=false;Last=R;}
        Check(Monotone,"the ring never shrinks");
        Check(ShockwaveHits(100.f,200.f,150.f,true,0.f,PoundBand,PoundJumpClearance)&&!ShockwaveHits(100.f,200.f,150.f,false,0.f,PoundBand,PoundJumpClearance)&&!ShockwaveHits(100.f,200.f,150.f,true,PoundJumpClearance+1.f,PoundBand,PoundJumpClearance),"the front hurts a grounded hero and not one in the air or above the jump clearance");
        Check(!ShockwaveHits(100.f,200.f,20.f,true,0.f,PoundBand,PoundJumpClearance)&&ShockwaveHits(100.f,200.f,40.f,true,0.f,PoundBand,PoundJumpClearance)&&!ShockwaveHits(100.f,200.f,400.f,true,0.f,PoundBand,PoundJumpClearance)&&!ShockwaveHits(NaN,200.f,150.f,true,0.f,PoundBand,PoundJumpClearance),"outside the swept band (widened by the band on each side), or with a broken value, nobody is hit");
        // Simulate: a ring of heroes at many distances, frame times from 4 ms to 250 ms. Every grounded hero inside the maximum is hit exactly once; a jumping one never.
        for(float Dt:{.004f,.016f,.033f,.066f,.12f,.25f}) {
            std::set<int> Hit,JumpHit;int Heroes=0;
            const int Count=60;float Distance[Count];
            for(int I=0;I<Count;++I) Distance[I]=20.f+float(I)*(PoundMaxRadius+200.f)/float(Count);
            float Prev=0.f; // the first step sweeps from the centre, so a hero standing right under the brute is caught too
            for(float Age=Dt;Age<4.f;Age+=Dt) {
                const float Radius=ShockwaveRadius(Age,PoundStartRadius,PoundSpeed,PoundMaxRadius);
                for(int I=0;I<Count;++I) {
                    if(ShockwaveHits(Prev,Radius,Distance[I],true,0.f,PoundBand,PoundJumpClearance)) Hit.insert(I);
                    if(ShockwaveHits(Prev,Radius,Distance[I],true,PoundJumpClearance+20.f,PoundBand,PoundJumpClearance)) JumpHit.insert(I);
                }
                Prev=Radius;
            }
            for(int I=0;I<Count;++I) if(Distance[I]<=PoundMaxRadius) {++Heroes;Check(Hit.count(I)==1,"a grounded hero inside the maximum radius is caught, whatever the frame time");}
            Check(JumpHit.empty(),"a hero who jumped is never caught, whatever the frame time");
            Check(Heroes>10,"the simulation really covered heroes");
        }
        Check(EnrageDue(false,100.f,200.f)&&!EnrageDue(false,101.f,200.f)&&!EnrageDue(true,10.f,200.f)&&!EnrageDue(false,0.f,200.f)&&!EnrageDue(false,NaN,200.f)&&!EnrageDue(false,10.f,0.f),"a brute enrages once, at half health, never dead and never on a broken value");
        Check(EnrageSpeedScale>1.f&&EnrageWindupScale<1.f&&EnrageCooldownScale<1.f&&EnrageRoarSeconds>=.8f,"enrage makes it faster, quicker to wind up and quicker to repeat, after a readable roar");
    }
    // ---- Hazards ----
    {
        for(int Style=0;Style<HazardStyles;++Style) {
            const HazardSpec& H=HazardFor(Style);
            Check(H.Radius>=150.f&&H.Radius<=400.f&&H.Seconds>=3.f&&H.Seconds<=8.f&&H.Damage>0.f&&H.Interval>=.3f&&H.Interval<=1.f,"hazard sizes, lifetimes and ticks are sane");
            Check(H.R>=0.f&&H.R<=1.f&&H.G>=0.f&&H.G<=1.f&&H.B>=0.f&&H.B<=1.f,"hazard colours are colours");
            const bool Cyan=H.G>.8f&&H.B>.8f&&H.R<.3f;
            Check((Style==HazardFrost)==Cyan,"only the frost hazard is cyan (blue is reserved for frost)");
        }
        Check(&HazardFor(-2)==&HazardFor(0)&&&HazardFor(77)==&HazardFor(HazardStyles-1),"an out-of-range style clamps");
        Check(HazardContains(100.f,0.f,170.f)&&!HazardContains(170.f,0.f,170.f)&&!HazardContains(100.f,200.f,170.f)&&HazardContains(100.f,-149.f,170.f)&&!HazardContains(NaN,0.f,170.f),"a hero inside the disc and within 150 cm of its height is in the hazard");
        Check(HazardScale(0.f,5.f)==1.f&&HazardScale(3.9f,5.f)==1.f&&Near(HazardScale(5.f,5.f),.2f,.001f)&&HazardScale(50.f,5.f)>=.19f&&HazardScale(1.f,0.f)==0.f&&HazardScale(NaN,5.f)==0.f,"a hazard keeps its size for 80% of its life then shrinks to a fifth");
        float Last=2.f;bool Falls=true;for(float A=0.f;A<=5.f;A+=.05f) {const float S=HazardScale(A,5.f);if(S>Last+1e-6f) Falls=false;Last=S;}
        Check(Falls,"a hazard never grows back");
    }
    // ---- Guard break ----
    {
        Check(GuardBreaks(24.f,200.f,true,false)&&!GuardBreaks(23.f,200.f,true,false)&&!GuardBreaks(240.f,200.f,false,false)&&!GuardBreaks(240.f,200.f,true,true)&&!GuardBreaks(NaN,200.f,true,false)&&!GuardBreaks(10.f,0.f,true,false),"a blow of 12% of its health from the front breaks the guard, once");
        Check(GuardDamageScale(false,.9f)==.5f&&GuardDamageScale(false,.2f)==1.f&&GuardDamageScale(true,.9f)==1.f&&GuardDamageScale(false,NaN)==1.f,"the guard halves frontal damage until it is broken");
        Check(GuardBreakSeconds>=2.f&&GuardBreakStagger>=.5f&&GuardBreakStagger<GuardBreakSeconds,"a broken guard lasts a few seconds and staggers for a moment");
    }
    // ---- Meteors ----
    {
        Check(MeteorCount(1)==MeteorsSolo&&MeteorCount(2)==MeteorsCoop&&MeteorsCoop<=MeteorMax,"three meteors alone, five in co-op");
        MeteorPoint P[MeteorMax];float HX[2]={100.f,900.f},HY[2]={50.f,-400.f};
        Check(MeteorVolley(HX,HY,2,5,7u,P)==5&&Near(P[0].X,100.f)&&Near(P[0].Y,50.f)&&Near(P[1].X,900.f)&&Near(P[1].Y,-400.f),"the first meteors land exactly on the heroes");
        Check(MeteorVolley(HX,HY,0,5,7u,P)==0&&MeteorVolley(nullptr,HY,2,5,7u,P)==0&&MeteorVolley(HX,HY,2,0,7u,P)==0&&MeteorVolley(HX,HY,2,99,7u,P)==MeteorMax,"no heroes or no count means no volley, and the count is capped");
        long Scattered=0,Apart=0;
        for(unsigned S=0;S<5000u;++S) {
            const int Heroes=1+int(S%2);const int Count=Heroes>1?MeteorsCoop:MeteorsSolo;
            const int N=MeteorVolley(HX,HY,Heroes,Count,S,P);
            Check(N==Count,"a volley has the requested size");
            for(int I=0;I<N;++I) {
                Check(Near(P[I].Delay,MeteorFirstDelay+float(I)*MeteorStagger,.0005f),"meteors land one after another");
                if(I>=Heroes) {
                    const int Who=I%Heroes;const float D=std::hypot(P[I].X-HX[Who],P[I].Y-HY[Who]);
                    Check(D>=MeteorScatterMin-.5f&&D<=MeteorScatterMax+.5f,"a scattered meteor lands 1.2 to 4.2 m from the hero it picked");
                    ++Scattered;bool Ok=true;for(int J=0;J<I;++J) if(std::hypot(P[I].X-P[J].X,P[I].Y-P[J].Y)<MeteorSpacing-.5f) Ok=false;Apart+=Ok;
                }
            }
        }
        Check(Scattered>5000&&Apart>=Scattered*98/100,"scattered meteors keep their distance from the others at least 98% of the time");
        MeteorPoint Q[MeteorMax];MeteorVolley(HX,HY,2,5,7u,P);MeteorVolley(HX,HY,2,5,7u,Q);
        Check(P[4].X==Q[4].X&&P[4].Y==Q[4].Y&&P[3].X==Q[3].X,"a volley is deterministic in its seed");
    }
    // ---- Blink ----
    {
        float X[5],Y[5];
        Check(BlinkCandidates(1.f,0.f,BlinkBehind,X,Y)==5&&Near(X[0],-BlinkBehind,.01f)&&Near(Y[0],0.f,.01f),"the best reappearance point is directly behind a hero facing forward");
        for(int I=0;I<5;++I) Check(Near(std::hypot(X[I],Y[I]),BlinkBehind,.05f),"every candidate is at the requested distance");
        Check(Y[1]*Y[2]<0.f&&X[1]<0.f&&X[2]<0.f,"the next two are either side behind it");
        BlinkCandidates(0.f,1.f,100.f,X,Y);Check(Near(Y[0],-100.f,.01f),"it follows the hero's facing");
        BlinkCandidates(NaN,0.f,100.f,X,Y);Check(Near(X[0],-100.f,.01f),"a broken facing falls back to forward");
        BlinkCandidates(0.f,0.f,100.f,X,Y);Check(Near(std::hypot(X[0],Y[0]),100.f,.01f),"no facing falls back to forward");
        Check(BlinkStrikeRadius>=MeleeRadius&&BlinkStrikeDamage>MeleeDamage,"the strike after a blink reaches a slash and hurts more");
    }
    // ---- Sniper tracking and relocation ----
    {
        Check(!SnipeLocked(.69f)&&SnipeLocked(.70f)&&SnipeLocked(1.f)&&!SnipeLocked(NaN),"the aim locks for the last 30% of the wind-up");
        Check(Near(TrackYaw(0.f,90.f,.1f,75.f),7.5f)&&Near(TrackYaw(0.f,5.f,.1f,75.f),5.f)&&Near(TrackYaw(0.f,-90.f,.1f,75.f),-7.5f),"it turns toward the target at its turn rate and does not overshoot");
        Check(Near(TrackYaw(350.f,10.f,.2f,75.f),365.f)&&Near(TrackYaw(10.f,350.f,.2f,75.f),-5.f),"it turns the short way round across 0/360 (365 is 5 degrees, -5 is 355)");
        Check(TrackYaw(NaN,10.f,.1f,75.f)==0.f&&TrackYaw(10.f,NaN,.1f,75.f)==10.f&&TrackYaw(10.f,50.f,-1.f,75.f)==10.f,"a broken value never moves the aim");
        float Yaw=0.f;for(int I=0;I<100;++I) Yaw=TrackYaw(Yaw,170.f,.016f,SnipeTurnRate);
        Check(Near(Yaw,SnipeTurnRate*.016f*100.f,.5f)&&Yaw<170.f,"over a hundred frames it has turned exactly rate times time");
        std::set<int> Quadrants;
        for(unsigned S=0;S<2000u;++S) {
            float X,Y;RelocateOffset(S,X,Y);const float D=std::hypot(X,Y);
            Check(D>=RelocateMin-.5f&&D<=RelocateMax+.5f,"the new vantage point is 13 to 16 m from the target");
            Quadrants.insert((X>=0?1:0)+(Y>=0?2:0));
        }
        float A,B,C,D2;RelocateOffset(5u,A,B);RelocateOffset(5u,C,D2);
        Check(Quadrants.size()==4&&A==C&&B==D2,"vantage points use every side and are deterministic in their seed");
        Check(RelocateSeconds>=2.f&&RelocateSpeed>1.f&&RelocateArrive>=100.f,"relocating is brisk and bounded");
    }
    // ---- Guard rules that depend on the facing, kit ownership, and warning sizes ----
    {
        Check(ParryBlocks(true,true,1.f)&&ParryBlocks(true,true,.01f)&&ParryBlocks(true,true,NaN),"a guard blocks a direct blow from the front, and a source-less one");
        Check(!ParryBlocks(true,true,-.2f)&&!ParryBlocks(true,true,0.f),"a blow from behind or from the side line goes through");
        Check(!ParryBlocks(true,false,1.f)&&!ParryBlocks(false,true,1.f),"status damage and a guard that is not up are never blocked");
        Check(Near(ParryDamage(true,100.f),15.f)&&ParryDamage(false,100.f)==100.f,"a block takes it down to 15%");
        Check(KitHas(KitFor(Brute),AbilityEnrage)&&KitHas(KitFor(Brute),AbilityPound)&&!KitHas(KitFor(Grunt),AbilityEnrage)&&!KitHas(KitFor(Grunt),AbilityNone),"kit ownership is read from the kit's list (and stops at its end)");
        Check(KitHas(KitFor(Duelist),AbilityRiposte)&&KitHas(KitFor(Leaper),AbilityBlinkStrike),"the chained follow-ups belong to their kits");
        Check(TellRadiusFor(AbilitySlash,false)==StrikeRadius(false)&&TellRadiusFor(AbilitySlash,true)==StrikeRadius(true),"the slash's warning is the strike radius, wider for an elite");
        Check(TellRadiusFor(AbilityShove,false)==ShoveRadius&&TellRadiusFor(AbilityShove,true)==ShoveRadius,"the shove's warning is its own size");
        Check(TellRadiusFor(AbilityRiposte,false)==RiposteRadius&&TellRadiusFor(AbilityBlinkStrike,false)==BlinkStrikeRadius,"the counter strikes warn at the radius they damage");
        Check(TellRadiusFor(AbilityPound,true)==PoundMaxRadius&&SpecFor(AbilityPound).Tell==TellDisc,"the pound warns with a full disc as wide as its ring ends up");
        for(int Id=1;Id<AbilityCount;++Id) {
            const AbilitySpec& A=SpecFor(Id);
            if(A.Tell==TellSector&&Id!=AbilitySlash) Check(Near(A.TellA,Id==AbilityShove?ShoveRadius:(Id==AbilityRiposte?RiposteRadius:BlinkStrikeRadius)),"a sector warning is drawn at the radius its strike damages");
            Check(!(A.Flags&StanceFlag)||A.Windup>=.9f,"a stance is held for most of a second at least");
        }
        Check(SpecFor(AbilityShove).Windup>=.5f&&SpecFor(AbilityShove).Windup<=.8f&&SpecFor(AbilityBackHop).Windup>=.4f&&SpecFor(AbilityBackHop).Attack>=BackHopSeconds-.001f&&SpecFor(AbilityBackHop).Attack<=BackHopSeconds+.001f,"the shove and the back-hop are announced for about half a second, and the hop lasts as long as its rule says");
        Check(SpecFor(AbilityPound).Windup>=1.f&&SpecFor(AbilityMeteors).Windup>=1.f&&SpecFor(AbilityBlink).Windup>=.5f&&SpecFor(AbilityBlinkStrike).Windup>=.35f,"the big attacks are announced for at least a second (the blink for half of one, its follow-up for a third)");
        Check(SpecFor(AbilityParry).Windup==ParryStanceSeconds&&SpecFor(AbilityEnrage).Windup==EnrageRoarSeconds,"the stance and roar rows use the rule constants");
        Check((SpecFor(AbilityParry).Flags&NoKitCooldownFlag)&&(SpecFor(AbilityBackHop).Flags&NoKitCooldownFlag)&&(SpecFor(AbilityEnrage).Flags&NoKitCooldownFlag)&&(SpecFor(AbilityBlink).Flags&NoKitCooldownFlag)&&!(SpecFor(AbilityShove).Flags&NoKitCooldownFlag),"repositions, stances and the roar leave the kit's attack timer alone; real attacks do not");
        Check(SpecFor(AbilityEnrage).Weight==0.f&&SpecFor(AbilityRiposte).Weight==0.f&&SpecFor(AbilityBlinkStrike).Weight==0.f,"the triggered abilities are never drawn by the chooser");
        Check(SpecFor(AbilityParry).CueRelease==CueNone,"the guard's clang comes from a block, not from the end of the stance");
    }
    std::printf("PASS: %lld enemy ability checks (shove, back-hop, parry, shockwave swept against heroes at six frame rates, enrage, hazards, guard break, meteor volleys, blink, sniper tracking and relocation).\nShared rules only; Unreal combat and gameplay are not executed.\n",Checks);
    return 0;
}
