#include "ASCharacter.h"
#include "ASAnimationRules.h"
#include "ASPose.h"
#include "ASMovementComponent.h"
#include "ASGameState.h"
#include "ASGameInstance.h"
#include "ASWorld.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "Engine/World.h"
void AASCharacter::Animate(float Delta) {
    if(Parts.Num()!=21) return;
    const float Now=AS::Time(this),Speed=GetVelocity().Size2D();const bool Airborne=GetCharacterMovement()->IsFalling();
    const bool Dashing=Movement()->DashRemaining>0;
    const bool Planted=Build.Weapon==1&&(HammerCommitted()||HammerLeapState==1||HammerLeapState==3);
    LocomotionWeight=FMath::Lerp(LocomotionWeight,FMath::Clamp(Planted?0.f:Speed/RunningSpeed(),0.f,1.f),ASAnimation::Blend(Delta,12.f));
    AirWeight=FMath::Lerp(AirWeight,Airborne?1.f:0.f,ASAnimation::Blend(Delta,13.f));
    if(bWasAirborne&&!Airborne) LandingAge=0;else LandingAge+=Delta;bWasAirborne=Airborne;
    const float Landing=FMath::Exp(-LandingAge*9.f)*FMath::Sin(FMath::Min(LandingAge*18.f,UE_PI));
    if(!Airborne&&!Dashing&&Speed>8.f) WalkPhase+=Delta*Speed/230.f*UE_TWO_PI;
    const float Step=FMath::Sin(WalkPhase)*LocomotionWeight*(1.f-AirWeight);
    const FVector LocalMove=GetActorTransform().InverseTransformVectorNoScale(GetVelocity()).GetSafeNormal2D();
    const float LeapPhase=ASMelee::Phase(Now,HammerLeapStarted,ASHammer::Startup);
    const float HammerCrouch=Build.Weapon==1?(HammerLeapState==1?-16.f*ASMelee::Smooth(LeapPhase):(HammerLeapState==2?-16.f*(1.f-ASMelee::Smooth((Now-HammerLeapStarted-ASHammer::Startup)/(ASHammer::Flight*.28f))):0.f)):0.f;
    const float BodyDrop=-4.f*LocomotionWeight-7.f*Landing+HammerCrouch;
    Figure->SetRelativeLocation(FVector(0,0,bDowned?-117.f:-88.f+BodyDrop+FMath::Abs(Step)*1.5f));
    Figure->SetRelativeRotation(FRotator(bDowned?65.f:0.f,0,0));
    const float Breath=FMath::Sin(Now*2.2f)*.6f;
    Parts[0]->SetRelativeRotation(FRotator(Dashing?-18.f:-LocomotionWeight*LocalMove.X*6.f-Landing*5.f,Step*2.5f,-LocalMove.Y*LocomotionWeight*5.f));
    Parts[0]->SetRelativeLocation(FVector(0,0,111+Breath));Parts[2]->SetRelativeRotation(FRotator(0,-Step*4.f,Step*2.f));
    Parts[1]->SetRelativeLocation(FVector(0,0,165+Breath));Parts[1]->SetRelativeRotation(FRotator(-Landing*3.f,Step,0));
    for(int32 Side=0;Side<2;++Side) {
        const float Sign=Side==0?-1.f:1.f;const auto F=ASAnimation::Foot(WalkPhase+Side*UE_PI,LocomotionWeight);
        FVector Foot=FVector(6,Sign*16,12)+LocalMove*F.Forward+FVector(0,0,F.Lift);
        Foot=FMath::Lerp(Foot,FVector(Side==0?15.f:-10.f,Sign*16,30.f),AirWeight);
        if(Planted) Foot=FVector(Side==0?24.f:-12.f,Sign*24.f,12.f-HammerCrouch);
        if(Dashing) Foot=FVector(-22.f+Side*10.f,Sign*16,22.f);
        if(!Airborne&&!Dashing) {
            const FVector WorldFoot=Figure->GetComponentTransform().TransformPosition(Foot);FHitResult Floor;
            if(GetWorld()->LineTraceSingleByObjectType(Floor,WorldFoot+FVector(0,0,30),WorldFoot-FVector(0,0,45),FCollisionObjectQueryParams(ECC_WorldStatic)))
                Foot.Z+=FMath::Clamp(float(Floor.ImpactPoint.Z+10.f-WorldFoot.Z),0.f,20.f);
        }
        const FVector Hip(0,Sign*16,79),KneePole(70,Sign*19,47);
        const FVector Ankle=ASPose::Limb(Parts[7+Side*2],Parts[8+Side*2],Hip,Foot,KneePole,38.f,38.f,38.f,38.f);
        Parts[11+Side]->SetRelativeLocation(Ankle+FVector(3,0,-2));Parts[11+Side]->SetRelativeRotation(FRotator(F.Pitch*(1.f-AirWeight),0,0));
    }
    PoseAge+=Delta;auto Pose=ASRules::WeaponGripPose(PoseAge,AttackPoseDuration,Step,Build.Weapon);
    if(Build.Weapon>=6) {Pose.X-=30.f;Pose.Y=25.f;}
    if(Build.Weapon==2) Pose.X-=8.f;
    const bool Reloading=IsReloading();const float ReloadMotion=Reloading?FMath::Sin(ReloadProgress()*UE_PI):0.f;
    FVector Grip=FVector(Pose.X,Pose.Y,Pose.Z)+FVector(-8,8,-18)*ReloadMotion;
    FVector LeftShoulder(0,-34,133),RightShoulder(0,34,133);
    const FVector LeftPole(-8,-78,96),RightPole(-8,78,96);
    FQuat WeaponRotation(FVector::RightVector,Pose.ForwardAngle+.28f*ReloadMotion);
    if(ASMelee::IsMelee(Build.Weapon)) {
        // A stable grip is the foundation; the same pose clock drives full swings.
        const bool Active=Now>=AttackPoseStarted&&Now<AttackPoseStarted+AttackPoseDuration;
        auto M=ASMelee::Swing(Build.Weapon,MeleeStep,ASMelee::Phase(Now,AttackPoseStarted,AttackPoseDuration),Active);
        ASHammer::Frame HammerFrame{};if(Build.Weapon==1) AnimateHammer(Now,M,HammerFrame);
        const float Rad=UE_PI/180.f;
        WeaponRotation=FQuat(FVector::UpVector,M.Yaw*Rad)*FQuat(FVector::RightVector,M.Pitch*Rad)*FQuat(FVector::ForwardVector,M.Roll*Rad);
        if(Build.Weapon==1) WeaponRotation=FRotationMatrix::MakeFromZY(FVector(HammerFrame.Z.X,HammerFrame.Z.Y,HammerFrame.Z.Z),FVector(HammerFrame.Y.X,HammerFrame.Y.Y,HammerFrame.Y.Z)).ToQuat();
        if(Build.Weapon==1) {
            const auto L=ASHammer::BodyPoint(M,{0,-34,133}),R=ASHammer::BodyPoint(M,{0,34,133});
            LeftShoulder=FVector(L.X,L.Y,L.Z);RightShoulder=FVector(R.X,R.Y,R.Z);
            Parts[13]->SetRelativeLocation(LeftShoulder+FVector(0,0,2));Parts[14]->SetRelativeLocation(RightShoulder+FVector(0,0,2));
            Parts[13]->SetRelativeRotation(FRotator(-M.BodyPitch,M.BodyYaw,0));Parts[14]->SetRelativeRotation(FRotator(-M.BodyPitch,M.BodyYaw,0));
            Parts[1]->SetRelativeRotation(FRotator(-M.BodyPitch*.3f,M.BodyYaw*.35f,0));
        }
        Grip=ASPose::Limb(Parts[5],Parts[6],RightShoulder,FVector(M.X,M.Y,M.Z),RightPole);Parts[18]->SetRelativeLocation(Grip);
        const FVector Support=Build.Weapon==0?FVector(12+Step*12,-39,86):Grip+WeaponRotation.RotateVector(FVector(0,0,ASMelee::SupportHeight(Build.Weapon)));
        Parts[17]->SetRelativeLocation(ASPose::Limb(Parts[3],Parts[4],LeftShoulder,Support,LeftPole));
        Parts[0]->AddRelativeRotation(FRotator(-M.BodyPitch,M.BodyYaw,0));Parts[2]->AddRelativeRotation(FRotator(0,M.BodyYaw*.35f,0));
    } else if(Build.Weapon==2) {
        Grip=ASPose::Limb(Parts[3],Parts[4],LeftShoulder,Grip,LeftPole);
        const FVector Draw=ASPose::Limb(Parts[5],Parts[6],RightShoulder,FVector(42+Pose.Swing*10-15*ReloadMotion,-5,Grip.Z-30*ReloadMotion),RightPole);
        Parts[17]->SetRelativeLocation(Grip);Parts[18]->SetRelativeLocation(Draw);
        for(int32 I=0;I<2;++I) {
            const FVector Tip=Grip+FVector(-24,0,I==0?73:-73),V=Draw-Tip;
            BowStrings[I]->SetRelativeLocation((Tip+Draw)*.5f);BowStrings[I]->SetRelativeRotation(FQuat::FindBetweenNormals(FVector::UpVector,V.GetSafeNormal()));
            BowStrings[I]->SetRelativeScale3D(FVector(.009f,.009f,V.Size()/100.f));
        }
        ArrowMesh->SetRelativeLocation(Draw+FVector(33,0,0));ArrowMesh->SetRelativeRotation(FRotator::ZeroRotator);ArrowMesh->SetVisibility(!Reloading&&PoseAge>AttackPoseDuration*.55f);
    } else {
        Grip=ASPose::Limb(Parts[5],Parts[6],RightShoulder,Grip,RightPole);Parts[18]->SetRelativeLocation(Grip);
        FVector Support=Build.Weapon>=6?Grip+FVector(22,-30,14)+FVector(-25,-8,-36)*ReloadMotion:FVector(15+Step*12+30*ReloadMotion,-38+32*ReloadMotion,68+20*ReloadMotion);
        if(bCharging) Support=FVector(40,-27,111);
        Support=ASPose::Limb(Parts[3],Parts[4],LeftShoulder,Support,LeftPole);Parts[17]->SetRelativeLocation(Support);
    }
    Parts[17]->SetRelativeRotation(ASMelee::IsMelee(Build.Weapon)&&Build.Weapon!=0?WeaponRotation:FQuat::Identity);
    Parts[18]->SetRelativeRotation(ASMelee::IsMelee(Build.Weapon)?WeaponRotation:FQuat::Identity);
    WeaponMesh->SetRelativeRotation(ASMelee::IsMelee(Build.Weapon)?FQuat::Identity:WeaponRotation);
    if(Build.Weapon==1) {
        // A square head is symmetric under a half-turn. Do not pass tangent-axis
        // sign changes into the wrists: hands use continuous anatomical roll.
        ASHammer::Pose HP;ASHammer::Frame HF;AnimateHammer(Now,HP,HF);const float R=UE_PI/180.f;
        const FQuat HandRotation=FQuat(FVector::UpVector,HP.Yaw*R)*FQuat(FVector::RightVector,HP.Pitch*R)*FQuat(FVector::UpVector,HP.Roll*R);
        Parts[17]->SetRelativeRotation(HandRotation);Parts[18]->SetRelativeRotation(HandRotation);
        WeaponMesh->SetRelativeRotation(HandRotation.Inverse()*WeaponRotation);
    }
    CapeLean=FMath::Lerp(CapeLean,LocomotionWeight*26.f+(Dashing?24.f:0.f)+AirWeight*9.f,ASAnimation::Blend(Delta,5.f));
    Parts[15]->SetRelativeRotation(FRotator(-4.f-CapeLean,Step*1.5f,FMath::Sin(Now*2.7f)*1.5f));
    Parts[19]->SetRelativeRotation(FRotator(-CapeLean*.4f+FMath::Sin(Now*3.1f)*3.f,Step*2.f,FMath::Sin(Now*2.1f)*2.f));
    if(Build.Weapon==1&&!bDowned) {
        ASHammer::Pose HP;ASHammer::Frame HF;AnimateHammer(Now,HP,HF);
        Parts[15]->AddRelativeRotation(FRotator(-FMath::Abs(HP.BodyPitch)*.45f,-HP.BodyYaw*.45f,HP.BodyYaw*.16f));
        Parts[19]->AddRelativeRotation(FRotator(-FMath::Abs(HP.BodyPitch)*.35f,-HP.BodyYaw*.25f,HP.BodyYaw*.22f));
        if(auto* Mat=Cast<UMaterialInstanceDynamic>(WeaponMesh->GetMaterial(0))) {
            const bool Active=AttackPoseStarted>=0&&Now<AttackPoseStarted+AttackPoseDuration;
            const float Glow=HammerLeapState?2.8f:(Active?.35f+MeleeStep*.6f+ASMelee::Phase(Now,AttackPoseStarted,AttackPoseDuration)*.6f:.15f);
            Mat->SetScalarParameterValue(TEXT("Emission"),Glow);
        }
    }
    HammerShakeAge+=Delta;
    if(IsLocallyControlled()) if(auto* GI=GetGameInstance<UASGameInstance>()) {
        const bool Shake=Build.Weapon==1&&!GI->bReduceMotion&&HammerShakeAge<.28f;
        const float Envelope=Shake?HammerShakeStrength*FMath::Square(1.f-HammerShakeAge/.28f):0.f;
        Camera->SetRelativeLocation(FVector(0,FMath::Sin(HammerShakeAge*83.f)*Envelope,FMath::Sin(HammerShakeAge*107.f)*Envelope*.6f));
    }
    const int32 WispCount=FMath::Min(3,TraitValue(ASBuild::Trait::Orbit));
    for(int32 I=0;I<Wisps.Num();++I) {const float A=Now*2.f+I*UE_TWO_PI/FMath::Max(1,WispCount);Wisps[I]->SetVisibility(I<WispCount&&!bDowned);Wisps[I]->SetRelativeLocation(FVector(FMath::Cos(A)*135,FMath::Sin(A)*135,35));}
    ChargeMesh->SetVisibility(bCharging&&!bDowned);ChargeMesh->SetRelativeScale3D(FVector(.15f+FMath::Clamp(Now-ChargeStart,0.f,1.4f)*.28f));
    Boom->TargetArmLength=FMath::FInterpTo(Boom->TargetArmLength,CameraZoom,Delta,10.f);
    const bool HideSelf=IsLocallyControlled()&&FVector::DistSquared(Camera->GetComponentLocation(),GetActorLocation()+FVector(0,0,45))<190.f*190.f;
    for(auto Part:Parts) Part->SetOwnerNoSee(HideSelf);
    WeaponMesh->SetOwnerNoSee(HideSelf);ArrowMesh->SetOwnerNoSee(HideSelf);ChargeMesh->SetOwnerNoSee(HideSelf);
    for(auto Part:BowStrings) Part->SetOwnerNoSee(HideSelf);for(auto Part:Wisps) Part->SetOwnerNoSee(HideSelf);
}
