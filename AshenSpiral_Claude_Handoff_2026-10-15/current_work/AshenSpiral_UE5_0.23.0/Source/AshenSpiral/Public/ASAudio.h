#pragma once
#include "CoreMinimal.h"
class USoundBase;class UObject;
// 0.26: the player's loudness (Settings > SOUND; rules in ASPresentationRules.h). Every sound of the game is played through here, so one master level and three bus levels reach all of it:
//   Combat   your weapons, abilities and hits, your partner's, the footsteps, the team skills and the skill tree's moments
//   World    the enemies' voices and attacks, the lightning and the arena's own sounds
//   Interface the menu's clicks, the reward and synergy words' chimes
// The levels are this machine's own (nothing here replicates); the game instance loads them from the settings file and calls SetLevels when a slider moves.
namespace ASAudio {
enum class Bus : uint8 {Combat=0,World=1,Interface=2};
void SetLevels(float Master,float Combat,float World,float Interface); // each is cleaned (ASPresentation::VolumeLevel)
float Master();
float BusLevel(Bus B);
// The same two calls as UGameplayStatics::PlaySoundAtLocation / PlaySound2D, with the level applied: silent sounds are not played.
void At(Bus B,const UObject* Context,USoundBase* Sound,const FVector& Location,float Volume=1.f,float Pitch=1.f);
void Flat(Bus B,const UObject* Context,USoundBase* Sound,float Volume=1.f,float Pitch=1.f);
// What a slider's sample plays (a short sound of that bus's kind) when it is let go, so the player hears the new level.
void Sample(const UObject* Context,int32 Which); // 0 master, 1 combat, 2 world, 3 interface
}
