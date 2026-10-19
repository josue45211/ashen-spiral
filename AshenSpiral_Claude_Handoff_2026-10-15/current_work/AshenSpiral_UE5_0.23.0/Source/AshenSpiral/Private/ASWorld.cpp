#include "ASWorld.h"
#include "Engine/Texture.h"
#include "ASNavigationRules.h"
#include "ASData.h"
#include "ASGameState.h"
#include "ASCharacter.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "ASGameInstance.h"
#include "ASExpeditionRules.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
namespace ASAssets {
UStaticMesh* Mesh(const FString& Name) {
    if(Name==TEXT("Cube") || Name==TEXT("Sphere") || Name==TEXT("Cylinder") || Name==TEXT("Cone"))
        return LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),*Name,*Name));
    auto* Result=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Art/Meshes/%s.%s"),*Name,*Name));
    return Result?Result:LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
}
bool HasMesh(const FString& Name) {
    if(Name==TEXT("Cube") || Name==TEXT("Sphere") || Name==TEXT("Cylinder") || Name==TEXT("Cone")) return true;
    static TMap<FString,bool> Known; // game thread only; a missing mesh stays missing until the editor restarts after the import
    if(const bool* Found=Known.Find(Name)) return *Found;
    const bool Present=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Art/Meshes/%s.%s"),*Name,*Name),nullptr,LOAD_NoWarn)!=nullptr;
    Known.Add(Name,Present);return Present;
}
UMaterialInstanceDynamic* Material(UObject* Outer,FLinearColor C,float Metal,float Glow,float Rough) {
    auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_Surface.M_Surface"));
    if(!Base) Base=UMaterial::GetDefaultMaterial(MD_Surface);
    auto* M=UMaterialInstanceDynamic::Create(Base,Outer);
    M->SetVectorParameterValue(TEXT("Tint"),C);M->SetScalarParameterValue(TEXT("Metallic"),Metal);
    M->SetScalarParameterValue(TEXT("Emission"),Glow);M->SetScalarParameterValue(TEXT("Roughness"),Rough);return M;
}
void Armor(UStaticMeshComponent* C,const FString& Name,FLinearColor Color,float Metal,float Glow,float Rough) {
    Style(C,Name,Color,Metal,Glow);
    if(auto* M=Cast<UMaterialInstanceDynamic>(C->GetMaterial(0))) {M->SetScalarParameterValue(TEXT("SurfaceDetail"),0.f);M->SetScalarParameterValue(TEXT("Roughness"),Rough);}
}
void Style(UStaticMeshComponent* C,const FString& Name,FLinearColor Color,float Metal,float Glow) {
    if(!C) return;C->SetStaticMesh(Mesh(Name));C->SetMaterial(0,Material(C,Color,Metal,Glow));
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetGenerateOverlapEvents(false);
}
}
AASWorld::AASWorld() {PrimaryActorTick.bCanEverTick=true;bReplicates=true;bAlwaysRelevant=true;SetReplicateMovement(false);RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));RootComponent->SetMobility(EComponentMobility::Static);}
void AASWorld::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASWorld,Config);}
void AASWorld::ConfigureEndless(int32 Seed,int32 Shuffle,const TArray<FVector>& Keep) {
    if(!HasAuthority()) return;
    Config.Room=ASRules::EndlessRoom;Config.Seed=Seed;Config.Shuffle=Shuffle;Config.KeepCount=FMath::Min(2,Keep.Num());
    Config.KeepX0=Keep.IsValidIndex(0)?Keep[0].X:0.f;Config.KeepY0=Keep.IsValidIndex(0)?Keep[0].Y:0.f;
    Config.KeepX1=Keep.IsValidIndex(1)?Keep[1].X:0.f;Config.KeepY1=Keep.IsValidIndex(1)?Keep[1].Y:0.f;
    ++Config.Generation;if(Shuffle==0) ++Config.Epoch;Rebuild();ForceNetUpdate();
}
void AASWorld::Configure(int32 Room,int32 Seed) {if(!HasAuthority()) return;Config.Room=Room;Config.Seed=Seed;++Config.Generation;++Config.Epoch;Rebuild();ForceNetUpdate();}
void AASWorld::BeginPlay() {
    Super::BeginPlay();Rebuild();
    if(GetNetMode()==NM_DedicatedServer) return;
    auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>();
    if(Sun) {
        auto* D=Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
        // ADirectionalLight starts stationary. Changing its transform first is rejected in PIE.
        D->bAtmosphereSunLight=true;D->SetForwardShadingPriority(1);
        D->SetMobility(EComponentMobility::Movable);
        Sun->SetActorRotation(FRotator(-42,-35,0));
        D->SetIntensity(4.5f);D->SetLightColor(FLinearColor(1,.83f,.65f));
    }
    // Keep shaded faces readable at Low/Medium, where Lumen may be disabled.
    // This inexpensive fill does not depend on a sky capture or baked lighting.
    auto* Fill=GetWorld()->SpawnActor<ADirectionalLight>();
    if(Fill) {
        auto* D=Cast<UDirectionalLightComponent>(Fill->GetLightComponent());
        D->bAtmosphereSunLight=false;D->SetForwardShadingPriority(0);
        D->SetMobility(EComponentMobility::Movable);
        Fill->SetActorRotation(FRotator(-55,145,0));
        D->SetIntensity(.8f);D->SetLightColor(FLinearColor(.65f,.76f,1.f));D->SetCastShadows(false);
    }
    GetWorld()->SpawnActor<ASkyAtmosphere>();
    auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();
    SunLight=Sun;FogActor=Fog;
    if(Fog) {
        Fog->GetComponent()->SetMobility(EComponentMobility::Movable);
        Fog->SetActorLocation(FVector(0,0,-450));
        Fog->GetComponent()->SetFogDensity(.018f);Fog->GetComponent()->SetFogHeightFalloff(.25f);
        Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.10f,.12f,.18f));Fog->GetComponent()->bEnableVolumetricFog=true;
    }
    auto* Sky=GetWorld()->SpawnActor<ASkyLight>();SkyLightActor=Sky;
    if(Sky) {
        auto* L=Sky->GetLightComponent();L->SetMobility(EComponentMobility::Movable);
        L->SetIntensity(.65f);
        // The sky/sun do not animate. Queue a capture after their components exist;
        // direct assignment of bRealTimeCapture after registration did not notify the renderer.
        L->SetRealTimeCaptureEnabled(false);L->RecaptureSky();
    }
    auto* Post=GetWorld()->SpawnActor<APostProcessVolume>();PostVolume=Post;
    if(Post) {
        Post->bUnbound=true;
        Post->Settings.bOverride_BloomIntensity=true;Post->Settings.BloomIntensity=.4f;
        Post->Settings.bOverride_VignetteIntensity=true;Post->Settings.VignetteIntensity=.20f;
        Post->Settings.bOverride_AutoExposureMethod=true;Post->Settings.AutoExposureMethod=AEM_Manual;
        // Our modest, stylized light values are not a photographic daylight rig.
        // Do not inherit shutter/ISO/aperture attenuation from physical camera defaults.
        Post->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;
        Post->Settings.AutoExposureApplyPhysicalCameraExposure=false;
        Post->Settings.bOverride_AutoExposureBias=true;Post->Settings.AutoExposureBias=1.f;
    }
    UE_LOG(LogTemp,Display,TEXT("AshenVisual 0.23.2: sun=%s rotation=%s fill=%s sky=%s exposureVolume=%s"),
        *GetNameSafe(Sun),Sun?*Sun->GetActorRotation().ToString():TEXT("MISSING"),
        *GetNameSafe(Fill),*GetNameSafe(Sky),*GetNameSafe(Post));
}
bool AASWorld::ShopOpen() const {
    const auto* S=AS::State(this);return IsEndlessArena()&&S&&S->bEndless&&S->bLuckyShop&&S->Phase==EASPhase::Reward;
}
void AASWorld::ApplyShopVisibility(bool bOpen) {
    if(ShopShown==(bOpen?1:0)) return;ShopShown=bOpen?1:0;
    if(auto* Stall=Batches.FindRef(FName(TEXT("Cube_EndlessShopStall_C"))).Get()) {Stall->SetVisibility(bOpen);Stall->SetCollisionEnabled(bOpen?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);}
    if(auto* L=Cast<USceneComponent>(ShopLabel.Get())) L->SetVisibility(bOpen);
}
void AASWorld::Tick(float Delta) {
    Super::Tick(Delta);
    if(!HasAuthority()&&Config.Room>=0) {
        // Clients must hold exactly the geometry the server built. If the replicated config arrived but the blocks did not (or are stale), rebuild.
        if(BuiltGeneration==Config.Generation&&!Batches.IsEmpty()) ClientRebuildRetries=0;
        else if(ClientRebuildRetries<3) {
            ++ClientRebuildRetries;BuiltGeneration=-1;BuiltEpoch=-1;Rebuild(); // force a FULL rebuild
            UE_LOG(LogTemp,Warning,TEXT("AshenVisual: client geometry safety rebuild %d (generation=%d batches=%d)"),ClientRebuildRetries,Config.Generation,Batches.Num());
        }
    }
    if(IsEndlessArena()) ApplyShopVisibility(ShopOpen());
    if(const auto* ShiftState=AS::State(this)) AnimateShift(ShiftState); // runs on dedicated servers too (collision); visuals and sound only where there is a screen
    if(GetNetMode()==NM_DedicatedServer) return;
    const auto* S=AS::State(this);if(!S) return;
    // Sun/fog/sky colours follow the Endless stage: clear (1-20), gloomy (21-49), red (50+). Hub and normal runs stay clear.
    const int32 Stage=ASExpedition::EndlessSkyStage(S->bEndless,S->Phase!=EASPhase::Hub,S->EndlessWave);
    if(Stage!=SkyTo) {SkyFrom=SkyTo;SkyTo=Stage;SkyBlendT=0.f;}
    if(SkyBlendT>=1.f&&SkyTo==SkyFrom&&SkyTo==0&&!SunLight) return;
    SkyBlendT=FMath::Min(1.f,SkyBlendT+Delta*ASExpedition::SkyBlendRate(SkyFrom,SkyTo));
    const auto Look=ASExpedition::LerpSky(ASExpedition::SkyLookFor(SkyFrom),ASExpedition::SkyLookFor(SkyTo),SkyBlendT);
    if(SunLight) if(auto* D=Cast<UDirectionalLightComponent>(SunLight->GetLightComponent())) {D->SetIntensity(Look.SunIntensity);D->SetLightColor(FLinearColor(Look.SunR,Look.SunG,Look.SunB));}
    if(FogActor) {FogActor->GetComponent()->SetFogDensity(Look.FogDensity);FogActor->GetComponent()->SetFogInscatteringColor(FLinearColor(Look.FogR,Look.FogG,Look.FogB));}
    if(SkyLightActor) if(auto* L=SkyLightActor->GetLightComponent()) L->SetIntensity(Look.SkyLight);
    if(PostVolume) {
        PostVolume->Settings.bOverride_VignetteIntensity=true;PostVolume->Settings.VignetteIntensity=Look.Vignette;
        PostVolume->Settings.bOverride_ColorSaturation=true;PostVolume->Settings.ColorSaturation=FVector4(Look.Saturation,Look.Saturation,Look.Saturation,1.f);
        PostVolume->Settings.bOverride_ColorGain=true;PostVolume->Settings.ColorGain=FVector4(Look.GainR,Look.GainG,Look.GainB,1.f);
    }
}
void AASWorld::RebuildEndlessCover() {
    const double Start=FPlatformTime::Seconds();
    for(auto It=Batches.CreateIterator();It;++It) if(It->Key.ToString().Contains(TEXT("EndlessCover"))||It->Key.ToString().Contains(TEXT("EndlessSolid"))) {if(It->Value) It->Value->DestroyComponent();It.RemoveCurrent();}
    BuildEndlessCover(EndlessCenter);LastCoverDrop=-9999.f;SolidState=-1;
    if(const auto* State=AS::State(this)) AnimateShift(State); // apply the right height and collision this frame
    UE_LOG(LogTemp,Display,TEXT("AshenVisual: endless cover rebuilt shuffle=%d batches=%d in %.2f ms"),Config.Shuffle,Batches.Num(),(FPlatformTime::Seconds()-Start)*1000.0);
}
float AASWorld::ShiftNow(const AASGameState* S) {
    const float Server=S->SimTime;
    if(HasAuthority()) {ShiftClock=Server;bShiftClockInit=true;return Server;}
    if(GFrameCounter!=ShiftClockFrame) { // once per frame, however many times this is called
        ShiftClockFrame=GFrameCounter;
        if(!bShiftClockInit||!FMath::IsFinite(ShiftClock)||FMath::Abs(Server-ShiftClock)>.5f) ShiftClock=Server;
        else {
            const float Step=S->Frozen()?0.f:FMath::Clamp(GetWorld()?GetWorld()->GetDeltaSeconds():0.f,0.f,.1f);
            ShiftClock+=Step;ShiftClock+=(Server-ShiftClock)*.1f; // follow the replicated clock without stepping when its updates arrive
        }
        bShiftClockInit=true;
    }
    return ShiftClock;
}
void AASWorld::SetCoverCollision(bool On) {
    for(auto& Pair:Batches) if(Pair.Value&&Pair.Key.ToString().Contains(TEXT("EndlessSolid"))) Pair.Value->SetCollisionEnabled(On?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
}
void AASWorld::AnimateShift(const AASGameState* S) {
    if(!S||!IsEndlessArena()) return;
    const float Now=ShiftNow(S);
    const auto* GI=GetGameInstance<UASGameInstance>();const bool Reduce=GI&&GI->bReduceMotion;
    // Collision never moves. It switches off while the blocks are sunk or half-risen and on again once the new ones are mostly up (about 70% of the wall
    // above ground, ShiftSolidBelow), in discrete steps, so every machine agrees up to one network delay. It also stays off while a hero stands inside a
    // rising wall's footprint, so nobody is pushed out of a wall that switches on under them (client-side depenetration was the P2 "glitch").
    int8 Solid=(Reduce||S->ShiftAt<=0.f||ASExpedition::ShiftCoverSolid(Now,S->ShiftAt))?1:0;
    if(Solid&&SolidState==0) {
        for(const auto& Weak:S->Heroes) if(const auto* Hero=Weak.Get()) {
            const FVector P=Hero->GetActorLocation();
            for(const auto& F:CoverFootprints) if(ASExpedition::CircleHitsFootprint(P.X,P.Y,70.f,F.X,F.Y,F.Length,F.Thickness,F.Yaw)) {Solid=0;break;}
            if(!Solid) break;
        }
    }
    if(Solid!=SolidState) {
        SolidState=Solid;SetCoverCollision(Solid!=0);
    }
    if(GetNetMode()==NM_DedicatedServer) return;
    // Visual blocks sink (ease-in, then slam flush) in the second before the swap, and the new ones rise (ease-in, slam into place) after it.
    const float Drop=Reduce?0.f:ASExpedition::ShiftCoverDrop*ASExpedition::ShiftCoverSunk(Now,S->ShiftAt);
    if(!FMath::IsNearlyEqual(Drop,LastCoverDrop,.5f)) {
        for(auto& Pair:Batches) if(Pair.Value&&Pair.Key.ToString().Contains(TEXT("EndlessCover"))) Pair.Value->SetRelativeLocation(FVector(0,0,-Drop));
        LastCoverDrop=Drop;
    }
    if(S->ShiftAt<=0.f) return;
    // Placeholder tomb sounds (existing assets, pitched low); each plays once per shift.
    const auto Cue=[&](float& Last,float At,const TCHAR* Path,float Volume,float Pitch) {
        if(Now<At||Now>=At+.6f||FMath::IsNearlyEqual(Last,S->ShiftAt)) return;
        Last=S->ShiftAt;
        if(auto* Sound=LoadObject<USoundBase>(nullptr,Path)) UGameplayStatics::PlaySound2D(this,Sound,Volume,Pitch);
    };
    Cue(SinkSoundAt,S->ShiftAt-ASExpedition::ShiftSinkSeconds,TEXT("/Game/Audio/S_Collapse.S_Collapse"),.7f,.4f);
    Cue(ThudDownAt,S->ShiftAt,TEXT("/Game/Audio/S_HammerHit.S_HammerHit"),.65f,.55f);
    Cue(RiseSoundAt,S->ShiftAt,TEXT("/Game/Audio/S_Collapse.S_Collapse"),.8f,.5f);
    Cue(ThudUpAt,S->ShiftAt+ASExpedition::ShiftRiseSeconds,TEXT("/Game/Audio/S_HammerSmash.S_HammerSmash"),.75f,.6f);
}
void AASWorld::Rebuild() {
    if(BuiltGeneration==Config.Generation) return;
    if(Config.Room==ASRules::EndlessRoom&&BuiltRoom==Config.Room&&BuiltEpoch==Config.Epoch&&!Batches.IsEmpty()) {BuiltGeneration=Config.Generation;RebuildEndlessCover();return;}
    BuiltGeneration=Config.Generation;BuiltEpoch=Config.Epoch;BuiltRoom=Config.Room;bVaultDimmed=false;LastCoverDrop=-9999.f;SolidState=-1;
    for(auto& P:Batches) if(P.Value) P.Value->DestroyComponent();Batches.Empty();
    for(auto C:Labels) if(C) C->DestroyComponent();Labels.Empty();
    for(auto A:LocalLights) if(IsValid(A)) A->Destroy();LocalLights.Empty();
    CoverFootprints.Reset();ShopLabel=nullptr;ShopShown=-1;Stations.Empty();Water.Empty();Layout=ASRules::Layout();Navigation=ASRules::NavGrid();GuideNavigation=ASRules::NavGrid();bGuideNavigationReady=false;GuideBreakableCount=-1;
    if(Config.Room<0) MakeHub();else {Layout=Config.Room==ASRules::EndlessRoom?ASRules::GenerateEndless():(ASRules::Boss(Config.Room)?ASRules::GenerateBoss(ASRules::Ward(Config.Room)):ASRules::Generate(uint32(Config.Seed)));MakeRoom();if(HasAuthority()) BuildNavigation();}
    UE_LOG(LogTemp,Display,TEXT("AshenVisual: room=%d generation=%d geometryBatches=%d stations=%d"),
        Config.Room,Config.Generation,Batches.Num(),Stations.Num());
}
void AASWorld::Add(const FString& MeshName,const FString& Group,const FVector& P,const FVector& Scale,FLinearColor Color,bool Collision,float Yaw,float Glow,float Metal) {
    const FName Key(*(MeshName+TEXT("_")+Group+(Collision?TEXT("_C"):TEXT("_N"))));
    auto* C=Batches.FindRef(Key).Get();
    if(!C) {
        C=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);C->SetupAttachment(RootComponent);
        C->SetStaticMesh(ASAssets::Mesh(MeshName));
        auto* Mat=ASAssets::Material(C,Color,Metal,Glow);
        if(Mat&&Config.Room>=0&&Biome()==0&&Glow==0) {
            const FString Surface=Group.StartsWith(TEXT("ArchiveWood"))?TEXT("Wood"):(Group.StartsWith(TEXT("ArchiveIron"))||Group==TEXT("ArchiveUplightIron")?TEXT("Rust"):TEXT("Stone"));
            for(const auto& Pair:TArray<TPair<FString,FString>>{{TEXT("AlbedoMap"),TEXT("")},{TEXT("NormalMap"),TEXT("_Normal")},{TEXT("RoughnessMap"),TEXT("_Roughness")}}) {
                const FString AssetName=TEXT("T_Archive")+Surface+Pair.Value;
                const FString AssetPath=FString::Printf(TEXT("/Game/Art/Textures/%s.%s"),*AssetName,*AssetName);
                if(auto* Texture=LoadObject<UTexture>(nullptr,*AssetPath)) Mat->SetTextureParameterValue(FName(*Pair.Key),Texture);
            }
            if(Group.StartsWith(TEXT("ArchiveBooks"))||Group.StartsWith(TEXT("ArchiveFolio"))||Group.StartsWith(TEXT("ArchiveDebris"))) Mat->SetScalarParameterValue(TEXT("SurfaceDetail"),.25f);
        }
        C->SetMaterial(0,Mat);
        C->SetMobility(Group.StartsWith(TEXT("EndlessCover"))?EComponentMobility::Movable:EComponentMobility::Static); // cover sinks and rises
        C->SetCollisionEnabled(Collision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
        C->SetCollisionObjectType(ECC_WorldStatic);C->SetCollisionResponseToAllChannels(ECR_Block);C->SetGenerateOverlapEvents(false);
        C->SetCanEverAffectNavigation(false);C->SetCullDistances(0,0);
        if(Group.StartsWith(TEXT("EndlessSolid"))) {C->SetVisibility(false);C->SetHiddenInGame(true);C->SetCastShadow(false);} // collision-only proxy
        C->RegisterComponent();Batches.Add(Key,C);
    }
    C->AddInstance(FTransform(FRotator(0,Yaw,0),P,Scale),true);
}
void AASWorld::Text(const FString& Value,const FVector& P,float Yaw,float Size) {
    if(GetNetMode()==NM_DedicatedServer) return;
    auto* T=NewObject<UTextRenderComponent>(this);T->SetupAttachment(RootComponent);T->SetText(FText::FromString(Value));
    T->SetHorizontalAlignment(EHTA_Center);T->SetWorldSize(Size);T->SetTextRenderColor(FColor(239,217,165));
    T->SetRelativeLocation(P);T->SetRelativeRotation(FRotator(0,Yaw,0));T->RegisterComponent();Labels.Add(T);
}
void AASWorld::Light(const FVector& P,FLinearColor Color,float Intensity,float Radius) {
    if(GetNetMode()==NM_DedicatedServer || LocalLights.Num()>=48) return;
    auto* A=GetWorld()->SpawnActor<APointLight>();if(!A) return;
    auto* L=Cast<UPointLightComponent>(A->GetLightComponent());L->SetMobility(EComponentMobility::Movable);
    A->SetActorLocation(P);
    L->SetLightColor(Color);L->SetIntensity(Intensity);L->SetAttenuationRadius(Radius);L->SetCastShadows(false);LocalLights.Add(A);
}
void AASWorld::MakeHub() {
    const FLinearColor Stone(.16f,.18f,.21f),Trim(.30f,.23f,.12f),ASWorldHubGold(1,.56f,.13f),ASWorldHubTeal(.12f,.75f,.63f);
    Add(TEXT("Cube"),TEXT("HubFoundation"),FVector(0,0,-180),FVector(34,34,3.6f),Stone);
    for(int32 X=-5;X<=5;++X) for(int32 Y=-5;Y<=5;++Y)
        Add(TEXT("SM_Tile"),TEXT("HubTile"),FVector(X*300,Y*300,0),FVector(1),FLinearColor(.32f,.30f,.26f),false);
    for(int32 I=0;I<4;++I) {
        const float Yaw=I*90.f;const FVector Dir=FRotator(0,Yaw,0).Vector();
        Add(TEXT("Cube"),TEXT("HubWall"),Dir*1680+FVector(0,0,210),FVector(.8f,34,4.2f),Stone,true,Yaw);
        for(int32 J=-3;J<=3;++J) {
            FVector At=Dir*1450+FVector(-Dir.Y,Dir.X,0)*J*410;
            Add(TEXT("SM_Pillar"),TEXT("HubPillar"),At,FVector(1.1f,1.1f,1.6f),Trim);
        }
    }
    Add(TEXT("SM_Ring"),TEXT("HubInlay"),FVector(0,0,12),FVector(14,14,1),Trim,false,0,.3f,.6f);
    Add(TEXT("SM_Ring"),TEXT("HubInlay"),FVector(0,0,12),FVector(7,7,1),Trim,false,0,.3f,.6f);
    Add(TEXT("SM_Obelisk"),TEXT("Hearth"),FVector(-600,800,0),FVector(1.1f),Stone);
    // 0.24: the old "THE SPIRAL / Expedition / Ready" station (orb, ring, light, label) is gone; departure is the archways. Station ids are not indexes.
    Stations={{FVector(0,-650,0),2,TEXT("THE ARMORY\nWalk up to a weapon / E")},
              {FVector(350,1000,0),3,TEXT("ABILITY SANCTUM\n6 active abilities + Spiral styles")},
              {FVector(-600,800,0),4,TEXT("ASHEN HEARTH\nPermanent ranks")},
              {FVector(-1100,-100,0),5,TEXT("IONE'S WORKBENCH\nCraft and remember")},
              {FVector(-300,1250,0),22,TEXT("ENDLESS PORTAL\nEnter / E")},
              {FVector(1000,-620,0),31,TEXT("EMBER PORTAL\nNormal / E to choose")},
              {FVector(1000,0,0),32,TEXT("ASHEN PORTAL\nHard / E to choose")},
              {FVector(1000,620,0),33,TEXT("BLOOD PORTAL\nVery Hard / E to choose")}};
    for(const auto& S:Stations) {
        const float Yaw=(-S.Position).Rotation().Yaw;
        Add(TEXT("SM_Ring"),TEXT("HubGlow"),S.Position+FVector(0,0,15),FVector(3.4f,3.4f,1),ASWorldHubGold,false,0,3);
        Text(S.Name,S.Position+FVector(0,0,280),Yaw,27);
        if(S.Menu!=1 && S.Menu!=4 && S.Menu<22) Add(TEXT("SM_Brazier"),TEXT("HubBrazier"),S.Position,FVector(1.3f),Trim);
        Add(TEXT("Sphere"),TEXT("HubFlame"),S.Position+FVector(0,0,S.Menu==4?380:185),FVector(.6f,.6f,1.1f),ASWorldHubGold,false,0,8);
        Light(S.Position+FVector(0,0,210),ASWorldHubGold,850,800);
    }
    // The difficulty portals (Ember / Ashen / Blood) and the Endless portal are AASPortal actors spawned by the game mode.
    for(int32 I=0;I<10;++I) {
        const FVector P(-1400,-1250+I*280.f,0);Add(TEXT("SM_Shelf"),TEXT("HubShelf"),P,FVector(1),FLinearColor(.24f,.13f,.055f));
        for(int32 J=0;J<12;++J) Add(TEXT("SM_Book"),FString::Printf(TEXT("Book%d"),J%3),P+FVector(10,(J%4-1.5f)*47,43+(J/4)*70),FVector(1),J%3==0?ASWorldHubTeal:Trim,false,90);
    }
    for(int32 I=0;I<ASRules::WeaponCount;++I) {
        const FVector P=WeaponStand(I);const auto Color=ASData::HitColor(ASData::WeaponElement(I));
        Add(TEXT("Cylinder"),TEXT("WeaponPlinth"),P+FVector(0,0,35),FVector(1.5f,1.5f,.7f),Stone);
        Add(TEXT("SM_Ring"),FString::Printf(TEXT("WeaponAccent%d"),I),P+FVector(0,0,72),FVector(1.6f,1.6f,1),Color,false,0,2);
        if(I%2) Add(TEXT("Cube"),TEXT("WeaponBackboard"),P+FVector(0,-90,180),FVector(2.9f,.2f,3.5f),Stone,true);
        Add(FString::Printf(TEXT("SM_Weapon%d"),I),TEXT("WeaponDisplay"),P+FVector(0,0,165),FVector(1),FLinearColor(.48f,.42f,.30f),false,90,0,.55f);
        if(I==2) Add(TEXT("Cylinder"),TEXT("DisplayString"),P+FVector(0,-24,165),FVector(.009f,.009f,1.46f),FLinearColor(.8f,.75f,.6f),false);
        Text(FString::Printf(TEXT("%02d"),I+1),P+FVector(0,0,90),90,18);
    }
    Add(TEXT("SM_Ring"),TEXT("PracticeRing"),DummyPoint()-FVector(0,0,93),FVector(5,5,1),ASWorldHubTeal,false,0,1);
    Text(TEXT("PRACTICE TARGET\nWeapons + Spiral + active ability / no rewards"),DummyPoint()+FVector(0,0,200),-130,23);
    Text(TEXT("ASHEN SPIRAL"),FVector(1550,0,750),180,70);
    Text(TEXT("THE EMBER NEXUS"),FVector(1550,0,650),180,30);
}
void AASWorld::BuildEndlessCover(const FVector& Center) {
    const FLinearColor Stone(.17f,.16f,.17f);
    CoverFootprints.Reset();
    std::array<ASRules::KeepOut,2> Keep;
    if(Config.KeepCount>0) Keep[0]={Config.KeepX0,Config.KeepY0,true};
    if(Config.KeepCount>1) Keep[1]={Config.KeepX1,Config.KeepY1,true};
    for(const auto& O:ASRules::EndlessProps(uint32(Config.Seed)^uint32(Config.Shuffle*7919),Keep)) {
        const FVector At=Center+FVector(O.X,O.Y,O.Height*.5f),Scale(O.Length/100.f,O.Thickness/100.f,O.Height/100.f);
        const FLinearColor Tint=Stone*(O.Height>=ASRules::EndlessWallHeight-10.f?1.15f:1.f);
        Add(TEXT("Cube"),FString::Printf(TEXT("EndlessCover%d"),O.Height>=ASRules::EndlessWallHeight-10.f?0:2),At,Scale,Tint,false,O.Yaw,0,.05f); // visual, animated
        CoverFootprints.Add({float(Center.X+O.X),float(Center.Y+O.Y),O.Length,O.Thickness,O.Yaw});
        Add(TEXT("Cube"),TEXT("EndlessSolid"),At,Scale,Tint,true,O.Yaw,0,.05f);                                                                  // collision proxy, fixed
    }
}
void AASWorld::EndlessArena(const FVector& Center) {
    // Placeholder identity: dark ruin stone with a faint red-gold chest sigil. P2 art-directs in 0.23.1.
    EndlessCenter=Center;BuildEndlessCover(Center);
    Add(TEXT("Cube"),TEXT("EndlessShopStall"),ExitPoint()+FVector(0,380,60),FVector(1.6f,.8f,1.2f),FLinearColor(.25f,.15f,.07f),true);
    Text(TEXT("LUCKY SHOP / spend Scrap and Ash"),ExitPoint()+FVector(0,380,260),90,22);ShopLabel=Labels.IsEmpty()?nullptr:Labels.Last();ShopShown=-1;
    // The three glowing rings that used to sit under the chest (outer sigil, inner sigil, arrival glyph) are gone: they read as stray lights once the sky darkens after waves 20 and 50.
    Light(Center+FVector(0,0,600),FLinearColor(1,.35f,.2f),2200,3200);
}
void AASWorld::BossArena(const FVector& Center) {
    const int32 W=Ward();const FLinearColor Glow=ASData::BiomeColor(Biome());
    const FLinearColor Stone=FLinearColor(.26f,.28f,.31f)*(W==1?.8f:1.f);
    const FLinearColor Identity[4]={FLinearColor(.9f,.55f,.2f),FLinearColor(1.f,.2f,.08f),FLinearColor(.25f,.6f,1.f),FLinearColor(.7f,.15f,.9f)};
    for(const auto& Prop:ASRules::BossProps(W)) {
        const FVector At=Center+FVector(Prop.X,Prop.Y,0);
        if(Prop.Kind==0) Add(TEXT("Cube"),FString::Printf(TEXT("BossCover%d"),W),At+FVector(0,0,160),FVector(Prop.Length/100.f,Prop.Thickness/100.f,3.2f),Stone,true,Prop.Yaw,0,.1f);
        else if(Prop.Kind==1) Add(TEXT("Cube"),FString::Printf(TEXT("BossLane%d"),W),At+FVector(0,0,5),FVector(Prop.Length/100.f,Prop.Thickness/100.f,.05f),Identity[W],false,Prop.Yaw,3);
        else Add(TEXT("SM_Ring"),FString::Printf(TEXT("BossSigil%d"),W),At+FVector(0,0,9),FVector(Prop.Length/100.f,Prop.Length/100.f,1),Identity[W],false,0,2.5f);
    }
    Light(Center+FVector(0,0,420),Identity[W],1800,2600);
    (void)Glow;
}
void AASWorld::MakeRoom() {
    const FLinearColor Palette[5]={FLinearColor(.26f,.28f,.31f),FLinearColor(.19f,.15f,.17f),FLinearColor(.16f,.18f,.26f),FLinearColor(.24f,.27f,.19f),FLinearColor(.12f,.20f,.24f)};
    const FLinearColor Floor=Palette[Biome()],Glow=ASData::BiomeColor(Biome());
    FRandomStream R(Config.Seed);
    const FVector Dirs[4]={FVector(1,0,0),FVector(-1,0,0),FVector(0,1,0),FVector(0,-1,0)};
    for(int32 I=0;I<int32(Layout.Cells.size());++I) {
        const FVector P=CellPosition(I);const FVector2D Half=CellHalfSize(I);
        Add(TEXT("Cube"),TEXT("Foundation"),P+FVector(0,0,-170),FVector(Half.X/50,Half.Y/50,3.4f),Floor*.65f);
        const int32 NX=FMath::RoundToInt(Half.X/120),NY=FMath::RoundToInt(Half.Y/120);
        if(Biome()==0||Biome()==1) for(int32 X=0;X<NX;++X) for(int32 Y=0;Y<NY;++Y) {
            const uint32 Hash=uint32(Config.Seed)^uint32(I*73856093)^uint32(X*19349663)^uint32(Y*83492791);
            const int32 V=int32((Hash^(Hash>>13))%4);
            Add(TEXT("SM_Tile"),Biome()==0?FString::Printf(TEXT("ArchiveFloor%d"),V):TEXT("Floor"),P+FVector(-Half.X+120+X*240,-Half.Y+120+Y*240,0),FVector(.8f,.8f,1),Biome()==0?Floor*(.82f+V*.12f):Floor,false,Biome()==0?V*90.f:0.f);
        }
        for(const FVector& D:Dirs) {
            bool Door=false;
            for(auto E:Layout.Edges) {int32 N=E.A==I?E.B:(E.B==I?E.A:-1);if(N>=0 && FVector::DotProduct((CellPosition(N)-P).GetSafeNormal(),D)>.95f) Door=true;}
            const float Yaw=D.Rotation().Yaw;const FVector Side(-D.Y,D.X,0);
            const float Edge=FMath::Abs(D.X)>.5f?Half.X:Half.Y,Across=FMath::Abs(D.X)>.5f?Half.Y:Half.X;
            if(!Door) {
                Add(TEXT("Cube"),TEXT("SolidDeadEnd"),P+D*(Edge-15)+FVector(0,0,170),FVector(.55f,Across/50,3.4f),Floor*.7f,true,Yaw);
                Add(TEXT("Cube"),TEXT("WallCap"),P+D*(Edge-15)+FVector(0,0,347),FVector(.72f,Across/50+.1f,.18f),Floor*1.4f,false,Yaw);
            } else {
                for(int32 S:{-1,1}) Add(TEXT("Cube"),TEXT("DoorWing"),P+D*(Edge-15)+Side*S*(Across+240)*.5f+FVector(0,0,145),FVector(.55f,(Across-240)/100,2.9f),Floor*.7f,true,Yaw);
                Add(TEXT("SM_Arch"),TEXT("Arch"),P+D*(Edge-20),FVector(1),Floor*1.3f,true,Yaw);
            }
        }
        if(Biome()==0||Biome()==1) {
            Add(TEXT("SM_BarrelVault"),TEXT("MasonryVault"),P,FVector(Half.X/300.f+0.05f,Half.Y/500.f+0.05f,1.05f),Biome()==0?Floor*.8f:Floor*.7f,true);
            for(int32 Sign:{-1,1}) Add(TEXT("Cube"),TEXT("WallUpper"),P+FVector(Sign*(Half.X-15),0,900),FVector(.55f,Half.Y/50.f,6.6f),Floor*.7f,true);
        }
        if(Biome()==2||Biome()==3||Biome()==4) Add(TEXT("SM_NaturalFloor"),TEXT("NaturalFloor"),P+FVector(0,0,4),FVector(Half.X/500.f,Half.Y/500.f,1),Floor,false);
        if(Biome()==4) {
            // Continuous vaulted shell plus rock walls, with all doorways kept open.
            Add(TEXT("SM_CaveVault"),TEXT("CaveVault"),P,FVector(Half.X/300.f+.1f,Half.Y/500.f+.08f,1.2f),FLinearColor(.24f,.22f,.18f),true);
            for(const FVector& D:Dirs) {
                bool Door=false;for(auto E:Layout.Edges) {int32 N=E.A==I?E.B:(E.B==I?E.A:-1);if(N>=0&&FVector::DotProduct((CellPosition(N)-P).GetSafeNormal(),D)>.95f) Door=true;}
                const float Edge=FMath::Abs(D.X)>.5f?Half.X:Half.Y,Across=FMath::Abs(D.X)>.5f?Half.Y:Half.X;
                const FVector Side(-D.Y,D.X,0);
                for(int32 J=-2;J<=2;++J) if(!Door||FMath::Abs(J)>1) Add(TEXT("SM_CaveRock"),TEXT("CaveWalls"),P+D*(Edge+110)+Side*J*(Across/2.7f)+FVector(0,0,350),FVector(1.4f,1.5f,4.2f),Floor*.65f,true,D.Rotation().Yaw);
                Add(TEXT("Cube"),TEXT("CaveUpperWall"),P+D*(Edge+25)+FVector(0,0,860),FVector(1.3f,Across/50.f,7.2f),FLinearColor(.24f,.22f,.18f),true,D.Rotation().Yaw);
            }
            for(int32 J=0;J<6;++J) {
                const float X=R.FRandRange(-Half.X*.8f,Half.X*.8f),Y=R.FRandRange(-Half.Y*.65f,Half.Y*.65f);
                const float Roof=1.2f*(420.f+620.f*FMath::Sqrt(FMath::Max(0.f,1.f-FMath::Square(Y/(Half.Y+40.f)))));
                Add(TEXT("SM_Stalactite"),TEXT("CeilingMinerals"),P+FVector(X,Y,Roof-20),FVector(R.FRandRange(.5f,1.3f)),FLinearColor(.32f,.29f,.22f),true,R.FRandRange(0,360));
            }
        }
        if(IsEndlessArena()&&I==0) EndlessArena(P);
        else if(HasBossArena()&&I==0) BossArena(P);
        else if(Biome()==0) Archives(P,I);else {Props(P,I,R);Landmarks(P,I);}
        if(I%4==2 && I!=Layout.Exit) {
            const FVector W=P+FVector(200,220,9);Water.Add(W);
            Add(TEXT("Cylinder"),TEXT("Water"),W,FVector(3.8f,3.8f,.025f),FLinearColor(.045f,.19f,.24f),false,0,.35f,.6f);
            Add(TEXT("SM_Ring"),TEXT("WaterRim"),W,FVector(3.85f,3.85f,2),Floor*1.3f,false);
        }
        if(Biome()==0||Biome()==1||Biome()==4) {
            const FVector Lamp=P+FVector(-Half.X*.65f,Half.Y*.63f,190);
            Add(TEXT("Cylinder"),TEXT("LanternStand"),Lamp-FVector(0,0,95),FVector(.06f,.06f,1.9f),FLinearColor(.16f,.12f,.07f),true);
            Add(TEXT("SM_Lantern"),TEXT("Lanterns"),Lamp,FVector(.7f),FLinearColor(.22f,.15f,.07f),false);
            Add(TEXT("Sphere"),TEXT("LanternGlow"),Lamp+FVector(0,0,38),FVector(.13f,.13f,.32f),FLinearColor(1,.58f,.22f),false,0,5);
            Light(Lamp+FVector(0,0,60),FLinearColor(1,.68f,.38f),2200,1600);
            if(Biome()==4&&I%3==0) Light(P+FVector(Half.X*.65f,-Half.Y*.6f,90),Glow,450,700);
        } else if(I%3==0) Light(P+FVector(-370,370,190),Glow,650,1200);
        if((I==0 || I==Layout.Exit)&&!IsEndlessArena()) Add(TEXT("SM_Ring"),TEXT("ArrivalGlyph"),P+FVector(0,0,13),FVector(4,4,1),Glow,false,0,3);
        // Floating underside rock spires and distant ruins provide depth without collision or ticking actors.
        Add(TEXT("SM_Rock"),TEXT("Underside"),P+FVector(0,0,-450),FVector(5,5,5),Floor*.5f,false);
    }
    for(auto E:Layout.Edges) {
        const FVector A=CellPosition(E.A),B=CellPosition(E.B),Dir=(B-A).GetSafeNormal(),Side(-Dir.Y,Dir.X,0);
        const auto HA=CellHalfSize(E.A),HB=CellHalfSize(E.B);const bool AlongX=FMath::Abs(Dir.X)>.5f;
        const FVector Start=A+Dir*(AlongX?HA.X:HA.Y),End=B-Dir*(AlongX?HB.X:HB.Y),Mid=(Start+End)*.5f;
        const float Length=FVector::Dist2D(Start,End)+40,Yaw=Dir.Rotation().Yaw;
        if(!E.Gap) Add(TEXT("Cube"),TEXT("Bridge"),Mid+FVector(0,0,-50),FVector(Length/100,4.4f,1),Floor,true,Yaw);
        else {
            const float Gap=FMath::Min(360.f,Length-40),Stub=(Length-Gap)*.5f;
            for(int32 Sign:{-1,1}) {
                if(Stub>5) Add(TEXT("Cube"),TEXT("GapApproach"),Mid+Dir*Sign*(Gap+Stub)*.5f+FVector(0,0,-50),FVector(Stub/100,4.4f,1),Floor,true,Yaw);
                Add(TEXT("SM_Ring"),TEXT("DashMark"),Mid+Dir*Sign*(Gap*.5f+100)+FVector(0,0,12),FVector(1.2f,1.2f,1),FLinearColor(1,.7f,.12f),false,0,5);
            }
            Text(TEXT("JUMP / DASH"),Mid+FVector(0,0,150),Yaw+180,24);
        }
        if(Biome()==4||Biome()==1||Biome()==0) {
            const TCHAR* Roof=Biome()==4?TEXT("SM_CaveVault"):TEXT("SM_BarrelVault");
            Add(Roof,TEXT("PassageRoof"),Mid,FVector((Length+140)/600.f,.62f,1.12f),Biome()==4?FLinearColor(.24f,.22f,.18f):Floor*.7f,true,Yaw);
            for(int32 Sign:{-1,1}) Add(TEXT("Cube"),TEXT("PassageWalls"),Mid+Side*Sign*310+FVector(0,0,240),FVector((Length+100)/100.f,1.5f,4.8f),Floor*.7f,true,Yaw);
        }
        for(int32 Sign:{-1,1}) Add(TEXT("Cube"),TEXT("BridgeRail"),Mid+Side*Sign*240+FVector(0,0,92),FVector((Length+20)/100,.22f,1.84f),Floor*.65f,true,Yaw);
    }
    if(HasVault()) {
        for(int32 Part=1;Part<=2;++Part) {
            const FVector P=VaultPoint(Part);Add(TEXT("SM_Obelisk"),TEXT("VaultSigil"),P,FVector(.3f),Glow,false,0,2);
            Add(TEXT("SM_Ring"),TEXT("VaultSigilRing"),P+FVector(0,0,8),FVector(1.1f,1.1f,1),Glow,false,0,4);

        }
        Add(TEXT("SM_Chest"),TEXT("VaultCache"),VaultPoint(3)+FVector(0,0,50),FVector(.85f),FLinearColor(.15f,.5f,.7f),false,0,1,.5f);

    }
    if(HasShrine()) {
        Add(TEXT("SM_Obelisk"),TEXT("BargainShrine"),ShrinePoint(),FVector(.42f),FLinearColor(.38f,.18f,.055f),true,0,.4f);
        Add(TEXT("SM_Ring"),TEXT("BargainSigil"),ShrinePoint()+FVector(0,0,8),FVector(1.6f,1.6f,1),FLinearColor(1,.5f,.15f),false,0,2);
        Text(TEXT("EMBER BARGAIN / E"),ShrinePoint()+FVector(0,0,220),180,23);
    }
    if(HasWell()) {
        Add(TEXT("SM_Ring"),TEXT("WishingWellRim"),WellPoint()+FVector(0,0,10),FVector(1.4f,1.4f,1),FLinearColor(.1f,.45f,.62f),true,0,.8f);
        Add(TEXT("SM_Ring"),TEXT("WishingWellWater"),WellPoint()+FVector(0,0,24),FVector(1.0f,1.0f,1),FLinearColor(.2f,.8f,1),false,0,3);
        Text(TEXT("WISHING WELL / E"),WellPoint()+FVector(0,0,220),180,23);
    }
    const FVector Exit=ExitPoint();
    if(ASRules::Final(Config.Room)) {
        Add(TEXT("SM_Arch"),TEXT("ReturnPortalArch"),Exit,FVector(1.35f),Floor,true);
        Add(TEXT("Sphere"),TEXT("ReturnPortal"),Exit+FVector(0,0,240),FVector(.08f,3.7f,4.5f),FLinearColor(.06f,.75f,.65f),false,0,1.5f);
        Text(TEXT("RETURN PORTAL / CLEAR THE FINAL CHAMBER"),Exit+FVector(0,0,590),180,23);
    } else {
        Add(TEXT("SM_Chest"),TEXT("Chest"),Exit+FVector(0,0,52),FVector(1.2f),FLinearColor(.5f,.29f,.09f),true,0,0,.45f);

    }
    Text(ASData::Biomes[Biome()],CellPosition(0)+FVector(560,0,580),180,38);
    // Put non-colliding silhouettes outside the map's full bounding square.
    // The old fixed 6500cm ring could overlap large playable rooms.
    FVector BackdropCenter;float BackdropSpan;MapBounds(BackdropCenter,BackdropSpan);
    const float BackdropRadius=FMath::Max(8000.f,BackdropSpan*.8f+2400.f);
    // Backdrop silhouettes are geometry, not flat source-game billboards.
    for(int32 I=0;I<32;++I) {
        float A=I*UE_TWO_PI/32;FVector P=BackdropCenter+FVector(FMath::Cos(A)*BackdropRadius,FMath::Sin(A)*BackdropRadius,-R.FRandRange(300,1500));
        Add(Biome()==4?TEXT("SM_CaveRock"):(Biome()==3?TEXT("SM_Tree"):(Biome()==2?TEXT("SM_CrystalCluster"):TEXT("SM_Pillar"))),TEXT("SkyRuins"),P,FVector(R.FRandRange(3,5),R.FRandRange(3,5),R.FRandRange(3,8)),Floor*.4f,false);
        Add(TEXT("SM_Rock"),TEXT("SkyIslands"),P-FVector(0,0,350),FVector(7,7,5),Floor*.4f,false);
    }
    if(Biome()==2) Add(TEXT("Sphere"),TEXT("Moon"),FVector(4500,5500,3900),FVector(18),FLinearColor(.50f,.65f,1),false,0,2);
}
void AASWorld::Props(const FVector& P,int32 I,FRandomStream& R) {
    const FLinearColor Bone(.68f,.60f,.40f),Wood(.17f,.082f,.035f),Stone(.28f,.28f,.26f),Glow=ASData::BiomeColor(Biome());
    if(IsVaultCell(I)) return;
    for(int32 X:{-1,1}) for(int32 Y:{-1,1}) {
        if(I!=0 && R.RandRange(0,4)==0) continue;
        const auto Half=CellHalfSize(I);const FVector Q=P+FVector(X*(Half.X-180),Y*(Half.Y-180),0);const float Yaw=R.FRandRange(0,360);
        if(Biome()==4) {
            Add(TEXT("SM_CrystalCluster"),TEXT("CaveCrystals"),Q,FVector(.85f,.85f,1.2f),Glow,true,Yaw,.8f,.35f);
            Add(TEXT("SM_CaveRock"),TEXT("CaveBoulders"),Q+FVector(-70,50,50),FVector(.8f,.7f,.6f),Stone*.55f,true,Yaw);
            if(X==Y) Add(TEXT("SM_Stalagmite"),TEXT("Calcite"),Q+FVector(-85,45,0),FVector(.85f,.85f,1.4f),FLinearColor(.42f,.37f,.27f),true,Yaw);
        } else if(Biome()==1) {
            Add(TEXT("SM_Skull"),TEXT("Skull"),Q,FVector(1.2f),Bone,true,Yaw);
            Add(TEXT("SM_Bone"),TEXT("Bones"),Q+FVector(0,65,25),FVector(1.3f),Bone,false,Yaw+35);
            Add(TEXT("SM_Bone"),TEXT("Bones"),Q+FVector(0,-35,25),FVector(1.3f),Bone,false,Yaw-40);
            if(X==Y) Add(TEXT("SM_Obelisk"),TEXT("Tombs"),Q,FVector(.6f),Stone,true,Yaw);
            for(int32 J=0;J<8;++J) {
                Add(TEXT("SM_Skull"),TEXT("OssuaryNiches"),Q+FVector((J%4)*37-55,55,80+(J/4)*42),FVector(.38f),Bone,false,Yaw);
                Add(TEXT("SM_Bone"),TEXT("OssuaryBones"),Q+FVector((J%4)*37-55,40,60+(J/4)*42),FVector(.4f),Bone,false,Yaw+90);
            }
        } else if(Biome()==2) {
            for(int32 J=0;J<4;++J) Add(TEXT("SM_BasaltColumn"),TEXT("ColumnarBasalt"),Q+FVector((J%2)*110,(J/2)*110,0),FVector(.65f,.65f,.6f+J*.4f),FLinearColor(.065f,.075f,.085f),true,Yaw);
            Add(TEXT("SM_Crystal"),TEXT("Crystals"),Q+FVector(-65,-65,0),FVector(.65f),Glow,true,Yaw,.25f,.2f);
            Add(TEXT("SM_Rock"),TEXT("Basalt"),Q+FVector(50,0,20),FVector(1.15f),FLinearColor(.07f,.075f,.12f),true,Yaw);
        } else {
            Add(TEXT("SM_Tree"),TEXT("Trees"),Q,FVector(.8f),Wood,true,Yaw);
            Add(TEXT("SM_Canopy"),TEXT("Canopy"),Q,FVector(.8f),FLinearColor(.06f,.22f,.06f),false,Yaw);
            Add(TEXT("SM_RootLog"),TEXT("NurseLog"),Q+FVector(-80,40,0),FVector(.7f),FLinearColor(.12f,.18f,.07f),true,Yaw);
            for(int32 J=0;J<7;++J) Add(TEXT("SM_Leaf"),TEXT("Ferns"),Q+FVector(80*FMath::Cos(static_cast<float>(J)),80*FMath::Sin(static_cast<float>(J)),18+J*7),FVector(.9f),Glow*.65f,false,Yaw+J*53);
        }
    }
    if(I%3==0) {
        FVector B=P+FVector(-370,370,0);Add(TEXT("SM_Brazier"),TEXT("Braziers"),B,FVector(.8f),Wood);
        Add(TEXT("Sphere"),TEXT("Flame"),B+FVector(0,0,132),FVector(.40f,.40f,.70f),Glow,false,0,7);
    }
}
void AASWorld::Landmarks(const FVector& P,int32 I) {
    const auto Half=CellHalfSize(I);if(Half.X<1100||Half.Y<1100) return;
    const FLinearColor Stone(.26f,.25f,.22f),Warm(.62f,.43f,.20f),Glow=ASData::BiomeColor(Biome());
    // Four generous quadrants around a permanently open 720cm cross. Vault,
    // shrine, arrival, chest and all cardinal doors stay connected to this spine.
    const int32 Motif=I%3;
    for(int32 SX:{-1,1}) for(int32 SY:{-1,1}) {
        const FVector Q=P+FVector(SX*Half.X*.62f,SY*Half.Y*.62f,0);
        if(Biome()==1) {
            // Processional burial courts, set outside the central combat lanes.
            Add(TEXT("Cube"),TEXT("TombDais"),Q+FVector(0,0,14),FVector(3.7f,4.6f,.28f),Stone,true);
            Add(TEXT("SM_Obelisk"),TEXT("CourtyardTomb"),Q+FVector(0,0,28),FVector(.55f,.55f,.8f),FLinearColor(.22f,.14f,.16f),true,SX*35);
            for(int32 Sign:{-1,1}) {
                Add(TEXT("SM_Brazier"),TEXT("FuneralBraziers"),Q+FVector(Sign*180,SY*180,0),FVector(.7f),Warm,true);
                Add(TEXT("Sphere"),TEXT("FuneralFlame"),Q+FVector(Sign*180,SY*180,116),FVector(.22f,.22f,.5f),Glow,false,0,3);
            }
            Add(TEXT("SM_Ring"),TEXT("FuneralInlay"),Q+FVector(0,0,32),FVector(3.9f,4.8f,1),Warm,false,0,.3f);
        } else if(Biome()==2) {
            // Irregular basalt fans frame clear, curved routes around each grove.
            for(int32 J=0;J<5;++J) {
                const float Angle=(35.f+J*28.f+Motif*18.f)*UE_PI/180.f;
                const FVector At=Q+FVector(SX*FMath::Cos(Angle)*175,SY*FMath::Sin(Angle)*175,0);
                Add(TEXT("SM_BasaltColumn"),TEXT("BasaltFans"),At,FVector(.55f,.55f,.60f+J*.20f),FLinearColor(.06f,.075f,.105f),true,J*30.f);
            }
            Add(TEXT("SM_CrystalCluster"),TEXT("BasaltHeart"),Q,FVector(.8f),Glow,true,35,.7f);
            Add(TEXT("Cube"),TEXT("LavaSeam"),Q+FVector(-SX*90,-SY*130,11),FVector(3.4f,.045f,.025f),Glow,false,SX*SY*25.f,1.4f);
        } else if(Biome()==3) {
            // Broken abbey arches and small tree islands make a readable cloister loop.
            if(SX==SY||Motif==2) {
                Add(TEXT("SM_Arch"),TEXT("GardenRuins"),Q,FVector(.85f),Stone,true,SY*90.f);
                Add(TEXT("SM_Leaf"),TEXT("IvyCrown"),Q+FVector(20,0,310),FVector(2.2f),FLinearColor(.10f,.30f,.13f),false,SX*45.f);
            } else {
                Add(TEXT("SM_Tree"),TEXT("GardenIsland"),Q,FVector(.95f),FLinearColor(.14f,.075f,.035f),true);
                Add(TEXT("SM_Canopy"),TEXT("GardenCrown"),Q,FVector(1.15f),FLinearColor(.075f,.23f,.11f),false);
                Add(TEXT("SM_RootLog"),TEXT("GardenRoots"),Q+FVector(0,110,0),FVector(.8f),Stone*.6f,true,45);
            }
            for(int32 J=0;J<5;++J) Add(TEXT("SM_Leaf"),TEXT("GardenFerns"),Q+FVector((J-2)*75,SY*170,14),FVector(1.2f),FLinearColor(.18f,.38f,.14f),false,J*58.f);
        } else {
            // Shallow turquoise pools sit between pale mineral terraces. The water
            // shares the existing slowdown radius; the central routes remain dry.
            if(SX==SY) {
                const FVector Pool=Q+FVector(0,0,10);Water.Add(Pool);
                Add(TEXT("Cylinder"),TEXT("MineralPool"),Pool,FVector(3.6f,3.6f,.03f),FLinearColor(.065f,.38f,.42f),false,0,.25f,.65f);
                for(int32 Ring=0;Ring<3;++Ring) Add(TEXT("SM_Ring"),TEXT("CalciteTerrace"),Q+FVector(0,0,13+Ring*3),FVector(3.8f+Ring*.28f,3.8f+Ring*.28f,1.4f),FLinearColor(.52f,.47f,.34f),false);
                Add(TEXT("SM_Stalagmite"),TEXT("PoolMinerals"),Q+FVector(SX*220,SY*90,0),FVector(.8f,.8f,1.5f),FLinearColor(.48f,.41f,.28f),true);
            } else {
                Add(TEXT("SM_CaveRock"),TEXT("KarstIsland"),Q,FVector(1.1f,1.4f,.7f),FLinearColor(.30f,.28f,.22f),true,25);
                Add(TEXT("SM_CrystalCluster"),TEXT("KarstLight"),Q+FVector(-SX*90,SY*80,35),FVector(.75f),Glow,false,0,.9f);
            }
        }
    }
    Add(TEXT("SM_Ring"),TEXT("RoomMedallion"),P+FVector(0,0,11),FVector(Motif==0?4.5f:3.4f,Motif==0?4.5f:3.4f,.7f),Biome()==4?Stone:Warm,false,0,.15f,.25f);
    if(Biome()==0||Biome()==1||Biome()==4) {
        // Warm/cool pools of light pick out the far ends of larger rooms.
        for(int32 Sign:{-1,1}) Light(P+FVector(Sign*Half.X*.55f,-Half.Y*.55f,330),Biome()==4?FLinearColor(.18f,.63f,.70f):FLinearColor(1,.63f,.30f),1800,1500);
    }
}
FVector AASWorld::CellPosition(int32 I) const {
    if(I<0||I>=int32(Layout.Cells.size())) return FVector::ZeroVector;
    return FVector(Layout.Cells[I].WorldX-Layout.Cells[0].WorldX,Layout.Cells[I].WorldY-Layout.Cells[0].WorldY,0);
}
FVector AASWorld::SpawnPoint(int32 I) const {
    if(HasBossArena()) return CellPosition(0)+FVector(-ASRules::BossArenaHalf+450.f,I==0?-150.f:150.f,100);
    return CellPosition(0)+FVector(-210,I==0?-100:100,100);
}
FVector AASWorld::ExitPoint() const {return CellPosition(Layout.Exit)+FVector(-220,0,0);}
int32 AASWorld::NearestCell(const FVector& P) const {
    int32 Best=0;double D=DBL_MAX;for(int32 I=0;I<int32(Layout.Cells.size());++I) {const FVector Offset=P-CellPosition(I);const auto Half=CellHalfSize(I);const double T=FMath::Square(FMath::Max(0.0,FMath::Abs(Offset.X)-Half.X))+FMath::Square(FMath::Max(0.0,FMath::Abs(Offset.Y)-Half.Y));if(T<D){D=T;Best=I;}}return Best;
}
FVector2D AASWorld::CellHalfSize(int32 I) const {
    if(I<0||I>=int32(Layout.Cells.size())) return FVector2D(600,600);
    return FVector2D(Layout.Cells[I].HalfX,Layout.Cells[I].HalfY);
}
void AASWorld::MapBounds(FVector& Center,float& Span) const {
    if(Config.Room<0||Layout.Cells.empty()) {Center=FVector::ZeroVector;Span=4100;return;}
    FVector Low(FLT_MAX,FLT_MAX,0),High(-FLT_MAX,-FLT_MAX,0);
    for(int32 I=0;I<int32(Layout.Cells.size());++I) {
        const auto P=CellPosition(I);const auto H=CellHalfSize(I);
        Low.X=FMath::Min(Low.X,P.X-H.X);Low.Y=FMath::Min(Low.Y,P.Y-H.Y);
        High.X=FMath::Max(High.X,P.X+H.X);High.Y=FMath::Max(High.Y,P.Y+H.Y);
    }
    Center=(Low+High)*.5f;Span=FMath::Max(High.X-Low.X,High.Y-Low.Y)+700.f;
}
bool AASWorld::FloorAt(const FVector& P) const {
    for(int32 I=0;I<int32(Layout.Cells.size());++I) {
        const auto C=CellPosition(I);const auto H=CellHalfSize(I);
        if(FMath::Abs(P.X-C.X)<=H.X&&FMath::Abs(P.Y-C.Y)<=H.Y) return true;
    }
    for(auto E:Layout.Edges) {
        const FVector A=CellPosition(E.A),B=CellPosition(E.B),D=(B-A).GetSafeNormal2D(),Side(-D.Y,D.X,0);
        if(FMath::Abs(FVector::DotProduct(P-A,Side))>220||FVector::DotProduct(P-A,D)<0||FVector::DotProduct(P-B,D)>0) continue;
        if(!E.Gap) return true;
        const auto HA=CellHalfSize(E.A),HB=CellHalfSize(E.B);const bool AlongX=FMath::Abs(D.X)>.5f;
        const FVector Start=A+D*(AlongX?HA.X:HA.Y),End=B-D*(AlongX?HB.X:HB.Y),Mid=(Start+End)*.5f;
        const float Gap=FMath::Min(360.f,float(FVector::Dist2D(Start,End)));
        if(FMath::Abs(FVector::DotProduct(P-Mid,D))>=Gap*.5f) return true; // Supported approach stubs, never the gap.
    }
    return false;
}
FVector AASWorld::NavPoint(int32 I) const {return NavOrigin+FVector((I%Navigation.Width)*120,(I/Navigation.Width)*120,0);}
int32 AASWorld::NearestNode(const FVector& P,bool ForGuide) const {
    const auto& Grid=ForGuide?GuideNavigation:Navigation;
    int32 Best=-1;double Distance=DBL_MAX;
    if(Grid.Width<=0) return -1;
    const int32 X=FMath::RoundToInt((P.X-NavOrigin.X)/120),Y=FMath::RoundToInt((P.Y-NavOrigin.Y)/120);
    for(int32 R=0;R<=6;++R) {
        for(int32 DY=-R;DY<=R;++DY) for(int32 DX=-R;DX<=R;++DX) {
            if(FMath::Max(FMath::Abs(DX),FMath::Abs(DY))!=R) continue;
            const int32 NX=X+DX,NY=Y+DY;if(NX<0||NY<0||NX>=Grid.Width||NY>=Grid.Height) continue;
            const int32 I=NY*Grid.Width+NX;if(!Grid.Open[I]) continue;
            const double D=FVector::DistSquared2D(P,NavPoint(I));if(D<Distance) {Distance=D;Best=I;}
        }
        if(Best>=0&&FMath::Square(FMath::Max(0,R*120-85))>Distance) return Best;
    }
    for(int32 I=0;I<int32(Grid.Open.size());++I) if(Grid.Open[I]) {
        const double D=FVector::DistSquared2D(P,NavPoint(I));if(D<Distance) {Distance=D;Best=I;}
    }
    return Best;
}
void AASWorld::BuildNavigation() {
    // The routing grids describe the cover as it will stand, so build them against SOLID proxies even in the middle of a shift (during the swap the
    // proxies are switched off, which used to make every route run straight through the new walls: the 0.23.4 "monsters stuck on walls" bug).
    const bool RestoreOff=SolidState==0;
    if(RestoreOff) SetCoverCollision(true);
    BuildNavigationGrids();
    if(RestoreOff) SetCoverCollision(false);
}
void AASWorld::BuildNavigationGrids() {
    Navigation.Blocked.clear();
    FVector Center;float Span;MapBounds(Center,Span);
    NavOrigin=FVector(FMath::FloorToFloat((Center.X-Span*.5f)/120)*120,FMath::FloorToFloat((Center.Y-Span*.5f)/120)*120,0);
    Navigation.Width=FMath::CeilToInt(Span/120)+1;Navigation.Height=Navigation.Width;
    Navigation.Open.assign(Navigation.Width*Navigation.Height,0);
    FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);FCollisionQueryParams Query;
    for(int32 I=0;I<int32(Navigation.Open.size());++I) {
        const FVector P=NavPoint(I);bool Supported=FloorAt(P);
        for(int32 J=0;J<8&&Supported;++J) {const float A=J*UE_TWO_PI/8;Supported=FloorAt(P+FVector(FMath::Cos(A)*92,FMath::Sin(A)*92,0));}
        if(Supported&&!GetWorld()->OverlapAnyTestByObjectType(P+FVector(0,0,214),FQuat::Identity,Objects,FCollisionShape::MakeCapsule(92,210),Query)) Navigation.Open[I]=1;
    }
    // A pair of clear endpoints can still straddle a thin obstacle. Validate the
    // entire capsule move once, so A* never repeatedly selects a blocked edge.
    for(int32 I=0;I<int32(Navigation.Open.size());++I) if(Navigation.Open[I]) {
        for(int32 N:{I+1,I+Navigation.Width}) if(Navigation.CanStep(I,N)) {
            FHitResult Hit;
            if(GetWorld()->SweepSingleByObjectType(Hit,NavPoint(I)+FVector(0,0,214),NavPoint(N)+FVector(0,0,214),FQuat::Identity,Objects,FCollisionShape::MakeCapsule(92,210),Query)) Navigation.BlockEdge(I,N);
        }
    }
    // Player routing uses the actual human-sized capsule. The enemy grid stays
    // conservative for the much taller elites and bosses.
    GuideNavigation.Width=Navigation.Width;GuideNavigation.Height=Navigation.Height;
    GuideNavigation.Open.assign(Navigation.Open.size(),0);GuideNavigation.Blocked.clear();
    for(int32 I=0;I<int32(GuideNavigation.Open.size());++I) if(WalkClear(NavPoint(I),NavPoint(I),true)) GuideNavigation.Open[I]=1;
    for(int32 I=0;I<int32(GuideNavigation.Open.size());++I) if(GuideNavigation.Open[I])
        for(int32 N:{I+1,I+GuideNavigation.Width}) if(GuideNavigation.CanStep(I,N)&&!WalkClear(NavPoint(I),NavPoint(N),true)) GuideNavigation.BlockEdge(I,N);
    Navigation.KeepConnected(NearestNode(SpawnPoint(0)));
    int32 Count=0;for(auto Open:Navigation.Open) Count+=Open?1:0;
    UE_LOG(LogTemp,Display,TEXT("AshenNavigation: shape=%d rooms=%d grid=%dx%d reachable=%d"),Layout.Shape,int32(Layout.Cells.size()),Navigation.Width,Navigation.Height,Count);
}
bool AASWorld::WalkClear(const FVector& From,const FVector& To,bool Player) const {
    const float Radius=Player?38.f:92.f,HalfHeight=Player?88.f:210.f,CenterZ=HalfHeight+4.f;
    const int32 Steps=FMath::Max(1,FMath::CeilToInt(FVector::Dist2D(From,To)/60.f));
    for(int32 I=0;I<=Steps;++I) {
        const FVector P=FMath::Lerp(From,To,float(I)/Steps);
        for(int32 J=0;J<8;++J) {const float A=J*UE_TWO_PI/8;if(!FloorAt(P+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,0))) return false;}
    }
    FHitResult Hit;FCollisionQueryParams Query;FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);
    return !GetWorld()->SweepSingleByObjectType(Hit,FVector(From.X,From.Y,CenterZ),FVector(To.X,To.Y,CenterZ),FQuat::Identity,Objects,FCollisionShape::MakeCapsule(Radius,HalfHeight),Query);
}
FVector AASWorld::Route(const FVector& From,const FVector& To) const {
    if(WalkClear(From,To)) return To;
    const int32 A=NearestNode(From),B=NearestNode(To);const auto Path=Navigation.Path(A,B);
    if(Path.empty()) return From; // No route: wait for a valid target instead of retreating to spawn.
    // Reaching one 120cm node must not make an enemy overshoot and reverse every
    // route tick. Look ahead only along capsule-clear, fully supported floor.
    for(int32 I=FMath::Min(6,int32(Path.size())-1);I>=0;--I) if(WalkClear(From,NavPoint(Path[I]))) return NavPoint(Path[I]);
    return NavPoint(A);
}
bool AASWorld::GuideJumpClear(const FVector& From,const FVector& To) const {
    // Matches the hero's 660 jump velocity and 1.6 gravity. Longer links need
    // the existing air dash, explicitly signposted by the route guide.
    if(!FloorAt(From)||!FloorAt(To)||FVector::Dist2D(From,To)>800.f) return false;
    const float Gravity=FMath::Abs(GetWorld()->GetGravityZ())*1.6f;
    if(Gravity<1.f) return false;const float Flight=1320.f/Gravity;
    FCollisionQueryParams Query;const FCollisionObjectQueryParams Objects(ECC_WorldStatic);
    FVector Previous=From+FVector(0,0,92);
    for(int32 Step=1;Step<=20;++Step) {
        const float T=float(Step)/20,Time=T*Flight;
        const FVector Next=FMath::Lerp(From,To,T)+FVector(0,0,92+660.f*Time-.5f*Gravity*Time*Time);
        FHitResult Hit;
        if(GetWorld()->SweepSingleByObjectType(Hit,Previous,Next,FQuat::Identity,Objects,FCollisionShape::MakeCapsule(34,88),Query)) return false;
        Previous=Next;
    }
    return true;
}
bool AASWorld::AtExit(const FVector& Position) const {
    const FVector Goal=ExitPoint();
    if(FVector::DistSquared2D(Position,Goal)>280.f*280.f||FMath::Abs(Position.Z-Goal.Z)>210.f) return false;
    FHitResult Hit;const FCollisionObjectQueryParams Objects(ECC_WorldStatic);
    return !GetWorld()->LineTraceSingleByObjectType(Hit,FVector(Position.X,Position.Y,95),Goal+FVector(0,0,95),Objects)
        ||FVector::DistSquared2D(Hit.ImpactPoint,Goal)<120.f*120.f;
}
bool AASWorld::GuideRoute(const FVector& From,const FVector& To,TArray<FVector>& Points,TArray<bool>& JumpLegs,float GoalRadius) {
    Points.Reset();JumpLegs.Reset();if(Config.Room<0) return false;
    const auto* State=AS::State(this);const int32 BreakableCount=State?State->Breakables.Num():0;
    if(!bGuideNavigationReady||BreakableCount!=GuideBreakableCount) {BuildNavigation();bGuideNavigationReady=true;GuideBreakableCount=BreakableCount;}
    const FVector Start(From.X,From.Y,0),Goal(To.X,To.Y,0);
    const auto AddPoint=[&](const FVector& Point,bool Jump=false){if(Points.IsEmpty()||!Points.Last().Equals(Point,1.f)) {Points.Add(Point);JumpLegs.Add(Jump);}};
    // The interaction ring is the goal, not a hard-coded point east of a chest.
    const FVector Direct=GoalRadius>0?Goal+(Start-Goal).GetSafeNormal2D()*FMath::Min(GoalRadius,float(FVector::Dist2D(Start,Goal))):Goal;
    if(WalkClear(Start,Direct,true)&&(GoalRadius<=0||AtExit(Direct))) {AddPoint(Start);AddPoint(Direct);return Points.Num()>1;}
    std::vector<ASNavigation::Endpoint> Starts,Goals;
    for(int32 I=0;I<int32(GuideNavigation.Open.size());++I) if(GuideNavigation.Open[I]) {
        const FVector P=NavPoint(I);const float A=FVector::Dist2D(Start,P),B=FVector::Dist2D(Goal,P);
        if(A<=800.f&&WalkClear(Start,P,true)) Starts.push_back({I,A/120.f});
        if(GoalRadius>0) {if(B<=GoalRadius&&AtExit(P)) Goals.push_back({I,0.f});}
        else if(B<=800.f&&WalkClear(P,Goal,true)) Goals.push_back({I,B/120.f});
    }
    if(Starts.empty()||Goals.empty()) return false;
    std::vector<ASNavigation::Link> Links;
    for(const auto& Edge:Layout.Edges) if(Edge.Gap) {
        const FVector A=CellPosition(Edge.A),B=CellPosition(Edge.B),D=(B-A).GetSafeNormal2D();
        const auto HA=CellHalfSize(Edge.A),HB=CellHalfSize(Edge.B);const bool AlongX=FMath::Abs(D.X)>.5f;
        const FVector LipA=A+D*(AlongX?HA.X:HA.Y),LipB=B-D*(AlongX?HB.X:HB.Y),Middle=(LipA+LipB)*.5f;
        const float Gap=FMath::Min(360.f,float(FVector::Dist2D(LipA,LipB)));
        // Validate both lip connectors: nearest-node snapping cannot cross the gap.
        const FVector PA=Middle-D*(Gap*.5f+120.f),PB=Middle+D*(Gap*.5f+120.f);
        const int32 IA=NearestNode(PA,true),IB=NearestNode(PB,true);
        if(IA>=0&&IB>=0&&IA!=IB&&WalkClear(PA,NavPoint(IA),true)&&WalkClear(PB,NavPoint(IB),true)&&GuideJumpClear(NavPoint(IA),NavPoint(IB))) Links.push_back({IA,IB,float(FVector::Dist2D(NavPoint(IA),NavPoint(IB)))/120.f});
    }
    const auto Path=ASNavigation::GuidePath(GuideNavigation,Starts,Goals,Links);if(Path.empty()) return false;
    AddPoint(Start);int32 At=0;
    while(At<int32(Path.size())) {
        const bool Jump=At>0&&ASNavigation::IsJump(Links,Path[At-1],Path[At]);
        if(Jump) {AddPoint(NavPoint(Path[At]),true);++At;continue;}
        int32 Limit=At;
        while(Limit+1<int32(Path.size())&&Limit<At+16&&!ASNavigation::IsJump(Links,Path[Limit],Path[Limit+1])) ++Limit;
        int32 Next=At;for(int32 Test=Limit;Test>At;--Test) if(WalkClear(Points.Last(),NavPoint(Path[Test]),true)) {Next=Test;break;}
        if(!WalkClear(Points.Last(),NavPoint(Path[Next]),true)) {Points.Reset();JumpLegs.Reset();return false;}
        AddPoint(NavPoint(Path[Next]));At=Next+1;
    }
    if(GoalRadius<=0) {if(!WalkClear(Points.Last(),Goal,true)) {Points.Reset();JumpLegs.Reset();return false;}AddPoint(Goal);}
    return Points.Num()>1;
}
void AASWorld::UpdateVaultAppearance() {
    const auto* State=AS::State(this);if(!State||!HasVault()||State->bVaultClaimed==bVaultDimmed) return;
    bVaultDimmed=State->bVaultClaimed;
    for(auto& Pair:Batches) if(Pair.Value&&Pair.Key.ToString().Contains(TEXT("_Vault"))) {
        if(auto* Material=Cast<UMaterialInstanceDynamic>(Pair.Value->GetMaterial(0))) {
            const bool Cache=Pair.Key.ToString().Contains(TEXT("VaultCache"));
            Material->SetVectorParameterValue(TEXT("Tint"),bVaultDimmed?FLinearColor(.20f,.24f,.28f):(Cache?FLinearColor(.15f,.5f,.7f):ASData::BiomeColor(Biome())));
            Material->SetScalarParameterValue(TEXT("Emission"),bVaultDimmed?0.f:(Cache?1.f:3.f));
        }
    }
}
FVector AASWorld::RecoverEnemy(const FVector& From,const FVector& To) const {
    const auto Path=Navigation.Path(NearestNode(From),NearestNode(To));
    if(Path.empty()) return From;
    for(int32 I=FMath::Min(3,int32(Path.size())-1);I>=1;--I) {
        const FVector Point=NavPoint(Path[I])+FVector(0,0,240);
        if(!GetWorld()->OverlapAnyTestByObjectType(Point,FQuat::Identity,FCollisionObjectQueryParams(ECC_Pawn),FCollisionShape::MakeCapsule(92,210))) return Point;
    }
    return From;
}
bool AASWorld::JumpPlan(const FVector& From,const FVector& To,FVector& Takeoff,FVector& Landing) const {
    const int32 Goal=NearestNode(To);if(Goal<0) return false;
    for(auto E:Layout.Edges) if(E.Gap) {
        const FVector A=CellPosition(E.A),B=CellPosition(E.B),D=(B-A).GetSafeNormal2D();
        const auto HA=CellHalfSize(E.A),HB=CellHalfSize(E.B);const bool X=FMath::Abs(D.X)>.5f;
        const FVector Start=A+D*(X?HA.X:HA.Y),End=B-D*(X?HB.X:HB.Y),Mid=(Start+End)*.5f;
        const float Gap=FMath::Min(360.f,float(FVector::Dist2D(Start,End)));
        const int32 Side=FVector::DotProduct(From-Mid,D)<0?-1:1;
        const int32 NI=NearestNode(Mid+D*Side*(Gap*.5f+170)),NJ=NearestNode(Mid-D*Side*(Gap*.5f+170));
        if(NI<0||NJ<0||NI==NJ) continue;
        const FVector P=NavPoint(NI),Q=NavPoint(NJ);
        if(FVector::DistSquared2D(From,P)>700*700||FVector::Dist2D(P,Q)>1000||!WalkClear(From,P)) continue;
        const auto Before=Navigation.Path(NI,Goal),After=Navigation.Path(NJ,Goal);
        if(Before.empty()||After.empty()||Before.size()<=After.size()+6) continue;
        const float Time=.95f,Gravity=FMath::Abs(GetWorld()->GetGravityZ());
        FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);FCollisionQueryParams Query;
        FVector Last=P+FVector(0,0,83);bool Clear=true;
        for(int32 I=1;I<=12;++I) {
            const float T=float(I)/12,Height=Gravity*Time*Time*.5f*T*(1.f-T);
            const FVector Next=FMath::Lerp(P,Q,T)+FVector(0,0,83+Height);FHitResult Hit;
            if(GetWorld()->SweepSingleByObjectType(Hit,Last,Next,FQuat::Identity,Objects,FCollisionShape::MakeCapsule(38,80),Query)) {Clear=false;break;}Last=Next;
        }
        if(Clear) {Takeoff=P;Landing=Q;return true;}
    }
    return false;
}
FVector AASWorld::SafeEnemyPoint(const FVector& Near) const {
    const int32 I=NearestNode(Near);return (I>=0?NavPoint(I):CellPosition(0))+FVector(0,0,240);
}
bool AASWorld::FreeEnemyPoint(const FVector& Near,FVector& Point) const {
    FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_Pawn);Objects.AddObjectTypesToQuery(ECC_WorldStatic);
    float Best=1600.f*1600.f;bool Found=false;
    for(int32 I=0;I<int32(Navigation.Open.size());++I) if(Navigation.Open[I]) {
        const FVector At=NavPoint(I)+FVector(0,0,240);const float D=FVector::DistSquared2D(At,Near);if(D>=Best) continue;
        if(!GetWorld()->OverlapAnyTestByObjectType(At,FQuat::Identity,Objects,FCollisionShape::MakeCapsule(92,210))) {Point=At;Best=D;Found=true;}
    }
    return Found;
}
FVector AASWorld::EnemySpawn(const FVector& Near,FRandomStream& Random) const {
    TArray<int32> Options;
    for(int32 I=0;I<int32(Navigation.Open.size());++I) if(Navigation.Open[I]) {
        const double D=FVector::DistSquared2D(NavPoint(I),Near);
        if(D>=650*650&&D<=3200*3200) Options.Add(I);
    }
    return Options.IsEmpty()?SafeEnemyPoint(Near+FVector(800,0,0)):NavPoint(Options[Random.RandRange(0,Options.Num()-1)])+FVector(0,0,240);
}
bool AASWorld::InWater(const FVector& P) const {for(const FVector& W:Water) if(FVector::DistSquared2D(P,W)<180*180) return true;return false;}
FVector AASWorld::WeaponStand(int32 I) const {return FVector(-1160+FMath::Clamp(I,0,ASRules::WeaponCount-1)*290,-1150,0);}
int32 AASWorld::WeaponAt(const FVector& P) const {
    if(Config.Room>=0) return -1;
    int32 Best=-1;float Distance=320.f*320.f;
    for(int32 I=0;I<ASRules::WeaponCount;++I) {const float D=FVector::DistSquared2D(P,WeaponStand(I));if(D<Distance) {Distance=D;Best=I;}}
    return Best;
}
FVector AASWorld::VaultPoint(int32 Part) const {
    const int32 Cell=Layout.Exit==2?3:2;const FVector Center=CellPosition(Cell);
    if(Part==1) return Center+FVector(-300,-200,0);
    if(Part==2) return Center+FVector(300,-200,0);
    if(Part==3) return Center+FVector(0,280,0);
    return Center;
}
int32 AASWorld::VaultInteraction(const FVector& P) const {
    if(!HasVault()) return 0;
    for(int32 I=1;I<=3;++I) if(ASAdventure::InReach(FVector::DistSquared2D(P,VaultPoint(I)),P.Z-VaultPoint(I).Z)) return I;
    return 0;
}
int32 AASWorld::StationAt(const FVector& P,FString* Label) const {
    if(HasShrine()&&ASAdventure::InReach(FVector::DistSquared2D(P,ShrinePoint()),P.Z-ShrinePoint().Z)) {if(Label) *Label=TEXT("Ember bargain / trade health for a trait");return 220;}
    if(ShopOpen()&&ASAdventure::InReach(FVector::DistSquared2D(P,ShopPoint()),P.Z-ShopPoint().Z)) {if(Label) *Label=TEXT("Lucky shop / spend Scrap and Ash");return 222;}
    if(HasWell()&&ASAdventure::InReach(FVector::DistSquared2D(P,WellPoint()),P.Z-WellPoint().Z)) {if(Label) *Label=TEXT("Wishing well / stake Cinders");return 221;}
    const int32 Vault=VaultInteraction(P);if(Vault) {if(Label) *Label=Vault==3?TEXT("Rune vault cache / both sigils required"):FString::Printf(TEXT("Activate vault sigil %d"),Vault);return 200+Vault;}
    const int32 Weapon=WeaponAt(P);if(Weapon>=0) {if(Label)*Label=FString(ASData::Weapons[Weapon].Name)+TEXT(" / inspect, buy or practice");return 100+Weapon;}
    if(Config.Room<0) {for(const auto& S:Stations) if(FVector::DistSquared2D(P,S.Position)<=320*320) {if(Label)*Label=S.Name;return S.Menu;}}
    else if(FVector::DistSquared2D(P,ExitPoint())<310*310) {if(Label)*Label=TEXT("Reward chest / next chamber");return 9;}
    return -1;
}

FVector AASWorld::InteractionPoint(int32 Id) const {
    if(Id>=201&&Id<=203) return VaultPoint(Id-200)+FVector(0,0,80);
    if(Id==220) return ShrinePoint()+FVector(0,0,210);
    if(Id==221) return WellPoint()+FVector(0,0,210);
    if(Id==222) return ShopPoint()+FVector(0,0,120);
    if(Id>=100&&Id<100+ASRules::WeaponCount) return WeaponStand(Id-100)+FVector(0,0,165);
    if(Id==9) return ExitPoint()+FVector(0,0,95);
    for(const auto& S:Stations) if(S.Menu==Id) return S.Position+FVector(0,0,150);
    return FVector::ZeroVector;
}
void AASWorld::InteractionCandidates(TArray<int32>& Ids) const {
    if(Config.Room<0) {for(const auto& S:Stations) Ids.Add(S.Menu);for(int32 I=0;I<ASRules::WeaponCount;++I) Ids.Add(100+I);}
    else {Ids.Add(9);if(HasVault()) {Ids.Add(201);Ids.Add(202);Ids.Add(203);}if(HasShrine()) Ids.Add(220);if(HasWell()) Ids.Add(221);if(IsEndlessArena()) Ids.Add(222);}
}

bool AASWorld::WallTrapPath(int32 Cell,FVector& Start,FVector& End) const {
    if(Cell<=0||Cell==Layout.Exit||IsVaultCell(Cell)) return false;
    const FVector P=CellPosition(Cell);const auto Half=CellHalfSize(Cell);
    for(const FVector& D:{FVector(1,0,0),FVector(-1,0,0),FVector(0,1,0),FVector(0,-1,0)}) {
        bool Door=false;for(auto E:Layout.Edges) {const int32 N=E.A==Cell?E.B:(E.B==Cell?E.A:-1);if(N>=0&&FVector::DotProduct((CellPosition(N)-P).GetSafeNormal(),D)>.95f) Door=true;}
        if(Door) continue;const float Edge=FMath::Abs(D.X)>.5f?Half.X:Half.Y;
        Start=P+D*(Edge-70)+FVector(0,0,90);End=Start-D*FMath::Min(900.f,Edge*1.4f);
        FHitResult Hit;if(GetWorld()->LineTraceSingleByObjectType(Hit,Start,End,FCollisionObjectQueryParams(ECC_WorldStatic))) End=Hit.ImpactPoint+D*8.f;
        return FVector::DistSquared(Start,End)>250.f*250.f;
    }
    return false;
}
