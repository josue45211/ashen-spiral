#include "ASEnemy.h"
#include "ASCharacter.h"
#include "ASAnimationRules.h"
#include "ASMeleeRules.h"
#include "ASPose.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
void AASEnemy::AnimateArmor(float Delta,float Windup) {
    if(bTrainingDummy||Kind==5||Kind==9) return;
    const float Speed=GetVelocity().Size2D(),Weight=FMath::Clamp(Speed/400.f,0.f,1.f);
    GaitPhase+=Delta*Speed/190.f*UE_TWO_PI;const float Step=FMath::Sin(GaitPhase)*Weight;
    ArmorParts[0]->SetRelativeRotation(FRotator(-5.f-Weight*4.f-Windup*10.f,Step*3.f,0));
    ArmorParts[1]->SetRelativeRotation(FRotator(5.f-Windup*12.f,-Step*2.f,0));
    for(int32 Side=0;Side<2;++Side) {
        const float Sign=Side==0?-1.f:1.f;const auto F=ASAnimation::Foot(GaitPhase+Side*UE_PI,Weight);
        const FVector Foot=ASPose::Limb(ArmorParts[7+Side*2],ArmorParts[8+Side*2],FVector(0,Sign*16,79),FVector(6+F.Forward,Sign*16,12+F.Lift),FVector(70,Sign*20,45),38,38,38,38);
        ArmorParts[11+Side]->SetRelativeLocation(Foot+FVector(3,0,-2));ArmorParts[11+Side]->SetRelativeRotation(FRotator(F.Pitch,0,0));
        const bool Gun=Kind==8;FVector Hand=Gun?FVector(Side==0?76:54,Side==0?-5:25,Side==0?133:119):FVector(24+(Side==0?-Step:Step)*12.f+Windup*28.f,Sign*45,74+Windup*40.f);
        Hand=ASPose::Limb(ArmorParts[Side==0?3:5],ArmorParts[Side==0?4:6],FVector(0,Sign*34,133),Hand,FVector(-8,Sign*75,92));ArmorParts[17+Side]->SetRelativeLocation(Hand);
    }
    ArmorWeapon->SetRelativeRotation(FRotator(Kind==8?0.f:-Windup*65.f,0,0));
    ArmorParts[15]->SetRelativeRotation(FRotator(-8.f-Weight*18.f,Step*2.f,0));
    ArmorParts[19]->SetRelativeRotation(FRotator(-Weight*8.f+FMath::Sin(AS::Time(this)*3.f)*3.f,0,Step*2.f));
}

