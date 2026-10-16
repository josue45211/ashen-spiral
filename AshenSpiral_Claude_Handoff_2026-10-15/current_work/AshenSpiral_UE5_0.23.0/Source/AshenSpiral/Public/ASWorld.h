#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASRules.h"
#include "ASAdventureRules.h"
#include "ASWorld.generated.h"
class UHierarchicalInstancedStaticMeshComponent;class UMaterialInstanceDynamic;class UStaticMesh;class UStaticMeshComponent;
USTRUCT() struct FASLayoutConfig {
    GENERATED_BODY()
    UPROPERTY() int32 Room=-1;
    UPROPERTY() int32 Seed=1103;
    UPROPERTY() int32 Generation=0;
};
struct FASStation {FVector Position;int32 Menu;FString Name;};
namespace ASAssets {
    UStaticMesh* Mesh(const FString& Name);
    UMaterialInstanceDynamic* Material(UObject* Outer,FLinearColor Color,float Metal=0.f,float Glow=0.f,float Rough=.75f);
    void Armor(UStaticMeshComponent* Component,const FString& MeshName,FLinearColor Color,float Metal=.8f,float Glow=0.f,float Rough=.42f);
    void Style(UStaticMeshComponent* Component,const FString& MeshName,FLinearColor Color,float Metal=0.f,float Glow=0.f);
}
UCLASS() class ASHENSPIRAL_API AASWorld : public AActor {
    GENERATED_BODY()
public:
    AASWorld();
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(ReplicatedUsing=Rebuild) FASLayoutConfig Config;
    UFUNCTION() void Rebuild();
    void Configure(int32 Room,int32 Seed);
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
    FVector ShrinePoint() const {return CellPosition(1)+FVector(0,260,0);}
    FVector InteractionPoint(int32 Id) const;
    void InteractionCandidates(TArray<int32>& Ids) const;
    int32 Ward() const {return ASRules::Ward(Config.Room);}
private:
    UPROPERTY() TMap<FName,TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Batches;
    UPROPERTY() TArray<TObjectPtr<UActorComponent>> Labels;
    UPROPERTY() TArray<TObjectPtr<AActor>> LocalLights;
    int32 BuiltGeneration=-1;
    bool bVaultDimmed=false;
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
    void Archives(const FVector& Center,int32 Index);
    void Props(const FVector& Center,int32 Index,FRandomStream& Random);
    void Landmarks(const FVector& Center,int32 Index);
    void Light(const FVector& Position,FLinearColor Color,float Intensity,float Radius);
};
