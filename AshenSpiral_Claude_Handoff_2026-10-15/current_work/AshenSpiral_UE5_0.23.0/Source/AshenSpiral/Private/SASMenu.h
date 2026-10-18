#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
class AASPlayerController;class UASGameInstance;class SScrollBox;class SVerticalBox;
class SASMenu : public SCompoundWidget {
public:
    SLATE_BEGIN_ARGS(SASMenu) {} SLATE_ARGUMENT(TWeakObjectPtr<AASPlayerController>,Controller) SLATE_ARGUMENT(int32,Page) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override {return true;}
    virtual FReply OnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    virtual FReply OnKeyUp(const FGeometry& Geometry,const FKeyEvent& Event) override;
    float ScrollOffset() const;void SetScrollOffset(float Value);
    int32 Page=0;
private:
    TWeakObjectPtr<AASPlayerController> PC;
    TSharedPtr<SScrollBox> Scroll;
    TSharedPtr<SVerticalBox> Body;
    void Text(const FString& Value,int32 Size=16,FLinearColor Color=FLinearColor(.7f,.76f,.8f));
    void Card(const FString& Title,const FString& Description,const FString& Button,int32 Code,int32 Index=0,bool Enabled=true);
    TSharedRef<SWidget> Button(const FString& Title,int32 Code,int32 Index=0);
    FString HoverDetails;
    void RewardScreen();
    TSharedRef<SWidget> ChoiceTile(const FString& Title,const FString& Brief,const FString& Detail,const FString& Type,int32 Icon,int32 Rarity,const FString& Label,int32 Code,int32 Index,bool Enabled=true,float TileWidth=200.f);
    TSharedRef<SWidget> DetailStrip();
    void Input(const FString& Label,int32 Field);
    void KeyRow(const FString& Label,FName Binding);
    void CameraSlider(const FString& Label,float UASGameInstance::* Setting,float Min,float Max,float Step,const FString& Unit);
};
