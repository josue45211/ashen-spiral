# Mouse capture changes — 0.17.0

## Where the fix belongs

Capture belongs to the locally controlled PlayerController and its LocalPlayer's GameViewportClient. The previous source accessed GEngine->GameViewport and used global Slate focus/release operations. With multiple PIE game instances, those operations can target a different window. This is a concrete source defect and a plausible cause of P2's symptoms; the exact failure has not been reproduced on P2's PC.

The update uses UASGameViewportClient for window-focus and refocus-click notifications. AASPlayerController applies gameplay or menu input to its own viewport. HUD and menu widgets are also attached to that viewport, so P2's layer is not attached to P1's window.

| File | Change |
|---|---|
| Source/AshenSpiral/Public/ASGameViewportClient.h | New runtime viewport subclass and gameplay-lock policy |
| Source/AshenSpiral/Private/ASGameViewportClient.cpp | Focus/activation callbacks; consume a refocus click |
| Source/AshenSpiral/Public/ASPlayerController.h | Capture state, owning viewport accessor and diagnostics declaration |
| Source/AshenSpiral/Private/ASPlayerController.cpp | Relative capture/confinement; menu release; recovery; F10 and ASInputStatus |
| Source/AshenSpiral/Public/ASHUD.h and Private/ASHUD.cpp | Cache/use the owning viewport for the Slate HUD layer |
| Config/DefaultEngine.ini | Register the viewport class |
| Config/DefaultInput.ini | Preserve permanent capture/LockAlways defaults; add F10 |
| Config/DefaultEditorPerProjectUserSettings.ini | Ask PIE to take mouse control on launch |

The complete unified source/config changes against 0.16.2 are in UPDATE_0.17.0.patch. The full project already contains them; do not apply that patch a second time. Core input functions are reproduced below exactly from this package.

## Intended behavior and relocking

- Active gameplay: hidden cursor, GameOnly input, high-precision relative mouse movement, permanent capture and confinement to the owning viewport. No manual screen-coordinate clipping or repeated cursor warping is used.
- Esc, Tab, reward/settings menus, or a replicated party pause: visible cursor, GameAndUI input, no capture and no lock. The pointer can move to the second monitor.
- Closing the menu: requests capture again once gameplay resumes. Closing a pause menu may wait for the server's unpause state to arrive.
- After Alt-Tab or clicking another application: click in the game window to regain focus/capture. A capture click is intended to be consumed, so it does not also fire a shot.
- F10 is a rebindable manual capture request. It works only when the game receives keyboard input and no menu/party pause is open; it cannot receive keys while another application owns focus.
- A foreground/focus guard prevents background co-op windows from repeatedly taking the mouse from the active window. Loss of focus also clears held combat actions.

Escape can be intercepted by Unreal Editor's Stop Play Session shortcut before this code receives it. In Edit → Editor Preferences → Keyboard Shortcuts, find Play World (PIE/SIE) → Stop, clear plain Escape from both shortcut slots, or bind Stop to Shift+Esc. The editor Stop button remains available. The startup setting below does not change your editor shortcut preferences. A saved per-user PIE setting may override GameGetsMouseControl; verify the editor's Game Gets Mouse Control option if needed. Play_Standalone.bat avoids the PIE Stop shortcut.

## Exact controller implementation

Required headers include Engine/LocalPlayer.h, ASGameViewportClient.h, Widgets/SViewport.h, Slate/SceneViewport.h, UnrealClient.h and Input/Reply.h.

```cpp
UGameViewportClient* AASPlayerController::LocalViewport() const {
    const auto* Local=GetLocalPlayer();return Local?Local->ViewportClient.Get():nullptr;
}
bool AASPlayerController::WantsGameplayInput() const {
    const auto* S=AS::State(this);return IsLocalController()&&!MenuOpen()&&!(S&&S->Frozen());
}
void AASPlayerController::QueueGameplayCapture() {bCaptureRequested=true;bMenuInputApplied=false;}
void AASPlayerController::ViewportFocusLost() {
    bHadViewportFocus=false;bGameplayInputApplied=false;bMenuInputApplied=false;bCaptureRequested=false;
    CancelActions();if(auto* H=Cast<AASCharacter>(GetPawn())) H->ResetInput();
}
void AASPlayerController::ApplyMenuInput() {
    if(!IsLocalController()) return;auto* Client=LocalViewport();if(!Client) return;
    bGameplayInputApplied=false;bMenuInputApplied=true;bShowMouseCursor=true;
    if(auto* Own=Cast<UASGameViewportClient>(Client)) Own->SetGameplayMouseLock(false);
    Client->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);Client->SetMouseLockMode(EMouseLockMode::DoNotLock);Client->SetHideCursorDuringCapture(false);
    auto* Viewport=Client->Viewport;if(!Viewport||!Viewport->IsForegroundWindow()) return;
    FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    if(Menu.IsValid()) Mode.SetWidgetToFocus(Menu);
    SetInputMode(Mode);
    Viewport->CaptureMouse(false);Viewport->LockMouseToViewport(false);
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
```

PlayerTick checks the owning viewport's foreground/focus/capture state. While gameplay is active, it reapplies input only when needed and refreshes LockMouseToViewport(true) while captured, including after resize. On menu transitions it calls ApplyMenuInput once. OpenMenu/CloseMenu and EndPlay remove only their cached viewport's widgets. SetupInputComponent binds RelockMouse to ApplyGameplayInput.

## Exact viewport subclass

```cpp
#pragma once
#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "ASGameViewportClient.generated.h"
class AASPlayerController;

UCLASS() class ASHENSPIRAL_API UASGameViewportClient : public UGameViewportClient {
    GENERATED_BODY()
public:
    virtual void ReceivedFocus(FViewport* InViewport) override;
    virtual void LostFocus(FViewport* InViewport) override;
    virtual void Activated(FViewport* InViewport,const FWindowActivateEvent& Event) override;
    virtual void Deactivated(FViewport* InViewport,const FWindowActivateEvent& Event) override;
    virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
    virtual bool ShouldAlwaysLockMouse() override {return bGameplayMouseLock;}
    virtual bool LockDuringCapture() override {return bGameplayMouseLock;}
    void SetGameplayMouseLock(bool Enabled) {bGameplayMouseLock=Enabled;}
private:
    bool bGameplayMouseLock=false;
    AASPlayerController* LocalController() const;
};
```

```cpp
#include "ASGameViewportClient.h"
#include "ASPlayerController.h"
#include "Engine/World.h"
#include "InputKeyEventArgs.h"
#include "InputCoreTypes.h"
#include "UnrealClient.h"
#include "Slate/SceneViewport.h"

AASPlayerController* UASGameViewportClient::LocalController() const {
    if(auto* ViewportWorld=GetWorld()) for(auto It=ViewportWorld->GetPlayerControllerIterator();It;++It)
        if(auto* PC=Cast<AASPlayerController>(It->Get())) if(PC->IsLocalController()&&PC->LocalViewport()==this) return PC;
    return nullptr;
}
void UASGameViewportClient::ReceivedFocus(FViewport* InViewport) {
    Super::ReceivedFocus(InViewport);if(auto* PC=LocalController()) PC->QueueGameplayCapture();
}
void UASGameViewportClient::LostFocus(FViewport* InViewport) {
    Super::LostFocus(InViewport);if(auto* PC=LocalController()) PC->ViewportFocusLost();
}
void UASGameViewportClient::Activated(FViewport* InViewport,const FWindowActivateEvent& Event) {
    Super::Activated(InViewport,Event);if(auto* PC=LocalController()) PC->QueueGameplayCapture();
}
void UASGameViewportClient::Deactivated(FViewport* InViewport,const FWindowActivateEvent& Event) {
    Super::Deactivated(InViewport,Event);if(auto* PC=LocalController()) PC->ViewportFocusLost();
}
bool UASGameViewportClient::InputKey(const FInputKeyEventArgs& EventArgs) {
    if(EventArgs.Event==IE_Pressed&&EventArgs.Key==EKeys::LeftMouseButton)
        if(auto* PC=LocalController()) if(PC->WantsGameplayInput()&&EventArgs.Viewport&&!EventArgs.Viewport->HasMouseCapture()) {
            PC->ApplyGameplayInput();return true; // Refocus click captures without firing.
        }
    return Super::InputKey(EventArgs);
}
```

## Configuration

DefaultEngine.ini:

```ini
[/Script/Engine.Engine]
GameViewportClientClassName=/Script/AshenSpiral.ASGameViewportClient
```

DefaultInput.ini, under the existing input settings section:

```ini
bCaptureMouseOnLaunch=True
DefaultViewportMouseCaptureMode=CapturePermanently_IncludingInitialMouseDown
DefaultViewportMouseLockMode=LockAlways
+ActionMappings=(ActionName="RelockMouse",Key=F10)
```

DefaultEditorPerProjectUserSettings.ini:

```ini
[/Script/UnrealEd.LevelEditorPlaySettings]
GameGetsMouseControl=True
```

Restart the editor after compiling. The new viewport class is instantiated at startup; an old running viewport is not replaced by Live Coding.

## Required Windows/Unreal tests — not yet run

Run with separate profiles and repeat as host and P2 client. This code path is designed for PIE, standalone and packaged builds; it has not been verified in those environments here.

| Scenario | Expected result |
|---|---|
| Gameplay in windowed mode; move left/right/up/down repeatedly and click | Continuous look; cursor stays confined and no desktop clicks occur |
| Window on a display left of or below the primary display | Same result at negative/offset desktop coordinates |
| Different monitor DPI settings; resize/move the game window | Confinement follows the game client area |
| Esc menu, Tab build menu, reward and Settings menus | Cursor appears and can cross every edge to another monitor |
| Close each menu | Cursor hides and relative look resumes |
| Alt-Tab away, then return and click | Background game leaves the active app alone; returning game recaptures |
| F10 with gameplay focused | Capture/lock restored without opening a menu |
| Two PIE windows, then separate host/client processes | Only the active player's window takes the mouse; both HUDs display |
| PIE with Stop shortcut remapped | Esc opens the game menu; editor Stop still ends play |
| Packaged Development game, windowed and borderless | Same capture/release/relock behavior |
| Host pauses while client is playing | Client cursor releases; unpause restores capture when focused |
| Final victory / return to hub | Gameplay remains usable; capture follows menu state |

Run the console command ASInputStatus when diagnosing. It logs world, viewport class, local ownership, foreground, focus, capture, menu/gameplay intent and capture/lock modes. The class should be ASGameViewportClient. Opening the console can itself alter input focus, so interpret its capture snapshot in that context. Collect both runtime logs with Collect_Diagnostics.bat and note the display layout, Windows scaling, active player, game mode and exact failing transition.

## Official API references

- UGameViewportClient, including one viewport client per game instance and multiple PIE windows: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UGameViewportClient
- ULocalPlayer and per-player Slate operations: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ULocalPlayer
- FReply relative mouse input: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/SlateCore/FReply/UseHighPrecisionMouseMovement
- FReply capture/lock/release operations: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/SlateCore/FReply
- PIE startup setting: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Editor/UnrealEd/ULevelEditorPlaySettings

The references establish the API design; they do not establish that this game has passed the test matrix. Unreal compilation and Windows runtime testing remain pending.
