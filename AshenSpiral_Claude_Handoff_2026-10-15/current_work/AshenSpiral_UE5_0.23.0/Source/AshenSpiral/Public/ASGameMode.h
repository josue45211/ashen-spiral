#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ASData.h"
#include "ASGameMode.generated.h"
class AASPlayerController;class AASPlayerState;class AASEnemy;class AASCharacter;
struct FASReconnect {
    FASBuild Build;
    int32 PendingExperience=0;TArray<int32> PendingCareer;bool RoomDamaged=false,RoomEligible=false;
    FVector Position=FVector::ZeroVector;
    float Health=110,MaxHealth=110,Shield=0,NextSpiral=0;
    int32 Relics=0,PlayerIndex=1,Ammo=0;
    float ReloadStart=0,ReloadEnd=0,NextAttack=0,NextHammerLeap=0;
    bool Mercy=false,SecondWind=false,Claimed=false;
    int32 LastGambleRoom=-100;
    // Endless run state that must survive a reconnect.
    TArray<int32> EndlessRanks;int32 EndlessAsh=0,EndlessScrap=0,ShopBought=0,RunKills=0;float RunDamage=0,RunDamageTaken=0,Bleed=25.f;bool Eliminated=false;
    TArray<int32> Offers;
    TArray<int32> RunRanks;
    TArray<int32> TraitRanks;
    uint64 EvolutionMask=0;
    int32 LastUpgrade=-1,Rerolls=1,CurseStacks=0,ConduitHits=0,DashCharges=1;
    float Focus=60,NextRune=0,GoldRemainder=0,RoomBuffUntil=0,KillBuffUntil=0,DashRecharge=0;
    int32 AirDashesUsed=0;
    bool PhoenixUsed=false,WagerVote=false,ShrineUsed=false;
    float WarCryUntil=0;
    TArray<FASReceipt> ReceiptQueue;
    FASReceipt Receipt;
};
UCLASS() class ASHENSPIRAL_API AASGameMode : public AGameModeBase {
    GENERATED_BODY()
public:
    AASGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void PreLogin(const FString& Options,const FString& Address,const FUniqueNetIdRepl& UniqueId,FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual FString InitNewPlayer(APlayerController* NewPlayer,const FUniqueNetIdRepl& UniqueId,const FString& Options,const FString& Portal) override;
    virtual void RestartPlayer(AController* NewPlayer) override;
    void ProfileArrived(AASPlayerController* PC,const FString& Id,const FASBuild& Build,int32 Experience,int32 ColorChoice,int32 ClearedMask,int32 EndlessChoices);
    void BeginRun(AASPlayerController* PC,int32 Seed);
    void SetDifficulty(AASPlayerController* PC,int32 Difficulty);
    void BeginEndless(AASPlayerController* PC,int32 Seed);
    // Astra portal flow: E at an archway opens its page; the host's ENTER starts a short walk-in, then the run launches.
    void StartPortal(AASPlayerController* PC,int32 Portal,int32 Seed);
    // Brief server-side slow-motion on the last kill of a chamber or Endless wave (cosmetic, TUNABLE).
    void ClearBeat();
    float ClearBeatEndsReal=0;
    void EnterRoom(int32 Room);
    void EnemyKilled(AASEnemy* Enemy,AASCharacter* Killer);
    void Reward(AASPlayerController* PC,int32 Index);
    void Reroll(AASPlayerController* PC);
    void ToggleWager(AASPlayerController* PC,bool Enabled);
    void SpawnMinion(const FVector& Position);
    void Interact(AASPlayerController* PC,int32 Target=0);
    void UseShrine(AASPlayerController* PC);
    void GambleAtWell(AASPlayerController* PC,int32 StakeIndex);
    void BuyShopItem(AASPlayerController* PC,int32 Index);
    void BackToHub();
    bool ReturningToHub() const {return bHubPending;}
    void EndRun(bool Victory);
    void AbandonReconnect();
    bool Host(const AASPlayerController* PC) const;
    bool AllReady() const;
    int32 PortalSelection=0,PortalSeed=0;float PortalFinish=0;TWeakObjectPtr<AASPlayerController> PortalLeader;TArray<FVector> PortalStarts;FVector PortalTarget=FVector::ZeroVector;
private:
    int32 RunSeed=1103;
    int32 Queued=0,Kills=0;
    float NextSpawn=0,ReconnectDeadline=0,NextReceiptRetry=0;
    bool bEnding=false,bAdvancePending=false,bHubPending=false;
    FRandomStream Random;
    TMap<FString,FASReconnect> Disconnected;
    TMap<FString,FASReconnect> LiveSnapshots;
    float NextSnapshot=0;
    void RollOffers(AASPlayerState* PS,AASCharacter* Hero);
    bool InteractVault(AASPlayerController* PC,int32 Target=0);
    void QueueWave();
    // ---- Endless mode (0.23 B8)
    void EnterEndless();
    void QueueEndlessWave();
    void EndlessTick(float Delta);
    void EndlessFinishWave();
    void EndlessAdvance(AASPlayerController* PC);
    void ShuffleEndlessArena();
    void ConfigureEndlessEnemy(AASEnemy* Enemy);
    int32 EndlessShuffles=0,EndlessWaveSize=0,NextShuffleWave=0;
    float EndlessWaveStart=0,EndlessShuffleClock=0,EndlessShuffleTarget=150.f,EndlessShuffleAt=-1.f;
    bool bEndlessBossPending=false;
    void SpawnPracticeDummy();
    void FinishRoom();
    void BankCinders();
    float RoomStarted=0;
    void ClearCombat(bool CarryWells=false);
    void SendReceipt(AASPlayerState* PS,const FASReceipt& Receipt);
};
