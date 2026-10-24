// Native tests for the 0.26 loudness settings (ASPresentationRules.h: VolumeLevel, PlayVolume). Not Unreal gameplay tests: the sliders, the settings file and the routing of the game's sounds are not run here.
#include "../Source/AshenSpiral/Public/ASPresentationRules.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
static long long Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.0001f) {return std::fabs(A-B)<E;}
int main() {
    using namespace ASPresentation;
    const float NaN=std::numeric_limits<float>::quiet_NaN(),Inf=std::numeric_limits<float>::infinity();
    // ---- a level: 0 to 200% in steps of 5%, a bad number is the default ----
    Check(VolumeLevel(1.f)==1.f,"the default level is exactly 1 (the game's own mix), so an untouched setting changes nothing");
    Check(VolumeLevel(0.f)==0.f&&VolumeLevel(2.f)==2.f,"off and double are the ends");
    Check(VolumeLevel(-.5f)==0.f&&VolumeLevel(-1000.f)==0.f&&VolumeLevel(9.f)==2.f&&VolumeLevel(2.01f)==2.f,"past the ends is the end");
    Check(VolumeLevel(NaN)==1.f&&VolumeLevel(Inf)==1.f&&VolumeLevel(-Inf)==1.f,"not a number, or infinite, is the default: a damaged settings file never mutes the game");
    Check(Near(VolumeLevel(.52f),.5f)&&Near(VolumeLevel(.53f),.55f)&&Near(VolumeLevel(.97f),.95f)&&Near(VolumeLevel(.99f),1.f),"a level snaps to the nearest 5%");
    for(int I=0;I<=40;++I) {const float V=VolumeLevel(float(I)*VolumeStep);Check(Near(V,float(I)*VolumeStep,.00001f),"every step of the slider is a level that maps to itself");Check(VolumeLevel(V)==V,"a level is stable: cleaning it again changes nothing");}
    for(float V:{.8f,.3f,.6f,1.2f,1.45f,1.75f}) Check(VolumeLevel(V)==VolumeLevel(VolumeLevel(V)),"cleaning twice is the same as once");
    Check(VolumeStep>0.f&&VolumeMax==2.f&&std::lround(VolumeMax/VolumeStep)==40,"forty steps from off to double");
    // ---- what a sound plays at: its own volume x the master x its bus ----
    Check(PlayVolume(.5f,1.f,1.f)==.5f&&PlayVolume(1.f,1.f,1.f)==1.f&&PlayVolume(.123f,1.f,1.f)==.123f,"at the defaults every sound plays at exactly the volume it asked for");
    Check(Near(PlayVolume(.8f,.5f,1.f),.4f)&&Near(PlayVolume(.8f,1.f,.5f),.4f)&&Near(PlayVolume(.8f,.5f,.5f),.2f),"master and bus multiply");
    Check(Near(PlayVolume(.5f,2.f,2.f),2.f),"double and double is four times: a quiet sound can be made loud");
    Check(PlayVolume(.8f,0.f,1.f)==0.f&&PlayVolume(.8f,1.f,0.f)==0.f&&PlayVolume(0.f,1.f,1.f)==0.f,"off at the master, off at the bus, or a sound that asked for nothing: nothing plays");
    Check(PlayVolume(-.5f,1.f,1.f)==0.f&&PlayVolume(NaN,1.f,1.f)==0.f&&PlayVolume(Inf,1.f,1.f)==0.f,"a bad volume from a call site plays nothing, never a blast");
    Check(Near(PlayVolume(.8f,NaN,1.f),.8f)&&Near(PlayVolume(.8f,1.f,NaN),.8f),"a bad level is the default, not silence");
    Check(Near(PlayVolume(.4f,.73f,1.f),.4f*.75f),"a master of 73% is 75%: the slider's own steps");
    std::printf("PASS: %lld loudness checks (levels 0-200%% in 5%% steps, bad numbers, master x bus, silent sounds).\nShared rules only; the sliders, the settings file and the routing of the sounds are not executed here.\n",Checks);
    return 0;
}
