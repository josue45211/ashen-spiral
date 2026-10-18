#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ASHUD.generated.h"
class UGameViewportClient;class AASGameState;class AASCharacter;class SWidget;class AASRouteGuide;
struct FASHUDLabel {FString Value;FVector2D Position;FLinearColor Color;int32 Pixels=16;};
UCLASS() class ASHENSPIRAL_API AASHUD : public AHUD {
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void DrawHUD() override;
    TArray<FASHUDLabel> Labels;
    FVector2D OutputSize=FVector2D(1920,1080);
private:
    TSharedPtr<SWidget> TextLayer;
    TWeakObjectPtr<UGameViewportClient> TextViewport;
    UPROPERTY() TObjectPtr<AASRouteGuide> RouteGuide;
    float Scale=1.f,Width=1920,Height=1080,SmoothedDelta=.0167f,TrailingHealth=1.f;
    float GroupScale=1.f;
    // Portal walk-in fade (run start) and sky-stage flash, both client-side and cosmetic.
    float TransitionStart=-10.f,StageFlashStart=-10.f;int32 LastHudPhase=-1,LastHudStage=0,TransitionTier=0;bool bTransitionEndless=false;
    // Wave banner (cosmetic, driven by replicated wave/phase changes).
    float BannerStart=-10.f;FString BannerTitle,BannerSub;FLinearColor BannerColor=FLinearColor::White;
    int32 BannerRoom=-99,BannerWave=-99,BannerEndlessWave=-99,BannerCleared=-99,BannerPhase=-1;
    int32 BannerAlertSeen=0;
    void DrawWaveBanner(AASGameState* State);
    FVector2D GroupOrigin=FVector2D::ZeroVector;
    void Group(float X=0,float Y=0,float Size=1.f) {GroupOrigin=FVector2D(X,Y);GroupScale=Size;}
    void Text(const FString& Value,float X,float Y,FLinearColor Color=FLinearColor::White,float Size=1.f);
    void Box(float X,float Y,float W,float H,FLinearColor Color);
    void Line(float X,float Y,float X2,float Y2,FLinearColor Color,float Thickness=1.f);
    void Bar(float X,float Y,float W,float H,float Fraction,FLinearColor Color);
    void Map(AASGameState* State,AASCharacter* Hero);
};
