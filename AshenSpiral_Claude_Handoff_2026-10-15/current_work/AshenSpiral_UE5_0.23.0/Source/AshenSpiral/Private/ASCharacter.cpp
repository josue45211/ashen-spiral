#include "ASCharacter.h"
#include "ASArsenalRules.h"
#include "ASPose.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ASMovementComponent.h"
#include "ASGameState.h"
#include "ASGameInstance.h"
#include "Math/RotationMatrix.h"
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
    CreateCape();
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
    return !bDowned&&!bReviving&&!HammerBusy()&&S&&!S->bEnteringPortal&&!S->Frozen()&&S->Phase!=EASPhase::Defeat
        &&Super::CanJumpInternal_Implementation();
}
void AASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASCharacter,bVengeanceUsed);DOREPLIFETIME(AASCharacter,bVengeanceFinisherReady);DOREPLIFETIME(AASCharacter,BleedRemaining);DOREPLIFETIME(AASCharacter,bEliminated);DOREPLIFETIME(AASCharacter,EndlessRanks);
    DOREPLIFETIME(AASCharacter,WarCryUntil);DOREPLIFETIME(AASCharacter,Build);DOREPLIFETIME(AASCharacter,RelicMask);DOREPLIFETIME(AASCharacter,Health);
    DOREPLIFETIME(AASCharacter,MaxHealth);DOREPLIFETIME(AASCharacter,Shield);DOREPLIFETIME(AASCharacter,ChargeStart);
    DOREPLIFETIME(AASCharacter,Ammo);DOREPLIFETIME(AASCharacter,ReloadStart);DOREPLIFETIME(AASCharacter,ReloadEnd);
    DOREPLIFETIME(AASCharacter,NextSpiral);DOREPLIFETIME(AASCharacter,NextAttack);DOREPLIFETIME(AASCharacter,InvulnerableUntil);
    DOREPLIFETIME(AASCharacter,ReviveProgress);DOREPLIFETIME(AASCharacter,bCharging);DOREPLIFETIME(AASCharacter,bDowned);
    DOREPLIFETIME(AASCharacter,ColorChoice);DOREPLIFETIME(AASCharacter,AttackSequence);DOREPLIFETIME(AASCharacter,ColorIndex);
    DOREPLIFETIME(AASCharacter,TraitRanks);DOREPLIFETIME(AASCharacter,EvolutionMask);DOREPLIFETIME(AASCharacter,LastUpgrade);
    DOREPLIFETIME(AASCharacter,Rerolls);DOREPLIFETIME(AASCharacter,CurseStacks);DOREPLIFETIME(AASCharacter,Focus);DOREPLIFETIME(AASCharacter,NextRune);
    DOREPLIFETIME(AASCharacter,ConduitHits);DOREPLIFETIME(AASCharacter,bPhoenixUsed);
    DOREPLIFETIME(AASCharacter,RunRanks);DOREPLIFETIME_CONDITION(AASCharacter,bDashActive,COND_SkipOwner);DOREPLIFETIME(AASCharacter,RunGains);DOREPLIFETIME(AASCharacter,bReviving);DOREPLIFETIME(AASCharacter,AttackPoseDuration);
    DOREPLIFETIME(AASCharacter,bHammerContact);DOREPLIFETIME(AASCharacter,HammerLeapState);
    DOREPLIFETIME(AASCharacter,HammerLeapStarted);DOREPLIFETIME(AASCharacter,HammerLandedAt);DOREPLIFETIME(AASCharacter,NextHammerLeap);
    DOREPLIFETIME(AASCharacter,AttackPoseStarted);DOREPLIFETIME(AASCharacter,MeleeStep);
}
void AASCharacter::ApplyBuild(const FASBuild& V,bool Fresh) {
    if(!HasAuthority()) return;Build=V;Build.Sanitize();
    if(auto* PS=GetPlayerState<AASPlayerState>()) {ColorIndex=PS->PlayerIndex;ColorChoice=PS->ColorChoice;}
    if(Fresh) {
        ResetTraits();bVengeanceUsed=bVengeanceFinisherReady=bEliminated=false;BleedRemaining=25.f;EndlessRanks.Init(0,4);
        RunRanks.Init(0,ASRules::RunUpgradeCount);RunGains.Init(0,ASExpedition::RunGainCount);
        MaxHealth=ASRules::StartingHealth(Rank(0),0);Health=MaxHealth;Shield=Rank(1)*5;RelicMask=0;
        bDowned=false;bMercyUsed=false;bSecondWindUsed=false;bNextCritical=false;bReviving=false;ReviveProgress=0;
        NextSpiral=NextAttack=NextHammerLeap=0;InvulnerableUntil=AS::Time(this)+2;Movement()->DashCooldown=0;Movement()->DashRemaining=0;Movement()->AirDashesUsed=0;Movement()->bAirDashing=false;
    }
    ResetMagazine();ResetInput(true);RefreshLook();ForceNetUpdate();
}
void AASCharacter::RefreshLook() {
    if(Parts.Num()!=21) return;
    const FLinearColor Fire=ColorChoice<0&&IsLocallyControlled()?FLinearColor(.08f,.55f,1.f):ViewColor(),Dark(.07f,.085f,.11f),Metal(.20f,.23f,.28f),Cloth(.012f,.016f,.023f);
    const TCHAR* Meshes[21]={TEXT("SM_Torso"),TEXT("SM_Helmet"),TEXT("SM_Pelvis"),TEXT("SM_Arm"),TEXT("SM_Forearm"),TEXT("SM_Arm"),TEXT("SM_Forearm"),TEXT("SM_Thigh"),TEXT("SM_Greave"),TEXT("SM_Thigh"),TEXT("SM_Greave"),TEXT("SM_Boot"),TEXT("SM_Boot"),TEXT("SM_Pauldron"),TEXT("SM_Pauldron"),TEXT("SM_Cape"),TEXT("SM_Visor"),TEXT("SM_Hand"),TEXT("SM_Hand"),TEXT("SM_CapeLower"),TEXT("SM_ArmorCore")};
    const FVector Positions[21]={FVector(0,0,111),FVector(0,0,165),FVector(0,0,78),FVector(0,-34,115),FVector(0,-38,78),FVector(0,34,115),FVector(0,38,78),FVector(0,-16,59),FVector(0,-16,25),FVector(0,16,59),FVector(0,16,25),FVector(6,-16,10),FVector(6,16,10),FVector(0,-34,135),FVector(0,34,135),FVector(-22,0,141),FVector::ZeroVector,FVector(12,-38,66),FVector(34,52,82),FVector(-3,0,-55),FVector::ZeroVector};
    for(int32 I=0;I<21;++I) {
        const bool Fabric=I==15||I==19,Glow=I==16||I==20;
        const FLinearColor C=Glow?Fire:(Fabric?Cloth:(I==1||I==0||I==13||I==14?Metal:Dark));
        ASAssets::Armor(Parts[I],Meshes[I],bDowned?FLinearColor(.3f,.02f,.025f):C,Fabric?0.f:.8f,Glow?4.f:0.f,Fabric?.95f:.42f);
        Parts[I]->SetRelativeLocation(Positions[I]);Parts[I]->SetRelativeScale3D(FVector(1));
    }
    RefreshCape();
    Parts[1]->SetRelativeScale3D(FVector(.95f,1.f,.85f));
    Parts[16]->AttachToComponent(Parts[1],FAttachmentTransformRules::KeepRelativeTransform);
    Parts[19]->AttachToComponent(Parts[15],FAttachmentTransformRules::KeepRelativeTransform);
    Parts[20]->AttachToComponent(Parts[0],FAttachmentTransformRules::KeepRelativeTransform);
    // 0.24.2 aesthetics: each weapon has its own finish (it used to wear the floor's stone texture): polished steel blades and hafts, the Hoarfrost Maul frost-blue steel (its glow
    // is this colour), the bow dark wood, the staff bronze, the guns gunmetal. Colour, metallic, roughness; TUNABLE.
    struct FWeaponFinish {FLinearColor Shade;float Metallic,Roughness;};
    static const FWeaponFinish Finishes[9]={{FLinearColor(.58f,.56f,.55f),.95f,.26f},{FLinearColor(.42f,.56f,.70f),.9f,.3f},{FLinearColor(.16f,.10f,.06f),0.f,.55f},
        {FLinearColor(.55f,.55f,.58f),.95f,.28f},{FLinearColor(.45f,.29f,.15f),.7f,.38f},{FLinearColor(.30f,.28f,.32f),.9f,.34f},
        {FLinearColor(.24f,.25f,.27f),.85f,.4f},{FLinearColor(.24f,.25f,.27f),.85f,.36f},{FLinearColor(.28f,.27f,.25f),.8f,.42f}};
    const FWeaponFinish& Coat=Finishes[FMath::Clamp(Build.Weapon,0,8)];
    ASAssets::Armor(WeaponMesh,FString::Printf(TEXT("SM_Weapon%d"),Build.Weapon),Coat.Shade,Coat.Metallic,0.f,Coat.Roughness);
    WeaponMesh->AttachToComponent(Build.Weapon==2?Parts[17].Get():Parts[18].Get(),FAttachmentTransformRules::KeepRelativeTransform);
    WeaponMesh->SetRelativeLocation(FVector::ZeroVector);WeaponMesh->SetRelativeRotation(FRotator::ZeroRotator);
    if(Build.Weapon==2) Parts[17]->SetRelativeLocation(FVector(82,-5,123));
    for(auto W:Wisps) {ASAssets::Style(W,TEXT("SM_Crystal"),FLinearColor(1,.42f,.06f),0,4);W->SetRelativeScale3D(FVector(.12f));}
    ASAssets::Style(ArrowMesh,TEXT("SM_Arrow"),FLinearColor(.55f,.38f,.17f),.25f);ArrowMesh->SetVisibility(Build.Weapon==2);
    for(auto* C: {BowStrings[0].Get(),BowStrings[1].Get()}) {ASAssets::Style(C,TEXT("Cylinder"),FLinearColor(.78f,.74f,.61f));C->SetVisibility(Build.Weapon==2);}
    Parts[18]->SetRelativeLocation(FVector(34,62,82));Parts[18]->SetRelativeRotation(FRotator::ZeroRotator);
    ASAssets::Style(ChargeMesh,TEXT("Sphere"),Fire,0,9);ChargeMesh->SetRelativeLocation(FVector(12,0,0));ChargeMesh->SetRelativeScale3D(FVector(.22f));
    SetupSkeletalHero(); // 0.24.2 Phase F: the Mannequin takes over the look when its assets are here (the rig above stays as the fallback)
}
void AASCharacter::OnAttackPose() {PoseAge=0;if(bSkeletalHero&&GetNetMode()!=NM_DedicatedServer) PlayAttackClip();} // 0.24.2 Phase F: the Mannequin's attack clip
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
void AASCharacter::Tick(float Delta) {
    Super::Tick(Delta);auto* S=AS::State(this);if(!S || S->Frozen()) return;
    if(GetNetMode()!=NM_DedicatedServer) Animate(Delta);
    if(!HasAuthority()) return;
    {const bool DashNow=!bDowned&&Movement()->DashRemaining>0;if(DashNow!=bDashActive) {bDashActive=DashNow;ForceNetUpdate();}}
    const float Now=S->SimTime;
    if(GetCharacterMovement()->IsMovingOnGround() && GetActorLocation().Z>40) LastSafe=GetActorLocation();
    if(GetActorLocation().Z<-600) {ResetInput(true);SetActorLocation(S->Arena?S->Arena->SpawnPoint(ColorIndex):LastSafe,false,nullptr,ETeleportType::TeleportPhysics);GetCharacterMovement()->StopMovementImmediately();Hurt(18);}
    if(bDowned) {
        ResetInput(true);Movement()->bWantsDash=false;Movement()->DashRemaining=0;
        AASCharacter* Helper=nullptr;
        for(auto W:S->Heroes) if(auto* Other=W.Get()) if(Other->bReviving&&Other->CanRevive(this)) {Helper=Other;break;}
        // Only the downed pawn advances its replicated timer. It no longer
        // depends on which player's Tick happens first, and works in Reward too.
        if(S->bEndless) {BleedRemaining=ASExpedition::Bleed(BleedRemaining,Delta,Helper!=nullptr&&!bEliminated);if(BleedRemaining<=0) bEliminated=true;}
        if(bEliminated) {ReviveProgress=0;return;}
        ReviveProgress=ASRules::AdvanceRevive(ReviveProgress,Delta,Helper!=nullptr);
        if(ReviveProgress>=ASRules::ReviveSeconds) {
            bDowned=false;BleedRemaining=25.f;Health=MaxHealth*.45f;InvulnerableUntil=Now+2.f;ReviveProgress=0;
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
    TickHammer(Now);TickMelee(Now);
    if(CanUseWeapons() && !IsReloading() && !bReviving && bAttackHeld && Now>=NextAttack&&!bMeleePending&&!HammerBusy()) Attack();
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
    const auto* S=AS::State(this);return IsValid(Enemy)&&!Enemy->SpawnProtected()&&S&&!S->Frozen()&&!bDowned&&ASRules::CanDamageTarget(int(S->Phase),Enemy->bTrainingDummy,false);
}
void AASCharacter::ResetInput(bool Force) {CancelMelee(Force);if(Force) CancelHammerLeap();RotaryHeat=0;bAttackHeld=false;bCharging=false;bReviving=false;StopJumping();Movement()->bWantsDash=false;}
bool AASCharacter::CanRevive(const AASCharacter* Target) const {
    const auto* S=AS::State(this);
    if(!S||S->Frozen()||!(S->Fighting()||S->Phase==EASPhase::Reward)||bDowned||Health<=0
        ||!IsValid(Target)||Target==this||!Target->bDowned||Target->bEliminated||!GetCharacterMovement()->IsMovingOnGround()) return false;
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
    if(HammerBusy()) {bCharging=false;return;}
    if(Held) bReviving=false;
    if(Held && !bCharging && S->SimTime>=NextSpiral) {bCharging=true;ChargeStart=S->SimTime;}
    if(!Held && bCharging) {CastSpiral(ASRules::Perfect(S->SimTime-ChargeStart,Rank(6),Relic(19)));bCharging=false;}
}
void AASCharacter::SetRevive_Implementation(bool Held) {
    const auto* S=AS::State(this);
    bReviving=Held&&!HammerBusy()&&!bDowned&&Health>0&&S&&!S->Frozen()&&(S->Fighting()||S->Phase==EASPhase::Reward);
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
    auto* S=AS::State(this);if(!HasAuthority()||!S||!CanUseWeapons()||IsReloading()||HammerBusy()||(Build.Weapon==1&&Movement()->DashRemaining>0)) return;
    if(ASArsenal::UsesMagazine(Build.Weapon)&&!ASArsenal::Consume(Ammo,false)) {RequestReload();return;}
    float Haste=1;if(CombatBenefits()) for(auto W:S->Wells) if(W.IsValid()&&W->CanAffectGameplay()&&IsValid(W->Hero)&&W->Hero.Get()!=this&&W->Hero->Relic(5)&&FVector::DistSquared2D(GetActorLocation(),W->GetActorLocation())<FMath::Square(W->Radius)) Haste=1.25f;
    const bool Melee=ASMelee::IsMelee(Build.Weapon);
    if(Melee) {if(S->SimTime>LastMeleeEnd+(Build.Weapon==1?HammerComboWindow:.9f)) NextMeleeStep=0;if(Build.Weapon==1&&NextMeleeStep==2&&!Movement()->IsMovingOnGround()) return;MeleeStep=NextMeleeStep;NextMeleeStep=(MeleeStep+1)%ASMelee::ComboLength(Build.Weapon);}
    const float Interval=ASRules::AttackInterval(ASData::Weapons[Build.Weapon].Rate,Rank(16),RunRank(1),Haste>1.f)*(Melee?ASMelee::TimeScale(Build.Weapon,MeleeStep):1.f);
    if(Build.Weapon==1) {HammerComboWindow=ASHammer::ComboWindow(Interval/ASMelee::TimeScale(1,MeleeStep));bHammerContact=false;if(MeleeStep==2) {bCharging=false;StopJumping();}}
    NextAttack=S->SimTime+Interval;AttackPoseDuration=Melee?Interval:FMath::Clamp(Interval*.9f,.08f,.28f);AttackPoseStarted=S->SimTime;++AttackSequence;OnAttackPose();
    const bool Critical=(CombatBenefits()&&bNextCritical)||FMath::FRand()<ASRules::CriticalChance(Rank(19),RunRank(5));if(CombatBenefits()) bNextCritical=false;
    FVector Forward=(AimPoint-GetActorLocation()).GetSafeNormal2D();if(Forward.IsNearlyZero()) Forward=GetActorForwardVector();
    if(Melee) {
        bMeleePending=true;bMeleeMotionStarted=false;bMeleeCritical=Critical;PendingMeleeWeapon=Build.Weapon;
        PendingMeleePhase=int32(S->Phase);PendingMeleeGeneration=S->Arena?S->Arena->Config.Epoch:-1;
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
    AbilityGesture();
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
    const float DashProtection=FMath::Max(Rank(11)? .4f:0.f,.26f+Rank(9)*.025f+.03f*TraitValue(ASBuild::Trait::LongDash));
    InvulnerableUntil=FMath::Max(InvulnerableUntil,S->SimTime+DashProtection);DashInvulEnd=S->SimTime+DashProtection; // 0.24 C-3: DashInvulEnd (+ ASPartC::PerfectDashWindow) is what Hurt() tests for a perfect dash
    if(CombatBenefits()&&TraitValue(ASBuild::Trait::Repulse)>0) for(auto W:S->Enemies) if(auto* E=W.Get()) if(CanDamage(E)&&FVector::DistSquared2D(GetActorLocation(),E->GetActorLocation())<FMath::Square((280.f+40.f*TraitValue(ASBuild::Trait::Repulse))*AreaScale())&&AS::ClearSight(GetWorld(),GetActorLocation(),E->GetActorLocation())) E->Control(0,E->GetActorLocation()-GetActorLocation(),600.f,0,this);
    if(CombatBenefits()&&Talent(4)) RallyUntil=S->SimTime+3;
    S->Effect(4,GetActorLocation()-FVector(0,0,65),Movement()->DashDirection,BodyColor(),100);
    if(Relic(22)) AASWell::Plant(this,GetActorLocation()-FVector(0,0,78),false,.55f,1);
    if(Relic(10)) AASField::Spawn(GetWorld(),1,GetActorLocation()-FVector(0,0,78),70,5,0,.65f+Rank(10)*.2f,this,GetActorLocation()+Movement()->DashDirection*500-FVector(0,0,78));
}
void AASCharacter::OnPerfectDash() {
    auto* S=AS::State(this);if(!HasAuthority()||!S) return;
    LastPerfectDashAt=S->SimTime;InvulnerableUntil=FMath::Max(InvulnerableUntil,S->SimTime+ASPartC::PerfectDashBonusProtection);
    PerfectBoostUntil=S->SimTime+ASPartC::PerfectDashBoostSeconds; // the reward is a damage boost (the old dash refund meant little: the cooldown is short)
    if(auto* Player=GetPlayerState<AASPlayerState>()) Player->RunPerfectDashes=FMath::Min(Player->RunPerfectDashes+1,100000);
    S->Effect(8,GetActorLocation()+FVector(0,0,40),FVector::UpVector,FLinearColor(1.f,.90f,.45f),130.f); // a gold ring: the blow found nothing
    if(auto* PC=Cast<AASPlayerController>(GetController())) PC->Notify(FString::Printf(TEXT("PERFECT DASH / +%d%% DAMAGE"),FMath::RoundToInt(ASPartC::PerfectDashBoost*100.f)));
    ForceNetUpdate();
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
    if(!Killed&&Kind==EASHitKind::Shock&&IsValid(E)) E->RegisterCoopElement(Kind,this,DamageScale(false));
    if(!Killed&&ApplyElement&&IsValid(E)) {const float Scale=OnHitScale();E->ApplyElement(Kind,this,DamageScale(true)*Scale,Scale,Dealt*ASRules::OwnElementShare*Scale);} // 0.24: the weapon's own element is an on-hit reward too (damage-over-time strength and frost buildup scale with the weapon's base rate)
    if(ShowNumber) if(auto* PC=Cast<AASPlayerController>(GetController())) PC->DamageNumber(At+FVector(0,0,80),Dealt,Critical,Kind);
    if(ApplyElement&&AllowProcs) HitProcs(Killed||!IsValid(E)||E->Health<=0?nullptr:E,At,Dealt);
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
    auto* S=AS::State(this);if(!HasAuthority()||!FMath::IsFinite(Amount)||Amount<=0||!S||!S->Fighting()||S->Frozen()||bDowned) return;
    // 0.24 C-3 perfect dash: a blow that lands inside the dash's protection, or within ASPartC::PerfectDashWindow after it ends, is dodged outright and pays out (a damage boost, a moment more protection).
    if(ASPartC::PerfectDash(S->SimTime,DashInvulEnd,LastPerfectDashAt)) {OnPerfectDash();return;}
    if(S->SimTime<InvulnerableUntil||HammerInvulnerable()) return;
    for(auto W:S->Wells) if(W.IsValid()&&W->CanAffectGameplay()&&W->Hero.Get()==this&&Relic(18)&&!W->bGuardUsed&&FVector::DistSquared2D(GetActorLocation(),W->GetActorLocation())<FMath::Square(W->Radius)) {W->bGuardUsed=true;InvulnerableUntil=S->SimTime+.3f;return;}
    if(FMath::FRand()<FMath::Min(.25f,.04f*TraitValue(ASBuild::Trait::Evasion))) {S->Effect(4,GetActorLocation(),GetActorForwardVector(),FLinearColor(.3f,1,1),65);return;}
    float Reduction=ASBuild::DamageReduction(TraitValue(ASBuild::Trait::Armor),TraitValue(ASBuild::Trait::Defiance),S->SimTime-LastDamageAt<3.f);
    if(Talent(1)) Reduction+=Health<MaxHealth*.4f?.12f:.08f;
    Amount*=1.f-FMath::Min(.65f,Reduction);Amount*=1.f+.10f*CurseStacks;
    if(Source||EnemyAttack) Amount*=ASBuild::ChallengeDamage(S->Challenge)*ASExpedition::EnemyDamageScale(S->Difficulty,S->bEndless?S->EndlessWave:0,Source?(Source->Boss()?2:(Source->Elite()?1:0)):0);
    LastDamageAt=S->SimTime;if(auto* PS=GetPlayerState<AASPlayerState>()) PS->bRoomDamaged=true;
    if(S->PlayerArray.Num()==2) Amount*=1.1f;
    if(CombatBenefits()&&Talent(3)&&S->SimTime>=UnbrokenUntil&&Health-FMath::Max(0.f,Amount-Shield)<MaxHealth*.25f) {Shield+=MaxHealth*.2f;UnbrokenUntil=S->SimTime+12;}
    if(CombatBenefits()&&Amount>MaxHealth*.15f) for(auto W:S->Heroes) if(W.IsValid()&&!W->bDowned&&W->Talent(5)&&FVector::DistSquared2D(GetActorLocation(),W->GetActorLocation())<=720.f*720.f&&S->SimTime>=W->GuardianUntil) {W->Shield+=W->MaxHealth*.1f;W->GuardianUntil=S->SimTime+12;}
    const float Before=Health;float Absorb=FMath::Min(Shield,Amount);Shield-=Absorb;Health=FMath::Max(0.f,Health-(Amount-Absorb));InvulnerableUntil=S->SimTime+.45f+.08f*TraitValue(ASBuild::Trait::Grace);
    if(auto* PC=Cast<AASPlayerController>(GetController())) {
        if(Absorb>0) PC->DamageNumber(GetActorLocation(),Absorb,false,EASHitKind::Shield);
        if(Before>Health) PC->DamageNumber(GetActorLocation(),Before-Health,false,EASHitKind::Received);
    }
    DamageReaction(IsValid(Source)?(GetActorLocation()-Source->GetActorLocation()).GetSafeNormal2D():-GetActorForwardVector(),FMath::Clamp(Amount/MaxHealth*3.f,.2f,1.f));
    if(auto* Player=GetPlayerState<AASPlayerState>()) Player->RunDamageTaken=FMath::Min(1.e12f,Player->RunDamageTaken+Before-Health+Absorb);
    S->Effect(5,GetActorLocation(),GetActorForwardVector(),FLinearColor(1,.12f,.08f),38);
    if(Health<=0) {
        if(Rank(3)&&!bMercyUsed) {bMercyUsed=true;Health=MaxHealth*.25f;InvulnerableUntil=S->SimTime+2;}
        else if(Talent(2)&&!bSecondWindUsed) {bSecondWindUsed=true;Health=MaxHealth*.3f;InvulnerableUntil=S->SimTime+2;}
        else if(TraitValue(ASBuild::Trait::Vengeance)>0&&!bVengeanceUsed) {bVengeanceUsed=true;bVengeanceFinisherReady=true;Health=1.f;InvulnerableUntil=S->SimTime+ASBuild::VengeanceProtection;if(auto* PC=Cast<AASPlayerController>(GetController())) PC->Notify(TEXT("VENGEANCE OATH / next finisher x5"));}
        else if(TraitValue(ASBuild::Trait::Phoenix)>0&&!bPhoenixUsed) {
            bPhoenixUsed=true;Health=MaxHealth*ASBuild::PhoenixHealthFraction;InvulnerableUntil=S->SimTime+ASBuild::PhoenixProtection;
            S->Effect(8,GetActorLocation(),FVector::UpVector,FLinearColor(1,.25f,.03f),ASBuild::PhoenixNovaRadius);
            const auto Targets=S->Enemies;for(auto W:Targets) if(auto* E=W.Get()) if(CanDamage(E)&&FVector::DistSquared2D(GetActorLocation(),E->GetActorLocation())<FMath::Square(ASBuild::PhoenixNovaRadius)&&AS::ClearSight(GetWorld(),GetActorLocation(),E->GetActorLocation())) HitEnemy(E,ASBuild::PhoenixNovaDamage,false,EASHitKind::Burn,true,false);
        }
        else {
            bDowned=true;BleedRemaining=25.f;bEliminated=false;ReviveProgress=0;ReloadStart=ReloadEnd=0;ResetInput(true);Movement()->bWantsDash=false;Movement()->DashRemaining=0;
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
