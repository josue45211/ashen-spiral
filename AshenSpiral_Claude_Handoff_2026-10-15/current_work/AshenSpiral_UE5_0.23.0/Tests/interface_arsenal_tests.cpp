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
        Check(BannerAt(BannerLife-.01f).Slide>0.f&&BannerAt(BannerLife-.01f).Alpha<.1f,"banner exits right while fading");
        for(float A=0;A<BannerLife;A+=.01f) {const auto B=BannerAt(A);Check(B.Alpha>=0&&B.Alpha<=1&&B.Slide>=-1.001f&&B.Slide<=1.001f&&B.Stretch>=0&&B.Stretch<=1.001f,"banner stays bounded");}
    }
    {
        const auto N=KillBurstFor(false,false),E=KillBurstFor(true,false),B=KillBurstFor(false,true);
        Check(N.Shards<E.Shards&&E.Shards<B.Shards&&N.Size<E.Size&&E.Size<B.Size&&N.Life<E.Life&&E.Life<B.Life,"kill bursts grow with enemy tier");
        Check(N.StopSeconds==0.f&&E.StopSeconds>0.f&&B.StopSeconds>E.StopSeconds&&B.StopSeconds<=.2f&&E.StopScale<1.f&&B.StopScale<=E.StopScale,"only elites and bosses hit-stop, bosses longest, always brief");
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
    std::cout<<"PASS: "<<Checks<<" interface/arsenal checks; HUD scale, fade duration, legacy unlock bits, firearm balance and upgrade eligibility.\n";
    std::cout<<"Shared rules only; Unreal rendering, actors, save I/O and networking were not run.\n";
}
