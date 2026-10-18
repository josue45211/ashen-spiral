#include "SASMenu.h"
#include "SASEffectIcon.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Framework/Application/SlateApplication.h"
#include "ASPlayerController.h"
#include "ASGameInstance.h"
#include "ASGameState.h"
#include "ASCharacter.h"
#include "ASRules.h"
#include "ASPresentationRules.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SInputKeySelector.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"

namespace {
const FLinearColor ASMenuGold(.98f,.64f,.25f),ASMenuMuted(.67f,.73f,.79f),ASMenuInk(.018f,.024f,.035f,.97f);
// Fixed design-space widths are known before Slate's prepass. AutoWrapText
// inside a ScaleBox can report an unwrapped desired size on its first frame.
// Leave room for the scroll bar, border padding, and button style padding.
constexpr float PanelWidth=880.f,PanelHeight=680.f;
constexpr float BodyTextWidth=760.f,CardTextWidth=520.f,HeaderTextWidth=570.f;
constexpr float ButtonWidth=196.f,ButtonTextWidth=152.f;
FSlateFontInfo Font(int32 Size,bool Bold=false) {return FCoreStyle::GetDefaultFontStyle(Bold?"Bold":"Regular",FMath::Max(14,Size));}
FText Txt(const FString& Value) {return FText::FromString(Value);}
int32 PermanentStatIndex(int32 Stat) {const int32 Indices[]={14,16,17,0,18,19};return Indices[FMath::Clamp(Stat,0,5)];}
FString StatValue(int32 Stat,const FASBuild& Build,const TArray<int32>& Ranks) {
    const auto Run=[&Ranks](int32 I){return Ranks.IsValidIndex(I)?ASRules::UpgradeRank(Ranks[I]):0;};
    switch(Stat) {
    case 0:return FString::Printf(TEXT("%.2f s"),ASRules::SpiralCooldown(Build.Rank(14),Run(0)));
    case 1:return FString::Printf(TEXT("+%.0f%% (%.2f %s)"),(ASRules::AttackSpeed(Build.Rank(16),Run(1))-1.f)*100.f,
        1.f/(ASRules::AttackInterval(ASData::Weapons[FMath::Clamp(Build.Weapon,0,ASRules::WeaponCount-1)].Rate,Build.Rank(16),Run(1))*ASMelee::AverageTimeScale(Build.Weapon)),ASMelee::IsMelee(Build.Weapon)?TEXT("swings/s"):TEXT("attacks/s"));
    case 2:return FString::Printf(TEXT("+%.0f%%"),(ASRules::DamageMultiplier(Build.Rank(17),Run(2))-1.f)*100.f);
    case 3:return FString::Printf(TEXT("%.0f HP"),ASRules::StartingHealth(Build.Rank(0),Run(3)));
    case 4:return FString::Printf(TEXT("%.0f cm/s"),ASRules::MoveSpeed(Build.Rank(18),Run(4)));
    default:return FString::Printf(TEXT("%.0f%% chance"),ASRules::CriticalChance(Build.Rank(19),Run(5))*100.f);
    }
}
FString StatPreview(int32 Stat,const FASBuild& Build,const TArray<int32>& Run,bool Permanent,float ActualHealth=-1.f) {
    FASBuild Next=Build;TArray<int32> NextRun=Run;NextRun.SetNumZeroed(ASRules::RunUpgradeCount);
    const int32 I=PermanentStatIndex(Stat);
    const bool Maxed=Permanent?Next.Rank(I)>=ASData::Hearth[I].MaxRank:!ASRules::CanUpgradeRunStat(Stat,NextRun[Stat],Build.Rank(I));
    if(Stat==3&&!Permanent&&ActualHealth>0) return Maxed?FString::Printf(TEXT("Current: %.0f HP. Maximum run rank reached."),ActualHealth):FString::Printf(TEXT("Current: %.0f HP -> Next: %.0f HP (includes Blood Pact)."),ActualHealth,ActualHealth+15.f);
    if(Maxed) return FString::Printf(TEXT("Current: %s. Maximum rank or effect reached."),*StatValue(Stat,Build,Run));
    if(Permanent) {Next.Sanitize();++Next.Ranks[I];}else ++NextRun[Stat];
    return FString::Printf(TEXT("Current: %s  ->  Next: %s"),*StatValue(Stat,Build,Run),*StatValue(Stat,Next,NextRun));
}
FString BuildSummary(const FASBuild& Build,const TArray<int32>& Run,const AASCharacter* Hero=nullptr) {
    return FString::Printf(TEXT("SPIRAL %s  |  ATTACK SPEED %s  |  DAMAGE %s\nHEALTH %s  |  MOVE %s  |  CRITICAL %s"),
        *StatValue(0,Build,Run),*StatValue(1,Build,Run),*StatValue(2,Build,Run),*(Hero?FString::Printf(TEXT("%.0f HP"),Hero->MaxHealth):StatValue(3,Build,Run)),*StatValue(4,Build,Run),*StatValue(5,Build,Run));
}
}
float SASMenu::ScrollOffset() const {return Scroll.IsValid()?Scroll->GetScrollOffset():0.f;}
void SASMenu::SetScrollOffset(float Value) {if(Scroll.IsValid()) Scroll->SetScrollOffset(Value);}
FReply SASMenu::OnKeyUp(const FGeometry& G,const FKeyEvent& Event) {
    // Hold-to-view: releasing TAB (or the controller's build button) closes the build panel.
    if(Page==17&&PC.IsValid()&&(Event.GetKey()==PC->BoundKey("Build")||Event.GetKey()==EKeys::Gamepad_Special_Left)) {
        const auto* GI=PC->GetGameInstance<UASGameInstance>();
        if(GI&&GI->bHoldBuild) {PC->Action(0);return FReply::Handled();}
    }
    return SCompoundWidget::OnKeyUp(G,Event);
}
FReply SASMenu::OnKeyDown(const FGeometry& G,const FKeyEvent& Event) {
    if(Page==17&&PC.IsValid()&&(Event.GetKey()==PC->BoundKey("Build")||Event.GetKey()==EKeys::Gamepad_Special_Left)) {
        const auto* GI=PC->GetGameInstance<UASGameInstance>();
        if(GI&&GI->bHoldBuild) return FReply::Handled(); // held key repeating: the panel stays open until release
    }
    if((Page==17&&PC.IsValid()&&(Event.GetKey()==PC->BoundKey("Build")||Event.GetKey()==EKeys::Gamepad_Special_Left))||Event.GetKey()==EKeys::Escape||Event.GetKey()==EKeys::Gamepad_Special_Right||Event.GetKey()==EKeys::Gamepad_FaceButton_Right) {
        if(auto* C=PC.Get()) C->Action(0);return FReply::Handled();
    }
    return SCompoundWidget::OnKeyDown(G,Event);
}
namespace {
// Per-button animation state shared by the hover/press lambdas and the render-transform attribute.
struct FASButtonJuice {bool Hover=false,Press=false;float From=1.f,To=1.f;double Changed=-10.0;};
float ButtonJuiceScale(const FASButtonJuice& J) {
    return ASPresentation::ButtonScaleAt(J.From,J.To,float(FSlateApplication::Get().GetCurrentTime()-J.Changed));
}
void ButtonJuiceSet(FASButtonJuice& J,bool Hover,bool Press) {
    J.From=ButtonJuiceScale(J);J.Hover=Hover;J.Press=Press;J.To=ASPresentation::ButtonScaleTarget(Hover,Press,true);J.Changed=FSlateApplication::Get().GetCurrentTime();
}
}
TSharedRef<SWidget> SASMenu::Button(const FString& Title,int32 Code,int32 Index) {
    const auto Weak=PC;
    const TSharedRef<FASButtonJuice> Juice=MakeShared<FASButtonJuice>();
    return SNew(SButton).ContentPadding(FMargin(14,10)).HAlign(HAlign_Center).VAlign(VAlign_Center)
        .ButtonColorAndOpacity(FLinearColor(.055f,.16f,.18f)).ForegroundColor(FLinearColor::White)
        .RenderTransformPivot(FVector2D(.5f,.5f))
        .RenderTransform_Lambda([Juice](){const float Scale=ButtonJuiceScale(*Juice);return TOptional<FSlateRenderTransform>(FSlateRenderTransform(FScale2D(Scale)));})
        .OnHovered_Lambda([Weak,Juice](){
            ButtonJuiceSet(*Juice,true,Juice->Press);
            // Placeholder hover tick: quiet, high-pitched existing sound.
            if(auto* C=Weak.Get()) if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Hit.S_Hit"))) UGameplayStatics::PlaySound2D(C,Cue,.12f,1.9f);
        })
        .OnUnhovered_Lambda([Juice](){ButtonJuiceSet(*Juice,false,false);})
        .OnPressed_Lambda([Juice](){ButtonJuiceSet(*Juice,Juice->Hover,true);})
        .OnReleased_Lambda([Juice](){ButtonJuiceSet(*Juice,Juice->Hover,false);})
        .OnClicked_Lambda([Weak,Code,Index](){if(auto* C=Weak.Get()) C->Action(Code,Index);return FReply::Handled();})
        [SNew(STextBlock).Text(Txt(Title)).Font(Font(14,true)).Justification(ETextJustify::Center)
            .WrapTextAt(ButtonTextWidth).WrappingPolicy(ETextWrappingPolicy::DefaultWrapping)];
}
void SASMenu::Text(const FString& Value,int32 Size,FLinearColor Color) {
    Body->AddSlot().AutoHeight().Padding(0,6)[SNew(STextBlock).Text(Txt(Value)).Font(Font(Size)).ColorAndOpacity(Color)
        .Justification(ETextJustify::Left).WrapTextAt(BodyTextWidth).WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)];
}
void SASMenu::Card(const FString& Title,const FString& Description,const FString& Label,int32 Code,int32 Index,bool Enabled) {
    Body->AddSlot().AutoHeight().Padding(0,0,0,9)
    [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.027f,.040f,.052f)).Padding(12)
        [SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(0,0,18,0)
                [SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(Title)).Font(Font(19,true)).ColorAndOpacity(ASMenuGold)
                        .WrapTextAt(CardTextWidth).WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)[SNew(STextBlock).Text(Txt(Description)).Font(Font(14)).ColorAndOpacity(ASMenuMuted)
                        .WrapTextAt(CardTextWidth).WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)]]
            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [SNew(SBox).WidthOverride(ButtonWidth).IsEnabled(Enabled)[Button(Label,Code,Index)]]]];
}
void SASMenu::Input(const FString& Label,int32 Field) {
    Text(Label,14);auto* C=PC.Get();const FString Value=!C?TEXT(""):(Field==0?C->JoinAddress:(Field==1?C->JoinCode:(Field==2?C->SeedText:(C->ProgressImportPath.IsEmpty()?C->GetGameInstance<UASGameInstance>()->DefaultImportPath():C->ProgressImportPath))));const auto Weak=PC;
    Body->AddSlot().AutoHeight().Padding(0,2,0,12)
        [SNew(SEditableTextBox).Text(Txt(Value)).Font(Font(18)).SelectAllTextWhenFocused(true)
            .OnTextChanged_Lambda([Weak,Field](const FText& T){if(auto* P=Weak.Get()) {
                const FString V=T.ToString().Left(Field==3?1024:(Field==0?128:12));if(Field==0) P->JoinAddress=V;else if(Field==1) P->JoinCode=V;else if(Field==2) P->SeedText=V;else P->ProgressImportPath=V;
            }})];
}
void SASMenu::KeyRow(const FString& Label,FName Binding) {
    const auto Weak=PC;const FKey Current=PC.IsValid()?PC->BoundKey(Binding):EKeys::Invalid;
    Body->AddSlot().AutoHeight().Padding(4,6)
        [SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(Txt(Label)).Font(Font(16)).ColorAndOpacity(ASMenuMuted).WrapTextAt(CardTextWidth)]
            +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(240)
                [SNew(SInputKeySelector).SelectedKey(FInputChord(Current)).AllowGamepadKeys(true).AllowModifierKeys(false).EscapeCancelsSelection(true)
                    .KeySelectionText(Txt(TEXT("Press a key..."))).Font(Font(14))
                    .OnKeySelected_Lambda([Weak,Binding](const FInputChord& Chord){if(auto* C=Weak.Get()) C->Rebind(Binding,Chord.Key);})]]];
}
void SASMenu::CameraSlider(const FString& Label,float UASGameInstance::* Setting,float Min,float Max,float Step,const FString& Unit) {
    const auto Weak=PC;
    const auto Value=[Weak,Setting,Min]() {auto* C=Weak.Get();auto* GI=C?C->GetGameInstance<UASGameInstance>():nullptr;return GI?GI->*Setting:Min;};
    const auto Save=[Weak]() {if(auto* C=Weak.Get()) if(auto* GI=C->GetGameInstance<UASGameInstance>()) {GI->SaveCameraSettings();GI->SaveHUDSettings();}};
    Body->AddSlot().AutoHeight().Padding(4,6)
        [SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,7)
                [SNew(STextBlock).Font(Font(16)).ColorAndOpacity(ASMenuMuted).WrapTextAt(BodyTextWidth)
                    .Text_Lambda([Label,Unit,Value](){return Txt(FString::Printf(TEXT("%s   %s%s"),*Label,*FString::SanitizeFloat(Value()),*Unit));})]
            +SVerticalBox::Slot().AutoHeight()
                [SNew(SSlider).MinValue(Min).MaxValue(Max).StepSize(Step).MouseUsesStep(true)
                    .Value_Lambda(Value).SliderHandleColor(ASMenuGold)
                    .OnValueChanged_Lambda([Weak,Setting,Min,Max,Step](float V) {
                        if(auto* C=Weak.Get()) if(auto* GI=C->GetGameInstance<UASGameInstance>()) {
                            GI->*Setting=FMath::Clamp(FMath::RoundToInt(V/Step)*Step,Min,Max);C->ApplyCameraSettings();
                        }
                    }).OnMouseCaptureEnd_Lambda(Save).OnControllerCaptureEnd_Lambda(Save)]];
}
void SASMenu::Construct(const FArguments& Args) {
    PC=Args._Controller;Page=Args._Page;auto* C=PC.Get();auto* GI=C?C->GetGameInstance<UASGameInstance>():nullptr;
    auto* S=C?AS::State(C):nullptr;auto* PS=C?C->GetPlayerState<AASPlayerState>():nullptr;
    const bool Hub=S&&S->Phase==EASPhase::Hub;
    const TCHAR* Titles[]={TEXT("THE EMBER NEXUS"),TEXT("ENTER THE SPIRAL"),TEXT("THE ARMORY"),TEXT("ABILITY SANCTUM"),TEXT("ASHEN HEARTH"),TEXT("IONE'S WORKBENCH"),TEXT("CONSTELLATIONS"),TEXT("TWO SOULS, ONE SPIRAL"),TEXT("SETTINGS"),TEXT("CLAIM YOUR REWARD"),TEXT("EXPEDITION COMPLETE"),TEXT("CONTROLS"),TEXT("RETURN TO THE HUB?"),TEXT("LEAVE THIS SESSION?"),TEXT("WEAPON SHOWROOM"),TEXT("YOUR PROGRESS"),TEXT("RESTORE PROGRESS?"),TEXT("YOUR RUN BUILD"),TEXT("EMBER BARGAIN"),TEXT("CHARACTER & ACHIEVEMENTS")};
    const FString Title=Titles[FMath::Clamp(Page,0,19)];const auto Weak=PC;
    if(Page==9&&GI&&S) {RewardScreen();return;}
    ChildSlot
    [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0,0,0,.28f)).Padding(24)
        [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly).HAlign(HAlign_Center).VAlign(VAlign_Center)
            [SNew(SBox).WidthOverride(PanelWidth).HeightOverride(PanelHeight)
                [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(ASMenuInk).Padding(FMargin(32,28))
                    [SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight()
                            [SNew(SHorizontalBox)
                                +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,16,0)
                                    [SNew(SVerticalBox)
                                        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("A S H E N   S P I R A L"))).Font(Font(12,true)).ColorAndOpacity(ASMenuGold)]
                                        +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,8)[SNew(STextBlock).Text(Txt(Title)).Font(Font(29,true)).ColorAndOpacity(FLinearColor::White)
                                            .WrapTextAt(HeaderTextWidth).WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)]]
                                +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Button(TEXT("CLOSE   /   ESC"),0)]]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,5,0,15)
                            [SNew(STextBlock).Font(Font(13)).ColorAndOpacity(ASMenuMuted).WrapTextAt(BodyTextWidth).Text_Lambda([Weak](){
                                auto* P=Weak.Get();auto* G=P?P->GetGameInstance<UASGameInstance>():nullptr;if(!G) return Txt(TEXT("Loading profile..."));
                                const auto& D=G->Profile;return Txt(FString::Printf(TEXT("ASH  %d     RELIC EMBERS  %d     TABLETS  %d     SKILL POINTS  %d"),D.Ash,D.RelicEmbers,D.Tablets,D.SkillPoints));})]
                        +SVerticalBox::Slot().FillHeight(1)
                            [SAssignNew(Scroll,SScrollBox)+SScrollBox::Slot().Padding(0,0,12,0)[SAssignNew(Body,SVerticalBox)]]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,14,0,10)
                            [SNew(STextBlock).Font(Font(13)).ColorAndOpacity(ASMenuGold).WrapTextAt(BodyTextWidth)
                                .WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping).Text_Lambda([Weak](){auto* P=Weak.Get();return Txt(P?P->Notice:TEXT(""));})]
                        +SVerticalBox::Slot().AutoHeight()
                            [SNew(SHorizontalBox)
                                +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[Button(TEXT("OVERVIEW"),10,0)]
                                +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[Button(TEXT("CONTROLS"),10,11)]
                                +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[Button(TEXT("SETTINGS"),10,8)]
                                +SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Right).VAlign(VAlign_Center)
                                    [SNew(STextBlock).Text(Txt(TEXT("ASHEN SPIRAL  0.23.2"))).Font(Font(11)).ColorAndOpacity(ASMenuMuted)]]]]]]];
    if(!GI||!S) {Text(TEXT("Loading the world and personal profile..."));return;}
    const auto& P=GI->Profile;
    const auto* Hero=Cast<AASCharacter>(C->GetPawn());
    const TArray<int32> EmptyRun;
    const auto& ActiveBuild=Hero?Hero->Build:P.Build;
    const auto& ActiveRun=!Hub&&Hero?Hero->RunRanks:EmptyRun;
    if(Page==19) {
        const int32 Level=ASCareer::Level(P.Experience);
        Text(FString::Printf(TEXT("LEVEL %d / %d of %d XP toward the next level"),Level,ASCareer::Progress(P.Experience),ASCareer::Cost(Level)),23,ASMenuGold);
        Text(TEXT("Every achievement tier grants 1 Relic Ember; tier III also grants 1 Tablet. Every 5 levels grants 1 Ember; every 10 also grants 1 Tablet. Earned milestones on older profiles receive these rewards once. Achievement tiers also unlock equipable titles shown to your partner. No stat or damage bonuses. Progress saves with receipts; practice never counts."));
        for(int32 I=0;I<ASCareer::Count;++I) {
            const auto& D=ASCareer::Achievements[I];const int32 Value=P.Career.IsValidIndex(I)?P.Career[I]:0,Tier=ASCareer::Tier(Value,I);
            const FString State=Tier==3?TEXT("ALL TIERS EARNED"):FString::Printf(TEXT("NEXT: %d / %d"),Value,D.Goals[Tier]);
            Card(FString::Printf(TEXT("%s / %d of 3 tiers"),UTF8_TO_TCHAR(D.Name),Tier),FString(UTF8_TO_TCHAR(D.Description))+FString::Printf(TEXT("\nMilestones: %d / %d / %d. %s"),D.Goals[0],D.Goals[1],D.Goals[2],*State),P.TitleChoice==I?TEXT("TITLE EQUIPPED"):(Tier>0?TEXT("EQUIP TITLE"):TEXT("TRACKED")),64,I,Tier>0&&P.TitleChoice!=I);
        }
    } else if(Page==0) {
        Text(BuildSummary(ActiveBuild,ActiveRun,Hero),14,ASMenuGold);
        Text(Hub?TEXT("An ember survives. Choose your weapon, shape your gravity, then descend together."):TEXT("The expedition is paused only when the shared pause indicator is shown. Escape resumes."));
        Card(TEXT("Expedition"),TEXT("Four wards. Eight rooms per ward. Automatic waves and personal rewards; either player can continue once both have chosen."),Hub?TEXT("PREPARE"):TEXT("RESUME"),Hub?10:0,1);
        if(Hub) {Card(TEXT("Armory and Sanctum"),TEXT("Walk to a displayed weapon and press E to try or buy it. Test weapons and Spiral on the practice target. Purchases save immediately."),TEXT("LOADOUT"),10,2);
            Card(TEXT("Permanent upgrades"),TEXT("Ability cooldown, attack speed, damage, health, movement speed, critical chance, and more."),TEXT("UPGRADES"),10,4);
            Card(TEXT("Ione and the Constellations"),TEXT("Craft permanent skill points, spend materials, and strengthen your next expedition."),TEXT("WORKBENCH"),10,5);}
        Card(TEXT("Run traits and synergies"),TEXT("Inspect every trait, its stacking limit, owned ranks and evolutions. Rune Lance uses Focus; primary hits can recharge skills with Battle Conduit."),TEXT("YOUR BUILD"),10,17);
        Card(TEXT("Character and achievements"),TEXT("Cosmetic levels, lifetime kills, clean clears, co-op rescues and 36 achievement milestones."),TEXT("ACHIEVEMENTS"),10,19);
        Card(TEXT("Keep your progress"),TEXT("Automatic saving between versions, portable exports, and imports from earlier Unreal builds."),TEXT("PROGRESS"),10,15);
        Card(TEXT("Co-op connection"),TEXT("Two independent PCs. Host on a reachable LAN or private VPN address; joining requires the host's session code."),TEXT("CONNECTION"),10,7);
        if(S->bConnectionHold) Card(TEXT("Partner disconnected"),TEXT("The game is held for up to 90 seconds. Continuing alone releases the reserved reconnect slot."),TEXT("CONTINUE ALONE"),34,0,PS&&PS->PlayerIndex==0);
        if(!Hub) Card(TEXT("Return to hub"),TEXT("Already banked rewards remain. This abandons the active expedition, but the partner stays connected."),TEXT("REVIEW"),10,12,PS&&PS->PlayerIndex==0);
        Card(TEXT("Leave session"),TEXT("Return to an offline hub. If you are the host, this ends the other player's connection."),TEXT("REVIEW"),10,13);
    } else if(Page==1) {
        Text(TEXT("Space jumps. Shift dashes on the ground or in the air. Hold Spiral and release in the gold window for a perfect well. Base Spiral cooldown is 3.3 seconds; cooldown upgrades reduce it."));
        Text(BuildSummary(ActiveBuild,ActiveRun,Hero),14,ASMenuGold);
        Body->AddSlot().AutoHeight().Padding(0,10)[SNew(STextBlock).Font(Font(16,true)).ColorAndOpacity(ASMenuGold).WrapTextAt(BodyTextWidth).Text_Lambda([Weak](){
            FString Status;auto* Local=Weak.Get();auto* State=Local?AS::State(Local):nullptr;if(!State) return Txt(TEXT("Waiting for host..."));
            for(auto V:State->PlayerArray) if(auto* Player=Cast<AASPlayerState>(V)) Status+=FString::Printf(TEXT("PLAYER %d / LEVEL %d / %s\n%s   |   %s\n"),Player->PlayerIndex+1,Player->CharacterLevel,*ASData::CareerTitle(Player->TitleIndex,Player->TitleTier),Player->bProfileReady?(Player->bReady?TEXT("READY"):TEXT("NOT READY")):TEXT("LOADING PROFILE"),ASData::Weapons[Player->Build.Weapon].Name);
            return Txt(Status);})];
        Card(TEXT("Your readiness"),TEXT("Changing a weapon, style, or permanent upgrade resets your ready check. Solo expeditions do not require the ready toggle."),TEXT("TOGGLE READY"),30,0,Hub&&PS&&PS->bProfileReady);
        Text(TEXT("To depart: gather every player at the Ember, Ashen or Blood archway (or the Endless portal once unlocked). Guests press E to ready up; when everyone is ready the host presses E to step through together."),16,ASMenuGold);
        Input(TEXT("Deterministic expedition seed (story portals)"),2);
        Card(TEXT("Endless records and upgrades"),TEXT("Your best Endless runs and the permanent chest-choices upgrade. Enter Endless at its portal in the hub."),TEXT("ENDLESS"),10,22);
        Card(TEXT("Choose equipment"),FString::Printf(TEXT("Equipped: %s  /  %s"),ASData::Weapons[P.Build.Weapon].Name,ASData::Styles[P.Build.Style].Name),TEXT("ARMORY"),10,2);
        Card(TEXT("Build your stats"),TEXT("Preview current and next values for permanent cooldown, attack-speed, damage, health, movement, and critical upgrades."),TEXT("UPGRADES"),10,4);
        Card(TEXT("Bring a partner"),TEXT("Host first, share the code and reachable IP, and wait for both ready checks."),TEXT("CO-OP"),10,7);
    } else if(Page==2) {
        for(int32 I=0;I<ASRules::WeaponCount;++I) {
            const bool Owned=(P.OwnedWeapons&(1<<I))!=0;
            Card(ASData::Weapons[I].Name,ASData::Weapons[I].Desc,P.Build.Weapon==I?TEXT("EQUIPPED"):(Owned?TEXT("EQUIP"):FString::Printf(TEXT("%d ASH"),ASData::Weapons[I].Cost)),20,I,Hub&&P.Build.Weapon!=I&&(Owned||P.Ash>=ASData::Weapons[I].Cost));
        }
        Card(TEXT("Spiral styles"),TEXT("Change how your ability pulls, slows, shields, or collapses."),TEXT("SANCTUM"),10,3);
    } else if(Page==23) {
        const bool Open=S&&S->bLuckyShop&&S->Phase==EASPhase::Reward;
        Text(Open?TEXT("The lucky shop. Spend Scrap for supplies; rare items cost Ash. One of each item per player."):TEXT("The shop is closed. It appears at some checkpoints from wave 10."),18,Open?ASMenuGold:ASMenuMuted);
        Text(FString::Printf(TEXT("Your Scrap: %d     Your Ash: %d (run earnings, banked when the run ends)"),PS?PS->AvailableScrap:0,PS?PS->AvailableAsh:0),16,ASMenuGold);
        for(int32 I=0;I<ASExpedition::ShopCount;++I) {
            const auto& Item=ASExpedition::ShopStock(I);if(Item.Rare&&!(S&&S->bRareShop)) continue;
            FString Name=UTF8_TO_TCHAR(Item.Name),Description=UTF8_TO_TCHAR(Item.Description);bool Fits=true;
            if(I==ASExpedition::ShopPickIndex) {
                const int32 Offer=PS?PS->ShopOffer:-1;const bool Trait=ASBuild::IsTrait(Offer);
                if(!Trait&&!ASRules::IsRunUpgradeOffer(Offer)) continue;
                Name=FString(TEXT("Trader's Pick: "))+(Trait?FString(UTF8_TO_TCHAR(ASBuild::Catalog[Offer-ASBuild::OfferBase].Name)):FString(ASData::RunUpgrades[Offer-100].Name));
                Description=Trait?FString(UTF8_TO_TCHAR(ASBuild::Catalog[Offer-ASBuild::OfferBase].Description)):FString(ASData::RunUpgrades[Offer-100].Desc);
                const auto* PickHero=PC.IsValid()?Cast<AASCharacter>(PC->GetPawn()):nullptr;Fits=PickHero&&PickHero->CanApplyChoice(Offer);
            }
            const FString Price=Item.Ash>0?FString::Printf(TEXT("%d ASH"),Item.Ash):FString::Printf(TEXT("%d SCRAP"),Item.Scrap);
            Card(Name,Description,(PS&&(PS->ShopBought&(1<<I)))?FString(TEXT("BOUGHT")):Price,75,I,Fits&&Open&&PS&&ASExpedition::CanBuyShopItem(I,PS->AvailableScrap,PS->AvailableAsh,PS->ShopBought,S->bRareShop));
        }
    } else if(Page==24) {
        // Portal page (Astra flow): details, seed, then the host's ENTER starts the walk-in.
        const int32 D=PC.IsValid()?FMath::Clamp(PC->SelectedPortal,0,2):0;
        const bool Unlocked=ASExpedition::DifficultyUnlocked(D,PS?PS->ClearedMask:0);
        Text(FString::Printf(TEXT("%s portal / %s"),UTF8_TO_TCHAR(ASExpedition::PortalName(D)),UTF8_TO_TCHAR(ASExpedition::PortalDifficultyName(D))),20,ASMenuGold);
        Text(Unlocked?FString(UTF8_TO_TCHAR(ASExpedition::PortalModifiers(D))):FString(TEXT("LOCKED: "))+UTF8_TO_TCHAR(ASExpedition::UnlockCondition(D))+TEXT("."),16,Unlocked?ASMenuGold:ASMenuMuted);
        Input(TEXT("Deterministic expedition seed"),2);
        Card(TEXT("Step through"),TEXT("Host only. Both players must be ready. You walk into the archway together and the first chamber begins."),!Unlocked?TEXT("LOCKED"):TEXT("ENTER PORTAL"),76,0,Hub&&PS&&PS->PlayerIndex==0&&Unlocked);
    } else if(Page==22) {
        const bool Open=PS&&PS->bEndlessUnlocked;
        Text(Open?TEXT("Endless: one huge arena, chained waves, a boss every ten waves. Many entered, none came back whole. Permanent skill-tree upgrades come with you; everything else starts fresh."):TEXT("The portal is sealed. Beat Normal mode once to open it."),18,Open?ASMenuGold:ASMenuMuted);
        Text(FString::Printf(TEXT("Best cleared wave: %d"),P.BestEndlessWave),16,ASMenuGold);
        Card(TEXT("Enter the Endless portal"),TEXT("Host only. Both players must be ready. Either player presses E at the chest to start the next wave; there is no timer."),TEXT("ENTER ENDLESS"),74,0,Hub&&PS&&PS->PlayerIndex==0&&Open);
        if(P.EndlessChoices<2) {
            const int32 Cost=ASExpedition::ChoiceCost(P.EndlessChoices);
            Card(FString::Printf(TEXT("Chest choices: %d -> %d"),3+P.EndlessChoices,4+P.EndlessChoices),TEXT("Permanent: more upgrade choices at every Endless chest."),FString::Printf(TEXT("%d ASH"),Cost),29,0,Hub&&P.Ash>=Cost);
        } else Text(TEXT("Chest choices: 5 (maximum)."),16,ASMenuGold);
        Text(TEXT("Top runs on this character"),18,ASMenuGold);
        for(int32 I=0;I<FMath::Min(5,P.EndlessRecords.Num());++I) {
            const auto& R=P.EndlessRecords[I];
            Text(FString::Printf(TEXT("%d. Wave %d   damage %.0f / %.0f   kills %d / %d   taken %.0f / %.0f"),I+1,R.Wave,R.Damage,R.PartnerDamage,R.Kills,R.PartnerKills,R.DamageTaken,R.PartnerDamageTaken),14);
        }
        if(P.EndlessRecords.IsEmpty()) Text(TEXT("No completed runs yet."),14);
    } else if(Page==21) {
        const int32 Pool=S?S->Cinders:0;const auto Odds=ASExpedition::Odds(Pool);
        Text(TEXT("Stake Cinders for one wish. One wish per player at each well. The result is decided by the host's server, never by your game."));
        Text(FString::Printf(TEXT("Cinders in the pool: %d. Minimum stake: %d."),Pool,ASExpedition::GambleMinimum),18,ASMenuGold);
        Text(FString::Printf(TEXT("Odds at this balance: %d%% lose the stake, %d%% stake back, %d%% x2, %d%% x5, %d%% x10."),Odds.Loss,Odds.Refund,Odds.Double,Odds.Five,Odds.Ten),16);
        const bool Spent=PS&&S&&PS->LastGambleRoom==S->Room;
        for(int32 I=0;I<3;++I) {
            const int32 Stake=ASExpedition::GambleStake(Pool,I);
            Card(FString::Printf(TEXT("Stake %d Cinders"),Stake),FString::Printf(TEXT("Lose: -%d. Best case: x10 = %d Cinders back (+%d)."),Stake,Stake*10,Stake*9),Stake>0?TEXT("MAKE A WISH"):TEXT("TOO FEW CINDERS"),73,I,Stake>0&&!Spent&&Hero&&!Hero->bDowned);
        }
        if(Spent) Text(TEXT("You have already made your wish at this well."),18,ASMenuGold);
    } else if(Page==18) {
        Text(TEXT("Trade current health for a random build-compatible upgrade. This is optional and can be used once per player in each shrine room."));
        const float Cost=Hero?ASAdventure::ShrineCost(Hero->MaxHealth):0.f;
        Card(TEXT("Offer an ember of vitality"),FString::Printf(TEXT("Cost: %.0f current HP (12%% of max HP). Gain one non-instant trait or core stat upgrade. Cannot kill you, and does not lower maximum HP."),Cost),TEXT("MAKE OFFERING"),62,0,Hero&&!Hero->bDowned&&PS&&!PS->bShrineUsed&&ASAdventure::CanSacrifice(Hero->Health,Hero->MaxHealth));
        if(PS&&PS->bShrineUsed) Text(TEXT("You have already used this shrine."),18,ASMenuGold);
    } else if(Page==17) {
        // Tab menu: current weapon, base stats with this run's modifiers, then ONLY what was gained this run (no permanent or skill-tree entries); the long stats block sits last.
        Text(FString::Printf(TEXT("%s  /  %s  /  %s"),ASData::Weapons[ActiveBuild.Weapon].Name,ASData::Styles[ActiveBuild.Style].Name,ASData::Abilities[ActiveBuild.Ability].Name),20,ASMenuGold);
        Text(BuildSummary(ActiveBuild,ActiveRun,Hero),14,ASMenuGold);
        Text(TEXT("YOUR RUN BUILD"),18,ASMenuGold);
        bool Any=false;
        // Every acquisition uses the same card: title "NAME  xN", one effect line, and a tag for where it came from.
        auto Acquired=[&](const FString& Title,const FString& Detail,const TCHAR* Source) {Any=true;Card(Title,Detail,Source,0,0,false);};
        if(Hero) {
            for(int32 I=0;I<ASRules::RunUpgradeCount;++I) if(Hero->RunRank(I)>0) Acquired(FString::Printf(TEXT("%s  x%d"),ASData::RunUpgrades[I].Name,Hero->RunRank(I)),ASData::RunUpgrades[I].Desc,TEXT("CORE STAT"));
            for(int32 I=0;I<ASExpedition::RunGainCount;++I) if(Hero->RunGain(I)>0) Acquired(UTF8_TO_TCHAR(ASExpedition::RunGainLine(I,Hero->RunGain(I)).c_str()),UTF8_TO_TCHAR(ASExpedition::RunGainDetail(I,Hero->RunGain(I)).c_str()),I<ASExpedition::ShopCount?TEXT("SHOP"):TEXT("CHEST"));
            for(int32 I=0;I<24;++I) if(Hero->Relic(I)) Acquired(FString::Printf(TEXT("%s  x1"),ASData::Relics[I].Name),ASData::Relics[I].Desc,TEXT("RELIC"));
            for(int32 I=0;I<4;++I) if(Hero->EndlessRanks.IsValidIndex(I)&&Hero->EndlessRanks[I]>0) Acquired(UTF8_TO_TCHAR(ASExpedition::AcquisitionTitle(ASExpedition::ExclusiveName(ASExpedition::ExclusiveBase+I),Hero->EndlessRanks[I]).c_str()),UTF8_TO_TCHAR(ASExpedition::ExclusiveDescription(ASExpedition::ExclusiveBase+I)),TEXT("ENDLESS"));
            for(int32 I=0;I<ASBuild::Count;++I) {
                const auto& D=ASBuild::Catalog[I];const int32 Rank=Hero->TraitRank(ASBuild::Trait(I));if(Rank==0) continue;
                const bool Evolved=ASBuild::Evolved(Hero->EvolutionMask,I);
                Acquired(UTF8_TO_TCHAR(ASExpedition::AcquisitionTitle(D.Name,Rank).c_str()),FString::Printf(TEXT("Rank %d of %d%s. %s"),Rank,D.Max,Evolved?TEXT(", evolved +1"):TEXT(""),UTF8_TO_TCHAR(D.Description)),TEXT("TRAIT"));
            }
        }
        if(!Any) Text(TEXT("Nothing gained yet this run."),15,ASMenuMuted);
        if(Hero) Text(FString::Printf(TEXT("Focus %.0f / %.0f  |  Crit damage %.0f%%  |  Personal rerolls %d  |  Curse stacks %d"),Hero->Focus,Hero->MaxFocus(),Hero->CriticalPower()*100.f,Hero->Rerolls,Hero->CurseStacks),17,ASMenuGold);
    } else if(Page==15) {
        Text(TEXT("Ash, relic embers, tablets, materials, unlocks and permanent upgrades now save outside the extracted game folder. Future 0.13+ builds use this same location automatically. Cinders and chest stat ranks remain run-only."));
        Text(FString::Printf(TEXT("Ash: %d   Relic embers: %d   Tablets: %d   Skill points: %d"),P.Ash,P.RelicEmbers,P.Tablets,P.SkillPoints),18,ASMenuGold);
        Text(GI->ProgressDirectory(),13);
        Text(FString::Printf(TEXT("Active character: %d. Each character has its own progress, currencies and unlocks."),GI->ActiveCharacterSlot+1),16,ASMenuGold);
        Card(TEXT("Character 1"),TEXT("Your original character."),GI->ActiveCharacterSlot==0?TEXT("ACTIVE"):TEXT("SWITCH"),71,0,Hub&&GI->ActiveCharacterSlot!=0);
        Card(TEXT("Character 2"),TEXT("A fully independent fresh character. Starts at zero."),GI->ActiveCharacterSlot==1?TEXT("ACTIVE"):TEXT("SWITCH"),71,1,Hub&&GI->ActiveCharacterSlot!=1);
        Card(TEXT("Portable progress backup"),TEXT("Create a separate verified export. Copy it to another PC or keep it before changing versions."),TEXT("EXPORT"),50);
        Card(TEXT("Find your saves and exports"),TEXT("Opens the progress folder in your file browser."),TEXT("OPEN FOLDER"),49);
        Text(TEXT("To move from 0.12 or earlier: run Import_Progress.bat and select your old .sav file. It prepares a copy for review here. You can also paste an existing .sav / .ashenprofile path below. Import is available only in an offline hub."));
        Input(TEXT("Save or export file to import"),3);
        Card(TEXT("Review a progress import"),TEXT("Shows the selected balances before restoring. Your current profile is backed up before replacement; currencies are not added together."),TEXT("REVIEW IMPORT"),47,0,Hub&&C->GetNetMode()==NM_Standalone);
        if(!GI->LastExportPath.IsEmpty()) Text(TEXT("Last export: ")+GI->LastExportPath,13);
    } else if(Page==16) {
        if(!GI->bImportReady) {Text(TEXT("Choose a supported save on the Progress page first."));Card(TEXT("Progress"),TEXT("Select a file to review."),TEXT("BACK"),10,15);return;}
        const auto& Import=GI->ImportPreview;Text(TEXT("Selected save: ")+GI->ImportSourcePath,13);
        Text(TEXT("This restores the selected profile, replacing the currently loaded progression. A separate backup of the current profile is created first. The offline hub then reloads."));
        Text(FString::Printf(TEXT("ASH: %d -> %d\nRELIC EMBERS: %d -> %d\nTABLETS: %d -> %d\nSKILL POINTS: %d -> %d\nBEST ROOM: %d -> %d"),P.Ash,Import.Ash,P.RelicEmbers,Import.RelicEmbers,P.Tablets,Import.Tablets,P.SkillPoints,Import.SkillPoints,P.BestRoom,Import.BestRoom),19,ASMenuGold);
        for(int32 I:{0,1,4}) Text(FString::Printf(TEXT("%s: %d -> %d"),ASData::Materials[I],P.Materials[I],Import.Materials[I]),15);
        Card(TEXT("Restore this progress"),TEXT("Creates a backup, restores the selected profile, and reloads the hub."),TEXT("RESTORE"),48,0,Hub&&C->GetNetMode()==NM_Standalone);
        Card(TEXT("Keep current progress"),TEXT("Return without applying the import."),TEXT("CANCEL"),10,15);
    } else if(Page==14) {
        const int32 I=FMath::Clamp(C->ShopWeapon,0,ASRules::WeaponCount-1);const bool Owned=(P.OwnedWeapons&(1<<I))!=0;
        const auto Type=ASData::WeaponElement(I);
        Text(FString(ASData::Weapons[I].Name)+TEXT(" / ")+ASData::ElementName(Type),23,ASData::HitColor(Type));
        Text(ASData::Weapons[I].Desc);
        Text(Type==EASHitKind::Burn?TEXT("BURN: 6 damage per second for 3 seconds. Hits refresh one burn; it does not stack."):
            (Type==EASHitKind::Poison?TEXT("POISON: 3 damage per second per stack for 4 seconds, up to 3 stacks. Hits refresh the duration."):
            TEXT("FROST: builds chill and slows targets. Repeated hits briefly freeze normal enemies; bosses are slowed instead. Freeze has an immunity window.")));
        Text(TEXT("Damage upgrades scale burn and poison. Status damage cannot crit or trigger more on-hit effects. Practice costs nothing and does not change your expedition loadout."));
        Card(TEXT("Take it to the practice target"),TEXT("Try this weapon at the target in the north-east side of the hub, including weapons you have not bought."),TEXT("TRY WEAPON"),45,I,Hub);
        Card(TEXT("Expedition loadout"),TEXT("Purchase or equip this weapon for future expeditions. This selection is saved."),
            P.Build.Weapon==I?TEXT("EQUIPPED"):(Owned?TEXT("EQUIP"):FString::Printf(TEXT("%d ASH"),ASData::Weapons[I].Cost)),20,I,Hub&&P.Build.Weapon!=I&&(Owned||P.Ash>=ASData::Weapons[I].Cost));
    } else if(Page==3) {
        Text(TEXT("ACTIVE ABILITIES / F OR RIGHT TRIGGER"),22,ASMenuGold);
        Text(TEXT("Choose one active ability alongside Spiral. All six are free to equip and test on the lobby dummy. Selection is saved. Ability cooldown upgrades shorten these cooldowns too."));
        TSharedPtr<SWrapBox> Skills;
        Body->AddSlot().AutoHeight()[SAssignNew(Skills,SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10,10))];
        const int32 Icons[]={ASEffectArt::Bolt,ASEffectArt::Frost,ASEffectArt::Fork,ASEffectArt::Venom,ASEffectArt::Shield,ASEffectArt::Flame};
        const TCHAR* Briefs[]={TEXT("Pierce with runic power"),TEXT("Freeze a cone of foes"),TEXT("Arc through four targets"),TEXT("Plant a poison field"),TEXT("Shield and melee boost"),TEXT("Set an explosive rune")};
        for(int32 I=0;I<ASAdventure::AbilityCount;++I) Skills->AddSlot()[ChoiceTile(ASData::Abilities[I].Name,Briefs[I],ASData::Abilities[I].Desc,TEXT("ACTIVE SKILL"),Icons[I],1,P.Build.Ability==I?TEXT("EQUIPPED"):TEXT("EQUIP + TEST"),28,I,Hub&&P.Build.Ability!=I,232.f)];
        Body->AddSlot().AutoHeight().Padding(0,12)[DetailStrip()];
        Text(TEXT("SPIRAL STYLES / RIGHT MOUSE"),22,ASMenuGold);
        for(int32 I=0;I<P.Build.StylePool;++I) Card(ASData::Styles[I].Name,ASData::Styles[I].Desc,P.Build.Style==I?TEXT("EQUIPPED"):TEXT("EQUIP"),21,I,Hub&&P.Build.Style!=I);
        Card(TEXT("Recover a lost style"),FString::Printf(TEXT("%d of 10 styles discovered. One tablet discovers the next style."),P.Build.StylePool),TEXT("1 TABLET"),24,0,Hub&&P.Build.StylePool<10&&P.Tablets>=1);
    } else if(Page==4) {
        Text(TEXT("Permanent upgrades carry into every run. Room-chest upgrades stack for the current run only. Costs increase with each rank; purchases keep your scroll position."));
        Text(BuildSummary(P.Build,EmptyRun),14,ASMenuGold);
        Text(TEXT("CORE BUILD UPGRADES"),19,ASMenuGold);
        for(int32 Stat=0;Stat<ASRules::RunUpgradeCount;++Stat) {
            const int32 I=PermanentStatIndex(Stat);const auto& N=ASData::Hearth[I];
            const int32 Rank=P.Build.Rank(I),Cost=ASRules::Cost(N.Cost,Rank,N.MaxRank);
            Card(FString::Printf(TEXT("%s   %d / %d"),Stat==3?TEXT("Maximum Health"):N.Name,Rank,N.MaxRank),
                FString(N.Desc)+TEXT("\n")+StatPreview(Stat,P.Build,EmptyRun,true),Cost<0?TEXT("MAX RANK"):FString::Printf(TEXT("%d ASH"),Cost),22,I,Hub&&Cost>=0&&P.Ash>=Cost);
        }
        FString Branch;
        for(int32 I=0;I<ASRules::HearthCount;++I) {
            if(I==0||I==14||I>=16) continue;
            const auto& N=ASData::Hearth[I];if(Branch!=N.Branch) {Branch=N.Branch;Text(Branch.ToUpper(),19,ASMenuGold);}
            const int32 Rank=P.Build.Rank(I),Cost=ASRules::Cost(N.Cost,Rank,N.MaxRank);
            Card(FString::Printf(TEXT("%s   %d / %d"),N.Name,Rank,N.MaxRank),N.Desc,Cost<0?TEXT("MAX RANK"):FString::Printf(TEXT("%d ASH"),Cost),22,I,Hub&&Cost>=0&&P.Ash>=Cost);
        }
        Card(TEXT("Constellations"),TEXT("Spend permanent skill points on Survivor, Tactician, and Scavenger talents."),TEXT("OPEN TREE"),10,6);
    } else if(Page==5) {
        FString Materials;for(int32 I:{0,1,4}) Materials+=FString::Printf(TEXT("%s: %d%s"),ASData::Materials[I],P.Materials[I],I==4?TEXT(""):TEXT("    "));Text(Materials,15,ASMenuGold);
        Card(TEXT("Ember Refinement"),TEXT("ONE purchase per profile. Exchange 15 Scrap and 8 Bone Dust for 40 Ash. This recipe does not grant a skill point."),P.bRefinementBought?TEXT("ALREADY CRAFTED"):TEXT("REFINE ONCE"),25,0,Hub&&!P.bRefinementBought&&P.Materials[0]>=15&&P.Materials[1]>=8);
        Card(TEXT("Memory Point"),TEXT("Repeatable. Every successful craft spends 3 Spiral Cores and 12 Bone Dust and grants one permanent skill point."),TEXT("CRAFT +1 POINT"),26,0,Hub&&P.Materials[4]>=3&&P.Materials[1]>=12);
        Card(TEXT("Relic discovery"),FString::Printf(TEXT("%d of 24 relics in your drop pool. Four relic embers discover the next four."),P.Build.RelicPool),TEXT("4 RELIC EMBERS"),23,0,Hub&&P.Build.RelicPool<24&&P.RelicEmbers>=4);
        Card(TEXT("Style discovery"),FString::Printf(TEXT("%d of 10 styles discovered. One tablet discovers the next."),P.Build.StylePool),TEXT("1 TABLET"),24,0,Hub&&P.Build.StylePool<10&&P.Tablets>=1);
        Card(TEXT("Constellations"),TEXT("Permanent talents persist after defeat. Each player's purchases stay on their own profile."),TEXT("SPEND POINTS"),10,6);
    } else if(Page==6) {
        const TCHAR* Names[]={TEXT("SURVIVOR"),TEXT("TACTICIAN"),TEXT("SCAVENGER")};
        for(int32 B=0;B<3;++B) {
            int32 Spent=0;for(int32 I=0;I<12;++I) if(ASData::Talents[I].Branch==B&&P.Build.Talent(I)) Spent+=ASData::Talents[I].Cost;
            Text(FString::Printf(TEXT("%s  /  %d points invested"),Names[B],Spent),19,ASMenuGold);
            for(int32 I=0;I<12;++I) if(I!=9&&ASData::Talents[I].Branch==B) {
                const auto& T=ASData::Talents[I];const bool Bought=P.Build.Talent(I);
                Card(T.Name,FString(T.Desc)+FString::Printf(TEXT(" Requires %d points invested in this branch."),T.Required),Bought?TEXT("ACQUIRED"):FString::Printf(TEXT("%d SKILL POINTS"),T.Cost),27,I,Hub&&!Bought&&P.SkillPoints>=T.Cost&&Spent>=T.Required);
            }
        }
    } else if(Page==7) {
        Text(TEXT("This is direct-IP co-op, not a matchmaking service. Both PCs need the same build. On different networks, use a trusted private VPN or a deliberately configured UDP 24567 route. The host must stay in the game."));
        Text(FString::Printf(TEXT("Mode: %s     Session code: %s"),C->GetNetMode()==NM_ListenServer?TEXT("HOSTING"):(C->GetNetMode()==NM_Client?TEXT("CLIENT"):TEXT("OFFLINE")),*S->SessionCode),20,ASMenuGold);
        Card(TEXT("Host a private party"),TEXT("Restarts this hub as a listen server. The new session code appears here; share that code plus your host IP."),TEXT("HOST"),35,0,Hub&&C->GetNetMode()!=NM_Client);
        Input(TEXT("Host IP or hostname (optional :24567)"),0);Input(TEXT("Host session code"),1);
        Card(TEXT("Join the host"),TEXT("Use your existing profile. A partner can rejoin a held expedition with the same profile within 90 seconds."),TEXT("JOIN"),36,0,Hub&&C->GetNetMode()!=NM_ListenServer);
        Card(TEXT("Local two-window test"),TEXT("Use Test_Two_Players.bat. It assigns different profile slots and joins over 127.0.0.1; no VPN or router changes are required."),TEXT("CLOSE"),0);
    } else if(Page==8) {
        Text(TEXT("INTERFACE SIZE"),22,ASMenuGold);
        Text(FString::Printf(TEXT("HUD: %.0f%%. Corner panels and interaction prompts scale together; aiming and damage numbers have their own controls."),GI->HUDScale*100.f),14);
        Body->AddSlot().AutoHeight().Padding(0,5,0,16)
            [SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(1).Padding(2)[Button(GI->HUDScale==.25f?TEXT("25%  /  ACTIVE"):TEXT("25%"),59,0)]
                +SHorizontalBox::Slot().FillWidth(1).Padding(2)[Button(GI->HUDScale==.5f?TEXT("50%  /  ACTIVE"):TEXT("50%"),59,1)]
                +SHorizontalBox::Slot().FillWidth(1).Padding(2)[Button(GI->HUDScale==.75f?TEXT("75%  /  ACTIVE"):TEXT("75%"),59,2)]
                +SHorizontalBox::Slot().FillWidth(1).Padding(2)[Button(GI->HUDScale==1.f?TEXT("100%  /  ACTIVE"):TEXT("100%"),59,3)]];
        Text(TEXT("CROSSHAIR & DAMAGE NUMBERS"),22,ASMenuGold);
        Card(TEXT("Crosshair shape"),FString::Printf(TEXT("Current: %s. The aim point stays at the screen center."),ASData::CrosshairStyles[GI->CrosshairStyle]),TEXT("NEXT SHAPE"),55);
        Card(TEXT("Crosshair color"),FString::Printf(TEXT("Current: %s. A dark outline keeps it visible on bright backgrounds."),ASData::CrosshairColorNames[GI->CrosshairColorIndex]),TEXT("NEXT COLOR"),56);
        CameraSlider(TEXT("Crosshair size (1 = 100%)"),&UASGameInstance::CrosshairScale,.5f,2.f,.1f,TEXT(" x"));
        Card(TEXT("Reset crosshair"),TEXT("White cross at 100% size."),TEXT("RESET"),57);
        Text(TEXT("Damage numbers rise above your crosshair. Critical hits retain their extra emphasis; incoming damage uses a separate higher row."),14);
        Body->AddSlot().AutoHeight().Padding(0,5,0,12)
            [SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(1).Padding(2)[Button(TEXT("80%"),52,0)]
                +SHorizontalBox::Slot().FillWidth(1).Padding(2)[Button(TEXT("100%"),52,1)]
                +SHorizontalBox::Slot().FillWidth(1).Padding(2)[Button(TEXT("120%"),52,2)]
                +SHorizontalBox::Slot().FillWidth(1).Padding(2)[Button(TEXT("140%"),52,3)]
                +SHorizontalBox::Slot().FillWidth(1).Padding(2)[Button(TEXT("160%"),52,4)]];
        Text(FString::Printf(TEXT("Selected damage-number size: %.0f%%"),GI->NumberScale*100.f),14,ASMenuGold);
        const int32 NextColor=(GI->Profile.ColorChoice+1)%8;
        Card(TEXT("Character color"),FString::Printf(TEXT("Current: %s. Visible to both players and saved with your profile. P1 is always cyan and P2 white on the minimap."),GI->Profile.ColorChoice<0?TEXT("PLAYER DEFAULT"):ASData::ArmorColorNames[GI->Profile.ColorChoice]),FString::Printf(TEXT("TRY %s"),ASData::ArmorColorNames[NextColor]),58,NextColor);
        Text(TEXT("CAMERA POSITION & ANGLE"),22,ASMenuGold);
        Text(TEXT("Drag a slider to adjust your view. Changes apply immediately and save when you release the slider or leave Settings. Close the menu to check your aim. These settings affect only your own camera."),14);
        CameraSlider(TEXT("Tilt / pitch (negative = look down)"),&UASGameInstance::CameraPitch,-30.f,20.f,1.f,TEXT(" degrees"));
        CameraSlider(TEXT("Camera height (higher = above your character)"),&UASGameInstance::CameraHeight,20.f,200.f,5.f,TEXT(" cm"));
        CameraSlider(TEXT("Shoulder offset (negative = left, positive = right)"),&UASGameInstance::CameraShoulder,-220.f,220.f,5.f,TEXT(" cm"));
        CameraSlider(TEXT("Camera distance / zoom"),&UASGameInstance::CameraDistance,360.f,780.f,1.f,TEXT(" cm"));
        CameraSlider(TEXT("Field of view (higher = wider view)"),&UASGameInstance::CameraFOV,65.f,105.f,1.f,TEXT(" degrees"));
        Card(TEXT("Hold TAB to view build"),TEXT("OFF: TAB toggles the build panel. ON: the panel shows only while TAB is held and closes when you let go."),GI->bHoldBuild?TEXT("ON / HOLD"):TEXT("OFF / TOGGLE"),77);
        Card(TEXT("Motion sickness reduction"),TEXT("Disable all hammer camera shake. Attack timing and impact sounds stay the same."),GI->bReduceMotion?TEXT("ON / NO SHAKE"):TEXT("OFF / SHAKE ENABLED"),70);
        Card(TEXT("Restore camera defaults"),TEXT("Reset tilt, height, shoulder offset, distance and field of view. Mouse wheel still zooms; Q swaps your selected shoulder offset."),TEXT("RESET CAMERA"),54);
        Text(TEXT("60 FPS is a design target, not a measured guarantee. Start with Balanced or High, then use F9 and Unreal's stat unit command to measure on your PC."));
        const TCHAR* Names[]={TEXT("PERFORMANCE"),TEXT("BALANCED"),TEXT("HIGH"),TEXT("EPIC")};
        for(int32 I=0;I<4;++I) Card(Names[I],I==0?TEXT("Low scalability, 65% internal resolution; prioritizes frame rate."):(I==1?TEXT("Medium scalability, 80% internal resolution."):TEXT("Full internal resolution. Higher effects and lighting cost.")),GI->Quality==I?TEXT("ACTIVE"):TEXT("APPLY"),40,I,GI->Quality!=I);
        Card(TEXT("Mouse sensitivity"),FString::Printf(TEXT("Current multiplier: %.2f"),GI->Sensitivity),TEXT("INCREASE"),41,1);Card(TEXT("Lower sensitivity"),TEXT("Sensitivity changes are saved when the game closes normally."),TEXT("DECREASE"),41,-1);
        Card(TEXT("Borderless fullscreen"),TEXT("Fill the desktop at its native resolution. Available in Standalone Game or packaged builds."),TEXT("FULLSCREEN"),51,1);
        Card(TEXT("Windowed mode"),TEXT("Return to a movable 1280 x 720 window."),TEXT("WINDOWED"),51,0);
        Card(TEXT("TV text size"),FString::Printf(TEXT("HUD text is rendered at native output resolution. Current size: %.0f%%. Use borderless fullscreen for your display's native resolution."),GI->HUDTextScale*100.f),TEXT("CHANGE SIZE"),53);
        Card(TEXT("Damage numbers"),TEXT("At most 64 recent numbers are drawn. Turn them off for a cleaner combat view."),GI->bShowNumbers?TEXT("ON - TOGGLE"):TEXT("OFF - TOGGLE"),42);
        Card(TEXT("Key bindings"),TEXT("Keyboard, mouse buttons, and digital controller buttons can be reassigned. Escape always closes the menu."),TEXT("REBIND"),10,11);
    } else if(Page==9) {
        Text(TEXT("Loading reward choices..."));
    } else if(Page==10) {
        Text(S->Phase==EASPhase::Victory?TEXT("The fourth sovereign falls. The Spiral remembers your victory."):TEXT("The ember dims, but what you recovered remains."),23,ASMenuGold);
        if(S->bEndless) {
            Text(FString::Printf(TEXT("Highest cleared wave: %d.   Best on this character: %d."),S->ClearedEndlessWave,P.BestEndlessWave),20,ASMenuGold);
            for(auto V:S->PlayerArray) if(auto* Other=Cast<AASPlayerState>(V)) Text(FString::Printf(TEXT("PLAYER %d   damage %.0f   kills %d   damage taken %.0f"),Other->PlayerIndex+1,Other->RunDamage,Other->RunKills,Other->RunDamageTaken),16);
            for(int32 I=0;I<FMath::Min(5,P.EndlessRecords.Num());++I) Text(FString::Printf(TEXT("#%d   wave %d"),I+1,P.EndlessRecords[I].Wave),14);
        } else
        Text(FString::Printf(TEXT("Reached room %d / 32. Best cleared room: %d. Victories: %d. Defeats: %d."),S->Room+1,P.BestRoom,P.Wins,P.Deaths));
        Body->AddSlot().AutoHeight()[SNew(STextBlock).Font(Font(16)).ColorAndOpacity(ASMenuGold).Text_Lambda([Weak](){auto* V=Weak.Get();auto* State=V?V->GetPlayerState<AASPlayerState>():nullptr;return Txt(State&&State->bSavePending?TEXT("Waiting for verified reward save..."):TEXT("Your reward save has been acknowledged."));})];
        Card(TEXT("Return together"),TEXT("The host returns both connected players to the same hub. This does not disconnect the party."),TEXT("RETURN TO HUB"),33,0,PS&&PS->PlayerIndex==0);
    } else if(Page==11) {
        Text(TEXT("Mouse wheel: zoom in / out. Q: switch shoulder. Tab: inspect acquired upgrades and equipped abilities."));
        KeyRow(TEXT("Build menu"),"Build");KeyRow(TEXT("Hammer former alt ability"),"HammerAlt");
        Text(TEXT("Move with WASD and look with the mouse. R reloads ranged weapons (left-stick click on controller). Melee weapons never reload. SPACE jumps; SHIFT spends available dash charges, including in the air. Hold Attack to chain swings: Cinder Blade lunges on hit three; Grav Hammer plants its feet for a frontal hit-three smash and can collapse its nearest well. With the hammer, RMB leaps (7.5s cooldown); G holds/releases Spiral. Hit-three wind-up commits you until recovery. Blade and ranged controls are unchanged. Hold Spiral and release in the gold timing window. F casts your equipped active ability; choose one at the Sanctum. Q swaps shoulders. Hold V within 3 meters of a downed ally for 2 seconds to revive, including after the room is cleared."));
        Text(TEXT("Controller defaults: left stick move, right stick look, RB attack, RT active ability, X Spiral, A jump, B dash, Y interact, LB revive, right-stick click shoulder, Select/View build, Start menu. Menu navigation uses D-pad / A / B."));
        Text(TEXT("Editor tip: if Esc stops your play test, change the editor's Stop shortcut in Edit > Editor Preferences > Keyboard Shortcuts. Search Stop and clear Escape or assign Shift+Escape."));
        Text(TEXT("Damage colors: burn orange, poison green, bleed pink, shock yellow, frost cyan, Spiral violet, physical white, health lost red, shield hits blue. Crits add a gold burst and CRIT label. Enemy bars stay visible for 3 seconds after damage, then fade through second 5."));
        Text(TEXT("Select a binding below, then press a key. Keyboard and gamepad mappings are stored separately; this view labels keyboard bindings. Do not assign one key to several actions."));
        KeyRow(TEXT("Move forward"),"Forward");KeyRow(TEXT("Move backward"),"Backward");KeyRow(TEXT("Move left"),"Left");KeyRow(TEXT("Move right"),"Right");
        KeyRow(TEXT("Reload ranged weapon"),"Reload");KeyRow(TEXT("Attack"),"Attack");KeyRow(TEXT("Spiral / Hammer leap"),"Spiral");KeyRow(TEXT("Active ability"),"Rune");KeyRow(TEXT("Jump"),"Jump");KeyRow(TEXT("Dash"),"Dash");KeyRow(TEXT("Interact / advance"),"Interact");KeyRow(TEXT("Revive ally (hold)"),"Revive");KeyRow(TEXT("Swap shoulder"),"Shoulder");KeyRow(TEXT("Performance overlay"),"Stats");KeyRow(TEXT("Re-lock gameplay mouse"),"RelockMouse");
        Card(TEXT("Restore movement defaults"),TEXT("Set keyboard Jump to Space and Dash to Shift; set controller Jump to A and Dash to B. Other bindings stay as configured."),TEXT("RESET JUMP/DASH"),44);
    } else if(Page==12) {
        Text(TEXT("This abandons the current expedition for both players. Cleared-room rewards already saved stay in each profile. Unfinished-room progress is not saved. The party remains connected."),20);
        Card(TEXT("Abandon expedition"),TEXT("Only the host can do this. Pending reward saves must finish before the game returns to the hub."),TEXT("CONFIRM RETURN"),33,0,PS&&PS->PlayerIndex==0);
        Card(TEXT("Keep playing"),TEXT("Close this panel and resume the expedition."),TEXT("CANCEL"),0);
    } else if(Page==13) {
        Text(TEXT("Leaving loads an offline hub. If you host, your partner loses the connection; there is no host migration. Already banked rewards remain."),20);
        Card(TEXT("Disconnect"),TEXT("This is separate from Return to Hub, which keeps the party together."),TEXT("CONFIRM LEAVE"),37);
        Card(TEXT("Stay connected"),TEXT("Return to play without ending this connection."),TEXT("CANCEL"),0);
    }
}

namespace {
int32 TraitIcon(int32 Index) {
    using ASBuild::Trait;using namespace ASEffectArt;
    switch(Trait(Index)) {
    case Trait::Magazine:return Magazine;case Trait::Reload:return Reload;case Trait::Precision:case Trait::Deadeye:return Precision;
    case Trait::Burn:return Flame;case Trait::Poison:case Trait::Root:return Venom;
    case Trait::Frost:case Trait::ChillAura:return Frost;case Trait::Shock:case Trait::Chain:return Bolt;
    case Trait::Split:case Trait::EchoShot:case Trait::Volley:case Trait::Ricochet:return Fork;
    case Trait::LongDash:case Trait::DashCharge:case Trait::DashStrike:case Trait::Momentum:case Trait::Grace:return Dash;
    case Trait::Gravity:case Trait::Area:case Trait::Orbit:case Trait::Refine:return Vortex;
    case Trait::ManaReserve:case Trait::ManaRegen:case Trait::Magic:case Trait::RuneNova:case Trait::Conduit:return Star;
    case Trait::Mend:case Trait::FullHeal:case Trait::Regrowth:case Trait::Leech:case Trait::Reaper:case Trait::MendingCoin:case Trait::BloodPact:return Heart;
    case Trait::Armor:case Trait::Barrier:case Trait::Defiance:case Trait::Phoenix:case Trait::Thorns:case Trait::Evasion:return Shield;
    case Trait::Reroll:case Trait::Fortune:case Trait::Mystery:case Trait::Greed:return Dice;
    case Trait::Ranged:case Trait::Pierce:return Bow;
    default:return Blade;
    }
}
FString ShortEffect(const FString& Detail) {
    FString First;Detail.Split(TEXT("."),&First,nullptr);if(First.IsEmpty()) First=Detail;
    if(First.Len()>78) {First=First.Left(75);int32 Space;if(First.FindLastChar(' ',Space)) First=First.Left(Space);First+=TEXT("...");}
    return First;
}
}
TSharedRef<SWidget> SASMenu::DetailStrip() {
    return SNew(SBox).MinDesiredHeight(66)
        [SNew(STextBlock).Font(Font(15)).ColorAndOpacity(FLinearColor(.89f,.94f,.95f)).WrapTextAt(Page==9?1100.f:740.f)
            .Text_Lambda([this](){return Txt(!HoverDetails.IsEmpty()?HoverDetails:TEXT("Hover a card for exact values and conditions."));})];
}
TSharedRef<SWidget> SASMenu::ChoiceTile(const FString& Title,const FString& Brief,const FString& Detail,const FString& Type,int32 Icon,int32 Rarity,const FString& Label,int32 Code,int32 Index,bool Enabled,float TileWidth) {
    const auto Weak=PC;
    const FLinearColor Rim=Rarity==2?FLinearColor(1.f,.67f,.24f):(Rarity==1?FLinearColor(.63f,.48f,1.f):FLinearColor(.46f,.73f,.77f));
    const bool Skill=Type.Contains(TEXT("SKILL"))||Type.Contains(TEXT("STYLE"));
    const FString Grade=Rarity==2?TEXT("LEGENDARY"):(Rarity==1?TEXT("RARE"):TEXT("COMMON"));
    const FString Full=Title+TEXT(" / ")+Type+TEXT(" / ")+Grade+TEXT("\n")+Detail;
    const FLinearColor Base=Skill?FLinearColor(.025f,.065f,.085f,.98f):FLinearColor(.018f,.027f,.043f,.98f);
    const auto Surface=SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Base).Padding(12)
        [SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(Type+TEXT(" / ")+Grade)).Font(Font(12,true)).ColorAndOpacity(Rim).WrapTextAt(TileWidth-30)]
            +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,4)[SNew(SBox).HeightOverride(82)[SNew(SASEffectIcon).Kind(Icon).Color(Rim).Size(82)]]
            +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(Title)).Font(Font(18,true)).ColorAndOpacity(FLinearColor::White).WrapTextAt(TileWidth-32)]
            +SVerticalBox::Slot().AutoHeight().Padding(0,7,0,10)[SNew(STextBlock).Text(Txt(Brief)).Font(Font(14)).ColorAndOpacity(ASMenuMuted).WrapTextAt(TileWidth-28)]
            +SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Bottom)[SNew(STextBlock).Text(Txt(Label)).Font(Font(13,true)).ColorAndOpacity(Rim).Justification(ETextJustify::Right)]];
    return SNew(SBox).WidthOverride(TileWidth).MinDesiredHeight(368)
        [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Rim).Padding(Skill?3.f:1.f)
            [SNew(SButton).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
                .ContentPadding(0).HAlign(HAlign_Fill).VAlign(VAlign_Fill)
                .OnHovered_Lambda([this,Full,Surface](){HoverDetails=Full;Surface->SetBorderBackgroundColor(TAttribute<FSlateColor>(FSlateColor(FLinearColor(.075f,.12f,.16f,.98f))));})
                .OnUnhovered_Lambda([this,Surface,Base](){HoverDetails.Empty();Surface->SetBorderBackgroundColor(TAttribute<FSlateColor>(FSlateColor(Base)));})
                .OnClicked_Lambda([Weak,Code,Index,Enabled](){if(Enabled) if(auto* Controller=Weak.Get()) Controller->Action(Code,Index);return FReply::Handled();})
                [Surface]]];
}
void SASMenu::RewardScreen() {
    auto* Controller=PC.Get();auto* State=Controller?AS::State(Controller):nullptr;
    auto* Me=Controller?Controller->GetPlayerState<AASPlayerState>():nullptr;
    auto* Hero=Controller?Cast<AASCharacter>(Controller->GetPawn()):nullptr;
    const auto Weak=PC;
    TSharedPtr<SWrapBox> Choices;
    ChildSlot
    [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0,0,0,.32f)).Padding(22)
        [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly).HAlign(HAlign_Center).VAlign(VAlign_Center)
            [SNew(SBox).WidthOverride(1120).HeightOverride(690)
                [SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,18)
                        [SNew(SHorizontalBox)
                            +SHorizontalBox::Slot().FillWidth(1)[SNew(SVerticalBox)
                                +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("CLAIM YOUR REWARD"))).Font(Font(29,true)).ColorAndOpacity(FLinearColor::White)]
                                +SVerticalBox::Slot().AutoHeight().Padding(0,5)[SNew(STextBlock).Text(Txt(TEXT("Choose one. Shape the next fight."))).Font(Font(15)).ColorAndOpacity(ASMenuGold)]]
                            +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("CLOSE / ESC"),0)]]
                    +SVerticalBox::Slot().AutoHeight()[SAssignNew(Choices,SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(12,12))]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)[DetailStrip()]
                    +SVerticalBox::Slot().FillHeight(1)
                        [SNew(SHorizontalBox)
                            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                                [SNew(SBox).IsEnabled(Hero&&Hero->Rerolls>0&&Me&&!Me->bRewardClaimed)[Button(Hero?FString::Printf(TEXT("REROLL / %d"),Hero->Rerolls):TEXT("REROLL"),60)]]
                            +SHorizontalBox::Slot().FillWidth(1).Padding(22,0).VAlign(VAlign_Center)
                                [SNew(SCheckBox).IsEnabled(State&&State->Room<31&&Hero&&!Hero->bDowned)
                                    .IsChecked_Lambda([Weak](){auto* C=Weak.Get();auto* GI=C?C->GetGameInstance<UASGameInstance>():nullptr;return GI&&GI->bRaiseStakes?ECheckBoxState::Checked:ECheckBoxState::Unchecked;})
                                    .OnCheckStateChanged_Lambda([Weak](ECheckBoxState){if(auto* C=Weak.Get()) C->Action(61);})
                                    [SNew(SHorizontalBox)
                                        +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SASEffectIcon).Kind(ASEffectArt::Dice).Color(ASMenuGold).Size(46)]
                                        +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(9,0)[SNew(SVerticalBox)
                                            +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("RAISE THE STAKES"))).Font(Font(17,true)).ColorAndOpacity(ASMenuGold)]
                                            +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Txt(TEXT("Harder chamber. Better rare rewards. Stays checked."))).Font(Font(13)).ColorAndOpacity(ASMenuMuted)]]]]
                            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Font(Font(15,true)).ColorAndOpacity(ASMenuGold)
                                .Text_Lambda([Weak](){auto* C=Weak.Get();auto* S=C?AS::State(C):nullptr;return Txt(S?FString::Printf(TEXT("%d CINDERS"),S->Cinders):TEXT(""));})]]
                    +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(Font(13)).ColorAndOpacity(ASMenuMuted).WrapTextAt(1100)
                        .Text_Lambda([Weak](){auto* C=Weak.Get();auto* S=C?AS::State(C):nullptr;if(!S) return Txt(TEXT(""));int32 Votes=0;for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) if(PS->bWagerVote) ++Votes;
                            const int32 Tier=ASBuild::ChallengeTier(S->Challenge,S->PlayerArray.Num(),Votes);
                            return Txt(FString::Printf(TEXT("%d/%d STAKES VOTES   /   NEXT CHAMBER: %s\nPer tier: +20%% enemy HP, +12%% damage, +3 Ash, +15 Cinders. After clearing: rare weight +12%%, legendary +20%%. Both players must opt in."),Votes,S->PlayerArray.Num(),Tier>0?*FString::Printf(TEXT("TIER %d"),Tier):TEXT("NORMAL")));})]]]]];
    if(!State||!Me||!Hero||State->Phase!=EASPhase::Reward) return;
    if(Me->bRewardClaimed) {Choices->AddSlot()[SNew(STextBlock).Text(Txt(TEXT("REWARD CLAIMED / Close and use E at the chest when both players are ready."))).Font(Font(22)).ColorAndOpacity(ASMenuGold)];return;}
    for(int32 Slot=0;Slot<Me->Offers.Num();++Slot) {
        const int32 Offer=Me->Offers[Slot];const bool Style=State->ChestKind==2;
        FString Name,Brief,Detail,Type=TEXT("TRAIT");int32 Icon=ASEffectArt::Star,Rarity=0;
        bool Enabled=!Hero->bDowned&&(State->ChestKind!=4||Slot==0||State->Cinders>=25);
        if(ASBuild::IsTrait(Offer)) {
            const int32 Id=Offer-ASBuild::OfferBase;const auto& D=ASBuild::Catalog[Id];
            Name=UTF8_TO_TCHAR(D.Name);Detail=UTF8_TO_TCHAR(D.Description);Brief=UTF8_TO_TCHAR(ASBuild::Briefs[Id]);Icon=TraitIcon(Id);Rarity=D.Rarity;Enabled&=Hero->CanApplyChoice(Offer);
            Detail+=FString::Printf(TEXT("\nRank %d -> %d / %d."),Hero->TraitRank(ASBuild::Trait(Id)),Hero->TraitRank(ASBuild::Trait(Id))+1,D.Max);
            if(Id==int32(ASBuild::Trait::Magazine)) {Brief=TEXT("More attacks per reload");Detail+=FString::Printf(TEXT(" Current weapon: %d -> %d capacity."),Hero->MagazineCapacity(),ASArsenal::Capacity(Hero->Build.Weapon,Hero->TraitRank(ASBuild::Trait::Magazine)+1));}
            if(Id==int32(ASBuild::Trait::Reload)) {Brief=TEXT("Reload and recharge faster");Detail+=FString::Printf(TEXT(" Current weapon: %.2f -> %.2f seconds."),Hero->ReloadDuration(),ASArsenal::ReloadSeconds(Hero->Build.Weapon,Hero->TraitRank(ASBuild::Trait::Reload)+1));}
            if(Id==int32(ASBuild::Trait::Precision)) Brief=TEXT("Tighter pellets and bonus shots");
            if(Id==int32(ASBuild::Trait::Refine)) {const int32 Target=Hero->EvolutionTarget();if(Target>=0) Detail+=FString::Printf(TEXT(" Evolves %s to effective rank %d."),UTF8_TO_TCHAR(ASBuild::Catalog[Target].Name),Hero->TraitValue(ASBuild::Trait(Target))+1);}
            if(Id==int32(ASBuild::Trait::Duplicate)) {const int32 Last=Hero->LastUpgrade;Detail+=TEXT(" Repeats: ");Detail+=ASBuild::IsTrait(Last)?UTF8_TO_TCHAR(ASBuild::Catalog[Last-200].Name):(ASRules::IsRunUpgradeOffer(Last)?ASData::RunUpgrades[Last-100].Name:TEXT("Unavailable"));}
        } else if(ASRules::IsRunUpgradeOffer(Offer)) {
            const int32 Id=Offer-100;const int32 Icons[]={ASEffectArt::Reload,ASEffectArt::Bolt,ASEffectArt::Blade,ASEffectArt::Heart,ASEffectArt::Dash,ASEffectArt::Precision};
            const TCHAR* Summaries[]={TEXT("Cast Spiral more often"),TEXT("Attack faster"),TEXT("Deal more weapon damage"),TEXT("Gain maximum health"),TEXT("Move faster"),TEXT("Land more critical hits")};
            Name=ASData::RunUpgrades[Id].Name;Brief=Summaries[Id];Detail=FString(ASData::RunUpgrades[Id].Desc)+TEXT("\n")+StatPreview(Id,Hero->Build,Hero->RunRanks,false,Hero->MaxHealth);Type=TEXT("CORE STAT");Icon=Icons[Id];
        } else if(ASExpedition::Exclusive(Offer)) {
            Name=UTF8_TO_TCHAR(ASExpedition::ExclusiveName(Offer));Detail=UTF8_TO_TCHAR(ASExpedition::ExclusiveDescription(Offer))+FString::Printf(TEXT("\nEndless exclusive. Stacks with diminishing returns after rank 5. Current rank: %d."),Hero->EndlessRanks.IsValidIndex(Offer-ASExpedition::ExclusiveBase)?Hero->EndlessRanks[Offer-ASExpedition::ExclusiveBase]:0);
            Brief=TEXT("Endless-only stacking power");Type=TEXT("ENDLESS");Icon=ASEffectArt::Heart;Rarity=2;
        } else if(Offer>=0&&Offer<(Style?10:24)) {
            Name=Style?ASData::Styles[Offer].Name:ASData::Relics[Offer].Name;Detail=Style?ASData::Styles[Offer].Desc:ASData::Relics[Offer].Desc;
            Brief=ShortEffect(Detail);Type=Style?TEXT("SPIRAL STYLE"):TEXT("RELIC");Icon=Style?ASEffectArt::Vortex:ASEffectArt::Star;Rarity=1;
        } else continue;
        Choices->AddSlot()[ChoiceTile(Name,Brief,Detail,Type,Icon,Rarity,State->ChestKind==4&&Slot>0?TEXT("25 CINDERS"):TEXT("SELECT"),32,Slot,Enabled)];
    }
}
