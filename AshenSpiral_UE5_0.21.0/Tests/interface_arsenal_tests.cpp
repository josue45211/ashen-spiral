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
    std::cout<<"PASS: "<<Checks<<" interface/arsenal checks; HUD scale, fade duration, legacy unlock bits, firearm balance and upgrade eligibility.\n";
    std::cout<<"Shared rules only; Unreal rendering, actors, save I/O and networking were not run.\n";
}
