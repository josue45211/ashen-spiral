#include "../Source/AshenSpiral/Public/ASPresentationRules.h"
#include "../Source/AshenSpiral/Public/ASArsenalRules.h"
#include "../Source/AshenSpiral/Public/ASBuildRules.h"
#include <iostream>
#include <limits>
#include <cstdlib>
static int Checks=0;
static void Check(bool OK,const char* Message) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<Message<<"\n";std::exit(1);}}
static bool Near(float A,float B) {return std::abs(A-B)<.0005f;}
int main() {
    using namespace ASPresentation;
    for(float Scale:{.25f,.5f,.75f,1.f}) Check(HUDScale(Scale)==Scale,"all four requested HUD sizes are exact");
    Check(HUDScale(-100)==.25f&&HUDScale(100)==1.f,"HUD size stays within selectable range");
    Check(HUDScale(std::numeric_limits<float>::quiet_NaN())==1.f,"invalid saved HUD size repairs to default");
    Check(NoticeAlpha(0)==1&&NoticeAlpha(4.5f)==1,"notices hold for four and a half seconds");
    Check(Near(NoticeAlpha(6.75f),.5f)&&NoticeAlpha(9)==0&&NoticeAlpha(20)==0,"notices fade slowly then expire");
    for(int Weapon=6;Weapon<ASRules::WeaponCount;++Weapon) {
        const float DPS=ASArsenal::Damage(Weapon)*ASArsenal::Pellets(Weapon)/ASArsenal::Rate(Weapon);
        Check(DPS>=120&&DPS<=140,"new firearms compensate for their reload downtime with comparable burst DPS");
        for(int Rank=0;Rank<=5;++Rank) {
            const float WithBonus=ASArsenal::Damage(Weapon,4.f*Rank)*ASArsenal::Pellets(Weapon)/ASArsenal::Rate(Weapon);
            Check(Near(WithBonus-DPS,12.5f*Rank),"flat primary upgrades add equal DPS to burst and rapid fire");
        }
        ASBuild::Context Build;Build.Weapon=Weapon;
        Check(ASBuild::CanChoose(Build,200+ASBuild::Index(ASBuild::Trait::Ranged)),"all new weapons can receive ranged upgrades");
        Check(ASBuild::CanChoose(Build,200+ASBuild::Index(ASBuild::Trait::Volley)),"all new weapons support extra shots");
        Check(ASBuild::CanChoose(Build,200+ASBuild::Index(ASBuild::Trait::Pierce))==(Weapon!=8),"explosive launcher is excluded from piercing");
        Check(ASBuild::CanChoose(Build,200+ASBuild::Index(ASBuild::Trait::Split))==(Weapon!=8),"explosive launcher is excluded from recursive split choices");
    }
    Check(ASRules::WeaponMask==511&&(63&ASRules::WeaponMask)==63,"original six unlock bits survive the nine-weapon mask");
    Check(ASArsenal::Pellets(6)==6&&ASArsenal::Pellets(7)==1&&ASArsenal::Pellets(8)==1,"burst and single-projectile patterns differ");
    float Previous=1.f;
    for(int Distance=0;Distance<=6000;Distance+=10) {
        const float Falloff=ASArsenal::ScatterFalloff(float(Distance));
        Check(Falloff<=Previous&&Falloff>=.35f,"scatter damage drops monotonically with a bounded minimum");Previous=Falloff;
    }
    Check(ASArsenal::ScatterFalloff(600)==1.f,"scatter keeps full damage for six meters");
    Check(ASArsenal::BlastFalloff(0,240)==1.f&&Near(ASArsenal::BlastFalloff(240,240),.35f),"rocket center and edge splash differ");
    for(int FPS:{30,60,120}) {
        float Heat=0;for(int Frame=0;Frame<FPS*2;++Frame) Heat=ASArsenal::Heat(Heat,1.f/FPS,true);
        Check(Near(Heat,1),"continuous rotary fire reaches a bounded spread");
        for(int Frame=0;Frame<FPS;++Frame) Heat=ASArsenal::Heat(Heat,1.f/FPS,false);
        Check(Near(Heat,0),"releasing rotary fire restores accuracy");
    }
    {
        // Damage-number personalities: bounded, finite, and every special source behaves differently.
        const NumberPersona All[]={NumberPersona::Plain,NumberPersona::Crit,NumberPersona::Burn,NumberPersona::Poison,NumberPersona::Frost,NumberPersona::Bleed,NumberPersona::Shock,NumberPersona::Spiral,NumberPersona::SynergyLightning,NumberPersona::SynergyStorm,NumberPersona::SynergyWild};
        float Signature[11][6];
        for(int P=0;P<11;++P) {
            for(float Age=0;Age<=1.7f;Age+=.01f) {
                const auto L=DamageLookFor(All[P],Age,P>=8?1.7f:1.25f,17.f);
                Check(std::isfinite(L.Scale)&&std::isfinite(L.Rise)&&std::isfinite(L.X)&&std::isfinite(L.Y)&&L.Scale>.5f&&L.Scale<4.f&&L.Alpha>=0&&L.Alpha<=1,"every damage-number persona stays finite and on screen");
            }
            const float Ages[3]={.05f,.4f,1.f};
            for(int I=0;I<3;++I) {const auto L=DamageLookFor(All[P],Ages[I],P>=8?1.7f:1.25f,17.f);Signature[P][I*2]=L.Scale*100+L.Rise+L.Y;Signature[P][I*2+1]=L.X;}
        }
        for(int A=0;A<11;++A) for(int B=A+1;B<11;++B) {
            bool Differs=false;for(int K=0;K<6;++K) if(std::abs(Signature[A][K]-Signature[B][K])>.5f) Differs=true;
            // The three synergy types share a motion language and differ by colour, so they may match each other.
            const bool BothSynergy=A>=8&&B>=8;
            Check(Differs||BothSynergy,"each element, crit and synergy has its own number motion");
        }
        const auto Plain=DamageLookFor(NumberPersona::Plain,.5f,1.25f,3.f);
        Check(Near(Plain.Scale,1.f)&&Near(Plain.Rise,27.5f)&&Plain.X==0&&Plain.Y==0&&Plain.Rings==0,"plain numbers keep the original look");
        Check(DamageLookFor(NumberPersona::Crit,0,1.25f,1.f).Scale>DamageLookFor(NumberPersona::Crit,.4f,1.25f,1.f).Scale+.5f,"crits punch in oversized then settle");
        Check(DamageLookFor(NumberPersona::SynergyStorm,0,1.7f,1.f).Scale>DamageLookFor(NumberPersona::Crit,0,1.25f,1.f).Scale,"synergy hits are the loudest numbers");
        Check(DamageLookFor(NumberPersona::Frost,.6f,1.25f,1.f).Rise==0&&DamageLookFor(NumberPersona::Bleed,.8f,1.25f,1.f).Y>DamageLookFor(NumberPersona::Bleed,.3f,1.25f,1.f).Y,"frost hangs still while bleed falls");
        Check(DamageLookFor(NumberPersona::Poison,1.2f,1.25f,1.f).Y>0&&DamageLookFor(NumberPersona::Burn,.8f,1.25f,1.f).Rise>DamageLookFor(NumberPersona::Plain,.8f,1.25f,1.f).Rise,"poison drips late and burn climbs faster than plain");
        Check(IsSynergyPersona(NumberPersona::SynergyWild)&&!IsSynergyPersona(NumberPersona::Crit),"synergy persona grouping");
    }
    {
        Check(BannerAt(-1).Alpha==0&&BannerAt(BannerLife).Alpha==0,"banner is invisible outside its life");
        Check(Near(BannerAt(0).Slide,-1.f)&&Near(BannerAt(BannerIn).Slide,0.f)&&BannerAt(.9f).Slide==0.f,"banner slides in from the left and holds centred");
        Check(BannerAt(.06f).TitleScale>1.8f&&Near(BannerAt(1.f).TitleScale,1.f)&&BannerAt(.5f).TitleScale<1.05f,"banner title scales in fast then settles");
        Check(BannerAt(BannerLife-.01f).Slide>0.f&&BannerAt(BannerLife-.01f).Alpha<.1f,"banner exits right while fading");
        for(float A=0;A<BannerLife;A+=.01f) {const auto B=BannerAt(A);Check(B.Alpha>=0&&B.Alpha<=1&&B.Slide>=-1.001f&&B.Slide<=1.001f&&B.Stretch>=0&&B.Stretch<=1.001f,"banner stays bounded");}
    }
    {
        const auto N=KillBurstFor(false,false),E=KillBurstFor(true,false),B=KillBurstFor(false,true);
        Check(N.Shards<E.Shards&&E.Shards<B.Shards&&N.Size<E.Size&&E.Size<B.Size&&N.Life<E.Life&&E.Life<B.Life,"kill bursts grow with enemy tier");
        Check(N.StopSeconds==0.f&&E.StopSeconds>0.f&&B.StopSeconds>E.StopSeconds&&B.StopSeconds<=.05f&&E.StopSeconds>=.03f&&E.StopScale<1.f&&B.StopScale<=E.StopScale,"only elites and bosses hit-stop, bosses longest, always brief");
        Check(Near(ShardHeight(500,0),0)&&ShardHeight(500,.25f)>0&&ShardHeight(500,1.2f)<0,"shards arc up then fall");
        Check(ShardScale(0,.6f)==1.f&&ShardScale(.6f,.6f)==0.f&&ShardScale(.3f,.6f)==1.f,"shards hold full size then shrink away");
    }
    {
        Check(Near(PickupHeight(0),0.f)&&PickupHeight(.2f)>60.f,"pickups are tossed up when they spawn");
        bool Bounced=false;float Prev=PickupHeight(.2f);bool Falling=false;for(float A=.2f;A<1.f;A+=.01f) {const float H=PickupHeight(A);if(H<Prev) Falling=true;if(Falling&&H>Prev+.5f) Bounced=true;Prev=H;}
        Check(Bounced,"pickups bounce before settling");
        float Lo=1e9f,Hi=-1e9f;for(float A=3.f;A<8.f;A+=.01f) {const float H=PickupHeight(A);Lo=std::min(Lo,H);Hi=std::max(Hi,H);}
        Check(Lo>=7.f&&Hi<=21.f,"settled pickups hover within a small band");
        Check(PickupMagnetSpeed(PickupMagnetRadius)==0.f&&PickupMagnetSpeed(400)<PickupMagnetSpeed(200)&&PickupMagnetSpeed(200)<PickupMagnetSpeed(50),"magnet pull speeds up as the pickup closes in");
        Check(PickupMagnetSpeed(PickupGrabRadius)>PickupMagnetSpeed(PickupMagnetRadius-1)&&PickupGrabRadius<PickupMagnetRadius,"grab radius sits inside the magnet radius");
    }
    {
        Check(BossIntroAt(-1,false).Bars==0&&BossIntroAt(BossIntroLife,false).Bars==0&&BossIntroAt(BossIntroLife,false).NameAlpha==0,"boss intro is invisible outside its window");
        Check(BossIntroAt(1.0f,false).Bars>.99f&&BossIntroAt(.05f,false).Bars<.5f&&BossIntroAt(1.95f,false).Bars<.1f,"bars close in fast, hold, and open before the boss can be hurt");
        Check(BossIntroAt(.31f,false).NameScale>2.f&&Near(BossIntroAt(1.2f,false).NameScale,1.f)&&BossIntroAt(1.2f,false).NameAlpha>.99f,"boss name slams in oversized and settles");
        Check(BossIntroAt(.9f,false).FovPush>5.f&&BossIntroAt(.9f,true).FovPush==0.f&&BossIntroAt(1.99f,false).FovPush<.5f,"camera push-in happens, is off under reduce-motion, and fully returns");
        for(float A=0;A<BossIntroLife;A+=.01f) {const auto P=BossIntroAt(A,false);Check(P.Bars>=0&&P.Bars<=1.001f&&P.NameAlpha>=0&&P.NameAlpha<=1.001f&&P.FovPush>=0&&P.FovPush<=10.001f&&P.NameScale>=1.f&&P.NameScale<=2.7f,"boss intro stays bounded");}
    }
    {
        Check(PortalKickFov(-1,false)==0&&PortalKickFov(PortalWalkSeconds+.5f,false)==0&&PortalKickFov(.5f,true)==0,"portal FOV kick is off outside the walk-in and under reduce-motion");
        Check(PortalKickFov(.25f,false)>13.5f&&PortalKickFov(1.2f,false)>5.f&&PortalKickFov(1.2f,false)<7.f&&PortalKickFov(0,false)<.1f,"portal FOV kicks fast then eases to a held push");
        for(float A=0;A<=PortalWalkSeconds+.3f;A+=.01f) Check(PortalKickFov(A,false)>=-.001f&&PortalKickFov(A,false)<=14.001f&&PortalFadeAlpha(A)>=0&&PortalFadeAlpha(A)<=.92f+.001f,"portal juice stays bounded");
        Check(PortalFadeAlpha(.3f)==0.f&&PortalFadeAlpha(PortalWalkSeconds)>.85f,"portal fade is short and lands at the end of the walk-in");
        Check(LowHpVignette(1.f,.3f).Alpha==0.f&&LowHpVignette(2.f,.3f).Alpha==0.f,"low-HP vignette is fully off at full health");
        float PeakHalf=0,PeakLow=0;for(float A=0;A<2.f;A+=.005f) {PeakHalf=std::max(PeakHalf,LowHpVignette(.5f,A).Alpha);PeakLow=std::max(PeakLow,LowHpVignette(.1f,A).Alpha);}
        Check(PeakLow>PeakHalf&&PeakHalf>0&&PeakLow<=.62f+.001f&&LowHpVignette(.1f,0).Hertz>LowHpVignette(.5f,0).Hertz,"vignette is stronger and pulses faster as health drops");
        Check(LowHpVignette(std::numeric_limits<float>::quiet_NaN(),1).Alpha==0.f&&LowHpVignette(-3.f,1).Alpha>=0.f,"invalid health never produces a vignette flash");
    }
    {
        Check(FloatTextAt(-1,false).Alpha==0&&FloatTextAt(FloatTextLife,false).Alpha==0,"floating reward text exists only during its life");
        Check(FloatTextAt(.02f,false).Scale>1.4f&&Near(FloatTextAt(1.f,false).Scale,1.f)&&FloatTextAt(.02f,true).Scale>FloatTextAt(.02f,false).Scale,"reward text pops in, trait banner pops harder");
        Check(FloatTextAt(1.f,false).Rise>FloatTextAt(.2f,false).Rise&&FloatTextAt(FloatTextLife-.01f,false).Alpha<.05f,"reward text floats up and fades out");
        for(float A=0;A<FloatTextLife;A+=.01f) {const auto F=FloatTextAt(A,A>.8f);Check(F.Alpha>=0&&F.Alpha<=1&&F.Rise>=0&&F.Rise<=80.001f&&F.Scale>=1.f&&F.Scale<=2.f,"reward text stays bounded");}
        Check(Near(ChestPopScale(0),1.f)&&ChestPopScale(ChestPopLife*.5f)>1.25f&&ChestPopScale(ChestPopLife)==1.f&&ChestPopScale(-1)==1.f,"chest swells and settles back to its own size");
    }
    {
        Check(!DashGhostDue(640.f,1,0)&&DashGhostDue(2350.f,1,0)&&!DashGhostDue(2350.f,1,2)&&!DashGhostDue(std::numeric_limits<float>::quiet_NaN(),1,0),"afterimages only spawn at dash speed, on a timer, never from bad speed");
        Check(DashGhostAlpha(0)==1.f&&DashGhostAlpha(DashGhostLife)==0.f&&DashGhostAlpha(DashGhostLife*.5f)<DashGhostAlpha(DashGhostLife*.25f),"afterimages fade out");
        Check(DashGhostShrink(0)==1.f&&Near(DashGhostShrink(DashGhostLife),.55f)&&DashGhostCap>=10&&DashGhostCap<=30,"afterimages shrink and are capped");
        Check(DashGhostSpacing*DashGhostCap>=DashGhostLife,"the cap never starves a continuous dash trail");
    }
    std::cout<<"PASS: "<<Checks<<" interface/arsenal checks; HUD scale, fade duration, legacy unlock bits, firearm balance and upgrade eligibility.\n";
    std::cout<<"Shared rules only; Unreal rendering, actors, save I/O and networking were not run.\n";
}
