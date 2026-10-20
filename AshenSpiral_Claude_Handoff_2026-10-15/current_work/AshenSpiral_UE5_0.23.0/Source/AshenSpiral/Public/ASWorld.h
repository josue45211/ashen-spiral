#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASRules.h"
#include "ASAdventureRules.h"
#include "ASWorld.generated.h"
class AASGameState;class ADirectionalLight;class AExponentialHeightFog;class ASkyLight;class APostProcessVolume;class UHierarchicalInstancedStaticMeshComponent;class UMaterialInstanceDynamic;class UStaticMesh;class UStaticMeshComponent;
USTRUCT() struct FASLayoutConfig {
    GENERATED_BODY()
    UPROPERTY() int32 Room=-1;
    UPROPERTY() int32 Seed=1103;
    UPROPERTY() int32 Generation=0;
    // Epoch changes only on a real room or run change, never on an Endless arena shuffle; melee and hammer swings survive a shuffle.
    UPROPERTY() int32 Epoch=0;
    // Endless arena: shuffle counter plus up to two keep-clear points (player positions) so new cover never lands on anyone.
    UPROPERTY() int32 Shuffle=0;
    UPROPERTY() int32 KeepCount=0;
    UPROPERTY() float KeepX0=0,KeepY0=0,KeepX1=0,KeepY1=0;
};
// One obstacle of the Endless arena's cover as the host publishes it (whole centimetres and degrees; ASRules::CoverRect). Every machine builds the cover from this list.
USTRUCT() struct FASCoverRect {
    GENERATED_BODY()
    UPROPERTY() int16 X=0;
    UPROPERTY() int16 Y=0;
    UPROPERTY() int16 Length=0;
    UPROPERTY() int16 Thickness=0;
    UPROPERTY() int16 Yaw=0;
};
struct FASStation {FVector Position;int32 Menu;FString Name;};
namespace ASAssets {
    UStaticMesh* Mesh(const FString& Name);
    bool HasMesh(const FString& Name); // false when a custom mesh has not been imported on this machine yet (Mesh() would silently substitute a cube); cached
    UMaterialInstanceDynamic* Material(UObject* Outer,FLinearColor Color,float Metal=0.f,float Glow=0.f,float Rough=.75f);
    void Armor(UStaticMeshComponent* Component,const FString& MeshName,FLinearColor Color,float Metal=.8f,float Glow=0.f,float Rough=.42f);
    void Style(UStaticMeshComponent* Component,const FString& MeshName,FLinearColor Color,float Metal=0.f,float Glow=0.f);
}
UCLASS() class ASHENSPIRAL_API AASWorld : public AActor {
    GENERATED_BODY()
public:
    AASWorld();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(ReplicatedUsing=Rebuild) FASLayoutConfig Config;
    // The Endless cover as the host decided it (0.24.1): the list and the Config.Generation it was made for. Clients build from it instead of re-running the generator.
    UPROPERTY(ReplicatedUsing=OnRep_Cover) TArray<FASCoverRect> CoverList;
    UPROPERTY(Replicated) int32 CoverGeneration=-1;
    UFUNCTION() void OnRep_Cover();
    UFUNCTION() void Rebuild();
    void Configure(int32 Room,int32 Seed);
    void ConfigureEndless(int32 Seed,int32 Shuffle,const TArray<FVector>& Keep);
    ASRules::Layout Layout;
    TArray<FASStation> Stations;
    TArray<FVector> Water;
    FVector CellPosition(int32 Index) const;
    FVector SpawnPoint(int32 PlayerIndex) const;
    FVector ExitPoint() const;
    int32 NearestCell(const FVector& P) const;
    FVector Route(const FVector& From,const FVector& To) const;
    bool GuideRoute(const FVector& From,const FVector& To,TArray<FVector>& Points,TArray<bool>& JumpLegs,float GoalRadius=0.f);
    bool CanWalkDirectly(const FVector& From,const FVector& To) const {return WalkClear(From,To,true);}
    bool AtExit(const FVector& Position) const;
    void UpdateVaultAppearance();
    bool JumpPlan(const FVector& From,const FVector& To,FVector& Takeoff,FVector& Landing) const;
    FVector RecoverEnemy(const FVector& From,const FVector& To) const;
    bool IsVaultCell(int32 I) const {return HasVault()&&I==(Layout.Exit==2?3:2);}
    bool WallTrapPath(int32 Cell,FVector& Start,FVector& End) const;
    void RefreshNavigation() {if(HasAuthority()) BuildNavigation();}
    bool HasVault() const {return Config.Room>=0&&Config.Room%4==1&&Layout.Cells.size()>3;}
    FVector VaultPoint(int32 Part=0) const;
    int32 VaultInteraction(const FVector& Position) const;
    FVector EnemySpawn(const FVector& Near,FRandomStream& Random) const;
    FVector SafeEnemyPoint(const FVector& Near) const;
    FVector ClampToFloor(const FVector& Point,float Inset) const; // 0.24: nearest point over walkable floor (a room or a corridor between rooms), pulled in by Inset: story rooms, where ClampToArena's room boxes miss the corridors
    FVector ClampToArena(const FVector& Point,float Margin) const; // nearest point inside any room box, each grown by Margin (negative = shrunk): keeps fliers, which ignore walls, in the arena
    bool FreeEnemyPoint(const FVector& Near,FVector& Point) const;
    FVector2D CellHalfSize(int32 Index) const;
    void MapBounds(FVector& Center,float& Span) const;
    FVector WeaponStand(int32 Index) const;
    int32 WeaponAt(const FVector& P) const;
    FVector DummyPoint() const {return FVector(850,950,105);}
    bool InWater(const FVector& P) const;
    int32 StationAt(const FVector& P,FString* Label=nullptr) const;
    int32 Biome() const {return ASAdventure::Biome(Config.Room);}
    bool HasShrine() const {return Config.Room>=0&&Config.Room%3==2&&Layout.Cells.size()>2;}
    // TUNABLE: the wishing well stands in shrine rooms (every third chamber), opposite the Ember Bargain.
    bool HasWell() const {return HasShrine();}
    // Endless lucky shop stall: beside the chest in the cleared centre.
    FVector ShopPoint() const {return ExitPoint()+FVector(0,380,0);}
    // The stall only exists (visible + solid + interactable) while a lucky shop is open at the checkpoint.
    bool ShopOpen() const;
    FVector WellPoint() const {return CellPosition(1)+FVector(0,-260,0);}
    FVector ShrinePoint() const {return CellPosition(1)+FVector(0,260,0);}
    FVector InteractionPoint(int32 Id) const;
    void InteractionCandidates(TArray<int32>& Ids) const;
    bool HasBossArena() const {return Layout.Shape>=ASRules::BossShapeBase&&Layout.Shape<ASRules::BossShapeBase+ASRules::WardCount;}
    bool IsEndlessArena() const {return Layout.Shape==ASRules::EndlessShape;}
    FVector BossSpawn() const {return CellPosition(0)+FVector(0,0,240);}
    int32 Ward() const {return ASRules::Ward(Config.Room);}
private:
    UPROPERTY() TMap<FName,TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Batches;
    UPROPERTY() TArray<TObjectPtr<UActorComponent>> Labels;
    UPROPERTY() TArray<TObjectPtr<AActor>> LocalLights;
    int32 BuiltGeneration=-1;
    int32 CoverBuiltFrom=-2; // which list the cover on screen was built from: the host's list for that generation, or -1 for one generated locally while the host's had not arrived
    // Endless shuffles only rebuild the cover blocks (same epoch + room), never the floors, so nobody loses ground and the frame stays cheap.
    int32 BuiltEpoch=-1,BuiltRoom=-99;
    FVector EndlessCenter=FVector::ZeroVector;
    float LastCoverDrop=-9999.f,SinkSoundAt=-1.f,ThudDownAt=-1.f,RiseSoundAt=-1.f,ThudUpAt=-1.f;
    int8 SolidState=-1; // collision proxies: -1 unknown, 0 off, 1 on
    // World-space footprints of the current cover (X, Y, length, thickness, yaw), so collision is never switched on under a hero's feet.
    struct FCoverFootprint {float X=0,Y=0,Length=0,Thickness=0,Yaw=0;};
    TArray<FCoverFootprint> CoverFootprints;
    // Remote machines read a smoothed copy of the replicated sim clock so the sink/rise (and its collision switches) never flicker backwards.
    float ShiftClock=0.f;uint64 ShiftClockFrame=0;bool bShiftClockInit=false;
    float ShiftNow(const AASGameState* State);
    void SetCoverCollision(bool On);
    void BuildNavigationGrids();
    void BuildEndlessCover(const FVector& Center);
    void BuildEndlessDecor(const FVector& Center);   // 0.25 phase C: the scenery ring beyond the walls and the loot beside the chest (visual only)
    void ShowDecorStage(int32 SkyStage);             // shows the decor stages the sky has reached
    int32 DecorStageShown=-1;
    void RebuildEndlessCover();
    void AnimateShift(AASGameState* State); // authority decides collision and publishes it in State->bCoverSolid; everyone else applies it
    bool AnyHeroNearCover(const AASGameState* State) const; // any hero within ASExpedition::ShiftHoldOffRadius of a current wall footprint
    void LogLayoutFingerprint(const TCHAR* Why) const; // "AshenShift:" log line: layout seed/shuffle/generation, footprint count and a hash, so host and client can be compared
    float HoldOffSince=-1.f; // SimTime the timeline first allowed collision while a hero kept it off (authority only)
    int32 ClientRebuildRetries=0; // client-side safety net: geometry that disagrees with the replicated config is rebuilt (max 3 tries)
    // Sky stages (Endless): lights and fog are driven toward the current stage's look on clients.
    UPROPERTY() TObjectPtr<ADirectionalLight> SunLight;
    UPROPERTY() TObjectPtr<AExponentialHeightFog> FogActor;
    UPROPERTY() TObjectPtr<ASkyLight> SkyLightActor;
    UPROPERTY() TObjectPtr<APostProcessVolume> PostVolume;
    float SkyBlendT=1.f;int32 SkyFrom=0,SkyTo=0;
    bool bVaultDimmed=false;
    UPROPERTY() TObjectPtr<UActorComponent> ShopLabel;
    int32 ShopShown=-1;
    void ApplyShopVisibility(bool bOpen);
    bool bGuideNavigationReady=false;
    int32 GuideBreakableCount=-1;
    ASRules::NavGrid Navigation,GuideNavigation;
    FVector NavOrigin=FVector::ZeroVector;
    void BuildNavigation();
    FVector NavPoint(int32 Index) const;
    int32 NearestNode(const FVector& Position,bool ForGuide=false) const;
    bool FloorAt(const FVector& Position) const;
    bool WalkClear(const FVector& From,const FVector& To,bool Player=false) const;
    bool GuideJumpClear(const FVector& From,const FVector& To) const;
    void Add(const FString& MeshName,const FString& Group,const FVector& Position,const FVector& Scale,FLinearColor Color,bool Collision=true,float Yaw=0.f,float Glow=0.f,float Metal=0.f);
    void Text(const FString& Value,const FVector& Position,float Yaw,float Size=32.f);
    void MakeHub();
    void MakeRoom();
    void BossArena(const FVector& Center);
    void EndlessArena(const FVector& Center);
    void Archives(const FVector& Center,int32 Index);
    void Props(const FVector& Center,int32 Index,FRandomStream& Random);
    void Landmarks(const FVector& Center,int32 Index);
    void Light(const FVector& Position,FLinearColor Color,float Intensity,float Radius);
};
