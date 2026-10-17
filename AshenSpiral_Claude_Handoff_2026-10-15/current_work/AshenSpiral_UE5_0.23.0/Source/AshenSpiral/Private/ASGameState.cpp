#include "ASGameState.h"
#include "ASSpell.h"
#include "ASWorld.h"
#include "ASPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
AASGameState::AASGameState() {PrimaryActorTick.bCanEverTick=true;SetNetUpdateFrequency(20.f);}
void AASGameState::Tick(float Delta) {Super::Tick(Delta);if(!Frozen()) SimTime+=Delta;if(Arena) Arena->UpdateVaultAppearance();}
void AASGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AASGameState,Difficulty);DOREPLIFETIME(AASGameState,bEnteringPortal);DOREPLIFETIME(AASGameState,bEndless);DOREPLIFETIME(AASGameState,EndlessWave);DOREPLIFETIME(AASGameState,ClearedEndlessWave);DOREPLIFETIME(AASGameState,bLuckyShop);DOREPLIFETIME(AASGameState,bRareShop);DOREPLIFETIME(AASGameState,ShuffleAt);
    DOREPLIFETIME(AASGameState,Phase);DOREPLIFETIME(AASGameState,bPartyPaused);DOREPLIFETIME(AASGameState,bConnectionHold);
    DOREPLIFETIME(AASGameState,ReconnectRemaining);DOREPLIFETIME(AASGameState,SimTime);DOREPLIFETIME(AASGameState,CountdownEnd);
    DOREPLIFETIME(AASGameState,Challenge);DOREPLIFETIME(AASGameState,VaultMask);DOREPLIFETIME(AASGameState,bVaultClaimed);
    DOREPLIFETIME(AASGameState,Room);DOREPLIFETIME(AASGameState,Wave);DOREPLIFETIME(AASGameState,Cinders);DOREPLIFETIME(AASGameState,bCindersBanked);DOREPLIFETIME(AASGameState,BankedAsh);
    DOREPLIFETIME(AASGameState,PendingSpawns);DOREPLIFETIME(AASGameState,LivingEnemies);DOREPLIFETIME(AASGameState,ChestKind);
    DOREPLIFETIME(AASGameState,SessionCode);DOREPLIFETIME(AASGameState,RunId);DOREPLIFETIME(AASGameState,Arena);DOREPLIFETIME(AASGameState,SpawnWarnings);
}
void AASPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AASPlayerState,bEndlessUnlocked);DOREPLIFETIME(AASPlayerState,ClearedMask);DOREPLIFETIME(AASPlayerState,ShopBought);DOREPLIFETIME(AASPlayerState,EndlessChoices);DOREPLIFETIME(AASPlayerState,AvailableAsh);DOREPLIFETIME(AASPlayerState,AvailableScrap);DOREPLIFETIME(AASPlayerState,RunDamage);DOREPLIFETIME(AASPlayerState,RunDamageTaken);DOREPLIFETIME(AASPlayerState,RunKills);DOREPLIFETIME(AASPlayerState,bShopClaimed);DOREPLIFETIME(AASPlayerState,ShopOffer);DOREPLIFETIME(AASPlayerState,LastGambleRoom);
    DOREPLIFETIME(AASPlayerState,ColorChoice);DOREPLIFETIME(AASPlayerState,CharacterLevel);DOREPLIFETIME(AASPlayerState,TitleIndex);DOREPLIFETIME(AASPlayerState,TitleTier);DOREPLIFETIME(AASPlayerState,Build);DOREPLIFETIME(AASPlayerState,PlayerIndex);DOREPLIFETIME(AASPlayerState,bReady);DOREPLIFETIME(AASPlayerState,bWagerVote);DOREPLIFETIME(AASPlayerState,bShrineUsed);
    DOREPLIFETIME(AASPlayerState,bProfileReady);DOREPLIFETIME(AASPlayerState,bRewardClaimed);DOREPLIFETIME(AASPlayerState,bSavePending);
    DOREPLIFETIME(AASPlayerState,ProfileId);DOREPLIFETIME_CONDITION(AASPlayerState,Offers,COND_OwnerOnly);
}
void AASGameState::Effect_Implementation(uint8 Kind,FVector_NetQuantize P,FVector_NetQuantizeNormal D,FLinearColor C,float Size) {
    if(GetNetMode()==NM_DedicatedServer) return;
    if(auto* FX=GetWorld()->SpawnActor<AASVisualEffect>()) FX->Setup(Kind,P,D,C,Size);
}
namespace AS {
AASGameState* State(const UObject* O) {return O&&O->GetWorld()?O->GetWorld()->GetGameState<AASGameState>():nullptr;}
float Time(const UObject* O) {auto* S=State(O);return S?S->SimTime:0.f;}
bool ClearSight(UWorld* W,const FVector& A,const FVector& B) {
    if(!W) return false;FHitResult Hit;FCollisionObjectQueryParams Q(ECC_WorldStatic);
    return !W->LineTraceSingleByObjectType(Hit,A,B,Q);
}
}

void AASPlayerState::OnOffersChanged() {if(auto* PC=Cast<AASPlayerController>(GetOwner())) PC->RefreshRewardMenu();}
