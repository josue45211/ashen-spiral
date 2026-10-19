#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ASData.h"
#include "ASPlayerController.generated.h"
class SASMenu;class SWidget;class UGameViewportClient;
struct FASNotice {FString Message;float Born=0,Offset=0;};
struct FASFloatText {FString Text;FLinearColor Color=FLinearColor::White;float Born=0;bool Big=false;int32 Slot=0;int32 Anchor=-1;}; // Anchor >= 0: show above that player index (partner trait banner)
struct FASDamageNumber {FVector Position;float Value=0,Expires=0;bool Critical=false;EASHitKind Kind=EASHitKind::Physical;float Drift=0;int32 Lane=0;int32 Synergy=-1;float Life=1.25f,Seed=0;};
UCLASS() class ASHENSPIRAL_API AASPlayerController : public APlayerController {
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float Delta) override;
    void OpenMenu(int32 Page=0,bool PreserveScroll=false);
    void CloseMenu();
    void RefreshRewardMenu();
    void Action(int32 Code,int32 Index=0);
    void Rebind(FName Binding,FKey Key);
    FKey BoundKey(FName Binding) const;
    bool MenuOpen() const {return Menu.IsValid();}
    bool bShowStats=false;
    FString JoinAddress=TEXT("127.0.0.1"),JoinCode,SeedText=TEXT("1103"),Notice;
    TArray<FASDamageNumber> DamageNumbers;
    TArray<FASNotice> Notifications;
    TArray<FASFloatText> FloatTexts;
    int32 FloatSlot=0;
    FString ProgressImportPath;
    int32 ShopWeapon=0;
    float LastCritAt=-100,LastHurtAt=-100,NoticeUntil=0;
    FString AlertTitle,AlertSub;int32 AlertSeq=0; // banner alerts raised through Notify("!BANNER|..")
    FVector AimTarget=FVector::ZeroVector;
    int32 FocusedInteraction=-1;
    UGameViewportClient* LocalViewport() const;
    bool WantsGameplayInput() const;
    void ApplyGameplayInput();
    void ApplyMenuInput();
    void QueueGameplayCapture();
    void ViewportFocusLost();
    void ApplyCameraSettings();
    void UpdateAim(bool Send);
    void BuildMenu();void Zoom(float Value);
    UFUNCTION(Server,Reliable) void SetCharacterTitle(int32 Index,int32 Tier);
    UFUNCTION(Server,Reliable) void SetCharacterColor(int32 Choice);
    UFUNCTION(Server,Reliable) void UseShrine();
    UFUNCTION(Server,Reliable) void GambleAtWell(int32 StakeIndex);
    UFUNCTION(Server,Reliable) void TryWeapon(int32 Weapon);
    UFUNCTION(Client,Reliable) void RequestProfile();
    UFUNCTION(Server,Reliable) void SubmitProfile(const FString& Id,const FASBuild& Build,int32 Protocol,int32 Experience,int32 ColorChoice,bool RaiseStakes,int32 TitleIndex,int32 TitleTier,int32 ClearedMask,int32 EndlessChoices);
    UFUNCTION(Server,Reliable) void CancelActions();
    UFUNCTION(Server,Reliable) void StartRun(int32 Seed);
    UFUNCTION(Server,Reliable) void ChooseReward(int32 Index);
    UFUNCTION(Server,Reliable) void RerollRewards();
    UFUNCTION(Server,Reliable) void ToggleWagerVote(bool Enabled);
    UFUNCTION(Server,Reliable) void SetPartyPaused(bool Paused);
    UFUNCTION(Server,Reliable) void ReturnHub();
    UFUNCTION(Server,Reliable) void SetDifficulty(int32 Difficulty);
    UFUNCTION(Server,Reliable) void StartEndless(int32 Seed);
    UFUNCTION(Server,Reliable) void EnterPortal(int32 Portal,int32 Seed);
    UFUNCTION(Client,Reliable) void CloseMenuForPortal();
    int32 SelectedPortal=0;
    float PortalStartedAt=-1.f;
    // Seconds since the portal walk-in began on this client; negative when not entering a portal.
    float PortalAge() const {return PortalStartedAt<0.f||!GetWorld()?-1.f:GetWorld()->GetTimeSeconds()-PortalStartedAt;}
    UFUNCTION(Server,Reliable) void BuyShopItem(int32 Index);
    UFUNCTION(Server,Reliable) void ContinueAlone();
    UFUNCTION(Server,Reliable) void InteractServer(int32 Target=0);
    UFUNCTION(Client,Reliable) void ReceiveReceipt(const FASReceipt& Receipt);
    UFUNCTION(Server,Reliable) void AckReceipt(const FString& Id,bool Success,int32 Experience);
    UFUNCTION(Client,Reliable) void Notify(const FString& Message);
    UFUNCTION(Client,Reliable) void ProfileRejected(const FString& KeptProfileId);
    void ResubmitProfile();
    void PortalInteract(int32 Page);
    bool HoldBuildKey(const FKey& Key,bool Down,bool Repeat);
    TSharedPtr<class IInputProcessor> BuildKeyProcessor;
    // Synergy feedback: a coloured word above the damage numbers plus a cue. Trigger indexes ASExpedition::Synergy results.
    UFUNCTION(Client,Unreliable) void SynergyCue(int32 Trigger);
    // Reward fanfare: floating "+X" text; Big is the trait-acquire banner and also plays the placeholder sound hook.
    UFUNCTION(Client,Reliable) void RewardText(const FString& InText,FLinearColor Color,bool Big);
    UFUNCTION(Client,Unreliable) void SynergyNumber(FVector_NetQuantize Position,float Value,int32 Trigger);
    UFUNCTION(Client,Unreliable) void DamageNumber(FVector_NetQuantize Position,float Value,bool Critical,EASHitKind Kind);
    UFUNCTION(Exec) void ASPerformance();
    UFUNCTION(Exec) void ASInputStatus();
private:
    TSharedPtr<SASMenu> Menu;
    TSharedPtr<SWidget> MenuLayer;
    TWeakObjectPtr<UGameViewportClient> MenuViewport;
    float NextAim=0;
    int32 DamageLane=0;
    bool bHadViewportFocus=false,bGameplayInputApplied=false,bMenuInputApplied=false,bCaptureRequested=false;
    int32 LastRoom=-100;
    EASPhase LastPhase=EASPhase::Hub;
    void MoveForward(float Value);void MoveRight(float Value);void Turn(float Value);void Look(float Value);void TurnStick(float Value);void LookStick(float Value);
    void AttackDown();void AttackUp();void SpiralDown();void SpiralUp();void HammerAltDown();void HammerAltUp();void Rune();void Reload();void Dash();void JumpDown();void JumpUp();void Interact();void ReviveDown();void ReviveUp();void ToggleMenu();void Shoulder();
};
