#include "ASCharacter.h"
#include "ASAnimationRules.h"
#include "ASPose.h"
#include "ASMovementComponent.h"
#include "ASGameState.h"
#include "ASGameInstance.h"
#include "ASSpell.h"
#include "ASPresentationRules.h"
#include "ASWorld.h"
#include "ASFoleyRules.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Animation/AnimInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "Engine/World.h"
void AASCharacter::Animate(float Delta) {
    if(Parts.Num()!=21) return;
    // Ownership can arrive after the first look pass; recolour once when this machine learns whether the character is its own.
    {const int8 LocalNow=IsLocallyControlled()?1:0;if(LocalNow!=LastTintLocal) {LastTintLocal=LocalNow;RefreshLook();}}
    const float Now=AS::Time(this),Speed=GetVelocity().Size2D();const bool Airborne=GetCharacterMovement()->IsFalling();
    const bool Dashing=Movement()->DashRemaining>0;
    // Dash trail: afterimages while moving at dash speed (works on every client from the replicated velocity).
    // Trails come only from an explicit dash (predicted locally, or the server-replicated flag on other machines) or hammer leap flight.
    // They never depend on measured speed, so walking and running cannot spawn one on any machine.
    {
        const bool TrailActive=!bDowned&&(Dashing||bDashActive||(Build.Weapon==1&&HammerLeapState==2));
        if(ASPresentation::DashGhostDue(TrailActive,Now,NextDashGhost)) {
            NextDashGhost=Now+ASPresentation::DashGhostSpacing;
            if(auto* World=GetWorld()) if(auto* Ghost=World->SpawnActor<AASDashGhost>()) Ghost->SetupFromCharacter(this,ViewColor());
        }
    }
    const bool Planted=Build.Weapon==1&&(HammerCommitted()||HammerLeapState==1||HammerLeapState==3);
    LocomotionWeight=FMath::Lerp(LocomotionWeight,FMath::Clamp(Planted?0.f:Speed/RunningSpeed(),0.f,1.f),ASAnimation::Blend(Delta,12.f));
    AirWeight=FMath::Lerp(AirWeight,Airborne?1.f:0.f,ASAnimation::Blend(Delta,13.f));
    if(Airborne) FallSpeed=FMath::Max(FallSpeed,-GetVelocity().Z);
    if(!bWasAirborne&&Airborne&&GetVelocity().Z>200.f&&!Dashing) PlayFoley(TEXT("S_Jump"),6,ASFoley::JumpVolume); // a push-off, only for a real jump (not stepping off a ledge)
    if(bWasAirborne&&!Airborne) {LandingAge=0;if(!bDowned) PlayFoley(TEXT("S_Land"),8,ASFoley::LandVolume*ASFoley::LandGain(FallSpeed));FallSpeed=0.f;} else LandingAge+=Delta;bWasAirborne=Airborne;
    const float Landing=FMath::Exp(-LandingAge*9.f)*FMath::Sin(FMath::Min(LandingAge*18.f,UE_PI));
    const float PreviousWalkPhase=WalkPhase;
    if(!Airborne&&!Dashing&&Speed>8.f) WalkPhase+=Delta*Speed/ASAnimation::StrideDistance(LocomotionWeight)*UE_TWO_PI;
    if(ASFoley::StepsCrossed(PreviousWalkPhase,WalkPhase)>0&&ASFoley::Audible(Airborne,Dashing,bDowned,Speed)) PlayFootstep(Speed); // a step each time the gait passes a half stride
    // Visual-only acceleration lean from smoothed velocity; never feeds back into movement.
    const FVector Velocity=GetVelocity();
    if(!bVisualInitialized||(Velocity-AnimationVelocity).SizeSquared2D()>2200.f*2200.f) {AnimationVelocity=Velocity;PreviousVisualVelocity=Velocity;bVisualInitialized=true;}
    PreviousVisualVelocity=AnimationVelocity;
    AnimationVelocity=FMath::Lerp(AnimationVelocity,Velocity,ASAnimation::Blend(Delta,9.f));
    const FVector LeanAccel=GetActorTransform().InverseTransformVectorNoScale((AnimationVelocity-PreviousVisualVelocity)/FMath::Max(Delta,.001f));
    CapeAccelLocal=FMath::Lerp(CapeAccelLocal,LeanAccel,ASAnimation::Blend(Delta,10.f));
    if(bSkeletalHero) AnimateSkeletal(Delta,Dashing,Airborne); // 0.24.2 Phase F: the Mannequin, posed by its animation blueprint and clips
    else {
    const float Step=FMath::Sin(WalkPhase)*LocomotionWeight*(1.f-AirWeight);
    const FVector LocalMove=GetActorTransform().InverseTransformVectorNoScale(GetVelocity()).GetSafeNormal2D();
    const float LeapPhase=ASMelee::Phase(Now,HammerLeapStarted,ASHammer::Startup);
    const float HammerCrouch=Build.Weapon==1?(HammerLeapState==1?-16.f*ASMelee::Smooth(LeapPhase):(HammerLeapState==2?-16.f*(1.f-ASMelee::Smooth((Now-HammerLeapStarted-ASHammer::Startup)/(ASHammer::Flight*.28f))):0.f)):0.f;
    const float BodyDrop=-4.f*LocomotionWeight-7.f*Landing+HammerCrouch;
    Figure->SetRelativeLocation(FVector(0,0,bDowned?-117.f:-88.f+BodyDrop+FMath::Abs(Step)*1.5f));
    // Player hit reaction: brief recoil away from the source. Visual only; velocity is never touched.
    const float Recoil=bDowned?0.f:ASAnimation::Reaction(DamageReactionAge,DamageReactionStrength,true);
    const FVector RecoilLocal=GetActorTransform().InverseTransformVectorNoScale(DamageReactionDirection);
    Figure->SetRelativeLocation(Figure->GetRelativeLocation()+FVector(RecoilLocal.X,RecoilLocal.Y,0.f)*(Recoil*10.f));
    Figure->SetRelativeRotation(FRotator(bDowned?65.f:-RecoilLocal.X*Recoil*14.f,0,RecoilLocal.Y*Recoil*10.f));
    const float LeanPitch=FMath::Clamp(LeanAccel.X/2600.f,-1.f,1.f)*6.f*(1.f-AirWeight),LeanRoll=FMath::Clamp(LeanAccel.Y/2600.f,-1.f,1.f)*5.f*(1.f-AirWeight);
    const float Breath=FMath::Sin(Now*2.2f)*.6f;
    Parts[0]->SetRelativeRotation(FRotator(Dashing?-18.f:-LocomotionWeight*LocalMove.X*6.f-Landing*5.f-LeanPitch,Step*2.5f,-LocalMove.Y*LocomotionWeight*5.f+LeanRoll));
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
        const FVector Support=Build.Weapon==0?FVector(12+Step*12,-39-FMath::Clamp(M.Yaw,-65.f,65.f)*.18f,86-FMath::Abs(M.BodyPitch)*.8f):Grip+WeaponRotation.RotateVector(FVector(0,0,ASMelee::SupportHeight(Build.Weapon)));
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
    } // the 21-part armour rig
    CapeLean=FMath::Lerp(CapeLean,LocomotionWeight*26.f+(Dashing?24.f:0.f)+AirWeight*9.f,ASAnimation::Blend(Delta,5.f));
    float CapeFlare=0,CapeTwist=0;
    // Cape drivers (cosmetic): dash flares, the hammer leap lifts, every melee swing throws the hem out; the Blade lunge most.
    if(Dashing) CapeFlare=1.f;
    if(ASMelee::IsMelee(Build.Weapon)&&!bDowned) {
        const bool Swinging=Now>=AttackPoseStarted&&Now<AttackPoseStarted+AttackPoseDuration;
        if(Swinging) CapeFlare=FMath::Max(CapeFlare,FMath::Sin(ASMelee::Phase(Now,AttackPoseStarted,AttackPoseDuration)*UE_PI)*(Build.Weapon==0&&MeleeStep==2?1.f:.6f));
    }
    const float WantLift=Build.Weapon==1?(HammerLeapState==2?1.f:(HammerLeapState==1?.35f:0.f)):0.f;
    CapeLift=FMath::Lerp(CapeLift,WantLift,ASAnimation::Blend(Delta,WantLift>CapeLift?7.f:2.2f));
    if(Build.Weapon==1&&!bDowned) {
        ASHammer::Pose HP;ASHammer::Frame HF;AnimateHammer(Now,HP,HF);
        CapeFlare=FMath::Clamp(FMath::Abs(HP.BodyPitch)/30.f,0.f,1.f);CapeTwist=HP.BodyYaw;
        if(auto* Mat=Cast<UMaterialInstanceDynamic>(WeaponMesh->GetMaterial(0))) {
            const bool Active=AttackPoseStarted>=0&&Now<AttackPoseStarted+AttackPoseDuration;
            const float Glow=HammerLeapState?2.8f:(Active?.35f+MeleeStep*.6f+ASMelee::Phase(Now,AttackPoseStarted,AttackPoseDuration)*.6f:.15f);
            Mat->SetScalarParameterValue(TEXT("Emission"),Glow);
        }
    }
    // Ability cast (F rune / skill) gesture: brief forward thrust of the torso. Cosmetic only.
    if(!bDowned&&Build.Weapon!=1) {
        const float Cast=ASAnimation::ReleaseGesture(AbilityGestureAge);
        if(Cast>0.f) {Parts[0]->AddRelativeRotation(FRotator(-Cast*9.f,0,0));Parts[1]->AddRelativeRotation(FRotator(Cast*4.f,0,0));CapeFlare=FMath::Max(CapeFlare,Cast*.5f);}
    }
    if(!bSkeletalHero) AnimateCape(Delta,CapeFlare,CapeTwist); // the Mannequin wears no cape
    DamageReactionAge+=Delta;AbilityGestureAge+=Delta;
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
    GetMesh()->SetOwnerNoSee(HideSelf);
    WeaponMesh->SetOwnerNoSee(HideSelf);ArrowMesh->SetOwnerNoSee(HideSelf);ChargeMesh->SetOwnerNoSee(HideSelf);
    for(auto Part:BowStrings) Part->SetOwnerNoSee(HideSelf);for(auto Part:Wisps) Part->SetOwnerNoSee(HideSelf);
}

void AASCharacter::DamageReaction_Implementation(FVector_NetQuantizeNormal Direction,float Strength) {
    DamageReactionAge=0;DamageReactionStrength=FMath::Clamp(Strength,0.f,1.f);DamageReactionDirection=Direction;
    if(bSkeletalHero) PlayHitClip(Direction,Strength);
}
void AASCharacter::AbilityGesture_Implementation() {AbilityGestureAge=0;if(bSkeletalHero) PlayHeroClip(TEXT("gideon_RMB_Cast"),FName(TEXT("UpperBody")),.55f,.06f,.18f);}
// 0.24.2 Phase F: a flinch from the side the blow came from (on the arms and torso; the legs keep running), a stagger for a heavy one
void AASCharacter::PlayHitClip(const FVector& Direction,float Strength) {
    if(bDowned) return;
    const FVector Local=GetActorTransform().InverseTransformVectorNoScale(-Direction).GetSafeNormal2D(); // where the blow came from, in the hero's frame
    const TCHAR* Clip=FMath::Abs(Local.X)>=FMath::Abs(Local.Y)?(Local.X>=0.f?TEXT("TwinBlast_HitReact_Front"):TEXT("TwinBlast_HitReact_Back")):(Local.Y>=0.f?TEXT("TwinBlast_HitReact_Right"):TEXT("TwinBlast_HitReact_Left"));
    if(Strength>=.85f) PlayHeroClip(TEXT("TwinBlast_KnockBack"),FName(TEXT("DefaultSlot")),.7f,.05f,.2f);
    else if(!SkeletalMovePlaying()) PlayHeroClip(Clip,FName(TEXT("UpperBody")),.45f,.04f,.16f); // a light flinch never cuts a swing, a dash or a cast short (only the stance)
}
void AASCharacter::CreateCape() {
    // Six linked cloth segments replace the rigid cape boards. Segments are visual only.
    for(int32 I=0;I<6;++I) {
        auto* C=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("CapeSegment%d"),I));
        C->SetupAttachment(Figure);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetGenerateOverlapEvents(false);
        FlowingCape.Add(C);
    }
}
void AASCharacter::RefreshCape() {
    if(Parts.Num()!=21) return;
    const FLinearColor Cloth(.012f,.016f,.023f);
    for(int32 I=0;I<FlowingCape.Num();++I) if(auto* C=FlowingCape[I].Get()) {
        ASAssets::Armor(C,TEXT("SM_Cape"),bDowned?FLinearColor(.3f,.02f,.025f):Cloth,0.f,0.f,.95f);
        C->SetVisibility(true);
    }
    // Legacy rigid pieces stay as unused hierarchy anchors, hidden.
    Parts[15]->SetVisibility(false,true);Parts[19]->SetVisibility(false,true);
}
void AASCharacter::AnimateCape(float Delta,float AttackFlare,float BodyTwist) {
    if(FlowingCape.Num()<6||Delta<=0.f) return;
    const float Now=AS::Time(this),Yaw=GetActorRotation().Yaw,Rate=FMath::FindDeltaAngleDegrees(PreviousVisualYaw,Yaw);PreviousVisualYaw=Yaw;
    // Large jumps are teleports/respawns: do not whip the cape, rebuild it settled.
    const bool Teleported=FMath::Abs(Rate)>120.f||Delta>.25f;
    TurnVelocity=Teleported?0.f:FMath::Lerp(TurnVelocity,Rate/Delta/540.f,ASAnimation::Blend(Delta,5.f));
    CapeTurn=FMath::Lerp(CapeTurn,FMath::Clamp(TurnVelocity,-1.f,1.f),ASAnimation::Blend(Delta,2.2f));
    ASAnimation::CapeDrive Drive;Drive.Time=Now;Drive.Speed=FMath::Clamp(CapeLean/40.f,0.f,1.5f);Drive.Turn=CapeTurn;
    Drive.Flare=FMath::Clamp(AttackFlare,0.f,1.f);Drive.Lift=CapeLift;
    // Idle wind: a broad, slow billow that fades while running (the cape streams instead).
    Drive.Wind=FMath::Clamp((.45f+.35f*FMath::Sin(Now*.45f))*(1.f-Drive.Speed),0.f,1.f);
    Drive.AccelX=CapeAccelLocal.X;Drive.AccelY=CapeAccelLocal.Y;Drive.AccelZ=CapeAccelLocal.Z;
    if(Teleported||bDowned) CapeSim.Reset(Drive);else CapeSim.Step(Delta,Drive);
    const float Twist=FMath::Clamp(BodyTwist,-40.f,40.f)*.3f;
    FVector Node[ASAnimation::CapeNodes];
    for(int32 I=0;I<ASAnimation::CapeNodes;++I) Node[I]=FVector(CapeSim.P[I][0],CapeSim.P[I][1]+Twist*float(I)/6.f,CapeSim.P[I][2]);
    for(int32 I=0;I<6;++I) if(auto* C=FlowingCape[I].Get()) {
        const FVector Down=Node[I+1]-Node[I];const float Length=Down.Size();
        if(Length<KINDA_SMALL_NUMBER) continue;
        const float T=(float(I)+.5f)/6.f;
        // Broader as it billows; the hem spreads with the flare and the lift.
        const float Spread=1.f+.35f*Drive.Wind*T+.30f*Drive.Flare*T+.25f*Drive.Lift*T;
        C->SetRelativeLocation(Node[I]);
        C->SetRelativeRotation(FRotationMatrix::MakeFromZX(-Down,FVector::ForwardVector).Rotator());
        C->SetRelativeScale3D(FVector(1.f,ASAnimation::CapeWidth(T)*Spread/36.f /* SM_Cape half-width is 36 */,Length/55.f*1.08f));
    }
}

void AASCharacter::CollectTrailMeshes(TArray<UStaticMeshComponent*>& Out) const {
    for(const auto& Part:Parts) if(Part&&Part->GetStaticMesh()&&Part->IsVisible()) Out.Add(Part.Get());
    for(const auto& Piece:FlowingCape) if(Piece&&Piece->GetStaticMesh()&&Piece->IsVisible()) Out.Add(Piece.Get());
    if(WeaponMesh&&WeaponMesh->GetStaticMesh()&&WeaponMesh->IsVisible()) Out.Add(WeaponMesh.Get());
}

// 0.25 phase E: footsteps. The sounds are 12 concrete running steps imported locally (/Game/Audio/Imported/Foley/S_Step01..12, Tools/import_vault_assets.py); a machine without them is silent, as it
// always was. Every machine plays the steps of every hero itself from the replicated velocity, so they cost no network traffic; dedicated servers have no ears.
void AASCharacter::PlayFootstep(float Speed) {
    if(GetNetMode()==NM_DedicatedServer||!GetWorld()) return;
    static bool bStepsAbsent=false; // the first lookup decides: no imported steps on this machine, no more lookups
    if(bStepsAbsent) return;
    // 0.24.2: the set by speed and by how the motion points against the facing (walk, run, strafe, backpedal); a machine with only the older running set falls back to it.
    const FVector Move=GetVelocity().GetSafeNormal2D();
    ASFoley::StepSet Set=ASFoley::SetFor(Speed,FVector::DotProduct(Move,GetActorForwardVector().GetSafeNormal2D()));
    const int32 Variant=ASFoley::NextVariantOf(ASFoley::SetVariants[Set],LastStepVariant,uint32(FMath::Rand()));
    auto Find=[](ASFoley::StepSet From,int32 Index) {const FString Name=FString::Printf(TEXT("%s%02d"),UTF8_TO_TCHAR(ASFoley::SetPrefix[From]),Index+1);return LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/Imported/Foley/%s.%s"),*Name,*Name),nullptr,LOAD_NoWarn);};
    USoundBase* Sound=Find(Set,Variant);
    if(!Sound&&Set!=ASFoley::SetRun) {Set=ASFoley::SetRun;Sound=Find(Set,Variant%ASFoley::StepVariants);}
    if(!Sound) {if(LastStepVariant<0) bStepsAbsent=true;return;}
    LastStepVariant=Variant;
    float Distance=0.f;
    if(!IsLocallyControlled()) {
        const APlayerController* Local=UGameplayStatics::GetPlayerController(this,0);const APawn* Pawn=Local?Local->GetPawn():nullptr;
        Distance=Pawn?float(FVector::Dist(Pawn->GetActorLocation(),GetActorLocation())):0.f;
    }
    const float Volume=ASFoley::SetVolume(Set)*ASFoley::StepGain(Distance,Speed);
    if(Volume<.01f) return;
    UGameplayStatics::PlaySoundAtLocation(this,Sound,GetActorLocation(),Volume,FMath::FRandRange(ASFoley::StepPitchLow,ASFoley::StepPitchHigh));
}
void AASCharacter::PlayFoley(const TCHAR* Prefix,int32 Count,float Volume) {
    if(GetNetMode()==NM_DedicatedServer||!GetWorld()||Volume<.01f) return;
    const int32 Variant=ASFoley::NextVariantOf(Count,LastFoleyVariant,uint32(FMath::Rand()));
    const FString Name=FString::Printf(TEXT("%s%02d"),Prefix,Variant+1);
    USoundBase* Sound=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/Imported/Foley/%s.%s"),*Name,*Name),nullptr,LOAD_NoWarn);
    if(!Sound) return; // not imported on this machine: silent, as before
    LastFoleyVariant=Variant;
    float Distance=0.f;
    if(!IsLocallyControlled()) {
        const APlayerController* Local=UGameplayStatics::GetPlayerController(this,0);const APawn* Pawn=Local?Local->GetPawn():nullptr;
        Distance=Pawn?float(FVector::Dist(Pawn->GetActorLocation(),GetActorLocation())):0.f;
    }
    const float Gain=Volume*ASFoley::StepGain(Distance,640.f);
    if(Gain>=.01f) UGameplayStatics::PlaySoundAtLocation(this,Sound,GetActorLocation(),Gain,FMath::FRandRange(ASFoley::StepPitchLow,ASFoley::StepPitchHigh));
}
