#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ASData.h"
#include "ASEnemy.generated.h"
class USceneComponent;class UStaticMeshComponent;class AASCharacter;
UCLASS() class ASHENSPIRAL_API AASEnemy : public ACharacter {
    GENERATED_BODY()
public:
    AASEnemy();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(ReplicatedUsing=RefreshLook) int32 Kind=0;
    UPROPERTY(Replicated) int32 Ward=0;
    UPROPERTY(ReplicatedUsing=RefreshLook) int32 Theme=0;
    UPROPERTY(Replicated) float Health=70;
    UPROPERTY(Replicated) float MaxHealth=70;
    UPROPERTY(ReplicatedUsing=RefreshLook) bool bTrainingDummy=false;
    UPROPERTY(Replicated) float LastDamageTime=-100.f;
    UPROPERTY(ReplicatedUsing=RefreshStatus) uint8 StatusBits=0;
    UPROPERTY(Replicated) float WindupEnd=0;
    UPROPERTY(Replicated) float WindupStart=0;
    UPROPERTY(Replicated) FVector Aim=FVector::ForwardVector;
    UPROPERTY(Replicated) bool bWinding=false;
    float SlowUntil=0,PulledUntil=0,MarkUntil=0,StunUntil=0,RootUntil=0,StunImmuneUntil=0,RootImmuneUntil=0;
    float LastHitTime=-10,NextDamageNumber=0;
    TWeakObjectPtr<AASCharacter> LastHitter;
    bool Hit(float Damage,AASCharacter* Source);
    void ApplyElement(EASHitKind Type,AASCharacter* Source,float Power);
    void Control(int32 Type,const FVector& Direction,float Strength);
    UFUNCTION() void RefreshStatus();
    bool Boss() const {return !bTrainingDummy&&Kind==4;}
    bool Elite() const {return !bTrainingDummy&&(Kind==3||Kind==4);}
    UFUNCTION() void RefreshLook();
private:
    UPROPERTY() TObjectPtr<USceneComponent> ArmorFigure;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ArmorParts;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ArmorWeapon;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ArmorShield;
    float GaitPhase=0;
    void AnimateArmor(float Delta,float Windup);
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Eye;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Tell;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Crown;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Aura;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Beam;
    ASRules::ElementState Elements;
    TWeakObjectPtr<AASCharacter> BurnSource,PoisonSource,BleedSource;
    float BurnPending=0,PoisonPending=0,BleedPending=0,NextStatusDamage=0;
    bool TickElements(float Delta);
    float NextAttack=0,NextRoute=0,ChargeUntil=0,LastContact=0,NextStuckCheck=0,StuckFor=0;
    float LeapUntil=0,BodyScale=1;
    FVector LastRoutePosition=FVector::ZeroVector;
    FVector MoveGoal=FVector::ZeroVector;
    int32 Pattern=0;
    void CommitAttack();
};
