// Native tests for the 0.24 Part C rules (perfect dash, proc chains, impact knockback, metamorphoses, ceremony, death-screen lines). Not Unreal gameplay tests.
#include "../Source/AshenSpiral/Public/ASPartCRules.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.01f) {return std::fabs(A-B)<E;}
int main() {
    using namespace ASPartC;
    const float NaN=std::numeric_limits<float>::quiet_NaN(),Inf=std::numeric_limits<float>::infinity();
    // ---- C-3 perfect dash ----
    Check(PerfectDash(10.f,10.3f,-100.f),"a blow inside the dash's invulnerability is perfect");
    Check(PerfectDash(10.4f,10.3f,-100.f),"a blow within .15 s after the dash ended is perfect");
    Check(!PerfectDash(10.5f,10.3f,-100.f),"a blow later than the window is not");
    Check(!PerfectDash(10.f,10.3f,9.8f),"the cooldown stops a second perfect right after the first");
    Check(PerfectDash(10.f,10.3f,9.3f),"after the cooldown a new perfect works");
    Check(!PerfectDash(10.f,0.f,-100.f)&&!PerfectDash(10.f,-3.f,-100.f),"no dash ever made = never perfect");
    Check(!PerfectDash(NaN,10.f,0.f)&&!PerfectDash(10.f,NaN,0.f)&&!PerfectDash(10.f,10.f,NaN)&&!PerfectDash(Inf,10.f,0.f),"non-finite inputs are never perfect");
    for(int I=0;I<200;++I) {const float T=9.f+float(I)*.01f;if(std::fabs(T-10.15f)>.002f) Check(PerfectDash(T,10.f,-100.f)==(T<10.15f),"the window is exactly the dash plus .15 s");}
    // ---- C-1 proc chains ----
    Check(ChainMayStart(1.f,1.f)&&!ChainMayStart(.9f,1.f)&&!ChainMayStart(NaN,1.f)&&!ChainMayStart(1.f,NaN),"a chain starts only after its cooldown, never with non-finite times");
    Check(ChainMayContinue(0)&&ChainMayContinue(1)&&!ChainMayContinue(2)&&!ChainMayContinue(-1)&&!ChainMayContinue(99),"chains go to depth 2 and stop");
    Check(Near(ChainMagnitude(0),.6f)&&Near(ChainMagnitude(1),.36f)&&ChainMagnitude(1)<ChainMagnitude(0)&&ChainMagnitude(500)>0.f,"each echo is weaker than the one before");
    {int Reach=0;for(int D=0;ChainMayContinue(D);++D) ++Reach;Check(Reach==ChainMaxDepth,"depth 2 means exactly two echoes");}
    Check(ChainConducts(1,0,0)&&ChainConducts(0,2,0)&&ChainConducts(0,0,1)&&!ChainConducts(0,0,0)&&!ChainConducts(-1,-1,-1),"only Chain, Storm Touch or Impact Bloom conduct");
    // ---- C-2 impact knockback ----
    Check(!ImpactWorthy(100.f)&&ImpactWorthy(ImpactMinStrength)&&ImpactWorthy(900.f)&&!ImpactWorthy(NaN)&&!ImpactWorthy(-5.f),"only a hard launch counts");
    Check(Near(ImpactDamage(420.f,0),10.f+420.f*.055f)&&ImpactDamage(800.f,0)>ImpactDamage(420.f,0),"harder launches hit harder");
    Check(ImpactDamage(600.f,1)<ImpactDamage(600.f,0)&&ImpactDamage(600.f,2)<ImpactDamage(600.f,1)&&ImpactDamage(600.f,9)==ImpactDamage(600.f,ImpactMaxGeneration),"each generation hits weaker, and generations are clamped");
    Check(Near(ImpactDamage(600.f,0,PinballImpactMultiplier),ImpactDamage(600.f,0)*2.5f)&&ImpactDamage(1e9f,0,10.f)<=220.f,"Pinball multiplies, and nothing exceeds the cap");
    Check(ImpactDamage(NaN,0)==0.f&&ImpactDamage(500.f,0,NaN)==0.f&&ImpactDamage(-1.f,0)==0.f&&ImpactDamage(500.f,0,-3.f)==0.f,"bad numbers give no damage, never NaN");
    Check(Near(ThrownOnStrength(1000.f),650.f)&&ThrownOnStrength(NaN)==0.f,"the body that is struck flies off at 65%");
    {float S=900.f;int Bodies=0;for(int G=0;G<=ImpactMaxGeneration&&ImpactWorthy(S);++G,S=ThrownOnStrength(S)) ++Bodies;Check(Bodies>=2&&Bodies<=ImpactMaxGeneration+1,"a 900 launch chains through at least one more enemy but stops");}
    // ---- 0.24.2 (F1-F3) the flight: a hop, then a skid; far enough to reach walls, and the farther the harder ----
    {
        const Flight Slam=LaunchFor(950.f),Leap=LaunchFor(700.f),Repulse=LaunchFor(600.f),Light=LaunchFor(230.f),Hard=LaunchFor(ImpactMinStrength);
        Check(Slam.Distance>=800.f&&Slam.Distance<=1000.f,"the maul's slam (950) throws a body 8 to 10 m (0.26 G8: was 6 to 9 m, 7.2 in the 0.24.2 playtest, which asked for a bit more)");
        Check(Slam.Distance>=LaunchFor(950.f/1.2f).Distance*1.15f,"0.26 G8: a slam flies at least 15 percent farther than it did at the 0.24.2 launch speed");
        Check(Leap.Distance>=300.f&&Repulse.Distance>=250.f,"the leap landing and Repulse throw bodies at least 3 and 2.5 m");
        Check(Hard.Distance>=80.f&&Light.Distance>=50.f&&Light.Distance<Hard.Distance,"even the weakest hard launch moves a body most of a metre; a light knock is a nudge");
        Check(LaunchFor(950.f*EliteLaunchScale).Distance<Slam.Distance*.5f&&LaunchFor(950.f*EliteLaunchScale).Distance>=150.f,"an elite flies under half as far, but still visibly");
        Check(Slam.Vertical>0.f&&Slam.Airtime>.3f&&Slam.Airtime<1.f&&Slam.Seconds<=LaunchMaxSeconds&&Slam.Seconds>=Slam.Airtime,"a slam is a real hop (off the floor, so nothing brakes it), then a skid");
        Check(Slam.Vertical*Slam.Vertical/(2.f*LaunchGravity)<80.f,"the hop stays low (under 80 cm): a launch never carries a body over a wall");
        float Previous=-1.f;bool Monotonic=true;
        for(int I=0;I<=300;++I) {const Flight F=LaunchFor(float(I)*10.f);if(F.Distance+.01f<Previous) Monotonic=false;Previous=F.Distance;if(!std::isfinite(F.Distance)||F.Seconds<0.f||F.Seconds>LaunchMaxSeconds) Monotonic=false;}
        Check(Monotonic,"distance never shrinks as strength grows, and every flight is finite and bounded");
        const Flight Bad=LaunchFor(NaN),Zero=LaunchFor(0.f),Negative=LaunchFor(-50.f);
        Check(Bad.Distance==0.f&&Zero.Distance==0.f&&Negative.Seconds==0.f&&LaunchFor(Inf).Horizontal==0.f,"bad strengths launch nothing");
        Check(Near(SkidDistance(0.f),0.f)&&SkidSeconds(-1.f)==0.f&&SkidSeconds(NaN)==0.f&&SkidDistance(900.f)>SkidDistance(400.f),"the skid is longer from a faster landing, and nothing for no speed");
        // the skid closed form against a step simulation of the same braking (CharacterMovement's: v' = -friction v - braking)
        {float V=800.f,X=0.f,T=0.f;const float Dt=1.f/2000.f;while(V>0.f) {V=std::max(0.f,V-(SkidFriction*V+SkidBraking)*Dt);X+=V*Dt;T+=Dt;}
         Check(std::fabs(X-SkidDistance(800.f))<6.f&&std::fabs(T-SkidSeconds(800.f))<.01f,"the skid's closed form matches a simulated brake");}
        Check(ContactStrength(950.f,950.f,950.f)==950.f&&Near(ContactStrength(950.f,950.f,475.f),475.f)&&ContactStrength(950.f,950.f,ImpactMinSpeed-1.f)==0.f,"contact strength follows the speed left, and a crawl hurts nothing");
        Check(ContactStrength(950.f,950.f,5000.f)==950.f&&ContactStrength(NaN,950.f,900.f)==0.f&&ContactStrength(950.f,0.f,900.f)==0.f&&ContactStrength(950.f,950.f,NaN)==0.f,"contact strength never exceeds the launch and is zero for bad numbers");
        Check(ImpactWorthy(ContactStrength(950.f,Slam.Horizontal,Slam.Horizontal*.6f)),"a slam that has lost 40% of its speed still counts as a hard impact");
        Check(SlamBounce>0.f&&SlamBounce<.6f&&SlamDaze>0.f&&SlamDaze<1.f&&PileCarry>=0.f&&PileCarry<.6f,"a slam rebounds softly and dazes briefly; a projectile gives up most of its speed to what it strikes");
        Check(Closing(Slam.Horizontal)&&!Closing(0.f)&&!Closing(Slam.Horizontal-Slam.Horizontal)&&!Closing(-400.f)&&!Closing(NaN),"only a body closing on another collides with it: two flung side by side at the same speed do not");
    }
    // ---- C-6 metamorphoses ----
    Check(AtMax(3,3)&&AtMax(5,3)&&!AtMax(2,3)&&!AtMax(0,0)&&!AtMax(1,-1),"a metamorphosis needs the catalogue maximum");
    Check(StormWebLinks(3,false)==3&&StormWebLinks(3,true)==5&&StormWebLinks(9,true)==6&&StormWebLinks(-2,false)==0,"Storm Web adds two links, capped at six");
    Check(WildfireDamage(100.f)==50.f&&WildfireDamage(5.f)==10.f&&WildfireDamage(1e9f)==400.f&&WildfireDamage(NaN)==10.f,"Wildfire damage is a fraction of the victim's health with floor and cap");
    Check(Near(ShatterDamage(100.f),60.f,.001f)&&ShatterDamage(5.f)==12.f&&ShatterDamage(1e9f)==400.f&&ShatterDamage(Inf)==12.f,"Shatter damage is a fraction of the victim's health with floor and cap");
    for(int M=1;M<int(Metamorphosis::Count);++M) Check(std::strlen(MetamorphosisName(Metamorphosis(M)))>0&&std::strlen(MetamorphosisText(Metamorphosis(M)))>20,"every metamorphosis has a name and a description");
    Check(std::strlen(MetamorphosisName(Metamorphosis::None))==0,"no metamorphosis has no name");
    // ---- C-3 perfect dash reward (0.24.1): a damage boost, not a refund ----
    Check(PerfectDashBoost>=.15f&&PerfectDashBoost<=.5f&&PerfectDashBoostSeconds>=2.f&&PerfectDashBoostSeconds<=8.f,"the boost is worth having and does not last forever");
    Check(PerfectBoostActive(10.f,10.5f)&&!PerfectBoostActive(10.5f,10.5f)&&!PerfectBoostActive(11.f,10.5f)&&!PerfectBoostActive(NaN,10.5f)&&!PerfectBoostActive(10.f,NaN)&&!PerfectBoostActive(10.f,Inf),"the boost runs until its end time and never on a broken clock");
    Check(PerfectDashBonusProtection>0.f&&PerfectDashBonusProtection<=.3f,"the extra protection stays short");
    // ---- C-5 ceremony ----
    Check(CeremonyPush(0.f)==0.f&&CeremonyPush(-1.f)==0.f&&CeremonyPush(NaN)==0.f&&Near(CeremonyPush(CeremonyPushSeconds),1.f,.0001f)&&Near(CeremonyPush(99.f),1.f,.0001f),"the push-in runs from 0 to 1 and holds");
    {float Last=0.f;bool Mono=true;for(int I=0;I<200;++I) {const float P=CeremonyPush(float(I)*.01f);if(P<Last-1e-6f) Mono=false;Last=P;}Check(Mono,"the push-in never reverses");}
    Check(CeremonyHitStopSeconds>=.03f&&CeremonyHitStopSeconds<=.12f&&CeremonyPushFov<0.f,"hit-stop is short and the push-in narrows the view");
    Check(CeremonyPushFov<=-20.f&&CeremonyPushFov>=-40.f,"the push-in is plainly visible (about a quarter of the view) without losing the arena");
    // ---- C-7 death-screen lines ----
    {RunFacts F;const Lines L=Learned(F);Check(L.Count==1&&L.Text[0]&&std::strlen(L.Text[0])>10,"an empty run still gets one line");}
    {RunFacts F;F.Metamorphosed=true;F.Echoes=9;F.Impacts=9;F.PerfectDashes=9;F.Traits=1;F.Seconds=999.f;F.Coop=true;F.Kills=50;F.Endless=true;F.Wave=60;const Lines L=Learned(F);
     Check(L.Count==3,"at most three lines are ever shown");for(int I=0;I<L.Count;++I) Check(L.Text[size_t(I)]&&std::strlen(L.Text[size_t(I)])>10,"every line is real text");
     Check(std::strstr(L.Text[0],"transformed")!=nullptr,"the metamorphosis line comes first");}
    {RunFacts F;F.PerfectDashes=0;F.DamageTaken=900.f;F.DamageDealt=1000.f;F.Kills=20;const Lines L=Learned(F);bool Found=false;for(int I=0;I<L.Count;++I) if(std::strstr(L.Text[size_t(I)],"perfect dash")) Found=true;Check(Found,"a run that took heavy damage without perfect dashes is taught the dash");}
    {RunFacts F;F.Impacts=4;const Lines L=Learned(F);Check(std::strstr(L.Text[0],"Launched")!=nullptr,"launched-enemy impacts earn the knockback line");}
    {RunFacts F;F.Endless=true;F.Wave=55;const Lines L=Learned(F);bool Found=false;for(int I=0;I<L.Count;++I) if(std::strstr(L.Text[size_t(I)],"wave 50")) Found=true;Check(Found,"a deep Endless run is told about the red sky");}
    for(int Run=0;Run<2000;++Run) { // random facts never crash and always give 1..3 non-null lines
        RunFacts F;F.Seconds=float(std::rand()%900);F.DamageDealt=float(std::rand()%5000);F.DamageTaken=float(std::rand()%5000);F.Kills=std::rand()%80;F.PerfectDashes=std::rand()%6;F.Impacts=std::rand()%6;
        F.Echoes=std::rand()%6;F.Traits=std::rand()%12;F.Room=std::rand()%20;F.Wave=std::rand()%120;F.Endless=std::rand()%2!=0;F.Coop=std::rand()%2!=0;F.Metamorphosed=std::rand()%2!=0;
        const Lines L=Learned(F);Check(L.Count>=1&&L.Count<=3,"1..3 lines for any run");for(int I=0;I<L.Count;++I) Check(L.Text[size_t(I)]!=nullptr,"no null line");
    }
    std::cout<<"PASS: "<<Checks<<" Part C checks; perfect dash window, proc-chain depth and falloff, impact damage and chains, metamorphoses, ceremony curve, death-screen lines.\nShared rules only; Unreal actors, replication and feel were not run.\n";
}
