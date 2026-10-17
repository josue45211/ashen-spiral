#include "ASHUD.h"
#include "ASRouteGuide.h"
#include "ASCharacter.h"
#include "ASMovementComponent.h"
#include "ASPlayerController.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "ASEnemy.h"
#include "ASSpell.h"
#include "ASGameInstance.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Canvas.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElementTypes.h"
#include "Engine/GameViewportClient.h"
#include "Styling/CoreStyle.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "ASRules.h"
namespace {const FLinearColor ASHudPurple(.72f,.24f,1.f),ASHudGold(1,.65f,.22f),ASHudTeal(.15f,.87f,.74f),ASHudMuted(.70f,.75f,.80f),ASHudRed(1,.15f,.16f);}
// Keep all dynamic text in Slate. Canvas remains responsible for bars/lines only.
class SASHUDText : public SLeafWidget {
public:
    SLATE_BEGIN_ARGS(SASHUDText) {} SLATE_ARGUMENT(TWeakObjectPtr<AASHUD>, HUD) SLATE_END_ARGS()
    void Construct(const FArguments& Args) {HUD=Args._HUD;SetVisibility(EVisibility::HitTestInvisible);ForceVolatile(true);}
    virtual FVector2D ComputeDesiredSize(float) const override {return FVector2D::ZeroVector;}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override {
        const auto* H=HUD.Get();if(!H||H->OutputSize.X<=0||H->OutputSize.Y<=0) return Layer;
        const FVector2D Ratio=FVector2D(G.GetLocalSize())/H->OutputSize;
        for(const auto& L:H->Labels) {
            const FVector2D At=L.Position*Ratio;
            const auto Font=FCoreStyle::GetDefaultFontStyle("Regular",FMath::Max(1,FMath::RoundToInt(L.Pixels*Ratio.Y)));
            FSlateDrawElement::MakeText(Out,Layer,G.ToPaintGeometry(G.GetLocalSize(),FSlateLayoutTransform(At+FVector2D(1,1))),L.Value,Font,ESlateDrawEffect::None,FLinearColor(0,0,0,L.Color.A));
            FSlateDrawElement::MakeText(Out,Layer+1,G.ToPaintGeometry(G.GetLocalSize(),FSlateLayoutTransform(At)),L.Value,Font,ESlateDrawEffect::None,L.Color);
        }
        return Layer+2;
    }
private:TWeakObjectPtr<AASHUD> HUD;
};
void AASHUD::BeginPlay() {
    Super::BeginPlay();if(auto* PC=Cast<AASPlayerController>(PlayerOwner)) if(auto* Viewport=PC->LocalViewport()) {
        TextViewport=Viewport;SAssignNew(TextLayer,SASHUDText).HUD(this);Viewport->AddViewportWidgetContent(TextLayer.ToSharedRef(),5);
    }
}
void AASHUD::EndPlay(const EEndPlayReason::Type Reason) {
    if(IsValid(RouteGuide)) RouteGuide->Destroy();
    if(TextLayer.IsValid()&&TextViewport.IsValid()) TextViewport->RemoveViewportWidgetContent(TextLayer.ToSharedRef());TextLayer.Reset();Super::EndPlay(Reason);
}
void AASHUD::Text(const FString& V,float X,float Y,FLinearColor C,float Size) {
    const auto* GI=GetGameInstance<UASGameInstance>();
    const int32 Pixels=FMath::Max(1,FMath::RoundToInt(FMath::Max(14.f,Size*20.f)*Scale*GroupScale*(GI?GI->HUDTextScale:1.f)));
    const FVector2D At=(GroupOrigin+FVector2D(X,Y)*GroupScale)*Scale;
    Labels.Add({V,FVector2D(FMath::RoundToFloat(At.X),FMath::RoundToFloat(At.Y)),C,Pixels});
}
void AASHUD::Box(float X,float Y,float W,float H,FLinearColor C) {DrawRect(C,(GroupOrigin.X+X*GroupScale)*Scale,(GroupOrigin.Y+Y*GroupScale)*Scale,W*GroupScale*Scale,H*GroupScale*Scale);}
void AASHUD::Line(float X,float Y,float X2,float Y2,FLinearColor C,float Thick) {DrawLine((GroupOrigin.X+X*GroupScale)*Scale,(GroupOrigin.Y+Y*GroupScale)*Scale,(GroupOrigin.X+X2*GroupScale)*Scale,(GroupOrigin.Y+Y2*GroupScale)*Scale,C,Thick*GroupScale*Scale);}
void AASHUD::Bar(float X,float Y,float W,float H,float F,FLinearColor C) {Box(X,Y,W,H,FLinearColor(.06f,.075f,.1f,.9f));Box(X,Y,W*FMath::Clamp(F,0.f,1.f),H,C);}
void AASHUD::Map(AASGameState* S,AASCharacter* Hero) {
    if(!S->Arena) return;auto* A=S->Arena.Get();const float Size=176,Left=8,Top=8,MidX=Left+Size/2,MidY=Top+Size/2;
    
    float WorldSpan=4100;FVector MapCenter;A->MapBounds(MapCenter,WorldSpan);
    auto XY=[&](const FVector& P){return FVector2D(MidX+(P.Y-MapCenter.Y)/WorldSpan*Size,MidY-(P.X-MapCenter.X)/WorldSpan*Size);};
    if(S->Phase==EASPhase::Hub) {
        Box(MidX-73,MidY-73,146,146,FLinearColor(.12f,.16f,.19f,.85f));
        for(const auto& Station:A->Stations) {const FVector2D P=XY(Station.Position);Box(P.X-3,P.Y-3,6,6,ASHudGold);}
    } else {
        for(auto E:A->Layout.Edges) {
            const FVector2D P=XY(A->CellPosition(E.A)),Q=XY(A->CellPosition(E.B));
            Line(P.X,P.Y,Q.X,Q.Y,E.Gap?ASHudGold:FLinearColor(.26f,.33f,.38f),E.Gap?1.4f:3.f);
        }
        for(int32 I=0;I<int32(A->Layout.Cells.size());++I) {auto P=XY(A->CellPosition(I));const auto Half=A->CellHalfSize(I);const float W=Half.Y*2/WorldSpan*Size,H=Half.X*2/WorldSpan*Size;Box(P.X-W*.5f,P.Y-H*.5f,W,H,FLinearColor(.16f,.20f,.24f,.97f));}
        auto Chest=XY(A->ExitPoint());Box(Chest.X-4,Chest.Y-4,8,8,ASHudGold);
        int32 Index=0;for(const FVector& Warning:S->SpawnWarnings) {
            const auto P=XY(Warning);const bool Large=ASRules::Boss(S->Room)||(ASRules::Elite(S->Room)&&S->Wave==2&&Index==S->SpawnWarnings.Num()-1);++Index;
            const float R=Large?5.f:3.f;const float Pulse=.65f+.35f*FMath::Sin(S->SimTime*8.f);
            Box(P.X-R-2,P.Y-R-2,(R+2)*2,(R+2)*2,FLinearColor(1,.12f,.12f,Pulse*.5f));Box(P.X-R,P.Y-R,R*2,R*2,ASHudRed);
        }
        for(auto W:S->Enemies) if(auto* E=W.Get()) if(!E->bTrainingDummy&&E->Health>0) {
            const auto P=XY(E->GetActorLocation());const float R=E->Boss()?5.f:(E->Elite()?4.f:2.4f);
            Box(P.X-R,P.Y-R,R*2,R*2,ASHudRed);
        }
    }
    for(auto W:S->Heroes) if(auto* H=W.Get()) {
        const auto P=XY(H->GetActorLocation());const auto C=ASData::MapPlayerColor(H->ColorIndex);
        Box(P.X-5,P.Y-5,10,10,FLinearColor::Black);Box(P.X-3,P.Y-3,6,6,C);
        if(H->bDowned) {Line(P.X-6,P.Y,P.X+6,P.Y,C,2);Line(P.X,P.Y-6,P.X,P.Y+6,C,2);}const FVector Dir=H->GetActorForwardVector();
        Line(P.X,P.Y,P.X+Dir.Y*10,P.Y-Dir.X*10,C,2.f);
    }
    Text(S->Phase==EASPhase::Hub?TEXT("EMBER NEXUS"):FString::Printf(TEXT("%d ROOMS"),int32(A->Layout.Cells.size())),12,192,ASHudMuted,.65f);
    Box(159,197,5,5,ASData::MapPlayerColor(0));Box(173,197,5,5,ASData::MapPlayerColor(1));
}
void AASHUD::DrawHUD() {
    Labels.Reset();Super::DrawHUD();auto* PC=Cast<AASPlayerController>(PlayerOwner);auto* S=AS::State(this);auto* H=PC?Cast<AASCharacter>(PC->GetPawn()):nullptr;
    if(!Canvas||!PC||!S||!H) return;
    // HUD ownership can arrive after BeginPlay on a joining client.
    if(!TextLayer.IsValid()&&PC->IsLocalController()) if(auto* Viewport=PC->LocalViewport()) {
        TextViewport=Viewport;SAssignNew(TextLayer,SASHUDText).HUD(this);Viewport->AddViewportWidgetContent(TextLayer.ToSharedRef(),5);
    }
    OutputSize=FVector2D(Canvas->ClipX,Canvas->ClipY);
    Scale=FMath::Max(.55f,FMath::Min(Canvas->ClipX/1920.f,Canvas->ClipY/1080.f));Width=Canvas->ClipX/Scale;Height=Canvas->ClipY/Scale;
    SmoothedDelta=FMath::Lerp(SmoothedDelta,GetWorld()->GetDeltaSeconds(),.08f);
    Group();
    if(!IsValid(RouteGuide)&&PC->IsLocalController()) {RouteGuide=GetWorld()->SpawnActor<AASRouteGuide>();if(RouteGuide) RouteGuide->SetOwner(PC);}
    {
        // Entering a run: the chosen portal's colour floods the screen and fades out over the first seconds in chamber 1.
        const float Time=GetWorld()->GetTimeSeconds();const int32 PhaseNow=int32(S->Phase);
        if(LastHudPhase==int32(EASPhase::Hub)&&S->Phase==EASPhase::Countdown) {TransitionStart=Time;TransitionTier=S->Difficulty;bTransitionEndless=S->bEndless;}
        LastHudPhase=PhaseNow;
        const int32 StageNow=ASExpedition::EndlessSkyStage(S->bEndless,S->Phase!=EASPhase::Hub,S->EndlessWave);
        if(StageNow==2&&LastHudStage<2) StageFlashStart=Time;
        LastHudStage=StageNow;
        const float Age=Time-TransitionStart;
        if(Age>=0.f&&Age<2.2f) {
            const FLinearColor Tint=bTransitionEndless?FLinearColor(.45f,.02f,.02f):(TransitionTier==1?FLinearColor(.80f,.82f,.86f):(TransitionTier==2?FLinearColor(.40f,.01f,.02f):FLinearColor(1.f,.45f,.08f)));
            const float Alpha=Age<.35f?1.f:FMath::Pow(1.f-(Age-.35f)/1.85f,1.6f);
            Group();Box(0,0,Width,Height,FLinearColor(Tint.R,Tint.G,Tint.B,Alpha));
        }
        const float Flash=Time-StageFlashStart;
        if(Flash>=0.f&&Flash<2.5f) {Group();Box(0,0,Width,Height,FLinearColor(.8f,.02f,.02f,.55f*FMath::Pow(1.f-Flash/2.5f,1.4f)));Text(TEXT("THE SKY BLEEDS"),Width*.5f-130,Height*.28f,FLinearColor(1.f,.25f,.2f,1.f-Flash/2.5f),1.4f);}
    }
    if(PC->MenuOpen()) return;
    const auto* Preferences=PC->GetGameInstance<UASGameInstance>();
    const float UI=Preferences?ASPresentation::HUDScale(Preferences->HUDScale):1.f;
    const float Delta=GetWorld()->GetDeltaSeconds(),Now=GetWorld()->GetTimeSeconds();
    Group(24,24,UI);Box(0,0,2,82,ASHudTeal);
    Text(S->Phase==EASPhase::Hub?TEXT("EMBER NEXUS"):ASData::Biomes[ASAdventure::Biome(S->Room)],14,10,ASHudGold,.83f);
    Text(S->Phase==EASPhase::Hub?TEXT("EXPEDITION HUB"):(S->bEndless?FString::Printf(TEXT("ENDLESS     WAVE %d     CLEARED %d     ENEMIES %d"),S->EndlessWave,S->ClearedEndlessWave,S->LivingEnemies+S->PendingSpawns):FString::Printf(TEXT("CHAMBER %02d / 32     WAVE %d / %d"),S->Room+1,S->Wave+1,ASRules::WaveCount(S->Room))),14,36,ASHudMuted,.65f);
    if(S->Phase==EASPhase::Hub&&Preferences) Text(FString::Printf(TEXT("LEVEL %d    ASH %d    EMBERS %d"),ASCareer::Level(Preferences->Profile.Experience),Preferences->Profile.Ash,Preferences->Profile.RelicEmbers),14,58,ASHudMuted,.65f);
    else Text(FString::Printf(TEXT("%d FOES    %d CINDERS%s"),S->LivingEnemies+S->PendingSpawns,S->Cinders,S->Challenge>0?*FString::Printf(TEXT("    WAGER %d"),S->Challenge):TEXT("")),14,58,ASHudMuted,.65f);
    Group(Width-24-192*UI,24,UI);Map(S,H);Group();
    if(S->Phase==EASPhase::Hub&&Preferences) {
        const auto& Profile=Preferences->Profile;
        Group(Width-24-292*UI,24+229*UI,UI);
        Text(FString::Printf(TEXT("TABLETS %d"),Profile.Tablets),12,8,ASHudGold,.60f);
        Text(FString::Printf(TEXT("SKILL POINTS %d"),Profile.SkillPoints),12,30,ASHudGold,.60f);
        int32 Row=0;for(int32 I:{0,1,4}) Text(FString::Printf(TEXT("%s  %d"),ASData::Materials[I],Profile.Materials.IsValidIndex(I)?Profile.Materials[I]:0),12,58+Row++*19,ASHudMuted,.6f);
        Group();
    }
    const float HP=H->Health/FMath::Max(1.f,H->MaxHealth);
    TrailingHealth=HP>=TrailingHealth?HP:FMath::FInterpTo(TrailingHealth,HP,Delta,4.f);
    Group(24,Height-24-100*UI,UI);Box(0,0,2,100,ASHudTeal);
    Text(ASData::Weapons[H->Build.Weapon].Name,14,10,FLinearColor::White,.85f);
    if(ASMelee::IsMelee(H->Build.Weapon)) {
        const bool Swing=S->SimTime<H->AttackPoseStarted+H->AttackPoseDuration;
        if(ASMelee::ComboLength(H->Build.Weapon)==3) for(int32 I=0;I<3;++I) Box(14+I*29,46,22,4,Swing&&I==H->MeleeStep?(I==2?ASHudGold:ASHudTeal):ASHudMuted*.35f);
        Text(Swing&&H->MeleeStep==2?TEXT("FINISHER"):TEXT("MELEE"),110,38,ASHudMuted,.60f);
    } else Text(FString::Printf(TEXT("%02d / %02d  %s"),H->Ammo,H->MagazineCapacity(),H->IsReloading()?TEXT("RELOADING"):TEXT("")),14,38,H->IsReloading()?ASHudGold:ASHudMuted,.72f);
    if(H->IsReloading()) {Bar(14,-10,302,3,H->ReloadProgress(),ASHudGold);Text(FString::Printf(TEXT("%.1fs"),FMath::Max(0.f,H->ReloadEnd-S->SimTime)),263,-32,ASHudGold,.65f);}
    Text(FString::Printf(TEXT("%.0f / %.0f"),H->Health,H->MaxHealth),205,38,FLinearColor::White,.72f);
    Box(14,63,302,10,FLinearColor(.09f,.12f,.16f,.95f));Box(14,63,302*TrailingHealth,10,FLinearColor(.94f,.39f,.28f,.72f));Box(14,63,302*HP,10,ASHudTeal);
    if(H->Shield>0) {Bar(14,82,235,4,H->Shield/FMath::Max(60.f,H->MaxHealth),ASData::HitColor(EASHitKind::Shield));Text(FString::Printf(TEXT("+%.0f"),H->Shield),266,76,ASData::HitColor(EASHitKind::Shield),.60f);}
    Group();
    if(S->SimTime-PC->LastHurtAt<.25f) {const float Alpha=(.25f-S->SimTime+PC->LastHurtAt)*2;Box(0,0,Width,4,FLinearColor(1,.15f,.2f,Alpha));Box(0,Height-4,Width,4,FLinearColor(1,.15f,.2f,Alpha));}
    const float Dash=H->Movement()->DashCooldown,Spiral=FMath::Max(0.f,H->NextSpiral-S->SimTime),Rune=FMath::Max(0.f,H->NextRune-S->SimTime);
    const auto Key=[PC](FName Binding) {const FKey Bound=PC->BoundKey(Binding);if(Bound==EKeys::RightMouseButton) return FString(TEXT("RMB"));if(Bound==EKeys::LeftMouseButton) return FString(TEXT("LMB"));return Bound.GetDisplayName().ToString().Replace(TEXT("Left "),TEXT("")).Replace(TEXT("Right "),TEXT("")).Left(6).ToUpper();};
    if(H->Build.Weapon==1) {
        const float Leap=FMath::Max(0.f,H->NextHammerLeap-S->SimTime);
        Group(Width-24-384*UI,Height-24-112*UI,UI);
        Text(FString::Printf(TEXT("%s  LEAP  %s"),*Key("Spiral"),Leap<=.02f?TEXT("READY"):*FString::Printf(TEXT("%.1fs"),Leap)),12,0,Leap<=.02f?ASHudGold:ASHudMuted,.62f);
        Bar(12,24,358,2,1.f-Leap/ASHammer::Cooldown,ASHudGold);Group();
    }
    Group(Width-24-384*UI,Height-24-76*UI,UI);
    for(int32 I=1;I<3;++I) Box(I*128,12,1,52,FLinearColor(.22f,.31f,.34f,.65f));
    Text(TEXT("DASH"),12,10,ASHudMuted,.65f);Text(Key("Dash"),68,10,ASHudGold,.6f);
    Text(Dash<=.02f?TEXT("READY"):FString::Printf(TEXT("%.1fs"),Dash),12,30,Dash<=.02f?ASHudGold:ASHudMuted,.85f);
    Text(FString::Printf(TEXT("%d / %d CHARGES"),H->Movement()->DashCharges,H->MaxDashCharges()),12,56,ASHudMuted,.55f);
    Text(TEXT("SPIRAL"),140,10,ASHudMuted,.65f);Text(Key(H->Build.Weapon==1?"HammerAlt":"Spiral"),206,10,ASHudTeal,.6f);
    Text(Spiral<=.02f?TEXT("READY"):FString::Printf(TEXT("%.1fs"),Spiral),140,30,Spiral<=.02f?ASHudTeal:ASHudMuted,.85f);
    Bar(140,61,102,3,1.f-Spiral/FMath::Max(.1f,H->SpiralCooldown()),ASHudTeal);
    Text(FString(ASData::Abilities[H->Build.Ability].Name).Left(14),268,10,ASHudMuted,.60f);
    Text(Rune<=.02f?FString::Printf(TEXT("[%s] READY"),*Key("Rune")):FString::Printf(TEXT("%.1fs"),Rune),268,30,Rune<=.02f?ASHudTeal:ASHudMuted,.75f);
    Bar(268,61,102,3,H->Focus/H->MaxFocus(),FLinearColor(.3f,.55f,1));Group();
    if(H->bCharging) {
        const float Held=S->SimTime-H->ChargeStart,Bonus=H->Rank(6)*.025f+(H->Relic(19)?.08f:0.f),Start=(.7f-Bonus)/1.4f,End=(.92f+Bonus)/1.4f;
        const bool Perfect=ASRules::Perfect(Held,H->Rank(6),H->Relic(19));
        Group(Width*.5f-160*UI,Height-210*UI,UI);
        Text(Perfect?TEXT("RELEASE / PERFECT SPIRAL"):TEXT("RELEASE IN THE GOLD WINDOW"),12,8,Perfect?ASHudGold:ASHudMuted,.66f);
        Box(12,37,296,5,FLinearColor(.18f,.22f,.26f));Box(12+Start*296,34,(End-Start)*296,11,ASHudGold);
        Box(12+FMath::Clamp(Held/1.4f,0.f,1.f)*292,31,3,17,FLinearColor::White);Group();
    }
    PC->Notifications.RemoveAll([Now](const FASNotice& Entry){return Now-Entry.Born>=ASPresentation::NoticeLife;});
    for(int32 I=PC->Notifications.Num()-1;I>=0;--I) {
        auto& Entry=PC->Notifications[I];const float Target=-84.f*(PC->Notifications.Num()-1-I);
        Entry.Offset=FMath::FInterpTo(Entry.Offset,Target,Delta,10.f);
        const float Alpha=ASPresentation::NoticeAlpha(Now-Entry.Born);
        Group(Width-24-440*UI,Height-24-280*UI+Entry.Offset*UI,UI);
        for(int32 Etch=0;Etch<5;++Etch) Line(float(Etch*12),3,float(Etch*12+7),-4,FLinearColor(1,.65f,.22f,Alpha*(.55f-.09f*Etch)),1.f);
        TArray<FString> Words;Entry.Message.ParseIntoArray(Words,TEXT(" "),true);FString Row;int32 RowIndex=0;
        const int32 Limit=FMath::FloorToInt(45.f/(Preferences?Preferences->HUDTextScale:1.f));
        for(const auto& Word:Words) {
            if(Row.Len()+Word.Len()>Limit&&!Row.IsEmpty()) {Text(Row,14,12+RowIndex*23,FLinearColor(1,.82f,.50f,Alpha),.70f);Row.Empty();if(++RowIndex>=3) break;}
            Row+=Word+TEXT(" ");
        }
        if(RowIndex<3&&!Row.IsEmpty()) Text(Row,14,12+RowIndex*23,FLinearColor(1,.82f,.50f,Alpha),.70f);
        Group();
    }
    const float CX=Width*.5f,CY=Height*.5f;
    if(RouteGuide&&RouteGuide->JumpAhead) {Group(CX-130*UI,Height-230*UI,UI);Text(TEXT("JUMP + AIR DASH / GOLD ARC"),0,0,ASHudGold,.68f);Group();}
    const auto* Settings=PC->GetGameInstance<UASGameInstance>();
    const float AimScale=Settings?Settings->CrosshairScale:1.f,R=10.f*AimScale,Gap=4.f*AimScale;
    const int32 Shape=Settings?Settings->CrosshairStyle:0;
    const FLinearColor AimColor=ASData::CrosshairColor(Settings?Settings->CrosshairColorIndex:0);
    const auto AimLine=[&](float X,float Y,float X2,float Y2){Line(X,Y,X2,Y2,FLinearColor::Black,4.f);Line(X,Y,X2,Y2,AimColor,2.f);};
    if(Shape==0||Shape==3) {
        AimLine(CX-R,CY,CX-Gap,CY);AimLine(CX+Gap,CY,CX+R,CY);
        AimLine(CX,CY-R,CX,CY-Gap);AimLine(CX,CY+Gap,CX,CY+R);
    }
    if(Shape==1||Shape==3) {const float Dot=2.f*AimScale;Box(CX-Dot-1,CY-Dot-1,Dot*2+2,Dot*2+2,FLinearColor::Black);Box(CX-Dot,CY-Dot,Dot*2,Dot*2,AimColor);}
    if(Shape==2) for(int32 I=0;I<32;++I) {
        const float A=float(I)*UE_TWO_PI/32.f,B=float(I+1)*UE_TWO_PI/32.f;
        AimLine(CX+FMath::Cos(A)*R,CY+FMath::Sin(A)*R,CX+FMath::Cos(B)*R,CY+FMath::Sin(B)*R);
    }
    if(PC->bAimBlocked) {
        FVector2D Impact;if(PC->ProjectWorldLocationToScreen(PC->ShotImpact,Impact)) {
            const float X=Impact.X/Scale,Y=Impact.Y/Scale;Line(X-7,Y-7,X+7,Y+7,ASHudRed,2);Line(X+7,Y-7,X-7,Y+7,ASHudRed,2);
        }
        Text(TEXT("COVER BLOCKS SHOT"),CX-95,CY+26,ASHudGold,.65f);
    }
    FString Objective;
    if(S->Phase==EASPhase::Countdown) Objective=FString::Printf(TEXT("WAVE IN %.0f / WATCH RED MARKERS"),FMath::CeilToFloat(S->CountdownEnd-S->SimTime));
    else if(S->Phase==EASPhase::Victory) Objective=TEXT("EXPEDITION COMPLETE / RETURN PORTAL");
    else if(S->Phase==EASPhase::Reward) Objective=TEXT("CLEARED / FOLLOW THE GOLD PATH");
    if(!Objective.IsEmpty()) {Group(CX-230*UI,24,UI);Text(Objective,14,10,ASHudGold,.68f);Group();}
    for(auto W:S->Enemies) if(auto* Enemy=W.Get()) if(Enemy->Boss()) {
        Group(CX-230*UI,24,UI);Text(ASData::Bosses[Enemy->Ward],14,8,ASHudGold,.8f);Bar(14,39,432,4,Enemy->Health/Enemy->MaxHealth,ASHudRed);Group();break;
    }
    float PartnerY=24+130*UI;for(auto W:S->Heroes) if(auto* Partner=W.Get()) if(Partner!=H) {
        Group(24,PartnerY,UI);
        Text(Partner->bDowned?TEXT("PARTNER DOWN / HOLD REVIVE"):TEXT("PARTNER"),12,8,Partner->bDowned?ASHudRed:ASHudTeal,.65f);
        Bar(12,35,220,4,Partner->Health/Partner->MaxHealth,ASHudTeal);Group();PartnerY+=60*UI;
        if(Partner->bDowned) {
            FVector CameraAt;FRotator CameraRotation;PC->GetPlayerViewPoint(CameraAt,CameraRotation);
            const bool Occluded=!AS::ClearSight(GetWorld(),CameraAt,Partner->GetActorLocation());
            if(Occluded) {
                TArray<FVector> Pose;Partner->DownedPose(Pose);TArray<FVector2D> Points;TArray<bool> Visible;
                for(const auto& Joint:Pose) {FVector2D Projected;Visible.Add(PC->ProjectWorldLocationToScreen(Joint,Projected));Points.Add(Projected/Scale);}
                const int32 Bones[][2]={{0,1},{1,2},{1,3},{3,4},{4,5},{1,6},{6,7},{7,8},{2,9},{9,10},{10,11},{2,12},{12,13},{13,14}};
                for(const auto& Bone:Bones) if(Points.IsValidIndex(Bone[1])&&Visible[Bone[0]]&&Visible[Bone[1]]) {
                    const auto A=Points[Bone[0]],B=Points[Bone[1]];Line(A.X,A.Y,B.X,B.Y,FLinearColor(.15f,0,0,.9f),10);Line(A.X,A.Y,B.X,B.Y,FLinearColor(1,.05f,.06f,.9f),6);
                }
                if(Points.Num()>0&&Visible[0]) Box(Points[0].X-6,Points[0].Y-6,12,12,ASHudRed);
            }
            FVector2D CrossPoint;const bool Front=PC->ProjectWorldLocationToScreen(Partner->GetActorLocation()+FVector(0,0,165),CrossPoint);
            if(Front) CrossPoint/=Scale;else {const FVector D=(Partner->GetActorLocation()-CameraAt).GetSafeNormal();CrossPoint=FVector2D(FVector::DotProduct(D,FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y))>0?Width-40:40,Height*.5f);}
            CrossPoint.X=FMath::Clamp(float(CrossPoint.X),40.f,Width-40);CrossPoint.Y=FMath::Clamp(float(CrossPoint.Y),50.f,Height-255);
            Box(CrossPoint.X-4,CrossPoint.Y-14,8,28,ASHudRed);Box(CrossPoint.X-14,CrossPoint.Y-4,28,8,ASHudRed);
            Text(FString::Printf(TEXT("%.0f m"),FVector::Dist(H->GetActorLocation(),Partner->GetActorLocation())/100.f),CrossPoint.X-19,CrossPoint.Y-40,ASHudRed,.65f);
            FVector2D P;if(PC->ProjectWorldLocationToScreen(Partner->GetActorLocation()+FVector(0,0,120),P)) {
            const bool CanHelp=H->CanRevive(Partner);
            Text(CanHelp?FString::Printf(TEXT("HOLD %s TO REVIVE"),*PC->BoundKey("Revive").GetDisplayName().ToString()):TEXT("MOVE CLOSER / CLEAR PATH TO ALLY"),P.X/Scale-125,P.Y/Scale,ASHudTeal,.66f);
            Bar(P.X/Scale-70,P.Y/Scale+24,140,7,Partner->ReviveProgress/ASRules::ReviveSeconds,ASHudTeal);
        }
        if(H->CanRevive(Partner)) {
            Group(CX-225*UI,Height*.62f,UI);
            Text(Partner->ReviveProgress>0?FString::Printf(TEXT("REVIVING ALLY  /  %.0f%%"),Partner->ReviveProgress/ASRules::ReviveSeconds*100.f):
                FString::Printf(TEXT("HOLD %s FOR 2 SECONDS TO REVIVE"),*PC->BoundKey("Revive").GetDisplayName().ToString()),14,12,ASHudTeal,.72f);
            Bar(14,50,422,5,Partner->ReviveProgress/ASRules::ReviveSeconds,ASHudTeal);Group();
        }}
    }
    if(H->bDowned) {
        Group(CX-255*UI,Height*.5f+100*UI,UI);
        Text(H->ReviveProgress>0?TEXT("YOUR PARTNER IS REVIVING YOU"):TEXT("DOWNED  /  YOUR PARTNER CAN REVIVE YOU"),14,12,H->ReviveProgress>0?ASHudTeal:ASHudRed,.8f);
        Bar(14,55,482,5,H->ReviveProgress/ASRules::ReviveSeconds,ASHudTeal);Group();
    }
    // Endless bleed-out bars above downed players, visible to both. Reviving pauses the bar (it does not reset).
    if(S->bEndless) for(auto W:S->Heroes) if(auto* Down=W.Get()) if(Down->bDowned||Down->bEliminated) {
        FVector2D P;if(PC->ProjectWorldLocationToScreen(Down->GetActorLocation()+FVector(0,0,185),P)) {
            const float X=P.X/Scale,Y=P.Y/Scale;
            if(Down->bEliminated) Text(TEXT("ELIMINATED"),X-45,Y-18,ASHudRed,.7f);
            else {Text(FString::Printf(TEXT("BLEEDING OUT  %.0f"),Down->BleedRemaining),X-58,Y-18,ASHudRed,.65f);Bar(X-60,Y,120,7,Down->BleedRemaining/25.f,ASHudRed);}
        }
    }
    FString Station;const int32 Near=S->Arena?S->Arena->StationAt(H->GetActorLocation(),&Station):-1;
    const int32 StationId=PC->FocusedInteraction>0?PC->FocusedInteraction:Near;
    if(S->Arena&&StationId>0&&(S->Phase==EASPhase::Hub||S->Phase==EASPhase::Reward||S->Phase==EASPhase::Victory||StationId>=200)) {
        const FVector At=S->Arena->InteractionPoint(StationId);
        const bool Reach=ASAdventure::InReach(FVector::DistSquared2D(H->GetActorLocation(),At),H->GetActorLocation().Z-At.Z);
        bool Done=false;
        if(StationId==201||StationId==202) {Done=(S->VaultMask&(1<<(StationId-201)))!=0;Station=Done?TEXT("SIGIL ACTIVATED"):TEXT("ACTIVATE SIGIL");}
        else if(StationId==203) {Done=S->bVaultClaimed;Station=Done?TEXT("VAULT COLLECTED"):(S->VaultMask==3?TEXT("OPEN RUNE VAULT"):TEXT("VAULT / ACTIVATE BOTH SIGILS"));}
        else if(StationId==220) Station=TEXT("EMBER BARGAIN / TRADE HEALTH");
        else if(StationId>=31&&StationId<=33) {
            const int32 D=StationId-31;const auto* Local=PC->GetPlayerState<AASPlayerState>();
            const bool Open=ASExpedition::DifficultyUnlocked(D,Local?Local->ClearedMask:0);
            Station=FString::Printf(TEXT("%s PORTAL / %s%s / %s%s"),UTF8_TO_TCHAR(ASExpedition::PortalName(D)),UTF8_TO_TCHAR(ASExpedition::PortalDifficultyName(D)),S->Difficulty==D?TEXT(" [CHOSEN]"):TEXT(""),UTF8_TO_TCHAR(ASExpedition::PortalModifiers(D)),Open?TEXT(""):*(FString(TEXT(" / LOCKED: "))+UTF8_TO_TCHAR(ASExpedition::UnlockCondition(D))));
        }
        else if(StationId==222) Station=S->bLuckyShop?TEXT("LUCKY SHOP / SPEND SCRAP AND ASH"):TEXT("LUCKY SHOP / CLOSED");
        else if(StationId==221) Station=TEXT("WISHING WELL / STAKE CINDERS");
        else if(StationId==9) Station=S->Phase==EASPhase::Victory?TEXT("RETURN TO HUB / BOTH PLAYERS"):TEXT("REWARD CHEST / NEXT ROOM");
        else if(StationId>=100&&StationId<100+ASRules::WeaponCount) Station=ASData::Weapons[StationId-100].Name;
        else for(const auto& Item:S->Arena->Stations) if(Item.Menu==StationId) Station=Item.Name.Replace(TEXT("\n"),TEXT(" / "));
        const auto Color=Done?ASHudTeal:(Reach?ASHudGold:ASHudMuted);FVector2D Mark;
        if(PC->ProjectWorldLocationToScreen(At,Mark)) {
            const float X=Mark.X/Scale,Y=Mark.Y/Scale,MarkerRadius=PC->FocusedInteraction==StationId?30.f:22.f;
            for(int32 Sign:{-1,1}) {Line(X+Sign*MarkerRadius,Y-MarkerRadius,X+Sign*MarkerRadius,Y-MarkerRadius+10,Color,2);Line(X+Sign*MarkerRadius,Y+MarkerRadius-10,X+Sign*MarkerRadius,Y+MarkerRadius,Color,2);Line(X+Sign*(MarkerRadius-10),Y-MarkerRadius,X+Sign*MarkerRadius,Y-MarkerRadius,Color,2);Line(X+Sign*(MarkerRadius-10),Y+MarkerRadius,X+Sign*MarkerRadius,Y+MarkerRadius,Color,2);}
        }
        Group(CX-260*UI,Height-24-130*UI,UI);
        Text(Done?Station:FString::Printf(TEXT("%s  %s"),Reach?*FString::Printf(TEXT("[%s]"),*Key("Interact")):TEXT("MOVE CLOSER"),*Station),14,10,Color,.7f);Group();
    }
    if(H->Movement()->IsFalling()) {
        const int32 Left=FMath::Min(H->Movement()->DashCharges,FMath::Max(0,H->MaxDashCharges()-H->Movement()->AirDashesUsed));
        Group(CX-150*UI,Height-58*UI,UI);Text(Left>0?FString::Printf(TEXT("AIR DASH %d / %d"),Left,H->MaxDashCharges()):TEXT("AIR DASH EMPTY / LAND TO RESET"),0,0,ASHudMuted,.65f);Group();
    }
    for(auto W:S->Enemies) if(auto* E=W.Get()) if(!E->Boss()) {
        const float Alpha=E->bTrainingDummy?1.f:ASRules::HealthBarAlpha(S->SimTime-E->LastDamageTime);
        if(Alpha<=0||FVector::DistSquared2D(H->GetActorLocation(),E->GetActorLocation())>4000*4000) continue;
        FVector CameraAt;FRotator CameraRotation;PC->GetPlayerViewPoint(CameraAt,CameraRotation);
        const FVector Top=E->GetActorLocation()+FVector(0,0,E->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+22);
        FVector2D P;if(!PC->ProjectWorldLocationToScreen(Top,P)||!AS::ClearSight(GetWorld(),CameraAt,Top)) continue;
        const float X=P.X/Scale,Y=P.Y/Scale,WBar=E->Elite()?76.f:58.f;
        Box(X-WBar*.5f-2,Y-2,WBar+4,9,FLinearColor(.012f,.019f,.026f,Alpha*.9f));
        Box(X-WBar*.5f,Y,WBar*FMath::Clamp(E->Health/E->MaxHealth,0.f,1.f),5,FLinearColor(.95f,.30f,.32f,Alpha));
        for(int32 I=0;I<4;++I) if(E->StatusBits&(I==3?16:1<<I)) {auto Color=ASData::HitColor(I==3?EASHitKind::Bleed:EASHitKind(I+2));Color.A=Alpha;Box(X-9+I*7,Y+11,5,3,Color);}
        if(E->bTrainingDummy) Text(TEXT("PRACTICE / RESETS AFTER 5s"),X-80,Y-25,ASHudMuted,.48f);
    }
    for(const auto& N:PC->DamageNumbers) {
        const float Age=N.Life-(N.Expires-S->SimTime);const bool Incoming=N.Kind==EASHitKind::Received||N.Kind==EASHitKind::Shield;
        // Persona decides how the number moves and pops; colour comes from the element or the synergy type.
        using ASPresentation::NumberPersona;
        NumberPersona Persona=NumberPersona::Plain;
        if(!Incoming) {
            if(N.Synergy>=0) Persona=NumberPersona(int(NumberPersona::SynergyLightning)+FMath::Clamp(N.Synergy,0,2));
            else if(N.Critical) Persona=NumberPersona::Crit;
            else switch(N.Kind) {case EASHitKind::Burn:Persona=NumberPersona::Burn;break;case EASHitKind::Poison:Persona=NumberPersona::Poison;break;case EASHitKind::Frost:Persona=NumberPersona::Frost;break;
                case EASHitKind::Bleed:Persona=NumberPersona::Bleed;break;case EASHitKind::Shock:Persona=NumberPersona::Shock;break;case EASHitKind::Spiral:Persona=NumberPersona::Spiral;break;default:break;}
        }
        const auto Look=ASPresentation::DamageLookFor(Persona,Age,N.Life,N.Seed);
        const FLinearColor SynergyTint[3]={FLinearColor(.6f,.8f,1),FLinearColor(.12f,1,1),FLinearColor(1,.35f,.05f)};
        FLinearColor Color=N.Synergy>=0?SynergyTint[FMath::Clamp(N.Synergy,0,2)]:ASData::HitColor(N.Kind);
        FLinearColor Edge=Color*.35f;Edge.A=1.f;
        if(Persona==NumberPersona::Burn) Color=FMath::Lerp(FLinearColor(.55f,.05f,.03f),Color,Look.Heat);
        if(Persona==NumberPersona::Crit) {Edge=FLinearColor(1.f,.45f,.05f);Color=FMath::Lerp(FLinearColor(1.f,.8f,.25f),FLinearColor(1.f,1.f,.9f),Look.Flash);}
        if(Persona==NumberPersona::Frost) Color=FMath::Lerp(Color,FLinearColor(.92f,.98f,1.f),Look.Flash);
        if(Persona==NumberPersona::Shock&&Look.Flash>0.f) Color=FLinearColor(1.f,1.f,.85f);
        if(ASPresentation::IsSynergyPersona(Persona)) {Edge=Color;Color=FMath::Lerp(FLinearColor(1.f,1.f,1.f),Color,.35f+.65f*(1.f-Look.Flash));}
        const float Alpha=Look.Alpha;Color.A=Alpha;
        const auto* GI=PC->GetGameInstance<UASGameInstance>();const float Pop=(GI?GI->NumberScale:1.2f)*Look.Scale;
        // UE's checked Printf requires a literal format in each call.
        const FString Value=N.Value<1
            ? FString::Printf(TEXT("%s%.1f"),Incoming?TEXT("-"):TEXT(""),N.Value)
            : FString::Printf(TEXT("%s%.0f"),Incoming?TEXT("-"):TEXT(""),N.Value);
        const float TextHeight=FMath::Max(14.f,Pop*20.f)*(GI?GI->HUDTextScale:1.f)+8.f;
        const float X=CX+N.Drift*(1.f+Age*.3f)+Look.X-float(Value.Len())*TextHeight*.27f;
        const float Y=ASPresentation::DamageTop(CY,AimScale,TextHeight,0.f,N.Lane,Incoming)-Look.Rise+Look.Y;
        // Coloured halo first (wider for the loud personas), then the dark outline, then the face.
        const bool Loud=Persona==NumberPersona::Crit||ASPresentation::IsSynergyPersona(Persona);
        if(Loud) for(const FVector2D Offset:{FVector2D(-4,0),FVector2D(4,0),FVector2D(0,-4),FVector2D(0,4)}) {FLinearColor Halo=Edge;Halo.A=Alpha*.55f;Text(Value,X+Offset.X,Y+Offset.Y,Halo,Pop);}
        for(const FVector2D Offset:{FVector2D(-1.5,-1.5),FVector2D(1.5,-1.5),FVector2D(-1.5,1.5),FVector2D(1.5,1.5)}) {FLinearColor Outline=Persona==NumberPersona::Plain?FLinearColor(0,0,0,Alpha):FLinearColor(Edge.R*.5f,Edge.G*.5f,Edge.B*.5f,Alpha);Text(Value,X+Offset.X,Y+Offset.Y,Outline,Pop);}
        Text(Value,X,Y,Color,Pop);
        // Shockwave rings and shards: shape language only, no words.
        for(int32 RingIndex=0;RingIndex<Look.Rings;++RingIndex) {
            const float RT=FMath::Clamp(Look.RingT-RingIndex*.18f,0.f,1.f);if(RT<=0.f||RT>=1.f) continue;
            const float Radius=(18.f+RT*(Persona==NumberPersona::Frost?60.f:110.f))*AimScale,A=Alpha*(1.f-RT);const FLinearColor RingColor(Edge.R,Edge.G,Edge.B,A);
            const float CXn=X+float(Value.Len())*TextHeight*.27f,CYn=Y+TextHeight*.5f;
            for(int32 K=0;K<12;++K) {const float A0=K*UE_PI/6.f,A1=A0+UE_PI/14.f;Line(CXn+FMath::Cos(A0)*Radius,CYn+FMath::Sin(A0)*Radius*.6f,CXn+FMath::Cos(A1)*Radius,CYn+FMath::Sin(A1)*Radius*.6f,RingColor,2.f);}
        }
    }
    if(S->SimTime-PC->LastCritAt<.2f) {
        const auto C=FLinearColor(1,.78f,.24f);for(int32 I=0;I<4;++I) {const float A=UE_PI*.25f+I*UE_PI*.5f;Line(CX+FMath::Cos(A)*13*AimScale,CY+FMath::Sin(A)*13*AimScale,CX+FMath::Cos(A)*21*AimScale,CY+FMath::Sin(A)*21*AimScale,C,2);}
    }
    DrawWaveBanner(S);
    if(S->bEnteringPortal) {Box(0,0,Width,Height,FLinearColor(0,0,0,.16f));Text(TEXT("ENTERING THE SPIRAL"),CX-145,Height*.75f,ASHudGold,.9f);}
    if(S->Frozen()) {
        Group(CX-380*UI,CY-45*UI,UI);
        Text(S->bConnectionHold?FString::Printf(TEXT("PARTNER DISCONNECTED  /  REJOIN WINDOW %.0fs"),S->ReconnectRemaining):TEXT("PARTY PAUSED  /  PRESS ESC TO RESUME"),20,15,ASHudGold,.85f);
        if(S->bConnectionHold) Text(TEXT("Host: Escape > Continue alone, or wait for the same profile to rejoin."),20,50,ASHudMuted,.7f);Group();
    }
    if(auto* PS=PC->GetPlayerState<AASPlayerState>()) if(PS->bSavePending) Text(TEXT("VERIFYING REWARD SAVE..."),Width-290,Height-80,ASHudGold,.7f);
    if(PC->bShowStats) {
        Text(FString::Printf(TEXT("%.0f FPS  /  %.2f ms FRAME"),1.f/FMath::Max(.0001f,SmoothedDelta),SmoothedDelta*1000),40,PartnerY+26,ASHudGold,.8f);
        Text(FString::Printf(TEXT("Enemies %d/%d  |  Projectiles %d/%d"),S->Enemies.Num(),ASRules::MaxEnemies,S->Projectiles.Num(),ASRules::MaxProjectiles),40,PartnerY+57,ASHudMuted,.67f);
        Text(TEXT("F9 hide  /  console: stat unit"),40,PartnerY+82,ASHudMuted,.65f);
    }

}

void AASHUD::DrawWaveBanner(AASGameState* S) {
    if(!S) return;
    const float Now=S->SimTime;
    const bool Fighting=S->Phase==EASPhase::Combat;
    // Detect transitions from replicated state; the first frame only records the baseline.
    if(BannerPhase<0) {BannerRoom=S->Room;BannerWave=S->Wave;BannerEndlessWave=S->EndlessWave;BannerCleared=S->ClearedEndlessWave;BannerPhase=int32(S->Phase);}
    auto Fire=[&](const FString& Title,const FString& Sub,FLinearColor Color) {BannerStart=Now;BannerTitle=Title;BannerSub=Sub;BannerColor=Color;};
    if(S->bEndless) {
        if(S->EndlessWave!=BannerEndlessWave&&S->EndlessWave>0&&S->Phase!=EASPhase::Hub) {
            const bool Boss=ASExpedition::BossWave(S->EndlessWave);
            Fire(FString::Printf(TEXT("WAVE %d"),S->EndlessWave),Boss?TEXT("BOSS"):TEXT(""),Boss?FLinearColor(1.f,.25f,.2f):FLinearColor(1.f,.78f,.3f));
        } else if(S->ClearedEndlessWave!=BannerCleared&&S->ClearedEndlessWave>0&&S->Phase==EASPhase::Reward)
            Fire(FString::Printf(TEXT("WAVE %d CLEARED"),S->ClearedEndlessWave),ASExpedition::RewardWave(S->ClearedEndlessWave)?TEXT("CHECKPOINT CHEST"):TEXT(""),FLinearColor(.5f,1.f,.7f));
    } else if(Fighting&&(S->Room!=BannerRoom||S->Wave!=BannerWave)) {
        const int32 Waves=ASRules::WaveCount(S->Room);const bool Boss=ASRules::Boss(S->Room);
        Fire(Boss?FString(TEXT("BOSS")):FString::Printf(TEXT("WAVE %d"),S->Wave+1),Boss?TEXT(""):FString::Printf(TEXT("OF %d"),Waves),Boss?FLinearColor(1.f,.25f,.2f):FLinearColor(1.f,.78f,.3f));
    } else if(S->Phase==EASPhase::Reward&&BannerPhase==int32(EASPhase::Combat)&&!S->bEndless)
        Fire(TEXT("CHAMBER CLEARED"),TEXT(""),FLinearColor(.5f,1.f,.7f));
    BannerRoom=S->Room;BannerWave=S->Wave;BannerEndlessWave=S->EndlessWave;BannerCleared=S->ClearedEndlessWave;BannerPhase=int32(S->Phase);
    const auto Pose=ASPresentation::BannerAt(Now-BannerStart);if(Pose.Alpha<=0.f) return;
    // A dark band with colour rails; the title slides with the band and the subtitle trails it.
    const float BandH=118.f*Pose.Stretch*2.f,Y=Height*.22f-BandH*.5f,Off=Pose.Slide*Width;
    Box(Off,Y,Width,BandH,FLinearColor(0,0,0,.62f*Pose.Alpha));
    Box(Off,Y,Width,4,FLinearColor(BannerColor.R,BannerColor.G,BannerColor.B,Pose.Alpha));Box(Off,Y+BandH-4,Width,4,FLinearColor(BannerColor.R,BannerColor.G,BannerColor.B,Pose.Alpha));
    const float TitleX=Off+Width*.5f-float(BannerTitle.Len())*22.f,TitleY=Y+BandH*.5f-34.f;
    Text(BannerTitle,TitleX+3,TitleY+3,FLinearColor(0,0,0,Pose.Alpha),2.2f);Text(BannerTitle,TitleX,TitleY,FLinearColor(BannerColor.R,BannerColor.G,BannerColor.B,Pose.Alpha),2.2f);
    if(!BannerSub.IsEmpty()) Text(BannerSub,Off*1.4f+Width*.5f-float(BannerSub.Len())*9.f,TitleY+54.f,FLinearColor(1,1,1,Pose.Alpha*.85f),.9f);
}
