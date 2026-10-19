#include "ASEnemy.h"
#include "ASEnemyRules.h"
#include "ASGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
// Enemy sound cues (0.24). Every telegraph and every release has a cue (ASEnemyRules::Cue*). Each machine plays them itself from the replicated AIState / Ability /
// AttackReleaseTime, so they cost no network traffic. A cue plays the file named by its Stem when that file has been delivered (SourceArt/Audio, imported to /Game/Audio) and
// otherwise an existing sound at the cue's own pitch and volume (its Fallback). Nothing here changes gameplay.
static USoundBase* LoadAudio(const char* Stem) {
    if(!Stem||!Stem[0]) return nullptr;
    static TMap<FString,TWeakObjectPtr<USoundBase>> Loaded;
    static TSet<FString> Absent; // a stem that is not imported on this machine is looked up once
    const FString Name(ANSI_TO_TCHAR(Stem));
    if(Absent.Contains(Name)) return nullptr;
    if(const TWeakObjectPtr<USoundBase>* Found=Loaded.Find(Name)) if(Found->IsValid()) return Found->Get();
    USoundBase* Sound=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),*Name,*Name));
    if(Sound) Loaded.Add(Name,Sound);else Absent.Add(Name);
    return Sound;
}
USoundBase* AASEnemy::ResolveCue(int32 CueId,float& Volume,float& Pitch) {
    const ASEnemyRules::CueSpec& Cue=ASEnemyRules::CueFor(CueId);
    Volume=Cue.Volume;Pitch=Cue.Pitch;
    if(CueId<=ASEnemyRules::CueNone) return nullptr;
    if(USoundBase* Delivered=LoadAudio(Cue.Stem)) {Pitch=1.f;return Delivered;} // a delivered file plays at its natural pitch
    return LoadAudio(Cue.Fallback);
}
void AASEnemy::PlayCue(int32 CueId) {
    if(CueId<=ASEnemyRules::CueNone||GetNetMode()==NM_DedicatedServer||!GetWorld()) return;
    // The cue sounds have no attenuation of their own, so the volume follows the distance to the local player (full inside 6 m, silent beyond 32 m).
    const APlayerController* Local=UGameplayStatics::GetPlayerController(this,0);const APawn* Pawn=Local?Local->GetPawn():nullptr;
    const float Gain=ASEnemyRules::CueDistanceGain(Pawn?float(FVector::Dist(Pawn->GetActorLocation(),GetActorLocation())):0.f);
    float Volume=0.f,Pitch=1.f;USoundBase* Sound=ResolveCue(CueId,Volume,Pitch);
    Volume*=Gain;
    if(!Sound||Volume<.02f) return;
    static uint64 BudgetFrame=0;static int32 Budget=0;
    if(GFrameCounter!=BudgetFrame) {BudgetFrame=GFrameCounter;Budget=0;}
    if(++Budget>4) return; // a crowd winding up on the same frame is heard as a few cues, not a wall of sound
    UGameplayStatics::PlaySoundAtLocation(this,Sound,GetActorLocation(),Volume,Pitch);
}
void AASEnemy::UpdateCues(float Now) {
    namespace ER=ASEnemyRules;
    if(bTrainingDummy||Health<=0) return;
    // 1. A release (every ability stamps AttackReleaseTime). It is played first, with the ability seen BEFORE this update: a chained ability (a parry's riposte, a blink's strike) is
    // begun by the server in the same update as the release of the one before it, and the release cue belongs to the one that ended. Only a fresh release plays: a client that
    // joins mid-fight must not replay an old attack.
    if(AttackReleaseTime!=SeenRelease) {
        if(Now-AttackReleaseTime<.5f&&Now>=AttackReleaseTime-.1f) PlayCue(ER::SpecFor(SeenAbility).CueRelease);
        SeenRelease=AttackReleaseTime;
    }
    // 2. A new telegraph: the state became Windup (or Special, a stance or a roar), or the ability being wound up changed.
    if(int32(AIState)!=SeenState||int32(Ability)!=SeenAbility) {
        const bool Telegraph=AIState==ER::StateWindup||AIState==ER::StateSpecial;
        const bool NewWindup=Telegraph&&Ability!=ER::AbilityNone&&(SeenState!=int32(AIState)||int32(Ability)!=SeenAbility);
        if(NewWindup) PlayCue(ER::SpecFor(Ability).CueWindup);
        SeenState=int32(AIState);
        if(Ability!=ER::AbilityNone) SeenAbility=int32(Ability); // remembered: the replicated ability resets when the recovery ends, the release cue may arrive after
    }
    // 3. The duelist's guard held against a blow (the server flips the bit on every block).
    if(((int32(Flags)^SeenFlags)&int32(ER::FlagParryToggle))!=0) PlayCue(ER::CueParryBlock);
    SeenFlags=int32(Flags);
}
