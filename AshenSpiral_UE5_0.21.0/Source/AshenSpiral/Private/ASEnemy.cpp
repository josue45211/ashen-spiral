#include "ASEnemy.h"
#include "ASMeleeRules.h"
#include "ASCharacter.h"
#include "ASGameState.h"
#include "ASGameMode.h"
#include "ASPlayerController.h"
#include "ASWorld.h"
#include "ASSpell.h"
#include "ASData.h"
#include "ASPose.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
AASEnemy::AASEnemy() {
    bReplicates=true;PrimaryActorTick.bCanEverTick=true;SetNetUpdateFrequency(20.f);SetMinNetUpdateFrequency(10.f);
    GetCapsuleComponent()->InitCapsuleSize(38,80);GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);GetCapsuleComponent()->CanCharacterStepUpOn=ECB_No;GetCharacterMovement()->bEnablePhysicsInteraction=false;
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);GetMesh()->SetHiddenInGame(true);
    GetCharacterMovement()->bRunPhysicsWithNoController=true;GetCharacterMovement()->bOrientRotationToMovement=true;
    GetCharacterMovement()->MaxWalkSpeed=310;GetCharacterMovement()->RotationRate=FRotator(0,540,0);GetCharacterMovement()->MaxStepHeight=35;
    ArmorFigure=CreateDefaultSubobject<USceneComponent>(TEXT("SinisterArmor"));ArmorFigure->SetupAttachment(RootComponent);
    for(int32 I=0;I<21;++I) {auto* Part=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("EnemyArmor%02d"),I));Part->SetupAttachment(ArmorFigure);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);ArmorParts.Add(Part);}
    ArmorWeapon=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyWeapon"));ArmorWeapon->SetupAttachment(ArmorParts[18]);
    ArmorShield=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyShield"));ArmorShield->SetupAttachment(ArmorParts[17]);
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));Body->SetupAttachment(RootComponent);
    Eye=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Eyes"));Eye->SetupAttachment(RootComponent);
    Tell=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Anticipation"));Tell->SetupAttachment(RootComponent);
    Crown=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crown"));Crown->SetupAttachment(RootComponent);
    Beam=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SniperSight"));Beam->SetupAttachment(RootComponent);
    Aura=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ElementAura"));Aura->SetupAttachment(RootComponent);
}
void AASEnemy::BeginPlay() {Super::BeginPlay();if(auto* S=AS::State(this)) S->Enemies.AddUnique(this);NextAttack=AS::Time(this)+1.5f;NextRoute=AS::Time(this)+FMath::FRandRange(0.f,.3f);LastRoutePosition=GetActorLocation();RefreshLook();}
void AASEnemy::EndPlay(const EEndPlayReason::Type R) {if(auto* S=AS::State(this)) S->Enemies.Remove(this);Super::EndPlay(R);}
void AASEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASEnemy,Kind);DOREPLIFETIME(AASEnemy,Ward);DOREPLIFETIME(AASEnemy,Theme);DOREPLIFETIME(AASEnemy,Health);
    DOREPLIFETIME(AASEnemy,MaxHealth);DOREPLIFETIME(AASEnemy,WindupEnd);DOREPLIFETIME(AASEnemy,WindupStart);DOREPLIFETIME(AASEnemy,Aim);DOREPLIFETIME(AASEnemy,bWinding);DOREPLIFETIME(AASEnemy,bTrainingDummy);DOREPLIFETIME(AASEnemy,LastDamageTime);DOREPLIFETIME(AASEnemy,StatusBits);
}
void AASEnemy::RefreshLook() {
    const bool Humanoid=!bTrainingDummy&&Kind!=5&&Kind!=9;
    for(auto Part:ArmorParts) Part->SetVisibility(Humanoid);
    ArmorWeapon->SetVisibility(Humanoid);ArmorShield->SetVisibility(Humanoid&&Kind==6);Body->SetVisibility(!Humanoid);
    if(bTrainingDummy) {
        GetCapsuleComponent()->SetCapsuleSize(42,100);ASAssets::Style(Body,TEXT("SM_TrainingDummy"),FLinearColor(.48f,.28f,.12f),.1f);
        Body->SetRelativeLocation(FVector(0,0,-100));Body->SetRelativeScale3D(FVector(1));
        Eye->SetVisibility(false);Tell->SetVisibility(false);Crown->SetVisibility(false);Beam->SetVisibility(false);RefreshStatus();return;
    }
    const float S=Boss()?2.15f:(Elite()?1.45f:1.f);BodyScale=S;
    const TCHAR* Models[]={TEXT("SM_Wraith"),TEXT("SM_Wraith"),TEXT("SM_Wraith"),TEXT("SM_Brute"),TEXT("SM_Brute"),TEXT("SM_RiftLeaper"),TEXT("SM_Bulwark"),TEXT("SM_EmberShaman"),TEXT("SM_RiftSniper"),TEXT("SM_Broodcaller")};
    const FLinearColor Colors[]={FLinearColor(.1f,.2f,.26f),FLinearColor(.2f,.1f,.35f),FLinearColor(.25f,.3f,.14f),FLinearColor(.32f,.065f,.05f),ASData::BiomeColor(Ward),FLinearColor(.28f,.1f,.5f),FLinearColor(.42f,.35f,.19f),FLinearColor(.55f,.12f,.025f),FLinearColor(.03f,.32f,.45f),FLinearColor(.1f,.38f,.12f)};
    ASAssets::Style(Body,Models[FMath::Clamp(Kind,0,9)],FMath::Lerp(Colors[FMath::Clamp(Kind,0,9)],ASData::BiomeColor(Theme),.35f),Kind==6?.8f:.4f);
    const float HalfHeight=Elite()?187.5f*S*.5f:83.f;GetCapsuleComponent()->SetCapsuleHalfHeight(HalfHeight);
    Body->SetRelativeLocation(FVector(0,0,Elite()?62.5f*S-HalfHeight:-HalfHeight));Body->SetRelativeScale3D(FVector(S));
    ASAssets::Style(Eye,TEXT("Cube"),Kind==8?FLinearColor(.1f,.7f,1):FLinearColor(1,.09f,.025f),0,7);Eye->SetRelativeLocation(FVector(33*S,0,55*S));Eye->SetRelativeScale3D(FVector(.08f,.44f,.065f)*S);Eye->SetVisibility(Kind<5);
    Crown->SetVisibility(false); // Crystals grow in the environment, never on enemy heads.
    ASAssets::Style(Tell,TEXT("SM_Ring"),Kind==8?FLinearColor(.1f,.65f,1):FLinearColor(1,.1f,.02f),0,5);Tell->SetRelativeLocation(FVector(0,0,-HalfHeight+4));Tell->SetRelativeScale3D(FVector(Boss()?9:4,Boss()?9:4,1));
    ASAssets::Style(Beam,TEXT("Cube"),FLinearColor(.1f,.65f,1),0,5);Beam->SetRelativeLocation(FVector(850,0,15));Beam->SetRelativeScale3D(FVector(17,.035f,.035f));Beam->SetVisibility(false);
    if(Humanoid) {
        const TCHAR* Meshes[]={TEXT("SM_Torso"),TEXT("SM_EnemyHelm"),TEXT("SM_Pelvis"),TEXT("SM_Arm"),TEXT("SM_Forearm"),TEXT("SM_Arm"),TEXT("SM_Forearm"),TEXT("SM_Thigh"),TEXT("SM_Greave"),TEXT("SM_Thigh"),TEXT("SM_Greave"),TEXT("SM_Boot"),TEXT("SM_Boot"),TEXT("SM_Pauldron"),TEXT("SM_Pauldron"),TEXT("SM_Cape"),TEXT("SM_Visor"),TEXT("SM_Hand"),TEXT("SM_Hand"),TEXT("SM_EnemyCape"),TEXT("SM_ArmorCore")};
        const FLinearColor Metal=FMath::Lerp(FLinearColor(.10f,.12f,.15f),Colors[FMath::Clamp(Kind,0,9)],.18f),Fire(1.f,.065f,.025f);
        for(int32 I=0;I<21;++I) {const bool Glow=I==16||I==20,Fabric=I==15||I==19;ASAssets::Armor(ArmorParts[I],Meshes[I],Glow?Fire:(Fabric?FLinearColor(.018f,.008f,.012f):Metal),Fabric?0.f:.78f,Glow?4.f:0.f,Fabric?.96f:.45f);}
        ArmorFigure->SetRelativeLocation(FVector(0,0,-HalfHeight));ArmorFigure->SetRelativeScale3D(FVector(.82f,.70f,1.f)*(Elite()?S:.90f));
        ArmorParts[0]->SetRelativeLocation(FVector(0,0,111));ArmorParts[1]->SetRelativeLocation(FVector(0,0,165));ArmorParts[2]->SetRelativeLocation(FVector(0,0,78));
        ArmorParts[13]->SetRelativeLocation(FVector(0,-34,135));ArmorParts[14]->SetRelativeLocation(FVector(0,34,135));ArmorParts[15]->SetRelativeLocation(FVector(-22,0,141));
        ArmorParts[1]->SetRelativeScale3D(FVector(.95f,1.f,.85f));
        ArmorParts[16]->AttachToComponent(ArmorParts[1],FAttachmentTransformRules::KeepRelativeTransform);ArmorParts[16]->SetRelativeLocation(FVector::ZeroVector);
        ArmorParts[19]->AttachToComponent(ArmorParts[15],FAttachmentTransformRules::KeepRelativeTransform);ArmorParts[19]->SetRelativeLocation(FVector(-3,0,-55));
        ArmorParts[20]->AttachToComponent(ArmorParts[0],FAttachmentTransformRules::KeepRelativeTransform);ArmorParts[20]->SetRelativeLocation(FVector::ZeroVector);
        ASAssets::Armor(ArmorWeapon,FString::Printf(TEXT("SM_Weapon%d"),Kind==8?7:(Kind==1||Kind==7?4:(Elite()||Kind==6?1:0))),Metal,.8f);
        ASAssets::Armor(ArmorShield,TEXT("SM_EnemyShield"),Metal,.8f);ArmorShield->SetRelativeLocation(FVector(14,0,15));
        Eye->SetVisibility(false);
    }
    GetCapsuleComponent()->SetCapsuleRadius(Boss()?80:(Elite()?52:38));RefreshStatus();
}

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

void AASEnemy::RefreshStatus() {
    const EASHitKind Type=(StatusBits&12)?EASHitKind::Frost:((StatusBits&2)?EASHitKind::Poison:((StatusBits&16)?EASHitKind::Bleed:EASHitKind::Burn));
    ASAssets::Style(Aura,TEXT("SM_Ring"),ASData::HitColor(Type),0,3);
    Aura->SetRelativeLocation(FVector(0,0,-GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+12));
    const float Size=Boss()?2.4f:1.3f;Aura->SetRelativeScale3D(FVector(Size,Size,1));Aura->SetVisibility(StatusBits!=0);
}
void AASEnemy::ApplyElement(EASHitKind Type,AASCharacter* Source,float Power) {
    if(!HasAuthority()||!IsValid(Source)||!Source->CanDamage(this)) return;
    if(Type==EASHitKind::Frost&&Power<=0) Elements.Frost=FMath::Max(Elements.Frost,12.f);
    else Elements.Apply(int(Type),Power,Boss(),ASBuild::StatusDuration(Source->TraitValue(ASBuild::Trait::Duration)));
    if(Type==EASHitKind::Burn) BurnSource=Source;if(Type==EASHitKind::Poison) PoisonSource=Source;if(Type==EASHitKind::Bleed) BleedSource=Source;
}
bool AASEnemy::TickElements(float Delta) {
    const auto Damage=Elements.Tick(Delta);BurnPending+=Damage.first;PoisonPending+=Damage.second;BleedPending+=Elements.BleedDamage;
    const uint8 Bits=(Elements.Burn>0?1:0)|(Elements.Poison>0?2:0)|(Elements.Frost>0?4:0)|(Elements.Frozen>0?8:0)|(Elements.Bleed>0?16:0);
    if(StatusBits!=Bits) {StatusBits=Bits;RefreshStatus();ForceNetUpdate();}
    const float Now=AS::Time(this);
    if(Now>=NextStatusDamage) {
        NextStatusDamage=Now+.5f;
        auto Apply=[this](float& Pending,TWeakObjectPtr<AASCharacter> Source,EASHitKind Type) {
            const float Amount=Pending;Pending=0;if(Amount<=0||!Source.IsValid()||!Source->CanDamage(this)) return false;
            const FVector P=GetActorLocation()+FVector(0,0,80);
            if(auto* PC=Cast<AASPlayerController>(Source->GetController())) PC->DamageNumber(P,Amount,false,Type);
            return Hit(Amount,Source.Get()); // DOT does not crit or trigger on-hit effects.
        };
        if(Apply(BurnPending,BurnSource,EASHitKind::Burn)) return false;
        if(Apply(PoisonPending,PoisonSource,EASHitKind::Poison)) return false;
        if(Apply(BleedPending,BleedSource,EASHitKind::Bleed)) return false;
    }
    return true;
}
void AASEnemy::Tick(float Delta) {
    Super::Tick(Delta);auto* S=AS::State(this);if(!S) return;
    GetCharacterMovement()->SetComponentTickEnabled(!S->Frozen());
    if(S->Frozen()) {GetCharacterMovement()->StopMovementImmediately();return;}
    Tell->SetVisibility(bWinding&&!bTrainingDummy);Beam->SetVisibility(Kind==8&&bWinding&&!bTrainingDummy);
    if(!bTrainingDummy) {
        const float T=bWinding?FMath::Clamp((S->SimTime-WindupStart)/FMath::Max(.01f,WindupEnd-WindupStart),0.f,1.f):0;
        if(GetNetMode()!=NM_DedicatedServer) AnimateArmor(Delta,T);
        Body->SetRelativeScale3D(FVector(BodyScale*(1.f+T*.07f),BodyScale*(1.f+T*.07f),BodyScale*(1.f-T*(Kind==5?.30f:.12f))));
        Body->SetRelativeRotation(FRotator(bWinding?(Kind==5?-15.f:-7.f)*T:0,0,0));
        if(bWinding) {Tell->SetRelativeRotation(FRotator(0,S->SimTime*100,0));if(auto* M=Cast<UMaterialInstanceDynamic>(Tell->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),3+T*6);}
    }
    if(!HasAuthority()||Health<=0) return;
    const bool Active=ASRules::CanDamageTarget(int(S->Phase),bTrainingDummy,false);
    if(!Active) {
        Elements=ASRules::ElementState();BurnPending=PoisonPending=BleedPending=0;
        if(StatusBits) {StatusBits=0;RefreshStatus();}GetCharacterMovement()->StopMovementImmediately();return;
    }
    if(!TickElements(Delta)) return;
    if(bTrainingDummy) {
        GetCharacterMovement()->StopMovementImmediately();
        if(S->SimTime-LastDamageTime>5.f) {Health=MaxHealth;Elements=ASRules::ElementState();}
        return;
    }
    if(Elements.Frozen>0||StunUntil>S->SimTime) {GetCharacterMovement()->StopMovementImmediately();bWinding=false;NextAttack=FMath::Max(NextAttack,S->SimTime+.25f);StuckFor=0;return;}
    AASCharacter* Target=nullptr;float Best=FLT_MAX;
    for(auto H:S->Heroes) if(H.IsValid()&&!H->bDowned) {float D=FVector::DistSquared2D(GetActorLocation(),H->GetActorLocation());if(D<Best){Best=D;Target=H.Get();}}
    if(!Target) return;
    if(GetActorLocation().Z<-500 && S->Arena) {SetActorLocation(S->Arena->SafeEnemyPoint(GetActorLocation()),false,nullptr,ETeleportType::TeleportPhysics);GetCharacterMovement()->StopMovementImmediately();}
    if(LeapUntil>S->SimTime) {
        GetCharacterMovement()->MaxWalkSpeed=1100;StuckFor=0;
        if(Best<170*170&&S->SimTime>LastContact&&AS::ClearSight(GetWorld(),GetActorLocation(),Target->GetActorLocation())) {LastContact=S->SimTime+.5f;Target->Hurt(16,this);}return;
    }
    if(ChargeUntil>S->SimTime) {
        GetCharacterMovement()->MaxWalkSpeed=1100;AddMovementInput(Aim,1,true);
        if(Best<200*200 && S->SimTime>LastContact&&AS::ClearSight(GetWorld(),GetActorLocation(),Target->GetActorLocation())) {LastContact=S->SimTime+.5f;Target->Hurt(Kind==6?20:28,this);}return;
    }
    if(bWinding) {StuckFor=0;LastRoutePosition=GetActorLocation();GetCharacterMovement()->StopMovementImmediately();if(S->SimTime>=WindupEnd) {bWinding=false;CommitAttack();NextAttack=S->SimTime+(Boss()?2.f:1.8f);}return;}
    const float Range=Kind==8?1700.f:((Kind==1||Kind==2||Kind==7||Kind==9||Boss())?1300.f:(Kind==5||Kind==6?650.f:180.f));
    GetCharacterMovement()->MaxWalkSpeed=(Boss()?235.f:(Kind==5?390.f:(Kind==6?230.f:(Kind==0?340.f:275.f))))*ASArsenal::EnemySpeedScale(Elite()||Boss())*ASMelee::ApproachScale(FMath::Sqrt(Best),Range,Boss())*(SlowUntil>S->SimTime?.45f:(Elements.Frost>0?(Boss()?.75f:.65f):1.f));
    const bool Sight=AS::ClearSight(GetWorld(),GetActorLocation(),Target->GetActorLocation());
    if(Best<FMath::Square(Range) && S->SimTime>=NextAttack && Sight&&((Kind!=5&&Kind!=6)||!S->Arena||S->Arena->CanWalkDirectly(GetActorLocation(),Target->GetActorLocation()))) {
        Aim=(Target->GetActorLocation()-GetActorLocation()).GetSafeNormal2D();WindupStart=S->SimTime;WindupEnd=S->SimTime+(Kind==8?1.5f:(Kind==9?1.4f:(Boss()||Kind==6?1.1f:.85f)));bWinding=true;SetActorRotation(Aim.Rotation());return;
    }
    if(RootUntil>S->SimTime) {GetCharacterMovement()->StopMovementImmediately();StuckFor=0;LastRoutePosition=GetActorLocation();return;}
    if(S->SimTime>=NextRoute||FVector::DistSquared2D(MoveGoal,GetActorLocation())<65*65) {
        NextRoute=S->SimTime+.25f;
        FVector Takeoff=FVector::ZeroVector,Landing=FVector::ZeroVector;const bool Jump=Kind==5&&S->Arena&&S->Arena->JumpPlan(GetActorLocation(),Target->GetActorLocation(),Takeoff,Landing);
        if(Jump) {
            MoveGoal=Takeoff;
            if(FVector::DistSquared2D(GetActorLocation(),Takeoff)<55*55&&!GetCharacterMovement()->IsFalling()) {
                const float Time=.95f;FVector V=(Landing-GetActorLocation())/Time;V.Z=FMath::Abs(GetCharacterMovement()->GetGravityZ())*Time*.5f;
                GetCharacterMovement()->MaxWalkSpeed=1100;GetCharacterMovement()->AirControl=0;LaunchCharacter(V,true,true);LeapUntil=S->SimTime+Time;return;
            }
        } else MoveGoal=S->Arena?S->Arena->Route(GetActorLocation(),Target->GetActorLocation()):Target->GetActorLocation();
    }
    FVector Move=(MoveGoal-GetActorLocation()).GetSafeNormal2D();
    // Local separation steers around blocking bodies without pushing or contact damage.
    FVector Separate=FVector::ZeroVector;
    for(auto W:S->Enemies) if(auto* Other=W.Get()) if(Other!=this) {
        const FVector Away=GetActorLocation()-Other->GetActorLocation();const float D=Away.Size2D();
        const float Spacing=GetCapsuleComponent()->GetScaledCapsuleRadius()+Other->GetCapsuleComponent()->GetScaledCapsuleRadius()+35.f;
        if(D>1&&D<Spacing&&FMath::Abs(Away.Z)<150.f) Separate+=Away.GetSafeNormal2D()*(1.f-D/Spacing);
    }
    if(!Move.IsNearlyZero()) {Separate-=Move*FVector::DotProduct(Separate,Move);Move=(Move+Separate*.8f).GetSafeNormal2D();}
    if(Best>145*145||!Sight) {
        if(RootUntil<=S->SimTime) AddMovementInput(Move,1,true);else GetCharacterMovement()->StopMovementImmediately();
        if(S->SimTime>=NextStuckCheck) {
            NextStuckCheck=S->SimTime+1.f;
            StuckFor=FVector::DistSquared2D(LastRoutePosition,GetActorLocation())<25*25?StuckFor+1.f:0.f;
            LastRoutePosition=GetActorLocation();
            if(StuckFor>=3.f&&S->Arena&&Best>600.f*600.f) {
                // Recover only after sustained failure. Navigation nodes were collision-
                // checked for the largest enemy and connected to the player spawn.
                SetActorLocation(S->Arena->RecoverEnemy(GetActorLocation(),Target->GetActorLocation()),false,nullptr,ETeleportType::TeleportPhysics);
                GetCharacterMovement()->StopMovementImmediately();NextRoute=0;StuckFor=0;
            }
        }
    } else {StuckFor=0;LastRoutePosition=GetActorLocation();}

}
void AASEnemy::CommitAttack() {
    auto* S=AS::State(this);if(!S) return;++Pattern;
    auto Shot=[&](const FVector& D,int32 Type,float Damage){auto* P=AASProjectile::Fire(GetWorld(),GetActorLocation()+D*75+FVector(0,0,15),D,nullptr,Type,Damage,true);if(P) P->EnemySource=this;};
    auto Blast=[&](const FVector& P,float R,float Damage,float Delay){auto* F=AASField::Spawn(GetWorld(),0,P,R,Damage,Delay,Delay+.4f);if(F) F->EnemySource=this;};
    if(Kind==5) {GetCharacterMovement()->MaxWalkSpeed=1100;LaunchCharacter(Aim*620+FVector(0,0,280),true,true);LeapUntil=S->SimTime+.65f;return;}
    if(Kind==6) {ChargeUntil=S->SimTime+.6f;Blast(GetActorLocation()-FVector(0,0,77),260,18,.2f);return;}
    if(Kind==7) {for(auto H:S->Heroes) if(H.IsValid()&&!H->bDowned) Blast(H->GetActorLocation()-FVector(0,0,77),220,18,.9f);return;}
    if(Kind==8) {Shot(Aim,8,24);return;}
    if(Kind==9) {
        if(Pattern<=2) {if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) for(int32 I=0;I<2;++I) GM->SpawnMinion(GetActorLocation()+FVector(160,I?150:-150,0));}
        else Shot(Aim,2,13);
        for(auto W:S->Enemies) if(auto* E=W.Get()) if(E!=this&&!E->Boss()&&FVector::DistSquared2D(E->GetActorLocation(),GetActorLocation())<550*550&&AS::ClearSight(GetWorld(),GetActorLocation(),E->GetActorLocation())) {E->Health=FMath::Min(E->MaxHealth,E->Health+8);E->ForceNetUpdate();}
        S->Effect(6,GetActorLocation(),FVector::UpVector,FLinearColor(.2f,1,.35f),550);return;
    }
    if(Boss()) {
        const bool PhaseTwo=Health<MaxHealth*.5f;
        if(Ward==1 && Pattern%2==0) {ChargeUntil=S->SimTime+.7f;return;}
        if(Ward==2 && PhaseTwo) for(auto W:S->Wells) if(W.IsValid()) W->bInverted=true;
        if(Ward==3 || Pattern%2==0) {
            for(auto H:S->Heroes) if(H.IsValid()&&!H->bDowned) {
                Blast(H->GetActorLocation()-FVector(0,0,77),PhaseTwo?390:300,28,.95f);
                if(Ward==3 && PhaseTwo) H->LaunchCharacter((GetActorLocation()-H->GetActorLocation()).GetSafeNormal2D()*450,false,false);
            }
        } else {
            const int32 Count=PhaseTwo?18:12;
            for(int32 I=0;I<Count;++I) Shot(FRotator(0,I*360.f/Count,0).Vector(),2,20);
        }
    } else if(Kind==1||Kind==2) {
        for(int32 I=0;I<(Kind==2?3:1);++I) Shot(Aim.RotateAngleAxis((I-(Kind==2?1:0))*13,FVector::UpVector),2,14);
    } else {
        S->Effect(1,GetActorLocation()-FVector(0,0,40),Aim,FLinearColor(1,.1f,.02f),Elite()?340:210);
        for(auto H:S->Heroes) if(H.IsValid() && FVector::DistSquared2D(GetActorLocation(),H->GetActorLocation())<FMath::Square(Elite()?340.f:210.f) && FVector::DotProduct(Aim,(H->GetActorLocation()-GetActorLocation()).GetSafeNormal2D())>0 && AS::ClearSight(GetWorld(),GetActorLocation(),H->GetActorLocation())) H->Hurt(Elite()?24:12,this);
    }
}
bool AASEnemy::Hit(float Damage,AASCharacter* Source) {
    if(!HasAuthority()||Health<=0||!FMath::IsFinite(Damage)||Damage<=0) return false;
    if(Kind==6&&IsValid(Source)&&FVector::DotProduct(GetActorForwardVector(),(Source->GetActorLocation()-GetActorLocation()).GetSafeNormal2D())>.35f) Damage*=.5f;
    LastDamageTime=AS::Time(this);Health=FMath::Max(bTrainingDummy?1.f:0.f,Health-Damage);ForceNetUpdate();
    if(Health<=0) {
        if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->EnemyKilled(this,Source);
        Destroy();return true;
    }
    return false;
}

void AASEnemy::Control(int32 Type,const FVector& Direction,float Strength) {
    if(!HasAuthority()||Boss()||bTrainingDummy||Health<=0) return;
    const float Now=AS::Time(this);
    if(Type==0) LaunchCharacter(Direction.GetSafeNormal2D()*Strength*(Elite()?.5f:1.f),true,false);
    if(Type==1&&Now>=StunImmuneUntil) {StunUntil=Now+.4f;StunImmuneUntil=Now+2.f;bWinding=false;}
    if(Type==2&&Now>=RootImmuneUntil) {RootUntil=Now+.8f;RootImmuneUntil=Now+3.f;}
}
