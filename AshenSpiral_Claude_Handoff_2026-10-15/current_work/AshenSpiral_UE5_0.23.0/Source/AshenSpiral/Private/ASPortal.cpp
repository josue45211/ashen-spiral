#include "ASPortal.h"
#include "ASWorld.h"
#include "ASGameState.h"
#include "ASExpeditionRules.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

namespace {
UStaticMeshComponent* MakePart(AASPortal* Owner,USceneComponent* Parent,const TCHAR* Name) {
    auto* C=Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
    C->SetupAttachment(Parent);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetGenerateOverlapEvents(false);C->SetCastShadow(false);
    return C;
}
}
AASPortal::AASPortal() {
    PrimaryActorTick.bCanEverTick=true;bReplicates=true;bAlwaysRelevant=true;SetNetUpdateFrequency(5.f);
    Root=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));RootComponent=Root;
    Arch=MakePart(this,Root,TEXT("Arch"));Arch->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Arch->SetCollisionResponseToAllChannels(ECR_Block);Arch->SetCastShadow(true);
    Lens=MakePart(this,Root,TEXT("Lens"));Rim=MakePart(this,Root,TEXT("Rim"));
    for(int32 I=0;I<20;++I) ChainParts.Add(MakePart(this,Root,*FString::Printf(TEXT("Chain%02d"),I)));
    for(int32 I=0;I<8;++I) Drips.Add(MakePart(this,Root,*FString::Printf(TEXT("Drip%d"),I)));
    for(int32 I=0;I<3;++I) Flames.Add(MakePart(this,Root,*FString::Printf(TEXT("Flame%d"),I)));
    for(int32 I=0;I<14;++I) Relics.Add(MakePart(this,Root,*FString::Printf(TEXT("Relic%02d"),I)));
}
void AASPortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASPortal,Kind);DOREPLIFETIME(AASPortal,bLocked);
}
void AASPortal::BeginPlay() {Super::BeginPlay();Refresh();}
void AASPortal::OnLook() {Refresh();}
void AASPortal::Refresh() {
    if(GetNetMode()==NM_DedicatedServer) return;
    const int32 K=FMath::Clamp(Kind,0,3);
    // Stone, lens and accent per portal: Ember burning orange, Ashen pale grey-white, Blood dark fresh red, Endless near-black with old blood.
    const FLinearColor Stone[4]={FLinearColor(.13f,.10f,.085f),FLinearColor(.36f,.36f,.38f),FLinearColor(.10f,.045f,.045f),FLinearColor(.06f,.045f,.05f)};
    const FLinearColor LensColor[4]={FLinearColor(1.f,.42f,.08f),FLinearColor(.86f,.89f,.94f),FLinearColor(.55f,.02f,.03f),FLinearColor(.65f,.03f,.03f)};
    const float OpenGlow[4]={3.2f,1.6f,2.6f,3.4f};
    // Archway: same mesh and framing as the old hub portal; the opening is about 470 wide and 500 tall.
    if(!bStyled) {
        ASAssets::Style(Arch,TEXT("SM_Arch"),Stone[K],.1f,0.f);Arch->SetRelativeScale3D(FVector(1.5f));
        ASAssets::Style(Rim,TEXT("SM_Ring"),LensColor[K],0.f,K==1?.8f:1.6f);Rim->SetRelativeLocation(FVector(40,0,6));Rim->SetRelativeScale3D(FVector(2.4f,2.4f,1.f));
        bStyled=true;
    }
    // Lens (the portal itself): glowing when open, a cold dark blank when locked or dormant.
    // 0.24.2 aesthetics: a turning vortex (M_Portal, made by the bootstrap) - bright and quick in the portal's colour when open, a slow dark smoke when sealed; the flat glow otherwise
    if(auto* Vortex=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_Portal.M_Portal"),nullptr,LOAD_NoWarn|LOAD_Quiet)) {
        Lens->SetStaticMesh(ASAssets::Mesh(TEXT("Sphere")));
        if(auto* M=UMaterialInstanceDynamic::Create(Vortex,Lens)) {
            M->SetVectorParameterValue(TEXT("Tint"),bLocked?FLinearColor(.07f,.05f,.06f):LensColor[K]);M->SetScalarParameterValue(TEXT("Emission"),bLocked?.8f:OpenGlow[K]);
            M->SetScalarParameterValue(TEXT("Speed"),bLocked?.25f:(K==3?1.4f:1.f));
            ASAssets::Paint(Lens,M);
        }
    } else ASAssets::Style(Lens,TEXT("Sphere"),bLocked?FLinearColor(.025f,.02f,.025f):LensColor[K],0.f,bLocked?0.f:OpenGlow[K]);
    Lens->SetRelativeLocation(FVector(20,0,245));Lens->SetRelativeScale3D(FVector(.12f,4.3f,5.0f));
    // Chains: two crossing bars of links across the archway, plus a padlock block. Visible only while locked (difficulty portals).
    const bool ShowChains=bLocked&&K<3;
    for(int32 I=0;I<ChainParts.Num();++I) {
        auto* C=ChainParts[I].Get();if(!C) continue;
        C->SetVisibility(ShowChains);
        if(!ShowChains) continue;
        ASAssets::Style(C,TEXT("Cube"),FLinearColor(.17f,.17f,.19f),.9f,0.f);
        if(I<18) {
            const int32 Bar=I/9,Link=I%9;const float T=(float(Link)+.5f)/9.f-.5f,Sign=Bar==0?1.f:-1.f;
            C->SetRelativeLocation(FVector(30,T*430.f*Sign,245.f+T*500.f));
            C->SetRelativeRotation(FRotator(0,0,Sign*(Link%2?45.f:-45.f)+Sign*0.f));
            C->SetRelativeScale3D(Link%2?FVector(.12f,.20f,.46f):FVector(.12f,.12f,.46f));
        } else {
            C->SetRelativeLocation(FVector(42,0,245));C->SetRelativeRotation(FRotator::ZeroRotator);C->SetRelativeScale3D(I==18?FVector(.22f,.46f,.58f):FVector(.14f,.2f,.3f));
            ASAssets::Style(C,TEXT("Cube"),FLinearColor(.5f,.34f,.1f),.9f,0.f);
        }
    }
    // Fresh dripping blood (Blood, Endless): thin red streaks that slide down from the arch top; Tick animates them.
    const bool Drip=K>=2;
    for(auto& D:Drips) if(auto* C=D.Get()) {
        C->SetVisibility(Drip);
        if(Drip) ASAssets::Style(C,TEXT("Cylinder"),K==2?FLinearColor(.75f,.02f,.03f):FLinearColor(.35f,.02f,.02f),0.f,K==2?1.8f:.6f);
    }
    // Ember burns: three flames at the arch feet.
    for(int32 I=0;I<Flames.Num();++I) if(auto* C=Flames[I].Get()) {
        C->SetVisibility(K==0);
        if(K==0) {
            ASAssets::Style(C,TEXT("Sphere"),FLinearColor(1.f,.4f,.05f),0.f,6.f);C->SetRelativeLocation(FVector(-10,(I-1)*250.f,40.f+(I%2)*18.f));C->SetRelativeScale3D(FVector(.32f,.32f,.7f));
            if(auto* Fire=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_Flame.M_Flame"),nullptr,LOAD_NoWarn|LOAD_Quiet)) // 0.24.2: a flame, not an orange egg
                if(auto* Burning=UMaterialInstanceDynamic::Create(Fire,C)) {Burning->SetVectorParameterValue(TEXT("Tint"),FLinearColor(1.f,.4f,.05f));Burning->SetScalarParameterValue(TEXT("Emission"),8.f);ASAssets::Paint(C,Burning);}
        }
    }
    // Endless: old blood, scratch marks, trophies of those who never came back whole.
    for(int32 I=0;I<Relics.Num();++I) if(auto* C=Relics[I].Get()) {
        const bool On=K==3;C->SetVisibility(On);if(!On) continue;
        if(I<3) {ASAssets::Style(C,TEXT("Cube"),FLinearColor(.17f,.015f,.015f),0.f,0.f);C->SetRelativeLocation(FVector(70.f+I*40.f,(I-1)*160.f,2.f));C->SetRelativeScale3D(FVector(1.6f+I*.4f,1.2f+I*.5f,.02f));C->SetRelativeRotation(FRotator(0,I*37.f,0));}
        else if(I<7) {ASAssets::Style(C,TEXT("SM_Helmet"),FLinearColor(.22f,.2f,.2f),.7f,0.f);C->SetRelativeLocation(FVector(95.f+(I%2)*30.f,-260.f+(I-3)*170.f,22.f));C->SetRelativeScale3D(FVector(.55f));C->SetRelativeRotation(FRotator(25.f*(I%3),I*50.f,I%2?35.f:-20.f));}
        else if(I<9) {ASAssets::Style(C,TEXT("SM_Weapon0"),FLinearColor(.28f,.26f,.26f),.8f,0.f);C->SetRelativeLocation(FVector(60.f,(I==7?-1.f:1.f)*210.f,70.f));C->SetRelativeScale3D(FVector(.9f));C->SetRelativeRotation(FRotator(70.f,I*40.f,I==7?15.f:-25.f));}
        else {// scratch marks: pale thin slashes on the arch legs and crown
            ASAssets::Style(C,TEXT("Cube"),FLinearColor(.55f,.5f,.45f),0.f,0.f);const float Side=(I%2)?1.f:-1.f;
            C->SetRelativeLocation(FVector(48.f,Side*(210.f+(I%3)*12.f),90.f+(I-9)*60.f));C->SetRelativeRotation(FRotator(0,0,Side*(20.f+(I%3)*12.f)));C->SetRelativeScale3D(FVector(.01f,.03f,1.1f+(I%3)*.3f));
        }
    }
}
void AASPortal::Tick(float Delta) {
    Super::Tick(Delta);auto* S=AS::State(this);if(!S) return;
    const bool InHub=S->Phase==EASPhase::Hub;
    if(HasAuthority()) {
        // The host's profile decides what is unlocked (TUNABLE rule: ASExpeditionRules.h).
        int32 Mask=0;bool Endless=false;
        for(auto P:S->PlayerArray) if(const auto* PS=Cast<AASPlayerState>(P)) if(PS->PlayerIndex==0&&PS->bProfileReady) {Mask=PS->ClearedMask;Endless=PS->bEndlessUnlocked;}
        const bool Lock=Kind==3?!Endless:!ASExpedition::DifficultyUnlocked(Kind,Mask);
        if(Lock!=bLocked) {bLocked=Lock;ForceNetUpdate();Refresh();}
    }
    if(IsHidden()==InHub) {SetActorHiddenInGame(!InHub);SetActorEnableCollision(InHub);}
    if(!InHub||GetNetMode()==NM_DedicatedServer) return;
    const float Now=GetWorld()->GetTimeSeconds();
    // Drips slide down from the crown and fade back in at the top.
    if(Kind>=2) for(int32 I=0;I<Drips.Num();++I) if(auto* C=Drips[I].Get()) {
        const float T=FMath::Frac(Now*(.22f+.03f*I)+DripPhase[I]),Length=70.f+40.f*FMath::Frac(I*.37f);
        C->SetRelativeLocation(FVector(55.f,-190.f+I*54.f,500.f-T*330.f));C->SetRelativeScale3D(FVector(.035f,.035f,Length/100.f*(1.f-.5f*T)));
    }
    // Open portals breathe.
    if(!bLocked) Lens->SetRelativeScale3D(FVector(.12f,4.3f*(1.f+.025f*FMath::Sin(Now*2.1f)),5.0f*(1.f+.02f*FMath::Sin(Now*1.7f+1.f))));
}
