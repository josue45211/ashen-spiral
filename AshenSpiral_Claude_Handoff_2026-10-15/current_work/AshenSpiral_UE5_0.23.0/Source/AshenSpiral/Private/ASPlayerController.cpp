#include "ASPlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "ASCharacter.h"
#include "ASMovementComponent.h"
#include "ASGameInstance.h"
#include "ASGameMode.h"
#include "ASGameState.h"
#include "ASPresentationRules.h"
#include "ASWorld.h"
#include "SASMenu.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerInput.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "ASGameViewportClient.h"
#include "Widgets/SViewport.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InputComponent.h"
#include "Widgets/SWeakWidget.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Slate/SceneViewport.h"
#include "Input/Reply.h"
#include "Widgets/SWindow.h"
#include "GenericPlatform/GenericWindow.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif
namespace {
#if PLATFORM_WINDOWS
// The engine's own cursor lock can be a pixel or two too generous at high DPI (a 4096x2160 display leaked on the right and bottom edges).
// While playing in fullscreen we additionally clip the cursor to the window's real client rectangle, inset by Inset pixels.
// A negative Inset releases the clip.
void ASClipCursorToClientArea(const TSharedPtr<SWindow>& Window,int32 Inset) {
    if(Inset<0||!Window.IsValid()) {::ClipCursor(nullptr);return;}
    const TSharedPtr<FGenericWindow> Native=Window->GetNativeWindow();if(!Native.IsValid()) return;
    HWND Handle=static_cast<HWND>(Native->GetOSWindowHandle());if(!Handle) return;
    RECT Client;if(!::GetClientRect(Handle,&Client)) return;
    POINT TopLeft={Client.left,Client.top},BottomRight={Client.right,Client.bottom};
    ::ClientToScreen(Handle,&TopLeft);::ClientToScreen(Handle,&BottomRight);
    const RECT Clip={TopLeft.x+Inset,TopLeft.y+Inset,BottomRight.x-Inset,BottomRight.y-Inset};
    if(Clip.right>Clip.left+8&&Clip.bottom>Clip.top+8) ::ClipCursor(&Clip);
}
#else
void ASClipCursorToClientArea(const TSharedPtr<SWindow>&,int32) {}
#endif
}

namespace {
bool TryParseInt32Strict(const FString& Text, int32& OutValue)
{
    const FString Clean = Text.TrimStartAndEnd();
    if (Clean.IsEmpty()) return false;

    int32 Index = 0;
    bool bNegative = false;
    if (Clean[0] == TCHAR('-') || Clean[0] == TCHAR('+'))
    {
        bNegative = Clean[0] == TCHAR('-');
        Index = 1;
        if (Index >= Clean.Len()) return false;
    }

    const int64 Limit = bNegative ? 2147483648LL : 2147483647LL;
    int64 ParsedValue = 0;
    for (; Index < Clean.Len(); ++Index)
    {
        const TCHAR C = Clean[Index];
        if (C < TCHAR('0') || C > TCHAR('9')) return false;
        const int32 Digit = static_cast<int32>(C - TCHAR('0'));
        if (ParsedValue > (Limit - Digit) / 10) return false;
        ParsedValue = ParsedValue * 10 + Digit;
    }

    const int64 SignedValue = bNegative ? -ParsedValue : ParsedValue;
    OutValue = static_cast<int32>(SignedValue);
    return true;
}
}

void AASPlayerController::BeginPlay() {
    Super::BeginPlay();if(!IsLocalController()) return;ApplyGameplayInput();
    if(auto* GI=GetGameInstance<UASGameInstance>()) {GI->SetQuality(GI->Quality);Notice=GI->LastMessage;}
    FTimerHandle Timer;GetWorldTimerManager().SetTimer(Timer,FTimerDelegate::CreateWeakLambda(this,[this]() {
        FVector ViewLocation;FRotator ViewRotation;GetPlayerViewPoint(ViewLocation,ViewRotation);
        int32 ViewWidth=0,ViewHeight=0;GetViewportSize(ViewWidth,ViewHeight);
        UE_LOG(LogTemp,Display,TEXT("AshenVisual: pawn=%s viewTarget=%s camera=%s rotation=%s viewport=%dx%d"),
            *GetNameSafe(GetPawn()),*GetNameSafe(GetViewTarget()),*ViewLocation.ToString(),*ViewRotation.ToString(),ViewWidth,ViewHeight);
        if(auto* S=AS::State(this)) if(S->Phase==EASPhase::Hub&&!MenuOpen()) OpenMenu(1);
    }),.8f,false);
}
void AASPlayerController::EndPlay(const EEndPlayReason::Type Reason) {
    if(IsLocalController()) ASClipCursorToClientArea(nullptr,-1);
    if(MenuLayer.IsValid()&&MenuViewport.IsValid()) MenuViewport->RemoveViewportWidgetContent(MenuLayer.ToSharedRef());
    if(auto* Client=LocalViewport()) if(Client->Viewport&&Client->Viewport->HasMouseCapture()) {Client->Viewport->CaptureMouse(false);Client->Viewport->LockMouseToViewport(false);}
    Menu.Reset();MenuLayer.Reset();Super::EndPlay(Reason);
}
void AASPlayerController::SetupInputComponent() {
    Super::SetupInputComponent();check(InputComponent);
    InputComponent->BindAxis("MoveForward",this,&AASPlayerController::MoveForward);InputComponent->BindAxis("MoveRight",this,&AASPlayerController::MoveRight);
    InputComponent->BindAxis("Turn",this,&AASPlayerController::Turn);InputComponent->BindAxis("LookUp",this,&AASPlayerController::Look);
    InputComponent->BindAxis("TurnStick",this,&AASPlayerController::TurnStick);InputComponent->BindAxis("LookStick",this,&AASPlayerController::LookStick);
    InputComponent->BindAction("Attack",IE_Pressed,this,&AASPlayerController::AttackDown);InputComponent->BindAction("Attack",IE_Released,this,&AASPlayerController::AttackUp);
    InputComponent->BindAction("Spiral",IE_Pressed,this,&AASPlayerController::SpiralDown);InputComponent->BindAction("Spiral",IE_Released,this,&AASPlayerController::SpiralUp);
    InputComponent->BindAction("HammerAlt",IE_Pressed,this,&AASPlayerController::HammerAltDown);InputComponent->BindAction("HammerAlt",IE_Released,this,&AASPlayerController::HammerAltUp);
    InputComponent->BindAction("Dash",IE_Pressed,this,&AASPlayerController::Dash);InputComponent->BindAction("Interact",IE_Pressed,this,&AASPlayerController::Interact);
    InputComponent->BindAction("Jump",IE_Pressed,this,&AASPlayerController::JumpDown);InputComponent->BindAction("Jump",IE_Released,this,&AASPlayerController::JumpUp);
    InputComponent->BindAction("Revive",IE_Pressed,this,&AASPlayerController::ReviveDown);InputComponent->BindAction("Revive",IE_Released,this,&AASPlayerController::ReviveUp);
    InputComponent->BindAction("Menu",IE_Pressed,this,&AASPlayerController::ToggleMenu);InputComponent->BindAction("Stats",IE_Pressed,this,&AASPlayerController::ASPerformance);
    InputComponent->BindAction("Shoulder",IE_Pressed,this,&AASPlayerController::Shoulder);
    InputComponent->BindAction("Reload",IE_Pressed,this,&AASPlayerController::Reload);
    InputComponent->BindAction("Rune",IE_Pressed,this,&AASPlayerController::Rune);
    InputComponent->BindAction("Build",IE_Pressed,this,&AASPlayerController::BuildMenu);
    InputComponent->BindAction("Build",IE_Released,this,&AASPlayerController::BuildMenuReleased);
    InputComponent->BindAxis("Zoom",this,&AASPlayerController::Zoom);
    InputComponent->BindAction("RelockMouse",IE_Pressed,this,&AASPlayerController::ApplyGameplayInput);
}
void AASPlayerController::MoveForward(float V) {if(!MenuOpen()) if(auto* H=Cast<AASCharacter>(GetPawn())) H->AddMovementInput(FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X),V);}
void AASPlayerController::MoveRight(float V) {if(!MenuOpen()) if(auto* H=Cast<AASCharacter>(GetPawn())) H->AddMovementInput(FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),V);}
bool AASPlayerController::CameraLocked() const {
    const auto* World=GetWorld();const auto* Settings=World?World->GetWorldSettings():nullptr;
    return Settings&&Settings->GetEffectiveTimeDilation()<.9f; // the final-kill slow-mo
}
void AASPlayerController::Turn(float V) {if(!MenuOpen()&&!CameraLocked()) {auto* GI=GetGameInstance<UASGameInstance>();AddYawInput(V*(GI?GI->Sensitivity:.85f));}}
void AASPlayerController::Look(float V) {if(!MenuOpen()&&!CameraLocked()) {auto* GI=GetGameInstance<UASGameInstance>();AddPitchInput(V*(GI?GI->Sensitivity:.85f));}}
void AASPlayerController::TurnStick(float V) {Turn(V*GetWorld()->GetDeltaSeconds()*115.f);}
void AASPlayerController::LookStick(float V) {Look(V*GetWorld()->GetDeltaSeconds()*90.f);}
void AASPlayerController::AttackDown() {if(!MenuOpen()) UpdateAim(true);if(!MenuOpen()) if(auto* H=Cast<AASCharacter>(GetPawn())) H->SetAttack(true);}
void AASPlayerController::AttackUp() {if(!MenuOpen()) UpdateAim(true);if(auto* H=Cast<AASCharacter>(GetPawn())) H->SetAttack(false);}
void AASPlayerController::SpiralDown() {if(!MenuOpen()) UpdateAim(true);if(!MenuOpen()) if(auto* H=Cast<AASCharacter>(GetPawn())) {if(H->Build.Weapon==1) H->RequestHammerLeap();else H->SetSpiral(true);}}
void AASPlayerController::SpiralUp() {if(!MenuOpen()) UpdateAim(true);if(auto* H=Cast<AASCharacter>(GetPawn())) if(H->Build.Weapon!=1) H->SetSpiral(false);}
void AASPlayerController::HammerAltDown() {if(!MenuOpen()) {UpdateAim(true);if(auto* H=Cast<AASCharacter>(GetPawn())) if(H->Build.Weapon==1) H->SetSpiral(true);}}
void AASPlayerController::HammerAltUp() {if(auto* H=Cast<AASCharacter>(GetPawn())) if(H->Build.Weapon==1) H->SetSpiral(false);}

void AASPlayerController::Rune() {if(!MenuOpen()) UpdateAim(true);if(!MenuOpen()) if(auto* H=Cast<AASCharacter>(GetPawn())) H->CastRune();}
void AASPlayerController::Reload() {if(!MenuOpen()) if(auto* H=Cast<AASCharacter>(GetPawn())) H->RequestReload();}
void AASPlayerController::Dash() {if(!MenuOpen()) if(auto* H=Cast<AASCharacter>(GetPawn())) H->Movement()->bWantsDash=true;}
void AASPlayerController::JumpDown() {if(!MenuOpen()) if(auto* H=Cast<AASCharacter>(GetPawn())) H->Jump();}
void AASPlayerController::JumpUp() {if(auto* H=Cast<AASCharacter>(GetPawn())) H->StopJumping();}
void AASPlayerController::ReviveDown() {if(!MenuOpen()) if(auto* H=Cast<AASCharacter>(GetPawn())) H->SetRevive(true);}
void AASPlayerController::ReviveUp() {if(auto* H=Cast<AASCharacter>(GetPawn())) H->SetRevive(false);}
void AASPlayerController::Shoulder() {
    if(MenuOpen()) return;
    if(auto* GI=GetGameInstance<UASGameInstance>()) {GI->CameraShoulder*=-1.f;GI->SaveCameraSettings();ApplyCameraSettings();}
}
void AASPlayerController::ASPerformance() {bShowStats=!bShowStats;}
void AASPlayerController::ASInputStatus() {
    auto* Client=LocalViewport();auto* Viewport=Client?Client->Viewport:nullptr;
    UE_LOG(LogTemp,Display,TEXT("AshenInput: world=%s viewport=%s class=%s local=%d foreground=%d focus=%d captured=%d menu=%d wantsGameplay=%d captureMode=%d lockMode=%d"),
        *GetNameSafe(GetWorld()),*GetNameSafe(Client),*GetNameSafe(Client?Client->GetClass():nullptr),IsLocalController(),
        Viewport&&Viewport->IsForegroundWindow(),Viewport&&Viewport->HasFocus(),Viewport&&Viewport->HasMouseCapture(),MenuOpen(),WantsGameplayInput(),
        Client?int32(Client->GetMouseCaptureMode()):-1,Client?int32(Client->GetMouseLockMode()):-1);
}
void AASPlayerController::Interact() {
    if(MenuOpen()) return;auto* H=Cast<AASCharacter>(GetPawn());auto* S=AS::State(this);if(!H||!S||!S->Arena) return;
    const int32 Page=FocusedInteraction>0?FocusedInteraction:S->Arena->StationAt(H->GetActorLocation());
    if(Page>0&&!ASAdventure::InReach(FVector::DistSquared2D(H->GetActorLocation(),S->Arena->InteractionPoint(Page)),H->GetActorLocation().Z-S->Arena->InteractionPoint(Page).Z)) {Notify(TEXT("Move closer to interact."));return;}
    if(Page==9&&S->Phase==EASPhase::Victory) {InteractServer(9);return;}
    if(Page==220) {OpenMenu(18);return;}
    if(Page==221) {OpenMenu(21);return;}
    // Portals use proximity interaction: E at the archway asks the server to start the walk-in (host only; the server checks range, locks and readiness).
    if(S->Phase==EASPhase::Hub&&((Page>=31&&Page<=33)||Page==22)) {PortalInteract(Page);return;}
    if(Page==222) {OpenMenu(23);return;}
    if(Page>=201&&Page<=203) {InteractServer(Page);return;}
    if(S->Phase==EASPhase::Hub&&Page>=100) {ShopWeapon=Page-100;OpenMenu(14);}
    else if(S->Phase==EASPhase::Hub&&Page>0) OpenMenu(Page);
    else if(S->Phase==EASPhase::Reward&&Page==9) {
        auto* PS=GetPlayerState<AASPlayerState>();if(PS&&!PS->bRewardClaimed) OpenMenu(9);else InteractServer();
    }
}
// E at an archway: guests toggle Ready; the host confirms the departure (the server re-checks range, locks and readiness).
void AASPlayerController::PortalInteract(int32 Page) {
    auto* S=AS::State(this);auto* H=Cast<AASCharacter>(GetPawn());auto* PS=GetPlayerState<AASPlayerState>();if(!S||!S->Arena||!H||!PS) return;
    const FVector Archway=S->Arena->InteractionPoint(Page)-FVector(0,0,150);
    if(FVector::DistSquared2D(H->GetActorLocation(),Archway)>360.f*360.f) {Notify(TEXT("Stand at the archway."));return;}
    if(PS->PlayerIndex!=0) {SetReady(!PS->bReady);return;}
    int32 Seed=1103;
    if(Page!=22&&!TryParseInt32Strict(SeedText,Seed)) {Notice=TEXT("Use a whole-number seed between -2147483648 and 2147483647.");return;}
    SelectedPortal=Page==22?3:Page-31;EnterPortal(SelectedPortal,Page==22?FMath::Rand():Seed);
}
void AASPlayerController::ToggleMenu() {
    if(MenuOpen()) {CloseMenu();SetPartyPaused(false);return;}
    if(auto* S=AS::State(this)) {if(S->bPartyPaused) {SetPartyPaused(false);QueueGameplayCapture();return;}if(S->Fighting()) SetPartyPaused(true);}
    OpenMenu(0);
}
void AASPlayerController::OpenMenu(int32 Page,bool PreserveScroll) {
    auto* Viewport=LocalViewport();if(!IsLocalController()||!Viewport) return;
    if(Menu.IsValid()&&Menu->Page==8) if(auto* GI=GetGameInstance<UASGameInstance>()) {GI->SaveCameraSettings();GI->SaveHUDSettings();}
    const float Offset=PreserveScroll&&Menu.IsValid()&&Menu->Page==Page?Menu->ScrollOffset():0;
    if(MenuLayer.IsValid()&&MenuViewport.IsValid()) MenuViewport->RemoveViewportWidgetContent(MenuLayer.ToSharedRef());
    MenuViewport=Viewport;
    CancelActions();if(auto* H=Cast<AASCharacter>(GetPawn())) H->ResetInput();
    SAssignNew(Menu,SASMenu).Controller(this).Page(Page);Menu->SetScrollOffset(Offset);
    SAssignNew(MenuLayer,SWeakWidget).PossiblyNullContent(Menu.ToSharedRef());
    Viewport->AddViewportWidgetContent(MenuLayer.ToSharedRef(),20);ApplyMenuInput();
}
void AASPlayerController::CloseMenu() {
    if(Menu.IsValid()&&Menu->Page==8) if(auto* GI=GetGameInstance<UASGameInstance>()) {GI->SaveCameraSettings();GI->SaveHUDSettings();}
    if(MenuLayer.IsValid()&&MenuViewport.IsValid()) MenuViewport->RemoveViewportWidgetContent(MenuLayer.ToSharedRef());
    Menu.Reset();MenuLayer.Reset();ApplyGameplayInput();QueueGameplayCapture();
}
void AASPlayerController::RefreshRewardMenu() {if(IsLocalController()&&Menu.IsValid()&&Menu->Page==9) OpenMenu(9,true);}
void AASPlayerController::PlayerTick(float Delta) {
    Super::PlayerTick(Delta);if(!IsLocalController()) return;auto* S=AS::State(this);if(!S) return;
    DamageNumbers.RemoveAll([S](const FASDamageNumber& N){return S->SimTime>N.Expires;});
    if(S->Room!=LastRoom||S->Phase!=LastPhase) {
        if(S->Phase==EASPhase::Defeat) OpenMenu(10);else if(S->Phase==EASPhase::Victory) {CloseMenu();Notify(TEXT("Expedition complete! Permanent rewards are being saved. Enter the teal portal and press E to return together."));}
        else if(S->Phase==EASPhase::Hub && LastPhase!=EASPhase::Hub) {
            ResubmitProfile();
            OpenMenu(1);
        } else if(S->Phase==EASPhase::Countdown && (S->Room!=LastRoom||LastPhase==EASPhase::Hub||LastPhase==EASPhase::Reward)) CloseMenu();
        LastRoom=S->Room;LastPhase=S->Phase;
    }
    if(S->bEnteringPortal&&PortalStartedAt<0.f) PortalStartedAt=GetWorld()->GetTimeSeconds();else if(!S->bEnteringPortal) PortalStartedAt=-1.f;
    auto* Client=LocalViewport();auto* Viewport=Client?Client->Viewport:nullptr;
    const bool Foreground=Viewport&&Viewport->IsForegroundWindow();
    const bool Focused=Foreground&&Viewport->HasFocus();
    if(!Focused&&bHadViewportFocus) ViewportFocusLost();
    if(WantsGameplayInput()) {
        if(Foreground&&(Focused||bCaptureRequested)) {
            if(!bGameplayInputApplied||bCaptureRequested||!Viewport->HasMouseCapture()) ApplyGameplayInput();
            // Refresh confinement every tick, independently of capture, including after a resize or alt-tab.
            Viewport->LockMouseToViewport(true);
            if(Viewport->IsFullscreen()) {
                if(auto* Local=GetLocalPlayer()) if(auto Widget=Client->GetGameViewportWidget()) Local->GetSlateOperations().LockMouseToWidget(Widget.ToSharedRef());
                ASClipCursorToClientArea(Client->GetWindow(),2);
            }
        }
    } else if(!bMenuInputApplied) ApplyMenuInput();
    bHadViewportFocus=Focused;
    ApplyCameraSettings();
    if(!MenuOpen()) {const bool Send=GetWorld()->GetTimeSeconds()>=NextAim;if(Send) NextAim=GetWorld()->GetTimeSeconds()+.033f;UpdateAim(Send);}

}
void AASPlayerController::RequestProfile_Implementation() {
    ResubmitProfile();
}
void AASPlayerController::SubmitProfile_Implementation(const FString& ID,const FASBuild& Build,int32 Protocol,int32 Experience,int32 ColorChoice,bool RaiseStakes,int32 TitleIndex,int32 TitleTier,int32 ClearedMask,int32 EndlessChoices) {
    if(auto* S=AS::State(this)) if(S->Phase==EASPhase::Hub) if(auto* PS=GetPlayerState<AASPlayerState>()) PS->bWagerVote=RaiseStakes;
    if(Protocol!=ASData::Protocol) {Notify(TEXT("Protocol mismatch. Use the same project build on both PCs."));return;}
    if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->ProfileArrived(this,ID,Build,Experience,ColorChoice,ClearedMask,EndlessChoices);
    SetCharacterTitle(TitleIndex,TitleTier);
}
void AASPlayerController::ResubmitProfile() {
    auto* GI=GetGameInstance<UASGameInstance>();if(!GI) return;
    const auto& P=GI->Profile;
    SubmitProfile(P.ProfileId,P.Build,ASData::Protocol,P.Experience,P.ColorChoice,GI->bRaiseStakes,P.TitleChoice,P.TitleChoice>=0?ASCareer::Tier(P.Career[P.TitleChoice],P.TitleChoice):0,ASExpedition::ClearedMask(P.Wins,P.ClearedMask),P.EndlessChoices);
}
void AASPlayerController::ProfileRejected_Implementation(const FString& KeptProfileId) {
    // The server refused the switch; return to the character it still has for us.
    if(auto* GI=GetGameInstance<UASGameInstance>()) {
        const FString Message=GI->LastMessage;
        // The server kept KeptProfileId. Return to that character without the hub-only guards, then resubmit.
        for(int32 Try=0;Try<2&&GI->Profile.ProfileId!=KeptProfileId;++Try) GI->SwitchCharacterSlot(GI->ActiveCharacterSlot==0?1:0,true);
        ResubmitProfile();
        GI->LastMessage=Message;
        if(Menu.IsValid()) OpenMenu(Menu->Page,true);
    }
}
void AASPlayerController::CancelActions_Implementation() {if(auto* H=Cast<AASCharacter>(GetPawn())) H->ResetInput();}
void AASPlayerController::SetReady_Implementation(bool Ready) {auto* S=AS::State(this);if(S&&S->Phase==EASPhase::Hub) if(auto* PS=GetPlayerState<AASPlayerState>()) if(PS->bProfileReady) PS->bReady=Ready;}
void AASPlayerController::SetDifficulty_Implementation(int32 Difficulty) {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->SetDifficulty(this,Difficulty);}
void AASPlayerController::GambleAtWell_Implementation(int32 StakeIndex) {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->GambleAtWell(this,StakeIndex);}
void AASPlayerController::BuyShopItem_Implementation(int32 Index) {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->BuyShopItem(this,Index);}
void AASPlayerController::EnterPortal_Implementation(int32 Portal,int32 Seed) {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->StartPortal(this,Portal,Seed);}
void AASPlayerController::CloseMenuForPortal_Implementation() {if(MenuOpen()) CloseMenu();}
void AASPlayerController::StartEndless_Implementation(int32 Seed) {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->BeginEndless(this,Seed);}
void AASPlayerController::StartRun_Implementation(int32 Seed) {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->BeginRun(this,Seed);}
void AASPlayerController::ChooseReward_Implementation(int32 Index) {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->Reward(this,Index);}
void AASPlayerController::RerollRewards_Implementation() {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->Reroll(this);}
void AASPlayerController::ToggleWagerVote_Implementation(bool Enabled) {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->ToggleWager(this,Enabled);}
void AASPlayerController::SetPartyPaused_Implementation(bool Paused) {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) if(GM->ReturningToHub()) return;if(auto* S=AS::State(this)) if(S->Fighting()) {S->bPartyPaused=Paused;for(auto W:S->Heroes) if(auto* H=W.Get()) H->ResetInput();}}
void AASPlayerController::ReturnHub_Implementation() {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) if(GM->Host(this)) GM->BackToHub();}
void AASPlayerController::ContinueAlone_Implementation() {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) if(GM->Host(this)) GM->AbandonReconnect();}
void AASPlayerController::InteractServer_Implementation(int32 Target) {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->Interact(this,Target);}
void AASPlayerController::SetCharacterTitle_Implementation(int32 Index,int32 Tier) {
    if(auto* PS=GetPlayerState<AASPlayerState>()) if(PS->bProfileReady) {
        PS->TitleIndex=Index>=0&&Index<ASCareer::Count?Index:-1;PS->TitleTier=PS->TitleIndex>=0?FMath::Clamp(Tier,1,3):0;PS->ForceNetUpdate();
    }
}
void AASPlayerController::SetCharacterColor_Implementation(int32 Choice) {
    auto* PS=GetPlayerState<AASPlayerState>();if(!PS||!PS->bProfileReady) return;
    PS->ColorChoice=FMath::Clamp(Choice,-1,7);PS->ForceNetUpdate();
    if(auto* H=Cast<AASCharacter>(GetPawn())) {H->ColorChoice=PS->ColorChoice;H->RefreshLook();H->ForceNetUpdate();}
}
void AASPlayerController::UseShrine_Implementation() {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->UseShrine(this);}
void AASPlayerController::TryWeapon_Implementation(int32 Weapon) {
    auto* S=AS::State(this);auto* H=Cast<AASCharacter>(GetPawn());
    if(!S||S->Phase!=EASPhase::Hub||!H||!S->Arena||Weapon<0||Weapon>=ASRules::WeaponCount||S->Arena->WeaponAt(H->GetActorLocation())!=Weapon) return;
    H->ResetInput(true);H->Build.Weapon=Weapon;H->NextHammerLeap=0;H->NextAttack=H->NextSpiral=0;H->ResetMagazine();H->RefreshLook();H->ForceNetUpdate();
    Notify(TEXT("Practice weapon equipped. Your purchased expedition loadout is unchanged; test it on the hub target."));
}
void AASPlayerController::ReceiveReceipt_Implementation(const FASReceipt& R) {
    if(auto* GI=GetGameInstance<UASGameInstance>()) {const bool First=!GI->Profile.Receipts.Contains(R.Id);const bool Saved=GI->Receive(R);if(!Saved||First) Notify(GI->LastMessage);AckReceipt(R.Id,Saved,GI->Profile.Experience);if(Saved&&GI->Profile.TitleChoice>=0) SetCharacterTitle(GI->Profile.TitleChoice,ASCareer::Tier(GI->Profile.Career[GI->Profile.TitleChoice],GI->Profile.TitleChoice));}
}
void AASPlayerController::AckReceipt_Implementation(const FString& ID,bool Success,int32 Experience) {
    if(auto* PS=GetPlayerState<AASPlayerState>()) if(PS->bSavePending&&PS->PendingReceipt.Id==ID&&Success) {
        PS->CharacterLevel=ASCareer::Level(Experience);
        if(!PS->ReceiptQueue.IsEmpty()) {PS->PendingReceipt=PS->ReceiptQueue[0];PS->ReceiptQueue.RemoveAt(0);ReceiveReceipt(PS->PendingReceipt);}
        else {PS->bSavePending=false;PS->PendingReceipt=FASReceipt();}
        PS->ForceNetUpdate();
    }
}
void AASPlayerController::Notify_Implementation(const FString& Message) {
    // Messages tagged "!BANNER|Title|Sub" go to the banner instead of the toast stack.
    if(Message.StartsWith(TEXT("!BANNER|"))) {
        TArray<FString> Parts;Message.ParseIntoArray(Parts,TEXT("|"),false);
        AlertTitle=Parts.IsValidIndex(1)?Parts[1].Left(40):FString();AlertSub=Parts.IsValidIndex(2)?Parts[2].Left(40):FString();++AlertSeq;return;
    }
    Notice=Message.Left(400);const float Now=GetWorld()->GetTimeSeconds();NoticeUntil=Now+ASPresentation::NoticeLife;
    Notifications.RemoveAll([Now](const FASNotice& N){return Now-N.Born>=ASPresentation::NoticeLife;});
    if(Notice.IsEmpty()) return;
    // Newest is at the bottom. Older entries retain their own fade-out clocks.
    if(Notifications.Num()>=4) Notifications.RemoveAt(0);
    Notifications.Add({Notice,Now,Notifications.IsEmpty()?0.f:Notifications.Last().Offset+84.f});
}
void AASPlayerController::DamageNumber_Implementation(FVector_NetQuantize Position,float Value,bool Critical,EASHitKind Kind) {
    if(Critical) LastCritAt=AS::Time(this);if(Kind==EASHitKind::Received||Kind==EASHitKind::Shield) LastHurtAt=AS::Time(this);
    auto* GI=GetGameInstance<UASGameInstance>();if(!GI||!GI->bShowNumbers) return;
    if(DamageNumbers.Num()>=64) DamageNumbers.RemoveAt(0);DamageNumbers.Add({Position,Value,AS::Time(this)+1.25f,Critical,Kind,FMath::FRandRange(-65.f,65.f),DamageLane,-1,1.25f,FMath::FRand()*100.f});DamageLane=(DamageLane+1)%3;
}
void AASPlayerController::SynergyCue_Implementation(int32 Trigger) {
    if(Trigger<0||Trigger>2) return;
    // Placeholder cues reuse existing assets with distinct pitch per synergy; P2 audio pass to replace.
    const TCHAR* Cues[]={TEXT("S_Hit"),TEXT("S_Collapse"),TEXT("S_Spiral")};const float Pitch[]={1.45f,1.f,.8f};
    if(auto* Cue=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),Cues[Trigger],Cues[Trigger]))) UGameplayStatics::PlaySound2D(this,Cue,.8f,Pitch[Trigger]);
}
void AASPlayerController::RewardText_Implementation(const FString& InText,FLinearColor Color,bool Big) {
    if(InText.IsEmpty()) return;
    FString Text=InText;int32 Anchor=-1;
    if(InText.StartsWith(TEXT("@"))) {
        int32 Bar=INDEX_NONE;
        if(InText.FindChar(TEXT('|'),Bar)&&Bar>1&&Bar<6) {Anchor=FMath::Clamp(FCString::Atoi(*InText.Mid(1,Bar-1)),0,7);Text=InText.Mid(Bar+1);}
    }
    if(Text.IsEmpty()) return;
    const float Now=GetWorld()?GetWorld()->GetTimeSeconds():0.f;
    FloatTexts.RemoveAll([Now](const FASFloatText& T){return Now-T.Born>=ASPresentation::FloatTextLife;});
    if(FloatTexts.Num()>=6) FloatTexts.RemoveAt(0);
    FloatTexts.Add({Text.Left(60),Color,Now,Big,FloatSlot,Anchor});FloatSlot=(FloatSlot+1)%4;
    // Placeholder audio hook for trait acquisition; P2 audio pass to replace.
    if(Big) if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Reward.S_Reward"))) UGameplayStatics::PlaySound2D(this,Cue,.7f,1.15f);
}
void AASPlayerController::SynergyNumber_Implementation(FVector_NetQuantize Position,float Value,int32 Trigger) {
    if(Trigger<0||Trigger>2) return;
    auto* GI=GetGameInstance<UASGameInstance>();if(!GI||!GI->bShowNumbers) return;
    if(DamageNumbers.Num()>=64) DamageNumbers.RemoveAt(0);
    DamageNumbers.Add({Position,Value,AS::Time(this)+1.7f,false,EASHitKind::Physical,FMath::FRandRange(-45.f,45.f),DamageLane,Trigger,1.7f,FMath::FRand()*100.f});DamageLane=(DamageLane+1)%3;
}
FKey AASPlayerController::BoundKey(FName Binding) const {
    auto* Settings=GetDefault<UInputSettings>();FName Axis;float Scale=1;
    if(Binding=="Forward"||Binding=="Backward") {Axis="MoveForward";Scale=Binding=="Forward"?1.f:-1.f;}
    if(Binding=="Right"||Binding=="Left") {Axis="MoveRight";Scale=Binding=="Right"?1.f:-1.f;}
    if(!Axis.IsNone()) {for(const auto& M:Settings->GetAxisMappings()) if(M.AxisName==Axis&&M.Scale==Scale&&!M.Key.IsGamepadKey()) return M.Key;}
    else {for(const auto& M:Settings->GetActionMappings()) if(M.ActionName==Binding&&!M.Key.IsGamepadKey()) return M.Key;}
    return EKeys::Invalid;
}
void AASPlayerController::Rebind(FName Binding,FKey Key) {
    if(!Key.IsValid()||Key.IsAxis1D()||Key.IsAxis2D()||Key.IsAxis3D()||Key==EKeys::Escape) {Notice=TEXT("Choose a digital key or mouse button. Escape remains available to close menus.");return;}
    auto* Settings=GetMutableDefault<UInputSettings>();FName Axis;float Scale=1;
    if(Binding=="Forward"||Binding=="Backward") {Axis="MoveForward";Scale=Binding=="Forward"?1.f:-1.f;}
    if(Binding=="Right"||Binding=="Left") {Axis="MoveRight";Scale=Binding=="Right"?1.f:-1.f;}
    if(!Axis.IsNone()) {
        auto Maps=Settings->GetAxisMappings();for(const auto& M:Maps) if(M.AxisName==Axis&&M.Scale==Scale&&M.Key.IsGamepadKey()==Key.IsGamepadKey()) Settings->RemoveAxisMapping(M,false);
        Settings->AddAxisMapping(FInputAxisKeyMapping(Axis,Key,Scale),false);
    } else {
        auto Maps=Settings->GetActionMappings();for(const auto& M:Maps) if(M.ActionName==Binding&&M.Key.IsGamepadKey()==Key.IsGamepadKey()) Settings->RemoveActionMapping(M,false);
        Settings->AddActionMapping(FInputActionKeyMapping(Binding,Key),false);
    }
    Settings->SaveKeyMappings();Settings->SaveConfig();Settings->ForceRebuildKeymaps();
    Notice=TEXT("Binding saved. Avoid assigning the same key to different gameplay actions.");
}
void AASPlayerController::Action(int32 Code,int32 Index) {
    auto* GI=GetGameInstance<UASGameInstance>();auto* S=AS::State(this);auto* PS=GetPlayerState<AASPlayerState>();if(!GI||!S) return;
    const int32 CurrentPage=Menu.IsValid()?Menu->Page:0;
    if(Code==0) {CloseMenu();SetPartyPaused(false);return;}
    if(Code==10) {OpenMenu(Index);return;}
    if(Code>=20&&Code<=29) {
        if(S->Phase!=EASPhase::Hub) {Notice=TEXT("Permanent loadouts can only be changed in the hub.");return;}
        if(GI->Purchase(Code-20,Index)) ResubmitProfile();
        Notice=GI->LastMessage;OpenMenu(CurrentPage,true);return;
    }
    if(Code==30&&PS) SetReady(!PS->bReady);
    else if(Code==31) {Notice=TEXT("Choose Ember, Ashen or Blood at the three hub portals. Press E at an archway to depart.");}
    else if(Code==76) {int32 Seed=1103;if(!TryParseInt32Strict(SeedText,Seed)) {Notice=TEXT("Use a whole-number seed between -2147483648 and 2147483647.");return;}EnterPortal(SelectedPortal,Seed);}
    else if(Code==32) {ChooseReward(Index);CloseMenu();}
    else if(Code==33) {ReturnHub();CloseMenu();}
    else if(Code==34) {ContinueAlone();CloseMenu();}
    else if(Code==35) {
        if(S->Phase!=EASPhase::Hub) {Notice=TEXT("Return to the hub before hosting.");return;}
        if(GetNetMode()==NM_ListenServer) {Notice=TEXT("Already hosting. Share the displayed code and your reachable IP.");return;}
        CloseMenu();UGameplayStatics::OpenLevel(this,FName(TEXT("/Game/Maps/AshenSpiral")),true,TEXT("listen"));return;
    } else if(Code==36) {
        FString Address=JoinAddress.TrimStartAndEnd(),CodeText=JoinCode.TrimStartAndEnd().ToUpper();
        bool Valid=!Address.IsEmpty()&&Address.Len()<128&&!CodeText.IsEmpty()&&CodeText.Len()<=12;
        for(TCHAR C:Address) if(!(FChar::IsAlnum(C)||C=='.'||C==':'||C=='-'||C=='['||C==']')) Valid=false;
        for(TCHAR C:CodeText) if(!FChar::IsAlnum(C)) Valid=false;
        if(!Valid) {Notice=TEXT("Enter an IP/hostname (optional :24567 port) and the host's session code.");return;}
        CloseMenu();ClientTravel(Address+TEXT("?Code=")+CodeText+FString::Printf(TEXT("?Protocol=%d?Profile="),ASData::Protocol)+GI->Profile.ProfileId,TRAVEL_Absolute);return;
    } else if(Code==37) {CloseMenu();UGameplayStatics::OpenLevel(this,FName(TEXT("/Game/Maps/AshenSpiral")));return;}
    else if(Code==40) GI->SetQuality(Index);
    else if(Code==41) GI->Sensitivity=FMath::Clamp(GI->Sensitivity+.1f*Index,.2f,2.f);
    else if(Code==42) GI->bShowNumbers=!GI->bShowNumbers;
    else if(Code==43) {ConsoleCommand(TEXT("quit"));return;}
    else if(Code==44) {
        Rebind("Jump",EKeys::SpaceBar);Rebind("Dash",EKeys::LeftShift);
        Rebind("Jump",EKeys::Gamepad_FaceButton_Bottom);Rebind("Dash",EKeys::Gamepad_FaceButton_Right);
        auto* Settings=GetMutableDefault<UInputSettings>();Settings->AddActionMapping(FInputActionKeyMapping("Dash",EKeys::RightShift),false);
        Settings->SaveKeyMappings();Settings->SaveConfig();Settings->ForceRebuildKeymaps();
        Notice=TEXT("Movement defaults restored: Space / A jumps; Shift / B dashes.");
    }
    else if(Code==45) {TryWeapon(ShopWeapon);CloseMenu();return;}
    else if(Code==47) {
        if(S->Phase!=EASPhase::Hub||GetNetMode()!=NM_Standalone) {Notice=TEXT("Return to an offline hub before importing progress.");return;}
        if(GI->PreviewImport(ProgressImportPath.IsEmpty()?GI->DefaultImportPath():ProgressImportPath)) {OpenMenu(16);return;}
        Notice=GI->LastMessage;
    } else if(Code==48) {
        if(S->Phase!=EASPhase::Hub||GetNetMode()!=NM_Standalone) return;
        if(GI->RestoreImport()) {CloseMenu();UGameplayStatics::OpenLevel(this,FName(TEXT("/Game/Maps/AshenSpiral")));return;}
        Notice=GI->LastMessage;
    } else if(Code==49) GI->OpenProgressFolder();
    else if(Code==50) {GI->ExportProgress();Notice=GI->LastMessage;}
    else if(Code==51) {GI->SetDisplayMode(Index);Notice=GI->LastMessage;}
    else if(Code==52) {GI->NumberScale=.8f+.2f*FMath::Clamp(Index,0,4);GI->SaveHUDSettings();}
    else if(Code==53) {GI->HUDTextScale=GI->HUDTextScale<1.1f?1.2f:1.f;GI->SaveHUDSettings();}
    else if(Code==59) {GI->HUDScale=float(FMath::Clamp(Index,0,3)+1)*.25f;GI->SaveHUDSettings();}
    else if(Code==71) {
        if(S->Phase!=EASPhase::Hub) {Notice=TEXT("Switch characters in the hub.");return;}
        if(GI->SwitchCharacterSlot(Index)) ResubmitProfile();
        Notice=GI->LastMessage;
    }
    else if(Code==73) {GambleAtWell(Index);}
    else if(Code==75) {BuyShopItem(Index);}
    else if(Code==74) {EnterPortal(3,FMath::Rand());return;}
    else if(Code==72) {
        if(S->Phase!=EASPhase::Hub) {Notice=TEXT("Difficulty is chosen in the hub.");return;}
        SetDifficulty(Index);Notice=TEXT("Difficulty selection sent to the host.");
    }
    else if(Code==77) {GI->bHoldBuild=!GI->bHoldBuild;GI->SaveCameraSettings();Notice=GI->bHoldBuild?TEXT("Hold TAB to view your build."):TEXT("TAB toggles your build panel.");OpenMenu(CurrentPage,true);return;}
    else if(Code==70) {GI->bReduceMotion=!GI->bReduceMotion;GI->SaveCameraSettings();ApplyCameraSettings();}
    else if(Code==54) {GI->ResetCameraSettings();ApplyCameraSettings();Notice=TEXT("Camera defaults restored and saved. Close this menu to check your view.");}
    else if(Code==55) {GI->CrosshairStyle=(GI->CrosshairStyle+1)%4;GI->SaveHUDSettings();}
    else if(Code==56) {GI->CrosshairColorIndex=(GI->CrosshairColorIndex+1)%8;GI->SaveHUDSettings();}
    else if(Code==57) {GI->CrosshairScale=1.f;GI->CrosshairStyle=GI->CrosshairColorIndex=0;GI->SaveHUDSettings();}
    else if(Code==58) {
        auto Candidate=GI->Profile;Candidate.ColorChoice=FMath::Clamp(Index,-1,7);
        if(GI->Commit(Candidate)) {SetCharacterColor(GI->Profile.ColorChoice);Notice=TEXT("Character color saved. Minimap identities remain P1 cyan and P2 white.");}
        else Notice=GI->LastMessage;
    }
    else if(Code==64&&Index>=0&&Index<ASCareer::Count&&ASCareer::Tier(GI->Profile.Career[Index],Index)>0) {
        auto Candidate=GI->Profile;Candidate.TitleChoice=Index;
        if(GI->Commit(Candidate)) {SetCharacterTitle(Index,ASCareer::Tier(GI->Profile.Career[Index],Index));Notice=TEXT("Achievement title equipped and saved.");}
    }
    else if(Code==62) {UseShrine();CloseMenu();return;}
    else if(Code==60) {RerollRewards();return;}
    else if(Code==61) {GI->bRaiseStakes=!GI->bRaiseStakes;GI->SaveHUDSettings();ToggleWagerVote(GI->bRaiseStakes);return;}
    if(Code>=40) OpenMenu(CurrentPage,true);
}

void AASPlayerController::ApplyCameraSettings() {
    if(!IsLocalController()) return;
    auto* H=Cast<AASCharacter>(GetPawn());auto* GI=GetGameInstance<UASGameInstance>();if(!H||!GI) return;
    H->CameraZoom=GI->CameraDistance;if(GI->bReduceMotion) H->Camera->SetRelativeLocation(FVector::ZeroVector);
    H->Boom->TargetOffset.Z=GI->CameraHeight;
    H->Boom->SocketOffset.Y=GI->CameraShoulder;
    H->Camera->SetRelativeRotation(FRotator(GI->CameraPitch,0.f,0.f));
    {const auto* IntroState=AS::State(this);const float IntroAge=IntroState?IntroState->BossIntroAge():-1.f;H->Camera->SetFieldOfView(GI->CameraFOV-ASPresentation::BossIntroAt(IntroAge,GI->bReduceMotion).FovPush+ASPresentation::PortalKickFov(PortalAge(),GI->bReduceMotion));}
    // Character animation is suspended during a party pause; settings still apply.
    if(auto* S=AS::State(this)) if(S->Frozen()) H->Boom->TargetArmLength=GI->CameraDistance;
}
UGameViewportClient* AASPlayerController::LocalViewport() const {
    const auto* Local=GetLocalPlayer();return Local?Local->ViewportClient.Get():nullptr;
}
bool AASPlayerController::WantsGameplayInput() const {
    const auto* S=AS::State(this);return IsLocalController()&&!MenuOpen()&&!(S&&S->Frozen());
}
void AASPlayerController::QueueGameplayCapture() {bCaptureRequested=true;bMenuInputApplied=false;}
void AASPlayerController::ViewportFocusLost() {
    ASClipCursorToClientArea(nullptr,-1);
    bHadViewportFocus=false;bGameplayInputApplied=false;bMenuInputApplied=false;bCaptureRequested=false;
    CancelActions();if(auto* H=Cast<AASCharacter>(GetPawn())) H->ResetInput();
}
void AASPlayerController::ApplyMenuInput() {
    if(!IsLocalController()) return;auto* Client=LocalViewport();if(!Client) return;
    bGameplayInputApplied=false;bMenuInputApplied=true;bShowMouseCursor=true;
    if(auto* Own=Cast<UASGameViewportClient>(Client)) Own->SetGameplayMouseLock(false);
    // Menus and pause leave the cursor free so it can cross monitors; confinement applies only during active gameplay.
    Client->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);Client->SetMouseLockMode(EMouseLockMode::DoNotLock);Client->SetHideCursorDuringCapture(false);
    auto* Viewport=Client->Viewport;if(!Viewport||!Viewport->IsForegroundWindow()) return;
    FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    if(Menu.IsValid()) Mode.SetWidgetToFocus(Menu);
    SetInputMode(Mode);
    Viewport->CaptureMouse(false);
    Viewport->LockMouseToViewport(false);
    if(auto* Local=GetLocalPlayer()) Local->GetSlateOperations().ReleaseMouseLock();
    ASClipCursorToClientArea(nullptr,-1);
}
void AASPlayerController::ApplyGameplayInput() {
    if(!WantsGameplayInput()) {ApplyMenuInput();return;}
    auto* Local=GetLocalPlayer();auto* Client=LocalViewport();if(!Local||!Client) return;
    if(auto* Own=Cast<UASGameViewportClient>(Client)) Own->SetGameplayMouseLock(true);
    Client->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
    Client->SetMouseLockMode(EMouseLockMode::LockAlways);Client->SetHideCursorDuringCapture(true);
    auto* Viewport=Client->Viewport;
    // Never make a background PIE/standalone window steal the other player's mouse.
    if(!Viewport||!Viewport->IsForegroundWindow()) {bCaptureRequested=true;return;}
    bShowMouseCursor=false;bEnableClickEvents=false;bEnableMouseOverEvents=false;
    FInputModeGameOnly Mode;Mode.SetConsumeCaptureMouseDown(true);SetInputMode(Mode);
    if(auto Widget=Client->GetGameViewportWidget()) {
        // Relative movement + capture + confinement must all target this local viewport.
        Local->GetSlateOperations().UseHighPrecisionMouseMovement(Widget.ToSharedRef())
            .CaptureMouse(Widget.ToSharedRef()).LockMouseToWidget(Widget.ToSharedRef());
    }
    Viewport->CaptureMouse(true);Viewport->LockMouseToViewport(true);
    bGameplayInputApplied=true;bMenuInputApplied=false;bCaptureRequested=false;
}
// Hold mode: the panel shows while TAB is held and closes on release. Toggle mode (default) is unchanged.
void AASPlayerController::BuildMenuReleased() {
    const auto* GI=GetGameInstance<UASGameInstance>();
    if(GI&&GI->bHoldBuild&&Menu.IsValid()&&Menu->Page==17) {CloseMenu();SetPartyPaused(false);}
}
void AASPlayerController::BuildMenu() {
    const auto* GI=GetGameInstance<UASGameInstance>();const bool Hold=GI&&GI->bHoldBuild;
    if(Hold&&Menu.IsValid()&&Menu->Page==17) return; // key repeat while held: stay open
    if(Menu.IsValid()&&Menu->Page==17) {CloseMenu();return;}
    OpenMenu(17);
}
void AASPlayerController::Zoom(float V) {
    if(MenuOpen()||FMath::IsNearlyZero(V)) return;
    if(auto* GI=GetGameInstance<UASGameInstance>()) GI->CameraDistance=ASAdventure::CameraDistance(GI->CameraDistance,V);
}
void AASPlayerController::UpdateAim(bool Send) {
    auto* H=Cast<AASCharacter>(GetPawn());auto* S=AS::State(this);if(!H||!S) return;
    int32 W=0,Height=0;GetViewportSize(W,Height);FVector Start,Direction;
    if(!DeprojectScreenPositionToWorld(W*.5f,Height*.5f,Start,Direction)) return;
    FCollisionQueryParams Query;Query.AddIgnoredActor(H);Query.bTraceComplex=true;
    for(auto Weak:S->Heroes) if(auto* Ally=Weak.Get()) Query.AddIgnoredActor(Ally);
    FHitResult Hit;const FVector End=Start+Direction*15000.f;
    AimTarget=GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Query)?Hit.ImpactPoint:End;
    if(Send) H->SetAim(AimTarget);
    FocusedInteraction=-1;if(!S->Arena) return;
    TArray<int32> Ids;S->Arena->InteractionCandidates(Ids);float Best=.965f;
    for(int32 Id:Ids) {
        const FVector Point=S->Arena->InteractionPoint(Id),To=Point-Start;
        if(FVector::DistSquared2D(H->GetActorLocation(),Point)>1400.f*1400.f) continue;
        FHitResult Occlusion;const bool Blocked=GetWorld()->LineTraceSingleByObjectType(Occlusion,Start,Point,FCollisionObjectQueryParams(ECC_WorldStatic));
        if(Blocked&&FVector::DistSquared(Occlusion.ImpactPoint,Point)>110.f*110.f) continue;
        const float Score=FVector::DotProduct(To.GetSafeNormal(),Direction);
        if(Score>Best) {Best=Score;FocusedInteraction=Id;}
    }
}
