#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "ASData.h"
#include "ASGameState.generated.h"
class AASBreakable;class AASWorld;class AASEnemy;class AASWell;class AASProjectile;class AASCharacter;class AASPickup;class AASField;
UCLASS() class ASHENSPIRAL_API AASPlayerState : public APlayerState {
    GENERATED_BODY()
public:
    UPROPERTY(Replicated) FASBuild Build;
    UPROPERTY(Replicated) bool bEndlessUnlocked=false;
    UPROPERTY(Replicated) int32 ClearedMask=0;
    UPROPERTY(Replicated) int32 EndlessChoices=0;
    UPROPERTY(Replicated) int32 AvailableAsh=0;
    UPROPERTY(Replicated) int32 AvailableScrap=0;
    UPROPERTY(Replicated) float RunDamage=0;
    UPROPERTY(Replicated) float RunDamageTaken=0;
    UPROPERTY(Replicated) int32 RunKills=0;
    UPROPERTY(Replicated) bool bShopClaimed=false;
    UPROPERTY(Replicated) int32 ShopOffer=-1;
    UPROPERTY(Replicated) int32 ShopBought=0;
    UPROPERTY(Replicated) int32 LastGambleRoom=-100;
    UPROPERTY(Replicated) int32 CharacterLevel=1;
    UPROPERTY(Replicated) int32 TitleIndex=-1;
    UPROPERTY(Replicated) int32 TitleTier=0;
    UPROPERTY(Replicated) int32 ColorChoice=-1;
    int32 PendingExperience=0;
    TArray<int32> PendingCareer;
    bool bRoomDamaged=false;
    bool bRoomEligible=false;
    void Record(ASCareer::Metric Metric,int32 Amount=1,int32 XP=0) {
        PendingCareer.SetNumZeroed(ASCareer::Count);PendingCareer[int32(Metric)]=ASCareer::Add(PendingCareer[int32(Metric)],Amount);PendingExperience=ASCareer::Add(PendingExperience,XP);
    }
    void DrainCareer(FASReceipt& Receipt) {Receipt.Experience=PendingExperience;Receipt.Career=PendingCareer;PendingExperience=0;PendingCareer.Empty();}
    UPROPERTY(Replicated) int32 PlayerIndex=0;
    UPROPERTY(Replicated) bool bReady=false;
    UPROPERTY(Replicated) bool bWagerVote=false;
    UPROPERTY(Replicated) bool bShrineUsed=false;
    UPROPERTY(Replicated) bool bProfileReady=false;
    UPROPERTY(Replicated) bool bRewardClaimed=false;
    UPROPERTY(Replicated) bool bSavePending=false;
    UPROPERTY(Replicated) FString ProfileId;
    UPROPERTY(ReplicatedUsing=OnOffersChanged) TArray<int32> Offers;
    UFUNCTION() void OnOffersChanged();
    FASReceipt PendingReceipt;
    TArray<FASReceipt> ReceiptQueue;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
UCLASS() class ASHENSPIRAL_API AASGameState : public AGameStateBase {
    GENERATED_BODY()
public:
    AASGameState();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) EASPhase Phase=EASPhase::Hub;
    UPROPERTY(Replicated) bool bPartyPaused=false;
    UPROPERTY(Replicated) bool bConnectionHold=false;
    UPROPERTY(Replicated) float ReconnectRemaining=0;
    UPROPERTY(Replicated) float SimTime=0;
    UPROPERTY(Replicated) float CountdownEnd=0;
    UPROPERTY(Replicated) int32 Difficulty=0;
    UPROPERTY(Replicated) bool bEnteringPortal=false;
    UPROPERTY(Replicated) bool bEndless=false;
    UPROPERTY(Replicated) int32 EndlessWave=0;
    UPROPERTY(Replicated) int32 ClearedEndlessWave=0;
    UPROPERTY(Replicated) bool bLuckyShop=false;
    UPROPERTY(Replicated) bool bRareShop=false;
    UPROPERTY(Replicated) float ShuffleAt=0;
    UPROPERTY(Replicated) int32 Challenge=0;
    UPROPERTY(Replicated) int32 VaultMask=0;
    UPROPERTY(Replicated) bool bVaultClaimed=false;
    UPROPERTY(Replicated) int32 Room=0;
    UPROPERTY(Replicated) int32 Wave=0;
    UPROPERTY(Replicated) int32 Cinders=0;
    UPROPERTY(Replicated) bool bCindersBanked=false;
    UPROPERTY(Replicated) int32 BankedAsh=0;
    UPROPERTY(Replicated) int32 PendingSpawns=0;
    UPROPERTY(Replicated) int32 LivingEnemies=0;
    UPROPERTY(Replicated) int32 ChestKind=0;
    UPROPERTY(Replicated) FString SessionCode;
    UPROPERTY(Replicated) FString RunId;
    UPROPERTY(Replicated) TObjectPtr<AASWorld> Arena;
    UPROPERTY(Replicated) TArray<FVector> SpawnWarnings;
    TArray<TWeakObjectPtr<AASEnemy>> Enemies;
    TArray<TWeakObjectPtr<AASWell>> Wells;
    TArray<TWeakObjectPtr<AASProjectile>> Projectiles;
    TArray<TWeakObjectPtr<AASCharacter>> Heroes;
    TArray<TWeakObjectPtr<AASPickup>> Pickups;
    TArray<TWeakObjectPtr<AASField>> Fields;
    TArray<TWeakObjectPtr<AASBreakable>> Breakables;
    bool Frozen() const {return bPartyPaused||bConnectionHold;}
    // Seconds since a boss appeared while still spawn-protected (the intro window); negative when no intro is playing.
    float BossIntroAge() const;
    bool Fighting() const {return Phase==EASPhase::Combat || Phase==EASPhase::Countdown;}
    UFUNCTION(NetMulticast,Unreliable) void Effect(uint8 Kind,FVector_NetQuantize Position,FVector_NetQuantizeNormal Direction,FLinearColor Color,float Size);
};
namespace AS {
    AASGameState* State(const UObject* Object);
    float Time(const UObject* Object);
    bool ClearSight(UWorld* World,const FVector& From,const FVector& To);
}
