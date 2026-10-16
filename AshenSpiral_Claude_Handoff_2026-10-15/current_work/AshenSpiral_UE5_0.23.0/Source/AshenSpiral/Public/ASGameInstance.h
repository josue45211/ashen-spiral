#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "ASData.h"
#include "ASGameInstance.generated.h"
UCLASS() class ASHENSPIRAL_API UASSaveData : public USaveGame {
    GENERATED_BODY()
public:
    UPROPERTY() int32 Revision=0;
    UPROPERTY() FASProfileData Data;
};
UCLASS() class ASHENSPIRAL_API UASGameInstance : public UGameInstance {
    GENERATED_BODY()
public:
    virtual void Init() override;
    virtual void Shutdown() override;
    UPROPERTY() FASProfileData Profile;
    FString LastMessage;
    int32 ActiveCharacterSlot=0;
    bool SwitchCharacterSlot(int32 Index);
    bool Commit(const FASProfileData& Candidate);
    bool Purchase(int32 Category,int32 Index);
    bool Receive(const FASReceipt& Receipt);
    FString ProgressDirectory() const;
    FString DefaultImportPath() const;
    bool ExportProgress();
    bool PreviewImport(const FString& Path);
    bool RestoreImport();
    bool bImportReady=false;
    UPROPERTY() FASProfileData ImportPreview;
    FString LastExportPath,ImportSourcePath;
    void OpenProgressFolder() const;
    void SetQuality(int32 Preset);
    void SetDisplayMode(int32 Mode);
    void SanitizeCameraSettings();
    void SaveCameraSettings();
    void ResetCameraSettings();
    void SaveHUDSettings();
    int32 Quality=2;
    float Sensitivity=.85f;
    float NumberScale=1.2f;
    float CrosshairScale=1.f;
    int32 CrosshairStyle=0,CrosshairColorIndex=0;
    float CameraDistance=528.f;
    float CameraHeight=100.f;
    float CameraShoulder=155.f;
    float CameraPitch=-8.f;
    float CameraFOV=85.f;
    float HUDTextScale=1.f;
    float HUDScale=1.f;
    bool bShowNumbers=true;
    bool bReduceMotion=false;
    bool bRaiseStakes=false;
private:
    FString Slot,BaseSlot;
    bool LoadCharacter();
    int32 Revision=0;
    bool bProgressBlocked=false;
    FDelegateHandle FailureHandle;
    void NetworkFailure(UWorld* World,UNetDriver* Driver,ENetworkFailure::Type Type,const FString& Error);
};
