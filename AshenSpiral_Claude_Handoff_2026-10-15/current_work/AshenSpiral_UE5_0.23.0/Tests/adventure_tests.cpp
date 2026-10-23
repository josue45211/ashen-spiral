#include "../Source/AshenSpiral/Public/ASAdventureRules.h"
#include "../Source/AshenSpiral/Public/ASRules.h"
#include "../Source/AshenSpiral/Public/ASPresentationRules.h"
#include <cstdlib>
#include <iostream>
#include <limits>
static int Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.001f) {return std::abs(A-B)<E;}
int main() {
    using namespace ASAdventure;
    const float Options[]={.8f,1.f,1.2f,1.4f,1.6f};
    for(float Option:Options) Check(Near(ASPresentation::NumberScale(Option),Option),"every requested damage-size option survives load/save normalization");
    Check(Near(ASPresentation::NumberScale(1.25f),1.2f),"old 125% preference migrates to nearest offered size");
    Check(Near(ASPresentation::NumberScale(-100),.8f)&&Near(ASPresentation::NumberScale(100),1.6f),"out of range number settings are bounded");
    Check(Near(ASPresentation::NumberScale(std::numeric_limits<float>::quiet_NaN()),1.2f),"invalid number setting has a usable default");
    for(float ViewHeight:{720.f,1080.f,2160.f}) for(float Reticle:{.5f,1.f,2.f})
        for(float Size:Options) for(float Crit:{1.f,1.45f,1.67f}) for(float HUD:{1.f,1.2f})
            for(float Age:{0.f,.15f,.5f,1.24f}) for(int Lane=0;Lane<3;++Lane) for(bool Incoming:{false,true}) {
                const float H=std::max(14.f,Size*Crit*20.f)*HUD+8.f;
                const float Y=ASPresentation::DamageTop(ViewHeight*.5f,Reticle,H,Age,Lane,Incoming);
                const float AimTop=ViewHeight*.5f-21.f*Reticle;
                Check(Y+H+2.f<AimTop,"popup rectangle and outline clear even the largest critical reticle at every supported size");
                const float Later=ASPresentation::DamageTop(ViewHeight*.5f,Reticle,H,Age+.1f,Lane,Incoming);
                Check(Later<Y,"damage popup rises away from the reticle over time");
            }

    Check(Near(CameraDistance(528,0),440*1.2f),"default camera is twenty percent farther");
    Check(CameraDistance(528,1)<528&&CameraDistance(528,-1)>528,"wheel direction");
    float Zoom=528;for(int I=0;I<100;++I) Zoom=CameraDistance(Zoom,1);
    Check(Zoom==360,"zoom in stops at safe near limit");
    for(int I=0;I<100;++I) Zoom=CameraDistance(Zoom,-1);
    Check(Zoom==780,"zoom out stops at far limit");
    Check(CameraDistance(std::numeric_limits<float>::quiet_NaN(),0)==528,"invalid saved camera recovers");
    Check(CameraDistance(528,std::numeric_limits<float>::infinity())==528,"invalid axis is ignored");
    for(int Room=0;Room<32;++Room) {
        const int Theme=Biome(Room);Check(Theme>=0&&Theme<5,"valid visual theme");
        Check(ASRules::Ward(Room)==Room/8,"visual cave does not change progression ward");
        if(Room>=16&&Room<=23) Check(Theme==4,"cave persists through all eight chambers, seventeen to twenty-four");
        else if(Room>=8&&Room<=11) Check(Theme==2,"obsidian theme retains four early chambers");
        else Check(Theme==ASRules::Ward(Room),"other chambers retain ward theme");
    }
    Check(InReach(320*320,300)&&InReach(320*320,-300),"inclusive interaction boundaries");
    Check(!InReach(321*321,0)&&!InReach(0,301)&&!InReach(0,-301),"reject far or wrong floor");
    Check(!InReach(std::numeric_limits<float>::quiet_NaN(),0),"invalid distance cannot interact");
    for(int Ability=0;Ability<AbilityCount;++Ability) {
        Check(AbilityCost(Ability)>0&&AbilityCost(Ability)<=60,"every ability usable from base focus pool");
        const float Base=AbilityCooldown(Ability,1);
        Check(Base>0&&Base<=16,"bounded ability base cooldown (the 0.26 team skills are payoffs of two players: up to 16 s)");
        float Last=Base;
        for(int I=100;I>=0;--I) {
            const float CD=AbilityCooldown(Ability,I/100.f);
            Check(CD<=Last+.001f&&CD>=Base*.5f,"cooldown upgrades monotonic and capped");Last=CD;
        }
        Check(Near(AbilityCooldown(Ability,-100),Base*.5f)&&Near(AbilityCooldown(Ability,100),Base),"recovery clamp");
    }
    Check(AbilityCost(-1)==AbilityCost(0)&&AbilityCost(99)==AbilityCost(AbilityCount-1),"out of range ability ids bounded");
    for(int Max=1;Max<=1000;++Max) {
        const float Cost=ShrineCost(float(Max));
        Check(Cost>=1&&Cost>=Max*.12f-.001f&&Cost<Max*.12f+1.001f,"offering rounded up to whole health");
        Check(!CanSacrifice(Cost+1,Max)&&CanSacrifice(Cost+2,Max),"shrine never allows lethal offering");
        if(CanSacrifice(Max,Max)) Check(Max-Cost>1,"full health remains alive after offering");
    }
    for(int Ward=0;Ward<4;++Ward) {
        Check(TrapDamage(Ward)>=16&&TrapDamage(Ward)<=28,"trap damage bounded across wards");
        if(Ward) Check(TrapDamage(Ward)>TrapDamage(Ward-1)&&TrapSpeed(Ward)>TrapSpeed(Ward-1),"trap danger escalates");
        const float Speed=TrapSpeed(Ward),Period=2.f*3.14159265358979323846f/(1.2f*Speed);
        Check(Near(Shuttle(0,Speed),0)&&Near(Shuttle(Period*.5f,Speed),1)&&Near(Shuttle(Period,Speed),0),"shuttle reaches both rail ends and returns");
        for(int Frame=0;Frame<3600;++Frame) {
            const float T=Frame/60.f,A=Shuttle(T,Speed),B=Shuttle(T+1.f/60.f,Speed);
            Check(A>=0&&A<=1,"moving trap remains on rail");
            Check(std::abs(B-A)*440<7,"no large sixty fps saw teleport");
        }
    }
    std::cout<<"PASS: "<<Checks<<" adventure checks; camera, popup clearance, interaction reach, nine abilities, shrine safety, biome mapping and four-ward trap motion.\n";
}
