#include "ASAudio.h"
#include "ASPresentationRules.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
// 0.26: see ASAudio.h. as.audio.log 1 logs every sound that is turned down or off ('AshenAudio:' lines).
static TAutoConsoleVariable<int32> CVarASAudioLog(TEXT("as.audio.log"),0,TEXT("1: log each sound the loudness settings change ('AshenAudio:' lines: its bus, the volume it asked for and the volume it plays at)."),ECVF_Default);
namespace ASAudio {
namespace {
float Levels[4]={1.f,1.f,1.f,1.f}; // master, combat, world, interface
void Log(Bus B,const USoundBase* Sound,float Asked,float Played) {
    if(CVarASAudioLog.GetValueOnGameThread()!=0) UE_LOG(LogTemp,Display,TEXT("AshenAudio: %s on bus %d asked %.3f, plays at %.3f (master %.2f, bus %.2f)"),*GetNameSafe(Sound),int32(B),Asked,Played,Levels[0],Levels[1+int32(B)]);
}
}
void SetLevels(float Master,float Combat,float World,float Interface) {
    Levels[0]=ASPresentation::VolumeLevel(Master);Levels[1]=ASPresentation::VolumeLevel(Combat);Levels[2]=ASPresentation::VolumeLevel(World);Levels[3]=ASPresentation::VolumeLevel(Interface);
}
float Master() {return Levels[0];}
float BusLevel(Bus B) {return Levels[1+FMath::Clamp(int32(B),0,2)];}
void At(Bus B,const UObject* Context,USoundBase* Sound,const FVector& Location,float Volume,float Pitch) {
    if(!Context||!Sound) return;
    const float Played=ASPresentation::PlayVolume(Volume,Levels[0],BusLevel(B));Log(B,Sound,Volume,Played);
    if(Played>0.f) UGameplayStatics::PlaySoundAtLocation(Context,Sound,Location,Played,Pitch);
}
void Flat(Bus B,const UObject* Context,USoundBase* Sound,float Volume,float Pitch) {
    if(!Context||!Sound) return;
    const float Played=ASPresentation::PlayVolume(Volume,Levels[0],BusLevel(B));Log(B,Sound,Volume,Played);
    if(Played>0.f) UGameplayStatics::PlaySound2D(Context,Sound,Played,Pitch);
}
void Sample(const UObject* Context,int32 Which) {
    static const TCHAR* Names[4]={TEXT("S_Reward"),TEXT("S_ClawHit1"),TEXT("S_Collapse"),TEXT("S_Reward")};
    static const float Volumes[4]={.7f,.8f,.5f,.7f},Pitches[4]={1.f,1.f,1.1f,1.2f};
    const int32 I=FMath::Clamp(Which,0,3);
    const float Played=ASPresentation::PlayVolume(Volumes[I],Levels[0],I==0?1.f:Levels[I]); // the master's own sample is heard at the master level alone
    if(!Context||!(Played>0.f)) return;
    if(auto* Cue=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),Names[I],Names[I]),nullptr,LOAD_NoWarn|LOAD_Quiet)) UGameplayStatics::PlaySound2D(Context,Cue,Played,Pitches[I]);
}
}
