#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ASMovementComponent.generated.h"
UCLASS() class ASHENSPIRAL_API UASMovementComponent : public UCharacterMovementComponent {
    GENERATED_BODY()
public:
    bool bWantsDash=false,bAirDashing=false;
    int32 AirDashesUsed=0;
    float DashCooldown=0,DashRemaining=0,DashVerticalVelocity=0;
    int32 DashCharges=1,KnownDashCapacity=1;
    float DashRecharge=0;
    FVector DashDirection=FVector::ForwardVector;
    void RefreshDashCollision();
    TArray<TWeakObjectPtr<AActor>> DashIgnoredActors;
    void RequestDash() {bWantsDash=true;}
    virtual void TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
    virtual void PerformMovement(float DeltaSeconds) override;
    virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
    virtual void PhysCustom(float DeltaTime,int32 Iterations) override;
    virtual void UpdateFromCompressedFlags(uint8 Flags) override;
    virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
    virtual float GetMaxSpeed() const override;
};
class FSavedMove_AS final : public FSavedMove_Character {
public:
    using Super=FSavedMove_Character;
    bool bSavedDash=false,bSavedAirDashing=false;
    int32 SavedAirDashesUsed=0;
    float SavedCooldown=0,SavedRemaining=0,SavedVertical=0;
    int32 SavedCharges=1,SavedCapacity=1;
    float SavedRecharge=0;
    FVector SavedDirection=FVector::ForwardVector;
    virtual void Clear() override;
    virtual uint8 GetCompressedFlags() const override;
    virtual bool CanCombineWith(const FSavedMovePtr& NewMove,ACharacter* Character,float MaxDelta) const override;
    virtual void SetMoveFor(ACharacter* Character,float Delta,FVector const& NewAcceleration,FNetworkPredictionData_Client_Character& ClientData) override;
    virtual void PrepMoveFor(ACharacter* Character) override;
};
class FPredictionData_AS final : public FNetworkPredictionData_Client_Character {
public:
    explicit FPredictionData_AS(const UCharacterMovementComponent& Movement):FNetworkPredictionData_Client_Character(Movement) {}
    virtual FSavedMovePtr AllocateNewMove() override {return FSavedMovePtr(new FSavedMove_AS());}
};
