#include "ASGameInstance.h"
#include "ASRules.h"
#include "ASExpeditionRules.h"
#include "ASGameState.h"
#include "ASPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
namespace {
bool ValidProgress(const UASSaveData* Save) {
    if(!Save||Save->Revision<0||Save->Data.Schema!=1||Save->Data.ProfileId.Len()!=32) return false;
    for(TCHAR C:Save->Data.ProfileId) if(!FChar::IsHexDigit(C)) return false;
    return true;
}
UASSaveData* ReadProgressFile(const FString& Path) {
    const int64 Size=IFileManager::Get().FileSize(*Path);
    if(Size<=0||Size>16*1024*1024) return nullptr;
    TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*Path)) return nullptr;
    auto* Save=Cast<UASSaveData>(UGameplayStatics::LoadGameFromMemory(Bytes));
    return ValidProgress(Save)?Save:nullptr;
}
bool WriteProgressFile(UASSaveData* Save,const FString& Path) {
    TArray<uint8> Bytes,Check;
    if(!Save||!UGameplayStatics::SaveGameToMemory(Save,Bytes)) return false;
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
    const FString Temp=Path+TEXT(".tmp");
    if(!FFileHelper::SaveArrayToFile(Bytes,*Temp)||!FFileHelper::LoadFileToArray(Check,*Temp)||Bytes!=Check) return false;
    auto* Verified=ReadProgressFile(Temp);
    if(!Verified||Verified->Revision!=Save->Revision||Verified->Data.ProfileId!=Save->Data.ProfileId) return false;
    if(!IFileManager::Get().Move(*Path,*Temp,true,true,false,true)) return false;
    return FFileHelper::LoadFileToArray(Check,*Path)&&Bytes==Check;
}
}
FString UASGameInstance::ProgressDirectory() const {
    return FPaths::Combine(FPlatformProcess::UserSettingsDir(),TEXT("AshenSpiral"),TEXT("Profiles"));
}
FString UASGameInstance::DefaultImportPath() const {return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Imports/SelectedProgress.sav")));}
void UASGameInstance::OpenProgressFolder() const {FPlatformProcess::ExploreFolder(*ProgressDirectory());}

void UASGameInstance::Init() {
    Super::Init();Slot=TEXT("AshenUE_Profile");
    int32 SlotNumber=0;
    if(FParse::Value(FCommandLine::Get(),TEXT("ProfileSlot="),SlotNumber)) Slot+=FString::Printf(TEXT("_%d"),FMath::Clamp(SlotNumber,0,99));
#if WITH_EDITOR
    else if(GetWorldContext() && GetWorldContext()->PIEInstance>=0) Slot+=FString::Printf(TEXT("_PIE%d"),GetWorldContext()->PIEInstance);
#endif
    BaseSlot=Slot;
    GConfig->GetInt(TEXT("AshenSpiral"),*(BaseSlot+TEXT("_Character")),ActiveCharacterSlot,GGameUserSettingsIni);
    ActiveCharacterSlot=FMath::Clamp(ActiveCharacterSlot,0,1);
    if(ActiveCharacterSlot==1) Slot+=TEXT("_Character2");
    LoadCharacter();
    GConfig->GetInt(TEXT("AshenSpiral"),TEXT("Quality"),Quality,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("AshenSpiral"),TEXT("Sensitivity"),Sensitivity,GGameUserSettingsIni);
    GConfig->GetBool(TEXT("AshenSpiral"),TEXT("RaiseStakes"),bRaiseStakes,GGameUserSettingsIni);
    GConfig->GetBool(TEXT("AshenSpiral"),TEXT("DamageNumbers"),bShowNumbers,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("AshenSpiral"),TEXT("NumberScale"),NumberScale,GGameUserSettingsIni);
    NumberScale=ASPresentation::NumberScale(NumberScale);
    GConfig->GetFloat(TEXT("AshenSpiral"),TEXT("CrosshairScale"),CrosshairScale,GGameUserSettingsIni);
    GConfig->GetInt(TEXT("AshenSpiral"),TEXT("CrosshairStyle"),CrosshairStyle,GGameUserSettingsIni);
    GConfig->GetInt(TEXT("AshenSpiral"),TEXT("CrosshairColor"),CrosshairColorIndex,GGameUserSettingsIni);
    CrosshairScale=ASPresentation::CrosshairScale(CrosshairScale);CrosshairStyle=FMath::Clamp(CrosshairStyle,0,3);CrosshairColorIndex=FMath::Clamp(CrosshairColorIndex,0,7);
    GConfig->GetFloat(TEXT("AshenSpiral"),TEXT("CameraDistance"),CameraDistance,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("AshenSpiral"),TEXT("CameraHeight"),CameraHeight,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("AshenSpiral"),TEXT("CameraShoulder"),CameraShoulder,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("AshenSpiral"),TEXT("CameraPitch"),CameraPitch,GGameUserSettingsIni);
    GConfig->GetBool(TEXT("AshenSpiral"),TEXT("ReduceMotion"),bReduceMotion,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("AshenSpiral"),TEXT("CameraFOV"),CameraFOV,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("AshenSpiral"),TEXT("HUDTextScale"),HUDTextScale,GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("AshenSpiral"),TEXT("HUDScale"),HUDScale,GGameUserSettingsIni);
    HUDScale=ASPresentation::HUDScale(HUDScale);
    SanitizeCameraSettings();HUDTextScale=FMath::Clamp(HUDTextScale,1.f,1.2f);
    Sensitivity=FMath::Clamp(Sensitivity,.2f,2.f);Quality=FMath::Clamp(Quality,0,3);
    if(GEngine) FailureHandle=GEngine->OnNetworkFailure().AddUObject(this,&UASGameInstance::NetworkFailure);
}
void UASGameInstance::Shutdown() {
    if(GEngine) GEngine->OnNetworkFailure().Remove(FailureHandle);
    GConfig->SetInt(TEXT("AshenSpiral"),TEXT("Quality"),Quality,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("AshenSpiral"),TEXT("Sensitivity"),Sensitivity,GGameUserSettingsIni);
    GConfig->SetBool(TEXT("AshenSpiral"),TEXT("DamageNumbers"),bShowNumbers,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("AshenSpiral"),TEXT("NumberScale"),NumberScale,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("AshenSpiral"),TEXT("HUDTextScale"),HUDTextScale,GGameUserSettingsIni);
    SaveHUDSettings();SaveCameraSettings();Super::Shutdown();
}
void UASGameInstance::SaveHUDSettings() {
    GConfig->SetBool(TEXT("AshenSpiral"),TEXT("RaiseStakes"),bRaiseStakes,GGameUserSettingsIni);
    HUDScale=ASPresentation::HUDScale(HUDScale);
    GConfig->SetFloat(TEXT("AshenSpiral"),TEXT("HUDScale"),HUDScale,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("AshenSpiral"),TEXT("HUDTextScale"),HUDTextScale,GGameUserSettingsIni);
    NumberScale=ASPresentation::NumberScale(NumberScale);CrosshairScale=ASPresentation::CrosshairScale(CrosshairScale);
    CrosshairStyle=FMath::Clamp(CrosshairStyle,0,3);CrosshairColorIndex=FMath::Clamp(CrosshairColorIndex,0,7);
    GConfig->SetFloat(TEXT("AshenSpiral"),TEXT("CrosshairScale"),CrosshairScale,GGameUserSettingsIni);
    GConfig->SetInt(TEXT("AshenSpiral"),TEXT("CrosshairStyle"),CrosshairStyle,GGameUserSettingsIni);
    GConfig->SetInt(TEXT("AshenSpiral"),TEXT("CrosshairColor"),CrosshairColorIndex,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("AshenSpiral"),TEXT("NumberScale"),NumberScale,GGameUserSettingsIni);
    GConfig->SetBool(TEXT("AshenSpiral"),TEXT("DamageNumbers"),bShowNumbers,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}
void UASGameInstance::SanitizeCameraSettings() {
    const auto Safe=[](float V,float Default,float Min,float Max) {return FMath::IsFinite(V)?FMath::Clamp(V,Min,Max):Default;};
    CameraDistance=Safe(CameraDistance,528.f,360.f,780.f);
    CameraHeight=Safe(CameraHeight,100.f,20.f,200.f);
    CameraShoulder=Safe(CameraShoulder,155.f,-220.f,220.f);
    CameraPitch=Safe(CameraPitch,-8.f,-30.f,20.f);
    CameraFOV=Safe(CameraFOV,85.f,65.f,105.f);
}
void UASGameInstance::SaveCameraSettings() {
    SanitizeCameraSettings();
    GConfig->SetBool(TEXT("AshenSpiral"),TEXT("ReduceMotion"),bReduceMotion,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("AshenSpiral"),TEXT("CameraDistance"),CameraDistance,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("AshenSpiral"),TEXT("CameraHeight"),CameraHeight,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("AshenSpiral"),TEXT("CameraShoulder"),CameraShoulder,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("AshenSpiral"),TEXT("CameraPitch"),CameraPitch,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("AshenSpiral"),TEXT("CameraFOV"),CameraFOV,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}
void UASGameInstance::ResetCameraSettings() {
    CameraDistance=528.f;CameraHeight=100.f;CameraShoulder=155.f;CameraPitch=-8.f;CameraFOV=85.f;
    SaveCameraSettings();
}
bool UASGameInstance::Commit(const FASProfileData& Candidate) {
    if(bProgressBlocked) {LastMessage=TEXT("Restore a valid progress backup before saving; unreadable existing files are being preserved.");return false;}
    for(const TCHAR* Suffix:{TEXT("_A.sav"),TEXT("_B.sav")}) {
        if(auto* Disk=ReadProgressFile(FPaths::Combine(ProgressDirectory(),Slot+Suffix))) if(Disk->Revision>Revision) {
            LastMessage=TEXT("Another game instance saved this profile. Close other copies and restart before making more purchases.");return false;
        }
    }
    if(Revision>=MAX_int32-1) {LastMessage=TEXT("Profile revision limit reached. Export progress before continuing.");return false;}
    auto* Save=Cast<UASSaveData>(UGameplayStatics::CreateSaveGameObject(UASSaveData::StaticClass()));
    if(!Save) return false;
    Save->Data=Candidate;Save->Data.Sanitize();Save->Revision=Revision+1;
    const FString Suffix=Save->Revision%2?TEXT("_A"):TEXT("_B");
    const FString Target=FPaths::Combine(ProgressDirectory(),Slot+Suffix+TEXT(".sav"));
    if(!WriteProgressFile(Save,Target)) {LastMessage=TEXT("Progress save failed. No currency or purchase was applied.");return false;}
    Profile=Save->Data;Revision=Save->Revision;
    // Keep a project-local recovery copy for older workflows; the shared user file
    // is the primary copy and was already read back and verified above.
    UGameplayStatics::SaveGameToSlot(Save,Slot+Suffix,0);
    return true;
}
bool UASGameInstance::ExportProgress() {
    auto* Save=Cast<UASSaveData>(UGameplayStatics::CreateSaveGameObject(UASSaveData::StaticClass()));
    if(!Save) return false;Save->Data=Profile;Save->Revision=Revision;
    const FString Name=Slot+TEXT("_")+FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(6)+TEXT(".ashenprofile");
    LastExportPath=FPaths::Combine(ProgressDirectory(),TEXT("Exports"),Name);
    if(!WriteProgressFile(Save,LastExportPath)) {LastMessage=TEXT("Could not create a verified progress export.");LastExportPath.Empty();return false;}
    LastMessage=TEXT("Export created. Open the progress folder to copy it to another PC or keep a backup.");return true;
}
bool UASGameInstance::PreviewImport(const FString& Path) {
    bImportReady=false;FString Clean=Path.TrimStartAndEnd();Clean.RemoveFromStart(TEXT("\""));Clean.RemoveFromEnd(TEXT("\""));
    auto* Save=ReadProgressFile(Clean);
    if(!Save) {LastMessage=TEXT("That file is not a supported Ashen Spiral Unreal save/export, or it is damaged. Select a .sav or .ashenprofile file.");return false;}
    FString Companion;
    if(Clean.EndsWith(TEXT("_A.sav"))) Companion=Clean.LeftChop(6)+TEXT("_B.sav");
    else if(Clean.EndsWith(TEXT("_B.sav"))) Companion=Clean.LeftChop(6)+TEXT("_A.sav");
    else if(Clean==DefaultImportPath()) Companion=FPaths::Combine(FPaths::GetPath(Clean),TEXT("SelectedProgress_Companion.sav"));
    if(!Companion.IsEmpty()) if(auto* Other=ReadProgressFile(Companion)) if(Other->Data.ProfileId==Save->Data.ProfileId&&Other->Revision>Save->Revision) {Save=Other;Clean=Companion;}
    const FString OtherSlot=BaseSlot+(ActiveCharacterSlot==0?TEXT("_Character2"):TEXT(""));
    for(const TCHAR* Suffix:{TEXT("_A.sav"),TEXT("_B.sav")}) if(auto* Other=ReadProgressFile(FPaths::Combine(ProgressDirectory(),OtherSlot+Suffix))) if(Other->Data.ProfileId==Save->Data.ProfileId) {LastMessage=TEXT("This export belongs to your other character. Switch to that character before restoring it.");return false;}
    ImportSourcePath=Clean;ImportPreview=Save->Data;ImportPreview.Sanitize();bImportReady=true;LastMessage=TEXT("Review the progress to restore. A backup of your current profile will be created first.");return true;
}
bool UASGameInstance::RestoreImport() {
    if(!bImportReady) return false;
    if(!ExportProgress()) return false; // Do not replace current progress without a verified backup.
    if(bProgressBlocked) {
        const FString Recovery=FPaths::Combine(ProgressDirectory(),TEXT("Recovery_"))+FGuid::NewGuid().ToString(EGuidFormats::Digits);
        if(!IFileManager::Get().MakeDirectory(*Recovery,true)) {LastMessage=TEXT("Could not preserve unreadable save files before restore.");return false;}
        for(const TCHAR* Suffix:{TEXT("_A.sav"),TEXT("_B.sav")}) {
            const FString Old=FPaths::Combine(ProgressDirectory(),Slot+Suffix);
            if(IFileManager::Get().FileExists(*Old)&&IFileManager::Get().Copy(*FPaths::Combine(Recovery,Slot+Suffix),*Old)!=COPY_OK) {LastMessage=TEXT("Could not back up an unreadable save. Restore cancelled.");return false;}
        }
        for(const TCHAR* Suffix:{TEXT("_A.sav"),TEXT("_B.sav")}) {
            const FString Old=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("SaveGames"),Slot+Suffix);
            if(IFileManager::Get().FileExists(*Old)&&IFileManager::Get().Copy(*FPaths::Combine(Recovery,TEXT("Project_")+Slot+Suffix),*Old)!=COPY_OK) {LastMessage=TEXT("Could not back up a project save. Restore cancelled.");return false;}
        }
    }
    const bool WasBlocked=bProgressBlocked;bProgressBlocked=false;
    if(!Commit(ImportPreview)) {bProgressBlocked=WasBlocked;return false;}
    bImportReady=false;LastMessage=TEXT("Progress restored and saved for future versions.");return true;
}
bool UASGameInstance::Purchase(int32 Category,int32 I) {
    FASProfileData C=Profile;bool Valid=false;LastMessage=TEXT("Not enough currency, locked, or already owned.");
    if(Category==0 && I>=0 && I<ASRules::WeaponCount) {
        Valid=(C.OwnedWeapons&(1<<I)) || ASRules::Spend(C.Ash,ASData::Weapons[I].Cost);
        if(Valid) {C.OwnedWeapons|=1<<I;C.Build.Weapon=I;}
    } else if(Category==1 && I>=0 && I<C.Build.StylePool) {C.Build.Style=I;Valid=true;}
    else if(Category==8 && I>=0 && I<ASAdventure::AbilityCount) {C.Build.Ability=I;Valid=true;}
    else if(Category==2 && I>=0 && I<ASRules::HearthCount) {
        Valid=ASRules::Spend(C.Ash,ASRules::Cost(ASData::Hearth[I].Cost,C.Build.Ranks[I],ASData::Hearth[I].MaxRank));
        if(Valid) ++C.Build.Ranks[I];
    } else if(Category==3 && C.Build.RelicPool<24 && ASRules::Spend(C.RelicEmbers,4)) {C.Build.RelicPool=FMath::Min(24,C.Build.RelicPool+4);Valid=true;}
    else if(Category==4 && C.Build.StylePool<10 && ASRules::Spend(C.Tablets,1)) {++C.Build.StylePool;Valid=true;}
    else if(Category==5) Valid=ASRules::Refinement(C.Materials[0],C.Materials[1],C.Ash,C.bRefinementBought);
    else if(Category==6) Valid=ASRules::Memory(C.Materials[4],C.Materials[1],C.SkillPoints);
    else if(Category==7 && I>=0 && I<12 && I!=9 && !C.Build.Talent(I)) {
        int32 Spent=0;for(int32 J=0;J<12;++J) if(C.Build.Talent(J) && ASData::Talents[J].Branch==ASData::Talents[I].Branch) Spent+=ASData::Talents[J].Cost;
        if(Spent>=ASData::Talents[I].Required && ASRules::Spend(C.SkillPoints,ASData::Talents[I].Cost)) {C.Build.TalentMask|=1<<I;Valid=true;}
    }
    if(Category==9&&I==0&&C.EndlessChoices<2&&ASRules::Spend(C.Ash,ASExpedition::ChoiceCost(C.EndlessChoices))) {++C.EndlessChoices;Valid=true;}
    if(!Valid || !Commit(C)) return false;
    LastMessage=TEXT("Saved. Your next expedition will use this loadout.");return true;
}
bool UASGameInstance::Receive(const FASReceipt& R) {
    if(R.ProfileId!=Profile.ProfileId||R.Id.IsEmpty() || R.Id.Len()>128 || R.Room<0 || R.Room>32) return false;
    if(Profile.Receipts.Contains(R.Id)) return true;
    FASProfileData C=Profile;
    if(R.AshCost<0||R.ScrapCost<0||R.AshCost>C.Ash||R.ScrapCost>C.Materials[0]) {LastMessage=TEXT("Receipt funds do not match this character. No change saved.");return false;}
    C.Ash-=R.AshCost;C.Materials[0]-=R.ScrapCost;
    C.Materials[0]=ASRules::Credit(C.Materials[0],FMath::Clamp(R.Scrap,0,10000));
    if(R.bEndless) {
        C.BestEndlessWave=FMath::Max(C.BestEndlessWave,FMath::Clamp(R.EndlessWave,0,10000));
        if(R.bRunEnded&&!R.EndlessRecord.RunId.IsEmpty()) {
            C.EndlessRecords.RemoveAll([&R](const FASEndlessRecord& Entry){return Entry.RunId==R.EndlessRecord.RunId;});
            C.EndlessRecords.Add(R.EndlessRecord);
        }
    }
    C.Ash=ASRules::Credit(C.Ash,FMath::Clamp(R.Ash,0,10000));
    C.RelicEmbers=ASRules::Credit(C.RelicEmbers,FMath::Clamp(R.Embers,0,10));
    C.Tablets=ASRules::Credit(C.Tablets,FMath::Clamp(R.Tablets,0,10));
    C.SkillPoints=ASRules::Credit(C.SkillPoints,FMath::Clamp(R.Points,0,10));
    const int32 W=ASRules::MaterialForWard(FMath::Clamp(R.Ward,0,3));
    C.Materials[W]=ASRules::Credit(C.Materials[W],FMath::Clamp(R.Material,0,100));
    C.Materials[4]=ASRules::Credit(C.Materials[4],FMath::Clamp(R.Cores,0,10));
    C.BestRoom=FMath::Max(C.BestRoom,R.Room);
    if(R.bWin) {++C.Wins;if(C.Build.Talent(11)) {C.Materials[0]=ASRules::Credit(C.Materials[0],30);C.Materials[1]=ASRules::Credit(C.Materials[1],10);}}
    if(R.bDeath) ++C.Deaths;
    C.Experience=ASCareer::Add(C.Experience,FMath::Clamp(R.Experience,0,100000));C.Career.SetNumZeroed(ASCareer::Count);
    FString Earned;
    for(int32 I=0;I<ASCareer::Count;++I) {
        const int32 Before=ASCareer::Tier(C.Career[I],I);C.Career[I]=ASCareer::Add(C.Career[I],R.Career.IsValidIndex(I)?FMath::Clamp(R.Career[I],0,10000):0);
        const int32 After=ASCareer::Tier(C.Career[I],I);if(After>Before) Earned+=FString::Printf(TEXT("%s %d  "),UTF8_TO_TCHAR(ASCareer::Achievements[I].Name),After);
    }
    const int32 OldLevel=ASCareer::Level(Profile.Experience);C.Receipts.Add(R.Id);if(!Commit(C)) return false;
    if(!Earned.IsEmpty()) LastMessage=TEXT("Achievement unlocked: ")+Earned;
    else if(ASCareer::Level(C.Experience)>OldLevel) LastMessage=FString::Printf(TEXT("Character level %d reached! Milestone rewards are saved."),ASCareer::Level(C.Experience));
    else LastMessage=TEXT("Progress saved.");return true;
}
void UASGameInstance::SetQuality(int32 Preset) {
    Quality=FMath::Clamp(Preset,0,3);
    if(auto* Settings=UGameUserSettings::GetGameUserSettings()) {
        Settings->SetOverallScalabilityLevel(Quality);Settings->SetFrameRateLimit(120.f);
        Settings->SetResolutionScaleValueEx(Quality==0?65.f:(Quality==1?80.f:100.f));
        // Quality changes must not resize/fullscreen the editor's PIE viewport or
        // override the standalone window's launch dimensions during BeginPlay.
        Settings->ApplyNonResolutionSettings();Settings->SaveSettings();
    }
}
void UASGameInstance::SetDisplayMode(int32 Mode) {
    if(!GetWorld()||GetWorld()->WorldType==EWorldType::PIE) {LastMessage=TEXT("Borderless fullscreen is available in Standalone Game or a packaged build. Use the editor viewport's immersive mode for viewport play.");return;}
    if(auto* Settings=UGameUserSettings::GetGameUserSettings()) {
        const bool Borderless=Mode!=0;Settings->SetFullscreenMode(Borderless?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);
        Settings->SetScreenResolution(Borderless?Settings->GetDesktopResolution():FIntPoint(1280,720));
        Settings->ApplySettings(false);Settings->ConfirmVideoMode();Settings->SaveSettings();
        LastMessage=Borderless?TEXT("Borderless fullscreen applied at desktop resolution."):TEXT("1280 x 720 window applied.");
    }
}
void UASGameInstance::NetworkFailure(UWorld* World,UNetDriver* Driver,ENetworkFailure::Type Type,const FString& Error) {
    LastMessage=TEXT("Connection ended: ")+Error;
    if(World && World->GetNetMode()==NM_Client) {
        World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,[this]() {
            UGameplayStatics::OpenLevel(this,FName(TEXT("/Game/Maps/AshenSpiral")));
        }));
    }
}

bool UASGameInstance::LoadCharacter() {
    Profile=FASProfileData();Revision=0;bProgressBlocked=false;
    bool Loaded=false,FoundStored=false,FoundLegacy=false,LegacyLoaded=false;
    // This folder is independent of the extracted project/version directory.
    for(const TCHAR* Suffix:{TEXT("_A.sav"),TEXT("_B.sav")}) {
        FoundStored|=IFileManager::Get().FileExists(*FPaths::Combine(ProgressDirectory(),Slot+Suffix));
        if(auto* Save=ReadProgressFile(FPaths::Combine(ProgressDirectory(),Slot+Suffix))) if(!Loaded||Save->Revision>Revision) {
            Profile=Save->Data;Revision=Save->Revision;Loaded=true;
        }
    }
    if(!Loaded) {
        // First-use migration when old SaveGames were copied alongside this build.
        for(const TCHAR* Suffix:{TEXT("_A"),TEXT("_B")}) {
            FoundLegacy|=UGameplayStatics::DoesSaveGameExist(Slot+Suffix,0);
            auto* Save=Cast<UASSaveData>(UGameplayStatics::LoadGameFromSlot(Slot+Suffix,0));
            if(ValidProgress(Save)&&Save->Revision>=Revision) {
                Profile=Save->Data;Revision=Save->Revision;LegacyLoaded=true;
            }
        }
        Profile.Sanitize();
        if((FoundStored||FoundLegacy)&&!LegacyLoaded) {bProgressBlocked=true;LastMessage=TEXT("Saved progress could not be read. Existing files have been preserved. Restore a backup from the Progress page.");}
        else if(!Commit(Profile)) {bProgressBlocked=true;LastMessage=TEXT("A new character save could not be created. Check that the progress folder is writable.");}
    } else {
        FASProfileData Migrated=Profile;Migrated.Sanitize();
        if(Profile.MigrationVersion<20) {if(!Commit(Migrated)) bProgressBlocked=true;}
        else Profile=Migrated;
    }
    return !bProgressBlocked;
}
bool UASGameInstance::SwitchCharacterSlot(int32 Index) {
    auto* State=AS::State(this);
    if(Index<0||Index>1||!State||State->Phase!=EASPhase::Hub) {LastMessage=TEXT("Switch characters in the hub.");return false;}
    for(auto P:State->PlayerArray) if(auto* Player=Cast<AASPlayerState>(P)) if(Player->bSavePending) {LastMessage=TEXT("Wait for pending saves before switching.");return false;}
    if(Index==ActiveCharacterSlot) return true;
    const auto OldProfile=Profile;const auto OldSlot=Slot;const int32 OldRevision=Revision;const bool OldBlocked=bProgressBlocked;
    Slot=BaseSlot+(Index==1?TEXT("_Character2"):TEXT(""));
    if(!LoadCharacter()) {const auto Error=LastMessage;Profile=OldProfile;Slot=OldSlot;Revision=OldRevision;bProgressBlocked=OldBlocked;LastMessage=Error;return false;}
    ActiveCharacterSlot=Index;bImportReady=false;ImportSourcePath.Empty();
    GConfig->SetInt(TEXT("AshenSpiral"),*(BaseSlot+TEXT("_Character")),Index,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);
    LastMessage=FString::Printf(TEXT("Character %d loaded. Progress is independent."),Index+1);return true;
}
