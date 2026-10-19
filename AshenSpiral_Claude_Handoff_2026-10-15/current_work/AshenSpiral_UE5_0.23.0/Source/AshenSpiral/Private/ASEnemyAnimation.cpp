#include "ASEnemy.h"
#include "ASEnemyRules.h"
#include "ASExpeditionRules.h"
#include "ASPresentationRules.h"
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
    if(bTrainingDummy||Kind==5||Kind==9||Kind==ASExpedition::FlierKind) return; // the Gloomwing has no armour parts to pose
    if(Flies()) AnimateFlightPose(Delta,Windup); // 0.24: the armoured flying boss no longer walks on air (the gait followed its sideways orbit velocity: a moonwalk)
    else {
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
    // Replicated attack release and hit reaction: readable, brief, cosmetic only.
    const float Now=AS::Time(this);
    const float Release=ASAnimation::ReleaseGesture(Now-AttackReleaseTime);
    const float Recoil=ASAnimation::Reaction(Now-HitReactionTime,HitReactionStrength);
    const FVector Push=GetActorTransform().InverseTransformVectorNoScale(HitReactionDirection);
    ArmorParts[0]->AddRelativeRotation(FRotator(Release*14.f-Push.X*Recoil*18.f,0,Push.Y*Recoil*12.f));
    ArmorParts[1]->AddRelativeRotation(FRotator(-Recoil*8.f,0,0));
    ArmorFigure->SetRelativeLocation(FVector(Push.X*Recoil*12.f+Release*6.f,Push.Y*Recoil*12.f,-GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
}


void AASEnemy::AnimateDeath() {
    const float Now=AS::Time(this);
    if(DeathSeenAt<0.f) {
        DeathSeenAt=Now;
        for(auto* Piece:{Tell.Get(),TellFill.Get(),Beam.Get(),Eye.Get(),Crown.Get(),WingL.Get(),WingR.Get(),WingBoneL.Get(),WingBoneR.Get()}) if(Piece) Piece->SetVisibility(false);
    }
    // Topples backward and shrinks away with a brief glow, over EnemyDeathSeconds.
    const auto Look=ASPresentation::EnemyDeathAt(Now-DeathSeenAt);
    const FRotator Fall(-82.f*Look.Topple,0.f,10.f*Look.Topple);
    const float Fade=FMath::Max(.02f,Look.Scale);
    if(Body) {Body->SetRelativeRotation(Fall);Body->SetRelativeScale3D(FVector(BodyScale*Fade));
        if(auto* M=Cast<UMaterialInstanceDynamic>(Body->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),7.f*Look.Glow);}
    if(ArmorFigure) {ArmorFigure->SetRelativeRotation(Fall);ArmorFigure->SetRelativeScale3D(FVector(Fade));}
}

void AASEnemy::AnimateFlier() {
    // Wings (membrane + bone frame together) flap: faster and wider while an attack is being telegraphed, slow and heavy for the big boss, and thrown wide open and
    // shuddering during the boss's molt scream. The tips also sweep a few degrees fore and aft. The left wing is mirrored, so its angles flip sign.
    const float Now=AS::Time(this);const bool Big=Boss();const float Id=float(GetUniqueID()%5);
    const float Rate=Big?(bWinding?13.f:7.5f):(bWinding?26.f:15.f),Swing=Big?(bWinding?34.f:26.f):(bWinding?32.f:20.f);
    float Degrees=FMath::Sin(Now*Rate+Id)*Swing;
    const float Sweep=Big?FMath::Cos(Now*Rate+Id)*6.f:0.f;
    if(bWinding&&Ability==ASEnemyRules::AbilityMolt) {
        const float T=FMath::Clamp((Now-WindupStart)/FMath::Max(.01f,WindupEnd-WindupStart),0.f,1.f);
        Degrees=FMath::Lerp(Degrees,52.f+FMath::Sin(Now*38.f)*4.f,FMath::Clamp(T*3.f,0.f,1.f)); // wings thrown up and back as the scream builds
    }
    for(auto* Piece:{WingR.Get(),WingBoneR.Get()}) if(Piece) Piece->SetRelativeRotation(FRotator(0.f,Sweep,Degrees));
    for(auto* Piece:{WingL.Get(),WingBoneL.Get()}) if(Piece) Piece->SetRelativeRotation(FRotator(0.f,-Sweep,-Degrees));
}
void AASEnemy::AnimateFlightPose(float Delta,float Windup) {
    // The armoured flying boss in the air: legs trail behind and below with soft knees (no steps), the torso pitches into its motion and rears back during a
    // wind-up, arms spread forward and rise with the telegraph, the cape streams. Uses the same two-bone limb solver as the gait.
    (void)Delta;
    const float Now=AS::Time(this);
    const FVector Local=GetActorTransform().InverseTransformVectorNoScale(GetVelocity());
    const float Lean=FMath::Clamp(Local.X/500.f,-1.f,1.f),Strafe=FMath::Clamp(Local.Y/500.f,-1.f,1.f),Bob=FMath::Sin(Now*2.3f+float(GetUniqueID()%7));
    const bool Molting=bWinding&&Ability==ASEnemyRules::AbilityMolt;
    const float Rear=Molting?1.6f:1.f; // the molt scream rears further back than a dive wind-up
    ArmorParts[0]->SetRelativeRotation(FRotator(-8.f-Lean*16.f-Windup*14.f*Rear,0.f,Strafe*10.f));
    ArmorParts[1]->SetRelativeRotation(FRotator(8.f+Lean*10.f+Windup*10.f*Rear,0.f,-Strafe*6.f));
    for(int32 Side=0;Side<2;++Side) {
        const float Sign=Side==0?-1.f:1.f;
        const FVector Foot=ASPose::Limb(ArmorParts[7+Side*2],ArmorParts[8+Side*2],FVector(0,Sign*16,79),FVector(-34.f-Lean*10.f,Sign*16.f,24.f+Bob*4.f+float(Side)*3.f),FVector(36,Sign*20,40),38,38,38,38);
        ArmorParts[11+Side]->SetRelativeLocation(Foot+FVector(3,0,-2));ArmorParts[11+Side]->SetRelativeRotation(FRotator(35.f,0,0)); // toes pointed
        FVector Hand(34.f+Windup*26.f*Rear,Sign*(56.f+Windup*14.f),100.f+Windup*40.f*Rear+Bob*3.f);
        Hand=ASPose::Limb(ArmorParts[Side==0?3:5],ArmorParts[Side==0?4:6],FVector(0,Sign*34,133),Hand,FVector(-8,Sign*75,92));ArmorParts[17+Side]->SetRelativeLocation(Hand);
    }
    ArmorWeapon->SetRelativeRotation(FRotator(-Windup*50.f,0,0));
    ArmorParts[15]->SetRelativeRotation(FRotator(-30.f-FMath::Abs(Lean)*20.f,0,Strafe*6.f));
    ArmorParts[19]->SetRelativeRotation(FRotator(-22.f-FMath::Abs(Lean)*16.f+FMath::Sin(Now*5.f)*4.f,0,Strafe*4.f));
}
