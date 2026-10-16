#include "ASCharacter.h"
#include "ASArsenalRules.h"
#include "ASPose.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ASMovementComponent.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "ASSpell.h"
#include "ASBreakable.h"
#include "ASEnemy.h"
#include "ASPlayerController.h"
#include "ASRules.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
AASCharacter::AASCharacter(const FObjectInitializer& Init):Super(Init.SetDefaultSubobjectClass<UASMovementComponent>(ACharacter::CharacterMovementComponentName)) {
    PrimaryActorTick.bCanEverTick=true;bReplicates=true;SetNetUpdateFrequency(50.f);SetMinNetUpdateFrequency(20.f);
    GetCapsuleComponent()->InitCapsuleSize(34,88);GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);GetCapsuleComponent()->CanCharacterStepUpOn=ECB_No;GetCharacterMovement()->bEnablePhysicsInteraction=false;
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);GetMesh()->SetHiddenInGame(true);
    bUseControllerRotationYaw=true;GetCharacterMovement()->bOrientRotationToMovement=false;
    GetCharacterMovement()->MaxWalkSpeed=640;GetCharacterMovement()->MaxAcceleration=2600;GetCharacterMovement()->BrakingDecelerationWalking=2100;
    GetCharacterMovement()->GravityScale=1.6f;GetCharacterMovement()->MaxStepHeight=42;
    GetCharacterMovement()->JumpZVelocity=660.f;GetCharacterMovement()->AirControl=.45f;
    JumpMaxCount=1;JumpMaxHoldTime=0.f;RunRanks.Init(0,ASRules::RunUpgradeCount);
    Boom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));Boom->SetupAttachment(RootComponent);Boom->TargetArmLength=528;
    Boom->TargetOffset=FVector(0,0,100);Boom->SocketOffset=FVector(0,155,20);Boom->bUsePawnControlRotation=true;Boom->bEnableCameraLag=false;Boom->CameraLagSpeed=14;Boom->ProbeSize=16;
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));Camera->SetupAttachment(Boom,USpringArmComponent::SocketName);Camera->FieldOfView=85;Camera->SetRelativeRotation(FRotator(-8,0,0));
    Figure=CreateDefaultSubobject<USceneComponent>(TEXT("ArticulatedArmor"));Figure->SetupAttachment(RootComponent);Figure->SetRelativeLocation(FVector(0,0,-88));Figure->SetRelativeScale3D(FVector(.82f,.70f,1.f));
    for(int32 I=0;I<21;++I) {
        auto* C=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ArmorPart%02d"),I));C->SetupAttachment(Figure);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);Parts.Add(C);
    }
    // The hand is the grip pivot. Attaching to the forearm previously inherited
    // its large negative pitch and drove the blade back through the body.
    WeaponMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon"));WeaponMesh->SetupAttachment(Parts[18]);
    ChargeMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpiralCharge"));ChargeMesh->SetupAttachment(Parts[17]);
    for(int32 I=0;I<3;++I) {auto* W=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("OrbitWisp%d"),I));W->SetupAttachment(RootComponent);Wisps.Add(W);}
    ArrowMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NockedArrow"));ArrowMesh->SetupAttachment(Figure);
    for(int32 I=0;I<2;++I) {auto* C=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("BowString%d"),I));C->SetupAttachment(Figure);BowStrings.Add(C);}
}
void AASCharacter::BeginPlay() {Super::BeginPlay();if(auto* S=AS::State(this)) S->Heroes.AddUnique(this);RefreshLook();LastSafe=GetActorLocation();}
void AASCharacter::EndPlay(const EEndPlayReason::Type R) {if(auto* S=AS::State(this)) S->Heroes.Remove(this);Super::EndPlay(R);}
UASMovementComponent* AASCharacter::Movement() const {return Cast<UASMovementComponent>(GetCharacterMovement());}
bool AASCharacter::CanJumpInternal_Implementation() const {
    const auto* S=AS::State(this);
    return !bDowned&&!bReviving&&S&&!S->Frozen()&&S->Phase!=EASPhase::Defeat
        &&Super::CanJumpInternal_Implementation();
}
void AASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AASCharacter,WarCryUntil);DOREPLIFETIME(AASCharacter,Build);DOREPLIFETIME(AASCharacter,RelicMask);DOREPLIFETIME(AASCharacter,Health);
    DOREPLIFETIME(AASCharacter,MaxHealth);DOREPLIFETIME(AASCharacter,Shield);DOREPLIFETIME(AASCharacter,ChargeStart);
    DOREPLIFETIME(AASCharacter,Ammo);DOREPLIFETIME(AASCharacter,ReloadStart);DOREPLIFETIME(AASCharacter,ReloadEnd);
    DOREPLIFETIME(AASCharacter,NextSpiral);DOREPLIFETIME(AASCharacter,NextAttack);DOREPLIFETIME(AASCharacter,InvulnerableUntil);
    DOREPLIFETIME(AASCharacter,ReviveProgress);DOREPLIFETIME(AASCharacter,bCharging);DOREPLIFETIME(AASCharacter,bDowned);
    DOREPLIFETIME(AASCharacter,ColorChoice);DOREPLIFETIME(AASCharacter,AttackSequence);DOREPLIFETIME(AASCharacter,ColorIndex);
    DOREPLIFETIME(AASCharacter,TraitRanks);DOREPLIFETIME(AASCharacter,EvolutionMask);DOREPLIFETIME(AASCharacter,LastUpgrade);
    DOREPLIFETIME(AASCharacter,Rerolls);DOREPLIFETIME(AASCharacter,CurseStacks);DOREPLIFETIME(AASCharacter,Focus);DOREPLIFETIME(AASCharacter,NextRune);
    DOREPLIFETIME(AASCharacter,ConduitHits);DOREPLIFETIME(AASCharacter,bPhoenixUsed);
    DOREPLIFETIME(AASCharacter,RunRanks);DOREPLIFETIME(AASCharacter,bReviving);DOREPLIFETIME(AASCharacter,AttackPoseDuration);
    DOREPLIFETIME(AASCharacter,AttackPoseStarted);DOREPLIFETIME(AASCharacter,MeleeStep);
}
void AASCharacter::ApplyBuild(const FASBuild& V,bool Fresh) {
    if(!HasAuthority()) return;Build=V;Build.Sanitize();
    if(auto* PS=GetPlayerState<AASPlayerState>()) {ColorIndex=PS->PlayerIndex;ColorChoice=PS->ColorChoice;}
    if(Fresh) {
        ResetTraits();
        RunRanks.Init(0,ASRules::RunUpgradeCount);
        MaxHealth=ASRules::StartingHealth(Rank(0),0);Health=MaxHealth;Shield=Rank(1)*5;RelicMask=0;
        bDowned=false;bMercyUsed=false;bSecondWindUsed=false;bNextCritical=false;bReviving=false;ReviveProgress=0;
        NextSpiral=NextAttack=0;InvulnerableUntil=AS::Time(this)+2;Movement()->DashCooldown=0;Movement()->DashRemaining=0;Movement()->AirDashesUsed=0;Movement()->bAirDashing=false;
    }
    ResetMagazine();ResetInput();RefreshLook();ForceNetUpdate();
}
void AASCharacter::RefreshLook() {
    if(Parts.Num()!=21) return;
    const FLinearColor Fire=ColorChoice<0?FLinearColor(.08f,.55f,1.f):BodyColor(),Dark(.07f,.085f,.11f),Metal(.20f,.23f,.28f),Cloth(.012f,.016f,.023f);
    const TCHAR* Meshes[21]={TEXT("SM_Torso"),TEXT("SM_Helmet"),TEXT("SM_Pelvis"),TEXT("SM_Arm"),TEXT("SM_Forearm"),TEXT("SM_Arm"),TEXT("SM_Forearm"),TEXT("SM_Thigh"),TEXT("SM_Greave"),TEXT("SM_Thigh"),TEXT("SM_Greave"),TEXT("SM_Boot"),TEXT("SM_Boot"),TEXT("SM_Pauldron"),TEXT("SM_Pauldron"),TEXT("SM_Cape"),TEXT("SM_Visor"),TEXT("SM_Hand"),TEXT("SM_Hand"),TEXT("SM_CapeLower"),TEXT("SM_ArmorCore")};
    const FVector Positions[21]={FVector(0,0,111),FVector(0,0,165),FVector(0,0,78),FVector(0,-34,115),FVector(0,-38,78),FVector(0,34,115),FVector(0,38,78),FVector(0,-16,59),FVector(0,-16,25),FVector(0,16,59),FVector(0,16,25),FVector(6,-16,10),FVector(6,16,10),FVector(0,-34,135),FVector(0,34,135),FVector(-22,0,141),FVector::ZeroVector,FVector(12,-38,66),FVector(34,52,82),FVector(-3,0,-55),FVector::ZeroVector};
    for(int32 I=0;I<21;++I) {
        const bool Fabric=I==15||I==19,Glow=I==16||I==20;
        const FLinearColor C=Glow?Fire:(Fabric?Cloth:(I==1||I==0||I==13||I==14?Metal:Dark));
        ASAssets::Armor(Parts[I],Meshes[I],bDowned?FLinearColor(.3f,.02f,.025f):C,Fabric?0.f:.8f,Glow?4.f:0.f,Fabric?.95f:.42f);
        Parts[I]->SetRelativeLocation(Positions[I]);Parts[I]->SetRelativeScale3D(FVector(1));
    }
    Parts[1]->SetRelativeScale3D(FVector(.95f,1.f,.85f));
    Parts[16]->AttachToComponent(Parts[1],FAttachmentTransformRules::KeepRelativeTransform);
    Parts[19]->AttachToComponent(Parts[15],FAttachmentTransformRules::KeepRelativeTransform);
    Parts[20]->AttachToComponent(Parts[0],FAttachmentTransformRules::KeepRelativeTransform);
    ASAssets::Style(WeaponMesh,FString::Printf(TEXT("SM_Weapon%d"),Build.Weapon),Metal,.75f);
    WeaponMesh->AttachToComponent(Build.Weapon==2?Parts[17].Get():Parts[18].Get(),FAttachmentTransformRules::KeepRelativeTransform);
    WeaponMesh->SetRelativeLocation(FVector::ZeroVector);WeaponMesh->SetRelativeRotation(FRotator::ZeroRotator);
    if(Build.Weapon==2) Parts[17]->SetRelativeLocation(FVector(82,-5,123));
    for(auto W:Wisps) {ASAssets::Style(W,TEXT("SM_Crystal"),FLinearColor(1,.42f,.06f),0,4);W->SetRelativeScale3D(FVector(.12f));}
    ASAssets::Style(ArrowMesh,TEXT("SM_Arrow"),FLinearColor(.55f,.38f,.17f),.25f);ArrowMesh->SetVisibility(Build.Weapon==2);
    for(auto* C: {BowStrings[0].Get(),BowStrings[1].Get()}) {ASAssets::Style(C,TEXT("Cylinder"),FLinearColor(.78f,.74f,.61f));C->SetVisibility(Build.Weapon==2);}
    Parts[18]->SetRelativeLocation(FVector(34,62,82));Parts[18]->SetRelativeRotation(FRotator::ZeroRotator);
    ASAssets::Style(ChargeMesh,TEXT("Sphere"),Fire,0,9);ChargeMesh->SetRelativeLocation(FVector(12,0,0));ChargeMesh->SetRelativeScale3D(FVector(.22f));
}
void AASCharacter::OnAttackPose() {PoseAge=0;}
float AASCharacter::ReloadProgress() const {return ASArsenal::ReloadProgress(ReloadStart,ReloadEnd,AS::Time(this));}
void AASCharacter::ResetMagazine() {if(HasAuthority()) {Ammo=MagazineCapacity();ReloadStart=ReloadEnd=0;RotaryHeat=0;}}
void AASCharacter::RequestReload_Implementation() {
    if(!ASArsenal::UsesMagazine(Build.Weapon)||!ASArsenal::CanReload(Ammo,MagazineCapacity(),IsReloading(),CanUseWeapons()&&!bReviving)) return;
    ReloadStart=AS::Time(this);ReloadEnd=ReloadStart+ReloadDuration();bCharging=false;RotaryHeat=0;
    ReloadCue(false);ForceNetUpdate();
}
void AASCharacter::ReloadCue_Implementation(bool Complete) {
    if(GetNetMode()==NM_DedicatedServer) return;
    const TCHAR* Name=Complete?TEXT("S_ReloadReady"):TEXT("S_ReloadStart");
    if(auto* Cue=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),Name,Name))) {
        if(IsLocallyControlled()) UGameplayStatics::PlaySound2D(this,Cue,.55f);
        else UGameplayStatics::PlaySoundAtLocation(this,Cue,GetActorLocation(),.25f);
    }
}
void AASCharacter::Animate(float Delta) {
    if(Parts.Num()!=21) return;
    const float Now=AS::Time(this),Speed=GetVelocity().Size2D();const bool Airborne=GetCharacterMovement()->IsFalling();
    const bool Dashing=Movement()->DashRemaining>0;
    LocomotionWeight=FMath::Lerp(LocomotionWeight,FMath::Clamp(Speed/RunningSpeed(),0.f,1.f),ASAnimation::Blend(Delta,12.f));
    AirWeight=FMath::Lerp(AirWeight,Airborne?1.f:0.f,ASAnimation::Blend(Delta,13.f));
    if(bWasAirborne&&!Airborne) LandingAge=0;else LandingAge+=Delta;bWasAirborne=Airborne;
    const float Landing=FMath::Exp(-LandingAge*9.f)*FMath::Sin(FMath::Min(LandingAge*18.f,UE_PI));
    if(!Airborne&&!Dashing&&Speed>8.f) WalkPhase+=Delta*Speed/230.f*UE_TWO_PI;
    const float Step=FMath::Sin(WalkPhase)*LocomotionWeight*(1.f-AirWeight);
    const FVector LocalMove=GetActorTransform().InverseTransformVectorNoScale(GetVelocity()).GetSafeNormal2D();
    const float BodyDrop=-4.f*LocomotionWeight-7.f*Landing;
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
    const FVector LeftShoulder(0,-34,133),RightShoulder(0,34,133);
    const FVector LeftPole(-8,-78,96),RightPole(-8,78,96);
    FQuat WeaponRotation(FVector::RightVector,Pose.ForwardAngle+.28f*ReloadMotion);
    if(ASMelee::IsMelee(Build.Weapon)) {
        // A stable grip is the foundation; the same pose clock drives full swings.
        const bool Active=Now>=AttackPoseStarted&&Now<AttackPoseStarted+AttackPoseDuration;
        const auto M=ASMelee::Swing(Build.Weapon,MeleeStep,ASMelee::Phase(Now,AttackPoseStarted,AttackPoseDuration),Active);
        const float Rad=UE_PI/180.f;
        WeaponRotation=FQuat(FVector::UpVector,M.Yaw*Rad)*FQuat(FVector::RightVector,M.Pitch*Rad)*FQuat(FVector::ForwardVector,M.Roll*Rad);
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
    CapeLean=FMath::Lerp(CapeLean,LocomotionWeight*26.f+(Dashing?24.f:0.f)+AirWeight*9.f,ASAnimation::Blend(Delta,5.f));
    Parts[15]->SetRelativeRotation(FRotator(-4.f-CapeLean,Step*1.5f,FMath::Sin(Now*2.7f)*1.5f));
    Parts[19]->SetRelativeRotation(FRotator(-CapeLean*.4f+FMath::Sin(Now*3.1f)*3.f,Step*2.f,FMath::Sin(Now*2.1f)*2.f));
    const int32 WispCount=FMath::Min(3,TraitValue(ASBuild::Trait::Orbit));
    for(int32 I=0;I<Wisps.Num();++I) {const float A=Now*2.f+I*UE_TWO_PI/FMath::Max(1,WispCount);Wisps[I]->SetVisibility(I<WispCount&&!bDowned);Wisps[I]->SetRelativeLocation(FVector(FMath::Cos(A)*135,FMath::Sin(A)*135,35));}
    ChargeMesh->SetVisibility(bCharging&&!bDowned);ChargeMesh->SetRelativeScale3D(FVector(.15f+FMath::Clamp(Now-ChargeStart,0.f,1.4f)*.28f));
    Boom->TargetArmLength=FMath::FInterpTo(Boom->TargetArmLength,CameraZoom,Delta,10.f);
    const bool HideSelf=IsLocallyControlled()&&FVector::DistSquared(Camera->GetComponentLocation(),GetActorLocation()+FVector(0,0,45))<190.f*190.f;
    for(auto Part:Parts) Part->SetOwnerNoSee(HideSelf);
    WeaponMesh->SetOwnerNoSee(HideSelf);ArrowMesh->SetOwnerNoSee(HideSelf);ChargeMesh->SetOwnerNoSee(HideSelf);
    for(auto Part:BowStrings) Part->SetOwnerNoSee(HideSelf);for(auto Part:Wisps) Part->SetOwnerNoSee(HideSelf);
}
void AASCharacter::Tick(float Delta) {
    Super::Tick(Delta);auto* S=AS::State(this);if(!S || S->Frozen()) return;
    if(GetNetMode()!=NM_DedicatedServer) Animate(Delta);
    if(!HasAuthority()) return;
    const float Now=S->SimTime;
    if(GetCharacterMovement()->IsMovingOnGround() && GetActorLocation().Z>40) LastSafe=GetActorLocation();
    if(GetActorLocation().Z<-600) {SetActorLocation(S->Arena?S->Arena->SpawnPoint(ColorIndex):LastSafe,false,nullptr,ETeleportType::TeleportPhysics);GetCharacterMovement()->StopMovementImmediately();Hurt(18);}
    if(bDowned) {
        ResetInput();Movement()->bWantsDash=false;Movement()->DashRemaining=0;
        AASCharacter* Helper=nullptr;
        for(auto W:S->Heroes) if(auto* Other=W.Get()) if(Other->bReviving&&Other->CanRevive(this)) {Helper=Other;break;}
        // Only the downed pawn advances its replicated timer. It no longer
        // depends on which player's Tick happens first, and works in Reward too.
        ReviveProgress=ASRules::AdvanceRevive(ReviveProgress,Delta,Helper!=nullptr);
        if(ReviveProgress>=ASRules::ReviveSeconds) {
            bDowned=false;Health=MaxHealth*.45f;InvulnerableUntil=Now+2.f;ReviveProgress=0;
            GetCharacterMovement()->StopMovementImmediately();
            if(GetCharacterMovement()->MovementMode==MOVE_Custom||GetCharacterMovement()->MovementMode==MOVE_None)
                GetCharacterMovement()->SetMovementMode(MOVE_Falling);
            if(auto* PS=Helper->GetPlayerState<AASPlayerState>()) PS->Record(ASCareer::Revives,1,15);
            Helper->bReviving=false;RefreshLook();Helper->ForceNetUpdate();ForceNetUpdate();
            if(auto* PC=Cast<AASPlayerController>(GetController())) PC->Notify(TEXT("Revived with 45% health and 2 seconds of protection."));
        }
        return;
    }
    ReviveProgress=0;TickTraits(Delta);
    if(ASArsenal::ReloadComplete(ReloadEnd,Now)) {Ammo=MagazineCapacity();ReloadStart=ReloadEnd=0;ReloadCue(true);ForceNetUpdate();}
    RotaryHeat=ASArsenal::Heat(RotaryHeat,Delta,bAttackHeld&&!IsReloading()&&Build.Weapon==ASArsenal::Rotary&&CanUseWeapons());
    TickMelee(Now);
    if(CanUseWeapons() && !IsReloading() && !bReviving && bAttackHeld && Now>=NextAttack&&!bMeleePending) Attack();
    if(Now>=ResonanceAt && Talent(7) && CombatBenefits()) {
        ResonanceAt=Now+8;bool Nearby=false;
        for(auto W:S->Heroes) if(W.IsValid()&&W.Get()!=this&&!W->bDowned&&FVector::DistSquared2D(GetActorLocation(),W->GetActorLocation())<720*720) {Nearby=true;W->Heal(W->MaxHealth*.02f);}
        if(Nearby || S->PlayerArray.Num()==1) Heal(MaxHealth*(Nearby?.02f:.01f));
    }
}
bool AASCharacter::CanUseWeapons() const {
    const auto* S=AS::State(this);return S&&!S->Frozen()&&!bDowned&&ASRules::CanUseWeapons(int(S->Phase));
}
bool AASCharacter::CombatBenefits() const {
    const auto* S=AS::State(this);return S&&!S->Frozen()&&!bDowned&&ASRules::CanGainCombatBenefits(int(S->Phase));
}
bool AASCharacter::CanDamage(const AASEnemy* Enemy) const {
    const auto* S=AS::State(this);return IsValid(Enemy)&&S&&!S->Frozen()&&!bDowned&&ASRules::CanDamageTarget(int(S->Phase),Enemy->bTrainingDummy,false);
}
void AASCharacter::ResetInput() {CancelMelee();RotaryHeat=0;bAttackHeld=false;bCharging=false;bReviving=false;StopJumping();Movement()->bWantsDash=false;}
bool AASCharacter::CanRevive(const AASCharacter* Target) const {
    const auto* S=AS::State(this);
    if(!S||S->Frozen()||!(S->Fighting()||S->Phase==EASPhase::Reward)||bDowned||Health<=0
        ||!IsValid(Target)||Target==this||!Target->bDowned||!GetCharacterMovement()->IsMovingOnGround()) return false;
    const FVector A=GetActorLocation(),B=Target->GetActorLocation();
    return ASRules::ReviveInReach(FVector::DistSquared2D(A,B),A.Z-B.Z)&&AS::ClearSight(GetWorld(),A,B);
}
bool AASCharacter::ApplyRunUpgrade(int32 I) {
    if(!HasAuthority()||!CanApplyRunUpgrade(I)) return false;
    const float OldCooldown=SpiralCooldown();RunRanks.SetNumZeroed(ASRules::RunUpgradeCount);++RunRanks[I];
    if(I==0) {const float Now=AS::Time(this);NextSpiral=Now+FMath::Max(0.f,NextSpiral-Now)*SpiralCooldown()/OldCooldown;}
    if(I==3) {MaxHealth+=15.f;Heal(15.f);}
    ForceNetUpdate();return true;
}
void AASCharacter::SetAim_Implementation(FVector_NetQuantize P) {if(!P.ContainsNaN() && FVector::DistSquared(GetActorLocation(),P)<20000.f*20000.f) AimPoint=P;}
void AASCharacter::SetAttack_Implementation(bool Held) {
    auto* S=AS::State(this);if(!S || S->Frozen() || bDowned) {bAttackHeld=false;return;}
    if(Held) bReviving=false;
    bAttackHeld=Held && CanUseWeapons();
    if(Held&&ASArsenal::UsesMagazine(Build.Weapon)&&Ammo<=0&&!IsReloading()) RequestReload();
}
void AASCharacter::SetSpiral_Implementation(bool Held) {
    auto* S=AS::State(this);if(!S || S->Frozen() || bDowned || !CanUseWeapons()) {bCharging=false;return;}
    if(Held) bReviving=false;
    if(Held && !bCharging && S->SimTime>=NextSpiral) {bCharging=true;ChargeStart=S->SimTime;}
    if(!Held && bCharging) {CastSpiral(ASRules::Perfect(S->SimTime-ChargeStart,Rank(6),Relic(19)));bCharging=false;}
}
void AASCharacter::SetRevive_Implementation(bool Held) {
    const auto* S=AS::State(this);
    bReviving=Held&&!bDowned&&Health>0&&S&&!S->Frozen()&&(S->Fighting()||S->Phase==EASPhase::Reward);
    if(bReviving) {CancelMelee();ReloadStart=ReloadEnd=0;bAttackHeld=false;bCharging=false;StopJumping();}
    ForceNetUpdate();
}
FVector AASCharacter::MuzzleLocation() const {
    const FVector Start=GetActorLocation()+FVector(0,0,35);
    const FVector Desired=Start+GetActorForwardVector()*(Build.Weapon==2?120.f:(Build.Weapon>=6?190.f:65.f))+GetActorRightVector()*18.f;
    FHitResult Hit;FCollisionQueryParams Query;Query.AddIgnoredActor(this);
    if(GetWorld()->SweepSingleByObjectType(Hit,Start,Desired,FQuat::Identity,FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeSphere(9.f),Query)) return Hit.Location;
    return Desired;
}
void AASCharacter::Attack() {
    auto* S=AS::State(this);if(!HasAuthority()||!S||!CanUseWeapons()||IsReloading()) return;
    if(ASArsenal::UsesMagazine(Build.Weapon)&&!ASArsenal::Consume(Ammo,false)) {RequestReload();return;}
    float Haste=1;if(CombatBenefits()) for(auto W:S->Wells) if(W.IsValid()&&W->CanAffectGameplay()&&IsValid(W->Hero)&&W->Hero.Get()!=this&&W->Hero->Relic(5)&&FVector::DistSquared2D(GetActorLocation(),W->GetActorLocation())<FMath::Square(W->Radius)) Haste=1.25f;
    const bool Melee=ASMelee::IsMelee(Build.Weapon);
    if(Melee) {if(S->SimTime>LastMeleeEnd+.9f) NextMeleeStep=0;MeleeStep=NextMeleeStep;NextMeleeStep=(MeleeStep+1)%ASMelee::ComboLength(Build.Weapon);}
    const float Interval=ASRules::AttackInterval(ASData::Weapons[Build.Weapon].Rate,Rank(16),RunRank(1),Haste>1.f)*(Melee?ASMelee::TimeScale(Build.Weapon,MeleeStep):1.f);
    NextAttack=S->SimTime+Interval;AttackPoseDuration=Melee?Interval:FMath::Clamp(Interval*.9f,.08f,.28f);AttackPoseStarted=S->SimTime;++AttackSequence;OnAttackPose();
    const bool Critical=(CombatBenefits()&&bNextCritical)||FMath::FRand()<ASRules::CriticalChance(Rank(19),RunRank(5));if(CombatBenefits()) bNextCritical=false;
    FVector Forward=(AimPoint-GetActorLocation()).GetSafeNormal2D();if(Forward.IsNearlyZero()) Forward=GetActorForwardVector();
    if(Melee) {
        bMeleePending=true;bMeleeMotionStarted=false;bMeleeCritical=Critical;PendingMeleeWeapon=Build.Weapon;
        PendingMeleePhase=int32(S->Phase);PendingMeleeGeneration=S->Arena?S->Arena->Config.Generation:-1;
        MeleeImpactAt=S->SimTime+Interval*ASMelee::ImpactFraction(Build.Weapon,MeleeStep);
        MeleeMotionAt=S->SimTime+Interval*ASMelee::MotionFraction(Build.Weapon);LastMeleeEnd=NextAttack;
    } else {
        const FVector Muzzle=MuzzleLocation();
        const FVector Direction=(AimPoint-Muzzle).GetSafeNormal();
        const float Base=ASArsenal::Damage(Build.Weapon,PrimaryDamageBonus());
        const int32 Pellets=ASArsenal::Pellets(Build.Weapon);
        for(int32 Pellet=0;Pellet<Pellets;++Pellet) {
            FRotator Aim=Direction.Rotation();
            if(Build.Weapon==ASArsenal::Scattergun&&Pellet>0) {
                const float Angle=(Pellet-1)*UE_TWO_PI/5.f;Aim.Yaw+=FMath::Cos(Angle)*4.5f*SpreadScale();Aim.Pitch+=FMath::Sin(Angle)*4.5f*SpreadScale();
            } else if(Build.Weapon==ASArsenal::Rotary) {
                const float Spread=(.4f+RotaryHeat*2.1f)*SpreadScale();Aim.Yaw+=FMath::FRandRange(-Spread,Spread);Aim.Pitch+=FMath::FRandRange(-Spread,Spread);
            }
            AASProjectile::Fire(GetWorld(),Muzzle,Aim.Vector(),this,Build.Weapon,Base,false,Critical,Pellet>0);
        }
        const float ExtraDamage=Base*Pellets;
        if(TraitValue(ASBuild::Trait::EchoShot)>0) AASProjectile::Fire(GetWorld(),Muzzle,Direction.RotateAngleAxis(5.f*SpreadScale(),FVector::UpVector),this,Build.Weapon,ExtraDamage*.45f,false,Critical,true);
        const int32 Extra=FMath::Min(3,TraitValue(ASBuild::Trait::Volley));
        for(int32 I=0;I<Extra;++I) AASProjectile::Fire(GetWorld(),Muzzle,Direction.RotateAngleAxis((I%2?1.f:-1.f)*(10.f+I*4.f)*SpreadScale(),FVector::UpVector),this,Build.Weapon,ExtraDamage*.35f,false,Critical,true);
        S->Effect(2,Muzzle,Forward,BodyColor(),25);
    }
    // Begin on the last trigger, even when Attack is released immediately.
    if(ASArsenal::UsesMagazine(Build.Weapon)&&Ammo==0) RequestReload();
    ForceNetUpdate();
}
void AASCharacter::CastSpiral(bool Perfect) {
    auto* S=AS::State(this);if(!S) return;
    if(CombatBenefits()&&Relic(13)&&Health<MaxHealth*.5f&&Health>3) {Health-=3;Perfect=true;}
    if(Perfect&&CombatBenefits()) if(auto* PS=GetPlayerState<AASPlayerState>()) PS->Record(ASCareer::Perfects);
    NextSpiral=S->SimTime+SpiralCooldown();
    FVector Dir=(AimPoint-GetActorLocation()).GetSafeNormal2D();if(Dir.IsNearlyZero()) Dir=GetActorForwardVector();
    FVector Place=GetActorLocation()+Dir*(Relic(16)?820.f:560.f);FHitResult Wall;
    if(GetWorld()->LineTraceSingleByObjectType(Wall,GetActorLocation(),Place,FCollisionObjectQueryParams(ECC_WorldStatic))) Place=Wall.ImpactPoint-Dir*70;
    FHitResult Ground;
    if(!GetWorld()->LineTraceSingleByObjectType(Ground,Place+FVector(0,0,350),Place-FVector(0,0,700),FCollisionObjectQueryParams(ECC_WorldStatic))) Place=GetActorLocation()-FVector(0,0,80);
    else Place=Ground.ImpactPoint+FVector(0,0,10);
    AASWell::Plant(this,Place,Perfect);
    if((Relic(8)&&Perfect)||Build.Style==6) AASWell::Plant(this,Place+FVector(-Dir.Y,Dir.X,0)*240,false,.72f);
    if(CombatBenefits()&&Relic(9)) Shield=ASRules::GrantShield(Shield,10.f,60.f);
    if(CombatBenefits()&&Build.Style==5) {Movement()->DashCharges=FMath::Max(1,Movement()->DashCharges);Movement()->DashCooldown=0;}
    if(CombatBenefits()&&Perfect&&(Relic(3)||Build.Style==9)) bNextCritical=true;
    S->Effect(3,Place,Dir,BodyColor(),Perfect?100:70);
}
void AASCharacter::OnDash() {
    CancelMelee();
    if(!HasAuthority()) return;auto* S=AS::State(this);if(!S) return;
    DashVictims.Empty();DashTraceStart=GetActorLocation();bDashTraceActive=true;
    InvulnerableUntil=FMath::Max(InvulnerableUntil,S->SimTime+FMath::Max(Rank(11)? .4f:0.f,.26f+Rank(9)*.025f+.03f*TraitValue(ASBuild::Trait::LongDash)));
    if(CombatBenefits()&&TraitValue(ASBuild::Trait::Repulse)>0) for(auto W:S->Enemies) if(auto* E=W.Get()) if(CanDamage(E)&&FVector::DistSquared2D(GetActorLocation(),E->GetActorLocation())<FMath::Square((280.f+40.f*TraitValue(ASBuild::Trait::Repulse))*AreaScale())&&AS::ClearSight(GetWorld(),GetActorLocation(),E->GetActorLocation())) E->Control(0,E->GetActorLocation()-GetActorLocation(),600.f);
    if(CombatBenefits()&&Talent(4)) RallyUntil=S->SimTime+3;
    S->Effect(4,GetActorLocation()-FVector(0,0,65),Movement()->DashDirection,BodyColor(),100);
    if(Relic(22)) AASWell::Plant(this,GetActorLocation()-FVector(0,0,78),false,.55f,1);
    if(Relic(10)) AASField::Spawn(GetWorld(),1,GetActorLocation()-FVector(0,0,78),70,5,0,.65f+Rank(10)*.2f,this,GetActorLocation()+Movement()->DashDirection*500-FVector(0,0,78));
}
void AASCharacter::HitEnemy(AASEnemy* E,float Amount,bool Critical,EASHitKind Kind,bool ApplyElement,bool AllowProcs) {
    if(!HasAuthority()||!CanDamage(E)||E->Health<=0) return;auto* S=AS::State(this);if(!S) return;
    Amount*=DamageScale(ApplyElement);
    if(E->Boss()) Amount*=1.f+.15f*TraitValue(ASBuild::Trait::BossHunter);
    else if(E->Elite()) Amount*=1.f+.15f*TraitValue(ASBuild::Trait::EliteHunter);
    if(E->Health>=E->MaxHealth*.99f) Amount*=1.f+.20f*TraitValue(ASBuild::Trait::Opener);
    if(CombatBenefits()) for(auto W:S->Heroes) if(W.IsValid()&&W->RallyUntil>S->SimTime&&FVector::DistSquared2D(GetActorLocation(),W->GetActorLocation())<720*720) {Amount*=1.1f;break;}
    if(CombatBenefits()&&S->PlayerArray.Num()==1 && Talent(6)) Amount*=1.05f;
    if(CombatBenefits()&&E->LastHitter.IsValid()&&E->LastHitter.Get()!=this&&S->SimTime-E->LastHitTime<.7f) Amount*=Talent(6)?1.18f:1.08f;
    if(Critical) Amount*=CriticalPower();
    E->LastHitter=this;E->LastHitTime=S->SimTime;
    const FVector At=E->GetActorLocation();const bool Pulled=E->PulledUntil>S->SimTime;const bool ShowNumber=Critical||ApplyElement||E->NextDamageNumber<=S->SimTime;if(ShowNumber) E->NextDamageNumber=S->SimTime+.14f;const float Before=E->Health;const bool Killed=E->Hit(Amount,this);const float Dealt=Killed?Before:Before-E->Health;
    if(!Killed&&ApplyElement) E->ApplyElement(Kind,this,DamageScale(true));
    if(ShowNumber) if(auto* PC=Cast<AASPlayerController>(GetController())) PC->DamageNumber(At+FVector(0,0,80),Dealt,Critical,Kind);
    if(ApplyElement&&AllowProcs) HitProcs(Killed?nullptr:E,At,Dealt);
    if(Critical) S->Effect(8,At+FVector(0,0,35),FVector::ForwardVector,ASData::HitColor(Kind),80);
    if(CombatBenefits()&&Killed&&Relic(17)&&Pulled) for(auto W:S->Wells) if(W.IsValid()&&W->CanAffectGameplay()&&W->Hero.Get()==this) W->Expires=FMath::Min(S->SimTime+6,W->Expires+.4f);
}
void AASCharacter::Area(const FVector& At,float Radius,float Damage,bool Critical) {
    Radius*=AreaScale();
    if(auto* S=AS::State(this)) {const auto Props=S->Breakables;for(auto W:Props) if(auto* B=W.Get()) if(FVector::DistSquared2D(At,B->GetActorLocation())<Radius*Radius&&B->VisibleFrom(At+FVector(0,0,55))) B->Strike(this,Damage*DamageScale(false));}
    if(auto* S=AS::State(this)) {const auto List=S->Enemies;for(auto W:List) if(W.IsValid()&&W->Health>0&&FVector::DistSquared2D(At,W->GetActorLocation())<FMath::Square(Radius) && AS::ClearSight(GetWorld(),At+FVector(0,0,55),W->GetActorLocation())) HitEnemy(W.Get(),Damage,Critical);}
}
void AASCharacter::Heal(float Amount) {if(HasAuthority()&&!bDowned) Health=FMath::Clamp(Health+Amount,0.f,MaxHealth);}
void AASCharacter::Hurt(float Amount,AASEnemy* Source,bool EnemyAttack) {
    auto* S=AS::State(this);if(!HasAuthority()||!FMath::IsFinite(Amount)||Amount<=0||!S||!S->Fighting()||S->Frozen()||bDowned||S->SimTime<InvulnerableUntil) return;
    for(auto W:S->Wells) if(W.IsValid()&&W->CanAffectGameplay()&&W->Hero.Get()==this&&Relic(18)&&!W->bGuardUsed&&FVector::DistSquared2D(GetActorLocation(),W->GetActorLocation())<FMath::Square(W->Radius)) {W->bGuardUsed=true;InvulnerableUntil=S->SimTime+.3f;return;}
    if(FMath::FRand()<FMath::Min(.25f,.04f*TraitValue(ASBuild::Trait::Evasion))) {S->Effect(4,GetActorLocation(),GetActorForwardVector(),FLinearColor(.3f,1,1),65);return;}
    float Reduction=ASBuild::DamageReduction(TraitValue(ASBuild::Trait::Armor),TraitValue(ASBuild::Trait::Defiance),S->SimTime-LastDamageAt<3.f);
    if(Talent(1)) Reduction+=Health<MaxHealth*.4f?.12f:.08f;
    Amount*=1.f-FMath::Min(.65f,Reduction);Amount*=1.f+.10f*CurseStacks;
    if(Source||EnemyAttack) Amount*=ASBuild::ChallengeDamage(S->Challenge);
    LastDamageAt=S->SimTime;if(auto* PS=GetPlayerState<AASPlayerState>()) PS->bRoomDamaged=true;
    if(S->PlayerArray.Num()==2) Amount*=1.1f;
    if(CombatBenefits()&&Talent(3)&&S->SimTime>=UnbrokenUntil&&Health-FMath::Max(0.f,Amount-Shield)<MaxHealth*.25f) {Shield+=MaxHealth*.2f;UnbrokenUntil=S->SimTime+12;}
    if(CombatBenefits()&&Amount>MaxHealth*.15f) for(auto W:S->Heroes) if(W.IsValid()&&!W->bDowned&&W->Talent(5)&&FVector::DistSquared2D(GetActorLocation(),W->GetActorLocation())<=720.f*720.f&&S->SimTime>=W->GuardianUntil) {W->Shield+=W->MaxHealth*.1f;W->GuardianUntil=S->SimTime+12;}
    const float Before=Health;float Absorb=FMath::Min(Shield,Amount);Shield-=Absorb;Health=FMath::Max(0.f,Health-(Amount-Absorb));InvulnerableUntil=S->SimTime+.45f+.08f*TraitValue(ASBuild::Trait::Grace);
    if(auto* PC=Cast<AASPlayerController>(GetController())) {
        if(Absorb>0) PC->DamageNumber(GetActorLocation(),Absorb,false,EASHitKind::Shield);
        if(Before>Health) PC->DamageNumber(GetActorLocation(),Before-Health,false,EASHitKind::Received);
    }
    S->Effect(5,GetActorLocation(),GetActorForwardVector(),FLinearColor(1,.12f,.08f),38);
    if(Health<=0) {
        if(Rank(3)&&!bMercyUsed) {bMercyUsed=true;Health=MaxHealth*.25f;InvulnerableUntil=S->SimTime+2;}
        else if(Talent(2)&&!bSecondWindUsed) {bSecondWindUsed=true;Health=MaxHealth*.3f;InvulnerableUntil=S->SimTime+2;}
        else if(TraitValue(ASBuild::Trait::Phoenix)>0&&!bPhoenixUsed) {bPhoenixUsed=true;Health=MaxHealth*.35f;InvulnerableUntil=S->SimTime+2.f;}
        else {
            bDowned=true;ReviveProgress=0;ReloadStart=ReloadEnd=0;ResetInput();Movement()->bWantsDash=false;Movement()->DashRemaining=0;
            GetCharacterMovement()->StopMovementImmediately();
            if(GetCharacterMovement()->MovementMode==MOVE_Custom) GetCharacterMovement()->SetMovementMode(MOVE_Falling);
            RefreshLook();ForceNetUpdate();
        }
    }
    // Resolve lethal saves/downing before retaliation: an on-kill heal must not
    // silently cancel a lethal hit. Downed heroes cannot retaliate or proc heals.
    if(CombatBenefits()&&IsValid(Source)&&S->SimTime>=NextThorns&&TraitValue(ASBuild::Trait::Thorns)>0) {NextThorns=S->SimTime+.3f;SecondaryDamage(Source,4.f*TraitValue(ASBuild::Trait::Thorns)*DamageScale(false),EASHitKind::Physical);}
}

void AASCharacter::DownedPose(TArray<FVector>& Points) const {
    for(int32 I:{1,0,2,3,4,17,5,6,18,7,8,11,9,10,12}) if(Parts.IsValidIndex(I)) Points.Add(Parts[I]->GetComponentLocation());
}
