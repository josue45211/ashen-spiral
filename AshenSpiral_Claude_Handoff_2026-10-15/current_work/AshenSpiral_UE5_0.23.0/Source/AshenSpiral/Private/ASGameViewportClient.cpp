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
