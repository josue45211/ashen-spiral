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
