#include <limits>
#include "ASEnemy.h"
#include "ASExpeditionRules.h"
#include "ASEnemyRules.h"
#include "ASEnemyAbilities.h"
#include "ASEnemyHazards.h"
#include "ASBreakable.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "ASPresentationRules.h"
#include "ASMeleeRules.h"
#include "ASExpeditionRules.h"
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
    GetCharacterMovement()->bRunPhysicsWithNoController=true;GetCharacterMovement()->bOrientRotationToMovement=false;bUseControllerRotationYaw=false; // 0.24: Tick turns it (toward the target when strafing or backing away, along its motion otherwise)
    GetCharacterMovement()->MaxWalkSpeed=310;GetCharacterMovement()->RotationRate=FRotator(0,540,0);GetCharacterMovement()->MaxStepHeight=35;
    ArmorFigure=CreateDefaultSubobject<USceneComponent>(TEXT("SinisterArmor"));ArmorFigure->SetupAttachment(RootComponent);
    for(int32 I=0;I<21;++I) {auto* Part=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("EnemyArmor%02d"),I));Part->SetupAttachment(ArmorFigure);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);ArmorParts.Add(Part);}
    ArmorWeapon=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyWeapon"));ArmorWeapon->SetupAttachment(ArmorParts[18]);
    ArmorShield=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyShield"));ArmorShield->SetupAttachment(ArmorParts[17]);
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));Body->SetupAttachment(RootComponent);
    Eye=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Eyes"));Eye->SetupAttachment(RootComponent);
    Tell=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Anticipation"));Tell->SetupAttachment(RootComponent);
    WingL=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WingL"));WingL->SetupAttachment(RootComponent);
    WingR=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WingR"));WingR->SetupAttachment(RootComponent);
    WingBoneL=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WingBoneL"));WingBoneL->SetupAttachment(RootComponent);
    WingBoneR=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WingBoneR"));WingBoneR->SetupAttachment(RootComponent);
    Crown=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crown"));Crown->SetupAttachment(RootComponent);
    Beam=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SniperSight"));Beam->SetupAttachment(RootComponent);
    TellFill=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnticipationFill"));TellFill->SetupAttachment(RootComponent);
    TellRipple=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnticipationRipple"));TellRipple->SetupAttachment(RootComponent);TellRipple->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Tell->SetVisibility(false);TellFill->SetVisibility(false);TellRipple->SetVisibility(false);Beam->SetVisibility(false); // warnings start hidden; Tick shows them only while an attack is being wound up
}
void AASEnemy::BeginPlay() {Super::BeginPlay();if(HasAuthority()&&Boss()) SpawnProtectedUntil=AS::Time(this)+2.f;
    if(HasAuthority()&&Kind==ASEnemyRules::Brute&&!bTrainingDummy&&Mutation==ASEnemyRules::MutationNone) Mutation=uint8(ASEnemyRules::MutationFromRoll(FMath::RandRange(0,3))); // every elite brute spawns mutated
   if(auto* S=AS::State(this)) S->Enemies.AddUnique(this);NextAttack=AS::Time(this)+1.5f;NextRoute=AS::Time(this)+FMath::FRandRange(0.f,.3f);LastRoutePosition=GetActorLocation();RefreshLook();
   BaseFriction=GetCharacterMovement()->GroundFriction;} // a launch's skid lowers the friction for a moment (TickKnock restores this)
void AASEnemy::EndPlay(const EEndPlayReason::Type R) {if(auto* S=AS::State(this)) S->Enemies.Remove(this);Super::EndPlay(R);}
void AASEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASEnemy,SpawnProtectedUntil);DOREPLIFETIME(AASEnemy,AttackReleaseTime);DOREPLIFETIME(AASEnemy,HitReactionTime);DOREPLIFETIME(AASEnemy,HitReactionStrength);DOREPLIFETIME(AASEnemy,HitReactionDirection);DOREPLIFETIME(AASEnemy,Kind);DOREPLIFETIME(AASEnemy,Ward);DOREPLIFETIME(AASEnemy,Theme);DOREPLIFETIME(AASEnemy,Health);
    DOREPLIFETIME(AASEnemy,MaxHealth);DOREPLIFETIME(AASEnemy,WindupEnd);DOREPLIFETIME(AASEnemy,WindupStart);DOREPLIFETIME(AASEnemy,Aim);DOREPLIFETIME(AASEnemy,bWinding);DOREPLIFETIME(AASEnemy,Ability);DOREPLIFETIME(AASEnemy,AIState);DOREPLIFETIME(AASEnemy,Flags);DOREPLIFETIME(AASEnemy,Mutation);DOREPLIFETIME(AASEnemy,bTrainingDummy);DOREPLIFETIME(AASEnemy,LastDamageTime);DOREPLIFETIME(AASEnemy,StatusBits);DOREPLIFETIME(AASEnemy,bAirborne);
}
void AASEnemy::RefreshLook() {
    const bool Humanoid=!bTrainingDummy&&Kind!=5&&Kind!=9&&Kind!=ASExpedition::FlierKind;
    for(auto Part:ArmorParts) Part->SetVisibility(Humanoid);
    ArmorWeapon->SetVisibility(Humanoid);ArmorShield->SetVisibility(Humanoid&&Kind==6);Body->SetVisibility(!Humanoid);
    if(bTrainingDummy) {
        GetCapsuleComponent()->SetCapsuleSize(42,100);ASAssets::Style(Body,TEXT("SM_TrainingDummy"),FLinearColor(.48f,.28f,.12f),.1f);
        Body->SetRelativeLocation(FVector(0,0,-100));Body->SetRelativeScale3D(FVector(1));
        Eye->SetVisibility(false);Tell->SetVisibility(false);TellFill->SetVisibility(false);Crown->SetVisibility(false);Beam->SetVisibility(false);return;
    }
    const float S=Boss()?2.15f:(Elite()?1.45f:1.f);BodyScale=S;
    const TCHAR* Models[]={TEXT("SM_Wraith"),TEXT("SM_Wraith"),TEXT("SM_Wraith"),TEXT("SM_Brute"),TEXT("SM_Brute"),TEXT("SM_RiftLeaper"),TEXT("SM_Bulwark"),TEXT("SM_EmberShaman"),TEXT("SM_RiftSniper"),TEXT("SM_Broodcaller")};
    const FLinearColor Colors[]={FLinearColor(.1f,.2f,.26f),FLinearColor(.2f,.1f,.35f),FLinearColor(.25f,.3f,.14f),FLinearColor(.32f,.065f,.05f),ASData::BiomeColor(Ward),FLinearColor(.28f,.1f,.5f),FLinearColor(.42f,.35f,.19f),FLinearColor(.55f,.12f,.025f),FLinearColor(.03f,.32f,.45f),FLinearColor(.1f,.38f,.12f)};
    ASAssets::Style(Body,Models[FMath::Clamp(Kind,0,9)],FMath::Lerp(Colors[FMath::Clamp(Kind,0,9)],ASData::BiomeColor(Theme),.35f),Kind==6?.8f:.4f); // 0.23.5: no red foe cast
    if(Kind==ASExpedition::FlierKind) ASAssets::Style(Body,TEXT("SM_Wraith"),FLinearColor(.30f,.05f,.38f),.3f,1.f); // Gloomwing body
    const float HalfHeight=Elite()?187.5f*S*.5f:83.f;GetCapsuleComponent()->SetCapsuleHalfHeight(HalfHeight);
    Body->SetRelativeLocation(FVector(0,0,Elite()?62.5f*S-HalfHeight:-HalfHeight));Body->SetRelativeScale3D(FVector(S));
    ASAssets::Style(Eye,TEXT("Cube"),Kind==8?FLinearColor(.1f,.7f,1):FLinearColor(1,.09f,.025f),0,7);Eye->SetRelativeLocation(FVector(33*S,0,55*S));Eye->SetRelativeScale3D(FVector(.08f,.44f,.065f)*S);Eye->SetVisibility(Kind<5||Kind==ASExpedition::FlierKind);
    Crown->SetVisibility(false); // Crystals grow in the environment, never on enemy heads.
    {   // Melee ground warning (0.24): a half disc exactly the shape the swing damages (ASEnemyRules::StrikeHits), dim at full size, with a bright copy growing over it.
        // Red only: blue means frost, violet-white means lightning. If SM_StrikeSector is not imported yet, fall back to the old ring at the same radius.
        const float StrikeR=ASEnemyRules::StrikeRadius(Elite());const bool HasSector=ASAssets::HasMesh(TEXT("SM_StrikeSector"));
        const FString TellMesh=HasSector?TEXT("SM_StrikeSector"):TEXT("SM_Ring");const float TellScale=StrikeR/(HasSector?50.f:53.f);
        ASAssets::Style(Tell,TellMesh,FLinearColor(.55f,.05f,.02f),0,1.5f);Tell->SetCastShadow(false);Tell->SetRelativeLocation(FVector(0,0,-HalfHeight+3));Tell->SetRelativeScale3D(FVector(TellScale,TellScale,1));
        ASAssets::Style(TellFill,TellMesh,FLinearColor(1.f,.16f,.03f),0,6.f);TellFill->SetCastShadow(false);TellFill->SetRelativeLocation(FVector(0,0,-HalfHeight+4.5f));TellFill->SetRelativeScale3D(FVector(.02f,.02f,1));
        Tell->SetVisibility(false);TellFill->SetVisibility(false); // Tick shows them while a melee swing is being wound up
        TellShape=ASEnemyRules::TellSector;
    }
    {   // Flier wings (0.24): a bat-style wing made of a membrane mesh and a bone-frame mesh, mirrored for the left side with a negative Y scale. The Gloomwing carries small ones;
        // the armoured flying boss carries big ones on its back. If the new meshes are not imported on this machine yet, the old flat slabs are used instead. Fliers also pass
        // over walls (no world-static collision), so they need no pathfinding; ConfineFlight keeps them in the arena.
        const bool Flier=Flies(),Armoured=Flier&&Boss(); // the flying boss keeps its armoured body; the Gloomwing is the wraith
        const bool NewWings=ASAssets::HasMesh(TEXT("SM_BossWingMembrane"))&&ASAssets::HasMesh(TEXT("SM_BossWingBones"));
        const float WingScale=Armoured?2.1f:.9f;                                         // the 138 cm wing becomes about 290 cm for the boss, 125 cm for the Gloomwing
        const FVector Root=Armoured?FVector(-38.f,46.f,82.f):FVector(-14.f,40.f,22.f);   // right shoulder in actor space; the left side mirrors Y
        const FLinearColor SkinColor=Armoured?FLinearColor(.28f,.04f,.20f):FLinearColor(.30f,.05f,.38f),FrameColor=Armoured?FLinearColor(.12f,.10f,.11f):FLinearColor(.10f,.06f,.14f);
        for(int32 Side=0;Side<2;++Side) {
            auto* Membrane=Side==0?WingL.Get():WingR.Get();auto* Bones=Side==0?WingBoneL.Get():WingBoneR.Get();
            if(Membrane) Membrane->SetVisibility(Flier);
            if(Bones) Bones->SetVisibility(Flier&&NewWings);
            if(!Flier) continue;
            const float Mirror=Side==0?-1.f:1.f; // WingL sits on -Y
            if(Membrane) {
                if(NewWings) {ASAssets::Style(Membrane,TEXT("SM_BossWingMembrane"),SkinColor,.1f,2.2f);Membrane->SetRelativeScale3D(FVector(WingScale,WingScale*Mirror,WingScale));Membrane->SetRelativeLocation(FVector(Root.X,Root.Y*Mirror,Root.Z));}
                else {ASAssets::Style(Membrane,TEXT("Cube"),FLinearColor(.16f,.03f,.26f),.2f,2.5f);Membrane->SetRelativeScale3D(FVector(.6f,1.2f,.05f)*(Armoured?S:1.f));Membrane->SetRelativeLocation(FVector(-8,70*Mirror,30)*(Armoured?S:1.f));}
                Membrane->SetCastShadow(false);
            }
            if(Bones&&NewWings) {ASAssets::Style(Bones,TEXT("SM_BossWingBones"),FrameColor,.6f,.3f);Bones->SetCastShadow(false);Bones->SetRelativeScale3D(FVector(WingScale,WingScale*Mirror,WingScale));Bones->SetRelativeLocation(FVector(Root.X,Root.Y*Mirror,Root.Z));}
        }
        if(Flier&&WingL&&WingR) {GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Ignore);GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Ignore);}
    }
    ASAssets::Style(Beam,TEXT("Cube"),FLinearColor(1.f,.62f,.12f),0,5);Beam->SetRelativeLocation(FVector(850,0,15));Beam->SetRelativeScale3D(FVector(17,.035f,.035f));Beam->SetVisibility(false);
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
        const int32 WeaponIndex=Kind==8?7:(Kind==1||Kind==7?4:(Elite()||Kind==6?1:0));
        ASAssets::Armor(ArmorWeapon,FString::Printf(TEXT("SM_Weapon%d"),WeaponIndex),Metal,.8f);
        // 0.25 phase A: an elite swinging the maul carries the Molten Face Hammer when it has been imported on this machine (Tools/import_vault_assets.py, Docs/0.25_ASSET_PLAN.md); the built-in maul stands in otherwise.
        if(WeaponIndex==1&&Elite()&&ASAssets::HasMesh(TEXT("SM_MoltenHammer"))) {
            ASAssets::Style(ArmorWeapon,TEXT("SM_MoltenHammer"),Metal,.8f);
            if(auto* Molten=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Imported/MoltenHammer/M_MoltenHammer.M_MoltenHammer"),nullptr,LOAD_NoWarn)) ArmorWeapon->SetMaterial(0,Molten);
        }
        ASAssets::Armor(ArmorShield,TEXT("SM_EnemyShield"),Metal,.8f);ArmorShield->SetRelativeLocation(FVector(14,0,15));
        Eye->SetVisibility(false);
    }
    GetCapsuleComponent()->SetCapsuleRadius(Boss()?80:(Elite()?52:38));
}

void AASEnemy::Glint() {
    FVector At=GetActorLocation()+GetActorForwardVector()*40.f*BodyScale+FVector(0.f,0.f,55.f*BodyScale); // a body without a weapon (the leaper) glints at its brow
    if(ArmorWeapon&&ArmorWeapon->IsVisible()&&ArmorWeapon->GetStaticMesh()) At=ArmorWeapon->GetComponentTransform().TransformPosition(FVector(0.f,0.f,ArmorWeapon->GetStaticMesh()->GetBoundingBox().Max.Z*.92f)); // the tip: weapons stand along +Z
    if(UWorld* W=GetWorld()) if(auto* FX=W->SpawnActor<AASVisualEffect>()) FX->Setup(20,At,FVector::ForwardVector,FLinearColor(1.f,.93f,.72f),Boss()?140.f:(Elite()?100.f:72.f));
}
bool AASEnemy::ShowsStrikeCircle() const {
    // A shape on the ground means "damage lands here when it fills". The ability being wound up says whether it has one: the melee swings' half disc, the ground pound's full
    // disc. Shots, leaps, charges, blasts and meteors (their own field telegraph), dives and the sniper beam draw none.
    const int32 Shape=ASEnemyRules::SpecFor(Ability).Tell;
    // 0.24.2: strips (the boss stampede) and travelling rings are drawn too - the strip's drawing existed but this test left it out, so it never showed.
    return !bTrainingDummy&&AIState==ASEnemyRules::StateWindup&&(Shape==ASEnemyRules::TellSector||Shape==ASEnemyRules::TellDisc||Shape==ASEnemyRules::TellLine||Shape==ASEnemyRules::TellRing);
}
void AASEnemy::TickStatusTint() {
    // 0.23.5: status effects pulse the body tint instead of drawing a ring at the feet. Cosmetic, runs where there is a screen.
    const float Now=AS::Time(this);
    const bool Enraged=(Flags&(ASEnemyRules::FlagEnraged|ASEnemyRules::FlagPhaseTwo))!=0,Mutated=Mutation!=ASEnemyRules::MutationNone; // a boss in its second phase is as angry as an enraged brute
    const bool Active=(StatusBits!=0||Enraged||Mutated)&&Health>0;
    if(!Active&&!bTintApplied) return;
    TArray<UStaticMeshComponent*> Parts;Parts.Add(Body.Get());for(const auto& Part:ArmorParts) Parts.Add(Part.Get());
    if(bTintApplied&&BaseTints.Num()!=Parts.Num()) {BaseTints.Reset();bTintApplied=false;} // part list changed: forget stale colours
    if(!Active) { // restore the colours we saved
        for(int32 I=0;I<Parts.Num();++I) if(Parts.IsValidIndex(I)&&Parts[I]&&BaseTints.IsValidIndex(I)) if(auto* M=Cast<UMaterialInstanceDynamic>(Parts[I]->GetMaterial(0))) M->SetVectorParameterValue(TEXT("Tint"),BaseTints[I]);
        BaseTints.Reset();bTintApplied=false;return;
    }
    if(!bTintApplied) {
        BaseTints.Reset();
        for(auto* Part:Parts) {FLinearColor C(.1f,.1f,.1f);if(Part) if(auto* M=Cast<UMaterialInstanceDynamic>(Part->GetMaterial(0))) C=M->K2_GetVectorParameterValue(TEXT("Tint"));BaseTints.Add(C);}
        bTintApplied=true;
    }
    FLinearColor Status(1.f,.16f,.03f);float Pulse=.32f+.28f*FMath::Sin(Now*11.f+float(GetUniqueID()%7)); // enraged: a fast hot-red pulse (a status effect on top of it shows its own colour)
    if(!Enraged&&Mutated) {float R,G,B;ASEnemyRules::MutationColor(Mutation,R,G,B);Status=FLinearColor(R,G,B);Pulse=.22f+.16f*FMath::Sin(Now*3.f+float(GetUniqueID()%7));} // a mutation: a slow steady pulse in its colour
    if(StatusBits!=0) {
        const EASHitKind Type=(StatusBits&12)?EASHitKind::Frost:((StatusBits&2)?EASHitKind::Poison:((StatusBits&16)?EASHitKind::Bleed:EASHitKind::Burn));
        const bool Frozen=(StatusBits&8)!=0;
        Status=ASData::HitColor(Type);Pulse=Frozen?.7f:.35f+.3f*FMath::Sin(Now*7.f+float(GetUniqueID()%7)); // frozen: a steady strong tint; others: a pulse
    }
    for(int32 I=0;I<Parts.Num();++I) if(Parts[I]&&BaseTints.IsValidIndex(I)) if(auto* M=Cast<UMaterialInstanceDynamic>(Parts[I]->GetMaterial(0))) M->SetVectorParameterValue(TEXT("Tint"),FMath::Lerp(BaseTints[I],Status,FMath::Clamp(Pulse,0.f,.85f)));
}
void AASEnemy::EndWindup() {
    namespace ER=ASEnemyRules;
    const float Now=AS::Time(this);
    const bool Idle=!bWinding&&Ability==ER::AbilityNone&&(AIState==ER::StateApproach||AIState==ER::StateStrafe||AIState==ER::StateRetreat)&&LeapUntil<=Now&&ChargeUntil<=Now&&DiveUntil<=Now&&HopUntil<=Now;
    if(Idle) return;
    Ability=uint8(ER::AbilityNone);LeapUntil=ChargeUntil=DiveUntil=HopUntil=0;bTraversalLeap=false;RiposteAt=-1.f;bParryBlocked=false;Flags&=uint8(0xFFu&~ASEnemyRules::FlagPackCall); // a stunned or interrupted charge, leap, dive, hop or stance does not resume afterwards
    SetAIState(ER::StateApproach);ForceNetUpdate(); // the replicated flags are what every machine draws the ground warning and the pose from
}
void AASEnemy::ApplyElement(EASHitKind Type,AASCharacter* Source,float Power,float Buildup,float Pool) {
    if(!HasAuthority()||!IsValid(Source)||!Source->CanDamage(this)) return;
    RegisterCoopElement(Type,Source,Power);
    // A synergy can kill this very target; do not keep applying status to a dead actor.
    if(!IsValid(this)||Health<=0) return;
    if(Type==EASHitKind::Frost&&Power<=0) Elements.Frost=FMath::Max(Elements.Frost,12.f);
    else Elements.Apply(int(Type),Power,Boss(),ASBuild::StatusDuration(Source->TraitValue(ASBuild::Trait::Duration)),Buildup,Pool);
    if(Type==EASHitKind::Burn) BurnSource=Source;if(Type==EASHitKind::Poison) PoisonSource=Source;if(Type==EASHitKind::Bleed) BleedSource=Source;if(Type==EASHitKind::Frost&&Pool>0.f) FrostSource=Source;
}
bool AASEnemy::TickElements(float Delta) {
    const auto Damage=Elements.Tick(Delta);BurnPending+=Damage.first;PoisonPending+=Damage.second;BleedPending+=Elements.BleedDamage;FrostPending+=Elements.FrostDamage;
    const uint8 Bits=(Elements.Burn>0?1:0)|(Elements.Poison>0?2:0)|(Elements.Frost>0?4:0)|(Elements.Frozen>0?8:0)|(Elements.Bleed>0?16:0);
    if(StatusBits!=Bits) {StatusBits=Bits;ForceNetUpdate();}
    const float Now=AS::Time(this);
    if(Now>=NextStatusDamage) {
        NextStatusDamage=Now+.5f;
        auto Apply=[this](float& Pending,TWeakObjectPtr<AASCharacter> Source,EASHitKind Type) {
            const float Amount=Pending;Pending=0;if(Amount<=0||!Source.IsValid()||!Source->CanDamage(this)) return false;
            const FVector P=GetActorLocation()+FVector(0,0,80);const float Before=Health;
            const bool Killed=Hit(Amount*Source->ElementalDamageMultiplier(),Source.Get(),false); // Attunement scales status damage; DOT does not crit or trigger on-hit effects (and no guard holds against it).
            if(auto* PC=Cast<AASPlayerController>(Source->GetController())) PC->DamageNumber(P,Killed?Before:Before-Health,false,Type); // the number shows what was dealt (frost brittleness included)
            return Killed;
        };
        if(Apply(BurnPending,BurnSource,EASHitKind::Burn)) return false;
        if(Apply(PoisonPending,PoisonSource,EASHitKind::Poison)) return false;
        if(Apply(BleedPending,BleedSource,EASHitKind::Bleed)) return false;
        if(Apply(FrostPending,FrostSource,EASHitKind::Frost)) return false; // 0.24.2: frostbite
    }
    return true;
}
void AASEnemy::Tick(float Delta) {
    Super::Tick(Delta);auto* S=AS::State(this);if(!S) return;
    GetCharacterMovement()->SetComponentTickEnabled(!S->Frozen());
    if(S->Frozen()) {GetCharacterMovement()->StopMovementImmediately();return;}
    if(Health<=0&&!bTrainingDummy) {if(GetNetMode()!=NM_DedicatedServer) AnimateDeath();return;}
    if(HasAuthority()) TickKnock(S->SimTime,Delta);
    const bool ShowWarning=bWinding&&!bTrainingDummy&&!SpawnProtected()&&ShowsStrikeCircle(); // a ground warning means "damage lands in exactly this shape when it fills"
    Tell->SetVisibility(ShowWarning);TellFill->SetVisibility(ShowWarning);if(TellRipple) TellRipple->SetVisibility(ShowWarning&&ASEnemyRules::SpecFor(Ability).Tell==ASEnemyRules::TellRing);
    Beam->SetVisibility(Kind==8&&bWinding&&!bTrainingDummy); // 0.24: this call used to sit inside a comment, so the sniper's aiming beam never showed
    if(!bTrainingDummy) {
        const float T=bWinding?FMath::Clamp((S->SimTime-WindupStart)/FMath::Max(.01f,WindupEnd-WindupStart),0.f,1.f):0;
        const bool Blob=ASEnemyRules::KitFor(Kind).AnimSet==ASEnemyRules::AnimBlob; // the leaper and the broodcaller: one body, posed by AnimateBlob
        if(GetNetMode()!=NM_DedicatedServer) {
            AnimateArmor(Delta,T);if(Flies()) AnimateFlier();if(Blob) AnimateBlob();TickStatusTint();UpdateCues(AS::Time(this));
            if(Kind==ASEnemyRules::Sniper&&bWinding) ShowBeam(T);
            if(Kind==ASEnemyRules::Bulwark&&ArmorShield) {const bool Up=!(Flags&ASEnemyRules::FlagGuardBroken);if(ArmorShield->IsVisible()!=Up) ArmorShield->SetVisibility(Up);} // a broken guard drops its shield for a few seconds
        }
        if(!Blob) {
            Body->SetRelativeScale3D(FVector(BodyScale*(1.f+T*.07f),BodyScale*(1.f+T*.07f),BodyScale*(1.f-T*.12f)));
            Body->SetRelativeRotation(FRotator(bWinding?-7.f*T:0,0,0));
        }
        if(ShowWarning) ShowTell(T); // the bright shape grows over the dim full-size one; both stay flat on the floor and face the attack
        if(ShowWarning&&GetNetMode()!=NM_DedicatedServer&&GlintedFor!=WindupStart&&WindupEnd-S->SimTime<=ASEnemyRules::GlintLead) {GlintedFor=WindupStart;Glint();} // once per wind-up
    }
    if(!HasAuthority()||Health<=0) return;
    if((Flags&ASEnemyRules::FlagGuardBroken)&&S->SimTime>=GuardBrokenUntil) {Flags&=uint8(0xFFu&~ASEnemyRules::FlagGuardBroken);ForceNetUpdate();}
    if(SpawnProtected()) {if(Flies()&&GetCharacterMovement()->MovementMode!=MOVE_Flying) GetCharacterMovement()->SetMovementMode(MOVE_Flying);GetCharacterMovement()->StopMovementImmediately();EndWindup();return;} // a flyer ignores the floor, so it must not be left walking
    const bool Active=ASRules::CanDamageTarget(int(S->Phase),bTrainingDummy,false);
    if(!Active) {
        Elements=ASRules::ElementState();BurnPending=PoisonPending=BleedPending=0;
        EndWindup(); // an attack wound up when the phase ended must not leave its warning on the floor
        if(StatusBits) {StatusBits=0;ForceNetUpdate();}GetCharacterMovement()->StopMovementImmediately();return;
    }
    if(!TickElements(Delta)) return;
    if(bTrainingDummy) {
        GetCharacterMovement()->StopMovementImmediately();
        if(S->SimTime-LastDamageTime>5.f) {Health=MaxHealth;Elements=ASRules::ElementState();}
        return;
    }
    if(Launched(S->SimTime)&&!Flies()) { // 0.24.2: a launched walker flies and skids on its own - nothing steers it or stops it dead until it comes to rest (fliers: TickFlier)
        if(GetActorLocation().Z<-500&&S->Arena) {SetActorLocation(S->Arena->SafeEnemyPoint(GetActorLocation()),false,nullptr,ETeleportType::TeleportPhysics);GetCharacterMovement()->StopMovementImmediately();KnockUntil=0.f;bKnocked=false;}
        StuckFor=0;LastRoutePosition=GetActorLocation();return;
    }
    if(Elements.Frozen>0||StunUntil>S->SimTime) {GetCharacterMovement()->StopMovementImmediately();EndWindup();NextAttack=FMath::Max(NextAttack,S->SimTime+.25f);StuckFor=0;return;}
    AASCharacter* Target=nullptr;float Best=FLT_MAX;
    for(auto H:S->Heroes) if(H.IsValid()&&!H->bDowned) {float D=FVector::DistSquared2D(GetActorLocation(),H->GetActorLocation());if(D<Best){Best=D;Target=H.Get();}}
    if(!Target) {EndWindup();return;} // nobody left to hit: drop the attack and its ground warning
    if(Flies()) {TickFlier(Delta,Target,Best);return;}
    if(GetActorLocation().Z<-500 && S->Arena) {SetActorLocation(S->Arena->SafeEnemyPoint(GetActorLocation()),false,nullptr,ETeleportType::TeleportPhysics);GetCharacterMovement()->StopMovementImmediately();}
    TickGround(Delta,Target,Best);
}
// ---- The walker state machine (0.24) ----
// AIState is replicated. The server moves between Approach (closing in) and Strafe (circling or backing away), Windup (the telegraph), Attack (committed motion: a leap, a
// charge), Recover (a short exposed pause) and back to moving, with Retreat (falling back once when badly hurt) and Special (long kind-specific behaviours) alongside.
// Stun, freeze, death and the end of combat abandon whatever is running through EndWindup(). Which abilities a kind owns, how long each takes and how it moves come from
// ASEnemyRules (kits and the ability table), so the numbers are tested as plain C++.
void AASEnemy::SetAIState(int32 NewState) {
    if(!HasAuthority()) return;
    const int32 Clamped=FMath::Clamp(NewState,0,ASEnemyRules::StateCount-1);
    const bool Telegraphing=Clamped==ASEnemyRules::StateWindup||Clamped==ASEnemyRules::StateSpecial;
    if(int32(AIState)!=Clamped) {AIState=uint8(Clamped);ForceNetUpdate();}
    if(bWinding!=Telegraphing) {bWinding=Telegraphing;ForceNetUpdate();}
}
void AASEnemy::BeginAbility(int32 AbilityId,const FVector& ToTarget) {
    namespace ER=ASEnemyRules;
    auto* S=AS::State(this);if(!S) return;
    Aim=FVector(ToTarget.X,ToTarget.Y,0.f).GetSafeNormal();if(Aim.IsNearlyZero()) Aim=GetActorForwardVector();
    const int32 Id=FMath::Clamp(AbilityId,0,ER::AbilityCount-1);const ER::AbilitySpec& Spec=ER::SpecFor(Id);
    Ability=uint8(Id);LastAbility=Id;
    if(Id==ER::AbilityParry) {RiposteAt=-1.f;bParryBlocked=false;}
    const float Anger=((Flags&ER::FlagEnraged)&&!(Spec.Flags&ER::StanceFlag))?ER::EnrageWindupScale:1.f; // an enraged enemy winds up faster
    WindupStart=S->SimTime;WindupEnd=S->SimTime+ER::WindupFor(ER::KitFor(Kind),Id)*Anger;
    SetAIState((Spec.Flags&ER::StanceFlag)?ER::StateSpecial:ER::StateWindup);SetActorRotation(Aim.Rotation());ForceNetUpdate();
}
void AASEnemy::ReleaseAbility() {
    namespace ER=ASEnemyRules;
    auto* S=AS::State(this);if(!S) return;
    const ER::Kit& Kit=ER::KitFor(Kind);const int32 Id=FMath::Clamp<int32>(Ability,0,ER::AbilityCount-1);const float Now=S->SimTime;
    const ER::AbilitySpec& Spec=ER::SpecFor(Id);
    const FVector ToTarget=CurrentTarget.IsValid()?CurrentTarget->GetActorLocation()-GetActorLocation():Aim;
    AttackReleaseTime=Now;ForceNetUpdate(); // every release is stamped (the pose and the release cue are timed from it)
    const int32 Flow=ReleaseOwnAbility(Id,ToTarget); // the 0.24 abilities with their own effect and flow
    if(Flow==FlowNotMine) CommitAttack(); // the effect of the attack by kind (it also starts a leap or a charge by setting LeapUntil / ChargeUntil)
    if(Id==ER::AbilitySnipe) StartRelocation(); // after the shot the sniper takes a new vantage point
    const float Anger=(Flags&ER::FlagEnraged)?ER::EnrageCooldownScale:1.f; // an enraged enemy recharges faster
    if(!(Spec.Flags&ER::NoKitCooldownFlag)) NextAttack=Now+Kit.Cooldown*Anger;
    AbilityReadyAt[Id]=Now+Spec.Cooldown*Anger;
    if(Flow==FlowChained) return; // another ability began inside the handler; its state is already set
    if(Flow==FlowAttack||LeapUntil>Now||ChargeUntil>Now) SetAIState(ER::StateAttack);
    else FinishAttackPhase();
}
void AASEnemy::FinishAttackPhase() {
    namespace ER=ASEnemyRules;
    auto* S=AS::State(this);if(!S) return;
    const ER::AbilitySpec& Spec=ER::SpecFor(Ability);
    if(Spec.Recover>0.f&&!bTraversalLeap) {RecoverUntil=S->SimTime+Spec.Recover;SetAIState(ER::StateRecover);}
    else {Ability=uint8(ER::AbilityNone);SetAIState(ER::StateApproach);}
    bTraversalLeap=false;
}
void AASEnemy::TickGround(float Delta,AASCharacter* Target,float DistanceSquared) {
    namespace ER=ASEnemyRules;
    auto* S=AS::State(this);auto* Move=GetCharacterMovement();if(!S||!Move||!Target) return;
    const float Now=S->SimTime;const ER::Kit Kit=WardBossKit()?ER::BossKitFor(Ward):ER::KitWithMutation(ER::KitFor(Kind),Mutation); // a ward's boss has its own kit, a mutated elite owns one more ability
    const FVector Here=GetActorLocation(),ToTarget=Target->GetActorLocation()-Here;
    CurrentTarget=Target;
    // ---- Committed phases: nothing else decides anything until they end. ----
    if(AIState==ER::StateWindup||AIState==ER::StateSpecial) {
        StuckFor=0;LastRoutePosition=Here;Move->StopMovementImmediately();
        if(Ability==ER::AbilitySnipe&&AIState==ER::StateWindup) { // the aiming beam follows the hero for the first part of the wind-up, then locks: the shot goes along the locked line
            const float Progress=FMath::Clamp((Now-WindupStart)/FMath::Max(.01f,WindupEnd-WindupStart),0.f,1.f);
            if(!ER::SnipeLocked(Progress)) {
                const float Yaw=ER::TrackYaw(Aim.Rotation().Yaw,FVector(ToTarget.X,ToTarget.Y,0.f).Rotation().Yaw,Delta,ER::SnipeTurnRate);
                Aim=FRotator(0.f,Yaw,0.f).Vector();SetActorRotation(FRotator(0.f,Yaw,0.f));
            }
        }
        if(Ability==ER::AbilityParry) { // the guard keeps facing its target, and a block is answered by a counter-slash a moment later
            const FVector Look=FVector(ToTarget.X,ToTarget.Y,0.f).GetSafeNormal();
            if(!Look.IsNearlyZero()) SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(),FRotator(0.f,Look.Rotation().Yaw,0.f),Delta,Kit.TurnRate));
            if(RiposteAt>=0.f&&Now>=RiposteAt) {
                AbilityReadyAt[ER::AbilityParry]=Now+ER::SpecFor(ER::AbilityParry).Cooldown;
                RiposteAt=-1.f;bParryBlocked=false;
                BeginAbility(ER::AbilityRiposte,ToTarget);return; // the counter begins straight out of the guard
            }
        }
        if(Now>=WindupEnd) ReleaseAbility();
        return;
    }
    if(AIState==ER::StateAttack) {
        Move->MaxAcceleration=2048.f; // the old default: a charge reaches full speed as it always did, whatever the kit's walking acceleration
        if(HopUntil>Now) { // a back-hop: a fast, straight push away from the target (the direction was chosen at the release), facing it all the while
            Move->MaxAcceleration=12000.f;Move->MaxWalkSpeed=ER::BackHopSpeed;StuckFor=0;AddMovementInput(HopDirection,1,true);
            const FVector Look=FVector(ToTarget.X,ToTarget.Y,0.f).GetSafeNormal();
            if(!Look.IsNearlyZero()) SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(),FRotator(0.f,Look.Rotation().Yaw,0.f),Delta,Kit.TurnRate));
            return;
        }
        if(HopUntil>0.f) {HopUntil=0.f;Move->StopMovementImmediately();S->Effect(11,Here-FVector(0,0,60),FVector::UpVector,FLinearColor(.55f,.55f,.6f),1.f);} // the landing: a puff of dust, and it stops dead
        if(LeapUntil>Now) { // a leap: fast, one contact hit
            Move->MaxWalkSpeed=1100;StuckFor=0;
            if(DistanceSquared<170*170&&Now>LastContact&&AS::ClearSight(GetWorld(),Here,Target->GetActorLocation())) {LastContact=Now+.5f;Target->Hurt(16,this);}
            return;
        }
        if(ChargeUntil>Now) { // a charge along the aim: one contact hit
            Move->MaxWalkSpeed=1100.f*((Boss()&&(Flags&ER::FlagPhaseTwo))?ER::BossChargeSpeedTwo:1.f);AddMovementInput(Aim,1,true);
            if(DistanceSquared<200*200&&Now>LastContact&&AS::ClearSight(GetWorld(),Here,Target->GetActorLocation())) {LastContact=Now+.5f;Target->Hurt(Kind==6?20:28,this);}
            return;
        }
        FinishAttackPhase(); // the committed motion is over
    }
    if(AIState==ER::StateRecover) {
        StuckFor=0;LastRoutePosition=Here;Move->StopMovementImmediately();
        if(Now<RecoverUntil) return;
        Ability=uint8(ER::AbilityNone);SetAIState(ER::StateApproach);
    }
    // ---- Moving: approach, strafe or retreat, and starting an attack. ----
    const float Range=Kit.AttackRange,Distance=FMath::Sqrt(DistanceSquared);
    const bool Sight=AS::ClearSight(GetWorld(),Here,Target->GetActorLocation());
    // Frail kinds fall back once when badly hurt (bounded, and never while only a couple of enemies remain).
    const bool Retreating=ER::UpdateRetreat(Kit,Health/FMath::Max(1.f,MaxHealth),S->Enemies.Num(),Now,Retreat);
    if(!Retreating&&ER::KitHas(Kit,ER::AbilityEnrage)&&ER::EnrageDue((Flags&ER::FlagEnraged)!=0,Health,MaxHealth)) {BeginAbility(ER::AbilityEnrage,ToTarget);return;} // at half health the brute roars (once)
    if(ER::KitHas(Kit,ER::AbilityBossPhase)&&ER::BossPhaseDue((Flags&ER::FlagPhaseTwo)!=0,Health,MaxHealth)) {BeginAbility(ER::AbilityBossPhase,ToTarget);return;} // and a ward's boss roars into its second phase (once)
    if(Now<RelocateUntil) { // the sniper's new vantage point after a shot: brisk and straight along the route, no attack until it arrives (or its few seconds are up)
        const FVector Goal(RelocateGoal.X,RelocateGoal.Y,Here.Z);
        if(FVector::Dist2D(Here,Goal)>ER::RelocateArrive) {
            const FVector Next=S->Arena?S->Arena->Route(Here,Goal):Goal;const FVector Dir=(Next-Here).GetSafeNormal2D();
            if(!Dir.IsNearlyZero()) {
                SetAIState(ER::StateApproach);
                SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(),FRotator(0.f,Dir.Rotation().Yaw,0.f),Delta,Kit.TurnRate));
                Move->MaxAcceleration=Kit.Accel;Move->BrakingDecelerationWalking=Kit.Brake;Move->MaxWalkSpeed=Kit.Speed*ER::RelocateSpeed*ASArsenal::EnemySpeedScale(false);
                AddMovementInput(Dir,1,true);return;
            }
        }
        RelocateUntil=0.f; // arrived, or no way there: back to the fight
    }
    if(!Retreating&&Sight&&Now>=NextAttack) {
        ER::Context C;C.Distance=Distance;C.Now=Now;C.Sight=true;C.ClearPath=true;C.PhaseTwo=(Flags&ER::FlagPhaseTwo)!=0;
        if((Kind==5||Kind==6||WardBossKit())&&Distance<Range&&S->Arena) C.ClearPath=S->Arena->CanWalkDirectly(Here,Target->GetActorLocation()); // leaps and charges need a clear straight walk
        const int32 Chosen=ER::ChooseAbility(Kit,C,AbilityReadyAt,LastAbility,FMath::FRand());
        if(Chosen!=ER::AbilityNone) {BeginAbility(Chosen,ToTarget);return;}
    }
    if(RootUntil>Now) {Move->StopMovementImmediately();StuckFor=0;LastRoutePosition=Here;return;}
    if(Now>=NextRoute||FVector::DistSquared2D(MoveGoal,Here)<65*65) {
        NextRoute=Now+.25f;
        bDirectClear=S->Arena&&S->Arena->CanWalkDirectly(Here,Target->GetActorLocation()); // pack spreading only fans out across open ground
        FVector Takeoff=FVector::ZeroVector,Landing=FVector::ZeroVector;const bool Jump=Kind==5&&S->Arena&&S->Arena->JumpPlan(Here,Target->GetActorLocation(),Takeoff,Landing);
        if(Jump) {
            MoveGoal=Takeoff;
            if(FVector::DistSquared2D(Here,Takeoff)<55*55&&!Move->IsFalling()) { // a leap across a gap: a committed Attack with no recovery afterwards
                const float Time=.95f;FVector V=(Landing-Here)/Time;V.Z=FMath::Abs(Move->GetGravityZ())*Time*.5f;
                Move->MaxWalkSpeed=1100;Move->AirControl=0;LaunchCharacter(V,true,true);LeapUntil=Now+Time;
                bTraversalLeap=true;Ability=uint8(ER::AbilityPounce);SetAIState(ER::StateAttack);return;
            }
        } else MoveGoal=S->Arena?S->Arena->Route(Here,Target->GetActorLocation()):Target->GetActorLocation();
    }
    // ---- Where to step (0.24): the kit decides how this kind moves. Shooters keep a distance band and side-step inside it, melee kinds circle just outside their reach
    // while an attack recharges, a hurt frail kind runs for a moment, and the pack spreads out across open ground (ASEnemyRules::PlanMovement, tested as pure rules).
    FVector RouteDir=(MoveGoal-Here).GetSafeNormal2D();
    const unsigned Id=GetUniqueID();
    ER::MovePlan Plan=ER::PlanMovement(Kit,Distance,ToTarget.X,ToTarget.Y,RouteDir.X,RouteDir.Y,Now>=NextAttack,Sight,Retreating,ER::StrafeSide(Id,Now),(bDirectClear&&Sight)?ER::FlankDegrees(Id,Distance):0.f);
    if(Plan.Mode==ER::ModeCircle||Plan.Mode==ER::ModeBackpedal||Plan.Mode==ER::ModeRetreat) {
        // A side-step, back-step or retreat is only worth taking if there is ground to take it on: probe a short step (at most every .12 s) and, if blocked, fan out to the
        // nearest clear direction. The chosen angle is kept between probes, so the circling direction stays smooth. Fully boxed in: close in along the route instead.
        if(Now>=NextClearProbe||ProbeMode!=Plan.Mode) {
            NextClearProbe=Now+.12f;ProbeMode=Plan.Mode;
            float Cx=Plan.X,Cy=Plan.Y;
            bProbeBlocked=S->Arena&&!ER::ChooseClearDirection(Plan.X,Plan.Y,[&](float Dx,float Dy) {return S->Arena->CanWalkDirectly(Here,Here+FVector(Dx,Dy,0.f)*130.f);},Cx,Cy);
            ProbeOffsetDegrees=bProbeBlocked?0.f:FMath::RadiansToDegrees(FMath::Atan2(Plan.X*Cy-Plan.Y*Cx,Plan.X*Cx+Plan.Y*Cy));
        }
        if(bProbeBlocked) {
            const FVector Fallback=RouteDir.IsNearlyZero()?FVector(ToTarget.X,ToTarget.Y,0.f).GetSafeNormal():RouteDir;
            Plan.Mode=ER::ModeAdvance;Plan.X=Fallback.X;Plan.Y=Fallback.Y;Plan.Speed=1.f;Plan.Face=false;
        } else if(!FMath::IsNearlyZero(ProbeOffsetDegrees)) {
            float Rx,Ry;ER::Detail::Rotate(Plan.X,Plan.Y,ProbeOffsetDegrees,Rx,Ry);Plan.X=Rx;Plan.Y=Ry;
        }
    }
    SetAIState(Plan.Mode==ER::ModeRetreat?ER::StateRetreat:((Plan.Mode==ER::ModeCircle||Plan.Mode==ER::ModeBackpedal)?ER::StateStrafe:ER::StateApproach));
    FVector Direction(Plan.X,Plan.Y,0.f);
    const bool Nudging=Now<NudgeUntil;
    if(Nudging) Direction=FVector(NudgeDirection.X,NudgeDirection.Y,0.f); // pinned a moment ago: a short sideways step out of it
    // Local separation steers around blocking bodies without pushing or contact damage.
    FVector Separate=FVector::ZeroVector;
    for(auto W:S->Enemies) if(auto* Other=W.Get()) if(Other!=this) {
        const FVector Away=Here-Other->GetActorLocation();const float D=Away.Size2D();
        const float Spacing=GetCapsuleComponent()->GetScaledCapsuleRadius()+Other->GetCapsuleComponent()->GetScaledCapsuleRadius()+35.f;
        if(D>1&&D<Spacing&&FMath::Abs(Away.Z)<150.f) Separate+=Away.GetSafeNormal2D()*(1.f-D/Spacing);
    }
    if(!Direction.IsNearlyZero()) {Separate-=Direction*FVector::DotProduct(Separate,Direction);Direction=(Direction+Separate*.8f).GetSafeNormal2D();}
    // Facing: toward the target while strafing, backing away or standing; along the motion otherwise, at the kit's turn rate.
    const FVector Look=(Plan.Face&&!Nudging)?FVector(ToTarget.X,ToTarget.Y,0.f).GetSafeNormal():Direction;
    if(!Look.IsNearlyZero()) SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(),FRotator(0.f,Look.Rotation().Yaw,0.f),Delta,Kit.TurnRate));
    Move->MaxAcceleration=Kit.Accel;Move->BrakingDecelerationWalking=Kit.Brake;
    Move->MaxWalkSpeed=Kit.Speed*ASArsenal::EnemySpeedScale(Elite()||Boss())*(S->bEndless?ASExpedition::AngerSpeed(S->EndlessWave):1.f)*ASMelee::ApproachScale(Distance,Range,Boss())
        *(SlowUntil>Now?.45f:(Elements.Frost>0?(Boss()?.75f:.65f):1.f))*((Flags&ER::FlagEnraged)?ER::EnrageSpeedScale:1.f)*(Plan.Mode==ER::ModeHold?1.f:FMath::Max(.05f,Plan.Speed));
    if(Plan.Mode!=ER::ModeHold) {
        AddMovementInput(Direction,1,true);
        if(Mutation==ER::MutationFire&&Now>=NextTrail) { // a burning elite leaves a fire pool every few seconds of walking
            NextTrail=Now+ER::TrailInterval;AASHazard::Spawn(GetWorld(),Here-FVector(0,0,GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-4.f),ER::HazardFire,this,0.f,0.f);
        }
        if(Now>=NextStuckCheck) {
            NextStuckCheck=Now+1.f;
            StuckFor=FVector::DistSquared2D(LastRoutePosition,Here)<25*25?StuckFor+1.f:0.f;
            LastRoutePosition=Here;
            if(StuckFor>=1.f&&DistanceSquared<600.f*600.f) { // pinned close to the target: step sideways for a moment before anything drastic
                float Nx,Ny;ER::UnstickDirection(Plan.X,Plan.Y,StuckFor,Id,Nx,Ny);
                NudgeDirection=FVector2D(Nx,Ny);NudgeUntil=Now+.8f;
            }
            if(StuckFor>=3.f&&S->Arena&&DistanceSquared>600.f*600.f) {
                // Recover only after sustained failure. Navigation nodes were collision-
                // checked for the largest enemy and connected to the player spawn.
                SetActorLocation(S->Arena->RecoverEnemy(Here,Target->GetActorLocation()),false,nullptr,ETeleportType::TeleportPhysics);
                Move->StopMovementImmediately();NextRoute=0;StuckFor=0;
            }
        }
    } else {StuckFor=0;LastRoutePosition=Here;}
}
void AASEnemy::CommitAttack() {
    auto* S=AS::State(this);if(!S||SpawnProtected()) return;++Pattern;AttackReleaseTime=S->SimTime;ForceNetUpdate();
    auto Shot=[&](const FVector& D,int32 Type,float Damage){auto* P=AASProjectile::Fire(GetWorld(),GetActorLocation()+D*75+FVector(0,0,15),D,nullptr,Type,Damage,true);if(P) P->EnemySource=this;};
    auto Blast=[&](const FVector& P,float R,float Damage,float Delay){auto* F=AASField::Spawn(GetWorld(),0,P,R,Damage,Delay,Delay+.4f);if(F) F->EnemySource=this;};
    if(Kind==5) {GetCharacterMovement()->MaxWalkSpeed=1100;LaunchCharacter(Aim*620+FVector(0,0,280),true,true);LeapUntil=S->SimTime+.65f;return;}
    if(Kind==6) {ChargeUntil=S->SimTime+.6f;Blast(GetActorLocation()-FVector(0,0,77),260,18,.2f);return;}
    if(Kind==7) {for(auto H:S->Heroes) if(H.IsValid()&&!H->bDowned) Blast(H->GetActorLocation()-FVector(0,0,77),220,18,.9f);return;}
    if(Kind==8) {Shot(Aim,8,24);return;}
    if(Kind==9) {
        // 0.24: summon in pairs while the living brood is under the cap and the cooldown is over; otherwise shoot. (It used to summon on its first two attacks only.)
        int32 Live=0;for(auto W:S->Enemies) if(const auto* M=W.Get()) if(M->Summoner.Get()==this&&M->Health>0) ++Live;
        for(auto W:S->Breakables) if(const auto* Egg=W.Get()) if(Egg->Kind==2&&Egg->Mother.Get()==this) ++Live; // an egg is a minion on its way
        if(ASEnemyRules::BroodShouldSummon(Live,S->PlayerArray.Num()>1?ASEnemyRules::BroodMinionCapCoop:ASEnemyRules::BroodMinionCap,S->SimTime,NextSummon)) {
            LayEggs(); // 0.24: eggs that hatch after a few seconds unless a hero pops them (minions straight away only when no egg can be laid)
            NextSummon=S->SimTime+ASEnemyRules::BroodSummonCooldown;
        } else Shot(Aim,2,13);
        for(auto W:S->Enemies) if(auto* E=W.Get()) if(E!=this&&!E->Boss()&&FVector::DistSquared2D(E->GetActorLocation(),GetActorLocation())<550*550&&AS::ClearSight(GetWorld(),GetActorLocation(),E->GetActorLocation())) {
            const float Before=E->Health;E->Health=FMath::Min(E->MaxHealth,E->Health+8);E->ForceNetUpdate();
            // 0.24: a green tether to each ally actually healed replaces the old 550 cm green ring (a circle on screen is reserved for damage zones)
            if(E->Health>Before) S->Effect(9,GetActorLocation()+FVector(0,0,60),(E->GetActorLocation()-GetActorLocation()).GetSafeNormal(),FLinearColor(.2f,1.f,.35f),FVector::Distance(GetActorLocation(),E->GetActorLocation()));
        }
        return;
    }
    if(Boss()&&S->bEndless&&S->EndlessWave>50&&Pattern%3!=2) {
        // Endless wave 50+: obviously lethal, telegraphed area attacks and homing shots. Dash to survive.
        if(Pattern%3==0) {
            for(auto H:S->Heroes) if(H.IsValid()&&!H->bDowned) {auto* F=AASField::Spawn(GetWorld(),0,H->GetActorLocation()-FVector(0,0,77),620.f,100000.f,1.9f,2.3f);if(F) F->EnemySource=this;}
        } else {
            AASCharacter* Target=nullptr;float Best=FLT_MAX;
            for(auto H:S->Heroes) if(H.IsValid()&&!H->bDowned) {const float D=FVector::DistSquared2D(GetActorLocation(),H->GetActorLocation());if(D<Best) {Best=D;Target=H.Get();}}
            if(Target) if(auto* P=AASProjectile::Fire(GetWorld(),GetActorLocation()+FVector(0,0,60),(Target->GetActorLocation()-GetActorLocation()).GetSafeNormal(),nullptr,2,100000.f,true)) {
                P->EnemySource=this;P->bHoming=true;P->HomingTarget=Target;P->Velocity=P->Velocity.GetSafeNormal()*560.f;
            }
        }
        return;
    }
    if(Boss()) {
        const bool PhaseTwo=Health<MaxHealth*.5f;
        if(Ward==1 && Pattern%2==0 && !bAirborne) {ChargeUntil=S->SimTime+.7f;return;} // a flying boss cannot charge along the ground
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
    } else if(Kind==1) {
        Shot(Aim,2,14); // 0.24: the old "Kind 2 fires three" loop lived here but this branch is Kind 1 only, so it never ran; Kind 2 gets its own duelist kit
    } else {
        // Melee swing (kinds 0, 2, 3). The same shape that was warned (ASEnemyRules::StrikeHits: a half disc of StrikeRadius in front) decides who is hit, and a red crescent
        // of that radius flashes on release. The colour's alpha 3 marks a hostile swing for the effect (no white wash).
        const float Radius=ASEnemyRules::StrikeRadius(Elite());
        S->Effect(17,GetActorLocation()-FVector(0,0,35),Aim,FLinearColor(1.f,.12f,.03f,3.f),Radius);
        for(auto H:S->Heroes) if(H.IsValid()) {
            const FVector To=H->GetActorLocation()-GetActorLocation();
            if(ASEnemyRules::StrikeHits(To.Size2D(),FVector::DotProduct(Aim,To.GetSafeNormal2D()),Radius)&&AS::ClearSight(GetWorld(),GetActorLocation(),H->GetActorLocation())) H->Hurt(ASEnemyRules::StrikeDamage(Elite()),this);
        }
    }
}
bool AASEnemy::Hit(float Damage,AASCharacter* Source,bool bDirect) {
    if(!HasAuthority()||SpawnProtected()||Health<=0||!FMath::IsFinite(Damage)||Damage<=0) return false;
    namespace ER=ASEnemyRules;
    if(Kind==ER::Bulwark) { // the shield guard: half damage from the front, unless a heavy blow from the front has ripped it open (a few seconds of full damage, and a stagger)
        const float Forward=IsValid(Source)?FVector::DotProduct(GetActorForwardVector(),(Source->GetActorLocation()-GetActorLocation()).GetSafeNormal2D()):0.f;
        const bool Open=(Flags&ER::FlagGuardBroken)!=0;
        if(bDirect&&ER::GuardBreaks(Damage,MaxHealth,Forward>ER::GuardFrontDot,Open)) {
            const float Now=AS::Time(this);Flags|=uint8(ER::FlagGuardBroken);GuardBrokenUntil=Now+ER::GuardBreakSeconds;
            StunUntil=FMath::Max(StunUntil,Now+ER::GuardBreakStagger);EndWindup(); // staggered: whatever it was winding up or charging with is lost
            if(auto* S=AS::State(this)) {
                S->Effect(5,GetActorLocation()+GetActorForwardVector()*50.f+FVector(0,0,40),FVector::UpVector,FLinearColor(1.f,.85f,.30f),110.f); // gold: a window for you
                S->Effect(11,GetActorLocation()+FVector(0,0,40),FVector::UpVector,FLinearColor(1.f,.85f,.30f),1.f);
            }
            ForceNetUpdate();
        } else Damage*=ER::GuardDamageScale(Open,Forward);
    }
    bool Blocked=false; // the duelist's parry: a direct blow from its front is taken at a fraction of its strength, the first of them is answered with a counter-slash
    if(Ability==ER::AbilityParry&&AIState==ER::StateSpecial) {
        const float Forward=IsValid(Source)?FVector::DotProduct(GetActorForwardVector(),(Source->GetActorLocation()-GetActorLocation()).GetSafeNormal2D()):std::numeric_limits<float>::quiet_NaN();
        if(ER::ParryBlocks(true,bDirect,Forward)) {
            Blocked=true;Damage=ER::ParryDamage(true,Damage);
            if(!bParryBlocked) {
                bParryBlocked=true;RiposteAt=AS::Time(this)+ER::ParryRiposteDelay;
                Flags^=uint8(ER::FlagParryToggle); // every machine plays the clang when this bit flips
                if(auto* S=AS::State(this)) S->Effect(5,GetActorLocation()+GetActorForwardVector()*55.f+FVector(0,0,45),FVector::UpVector,FLinearColor(1.f,.9f,.55f),70.f); // a gold spark where the blades met
            }
        }
    }
    Damage*=ASRules::FrostVulnerability(Elements.Frost,Elements.Frozen); // 0.24.2: a chilled body is brittle, a frozen one more so
    if(FirstEngagedAt<0) FirstEngagedAt=AS::Time(this);
    if(IsValid(Source)&&!bTrainingDummy) if(auto* Player=Source->GetPlayerState<AASPlayerState>()) Player->RunDamage=FMath::Min(1.e12f,Player->RunDamage+FMath::Min(Damage,Health));
    if(!Blocked) BeginHitReaction(IsValid(Source)?Source->GetActorLocation():GetActorLocation()-GetActorForwardVector(),Damage>=MaxHealth*.12f); // a blocked blow does not stagger it
    LastDamageTime=AS::Time(this);Health=FMath::Max(bTrainingDummy?1.f:0.f,Health-Damage);ForceNetUpdate();
    if(Health<=0) {
        if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->EnemyKilled(this,Source);
        // Death animation: the enemy leaves every target list and loses collision now, but stays visible while it topples (all machines
        // see Health<=0 and animate), then the server destroys it.
        if(auto* S=AS::State(this)) S->Enemies.Remove(this);
        EndWindup();GetCharacterMovement()->StopMovementImmediately();GetCharacterMovement()->DisableMovement();
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SetLifeSpan(ASPresentation::EnemyDeathSeconds);ForceNetUpdate();return true;
    }
    return false;
}

void AASEnemy::Control(int32 Type,const FVector& Direction,float Strength,int32 Generation,AASCharacter* Source) {
    if(!HasAuthority()||SpawnProtected()||bTrainingDummy||Health<=0) return;
    const float Now=AS::Time(this);
    if(Type==0&&Strength>=700.f) BeginHitReaction(GetActorLocation()-Direction,true);
    if(Boss()) return;
    if(Type==0) {
        const FVector Dir=Direction.GetSafeNormal2D();if(Dir.IsNearlyZero()||!FMath::IsFinite(Strength)||Strength<=0.f) return;
        const float Launch=Strength*(Elite()?ASPartC::EliteLaunchScale:1.f);
        auto* Move=GetCharacterMovement();
        if(Generation<0) {LaunchCharacter(Dir*Launch,true,false);return;} // a gravity pull drags along the floor as it always did: not a launch, no hop, no impacts
        // 0.24.2 (F1-F3): a launch is a hop and a skid (ASPartC::LaunchFor) with the AI suspended until the body comes to rest. A light knock (below ImpactMinStrength) does not
        // move an enemy that is committed to an attack (it would only be stopped dead again); a hard launch knocks it out of whatever it was doing.
        namespace ER=ASEnemyRules;
        const bool Hard=ASPartC::ImpactWorthy(Launch),Committed=AIState==ER::StateWindup||AIState==ER::StateSpecial||AIState==ER::StateAttack;
        if(!Hard&&Committed) return;
        if(Hard) EndWindup();
        const ASPartC::Flight F=ASPartC::LaunchFor(Launch);
        Flags|=uint8(ER::FlagLaunched); // every machine poses it thrown until it comes to rest
        if(Flies()) {Move->Velocity=Dir*F.Horizontal;KnockUntil=Now+FMath::Min(.5f,F.Seconds);ForceNetUpdate();return;} // fliers drift back through the air (TickFlier slows them, keeps them in the arena; no world collision, so no slams)
        LaunchCharacter(Dir*F.Horizontal+FVector(0.f,0.f,F.Vertical),true,true);
        Move->GroundFriction=ASPartC::SkidFriction;Move->BrakingDecelerationWalking=ASPartC::SkidBraking;bSkidFriction=true;
        KnockUntil=Now+F.Seconds;ForceNetUpdate();
        if(Hard) { // remember who launched it so a wall slam or a pile-up is credited and can chain
            bKnocked=true;KnockStart=Now;KnockStrength=Launch;KnockSpeed=F.Horizontal;KnockVelocity=Dir*F.Horizontal;
            KnockGeneration=FMath::Clamp(Generation,0,ASPartC::ImpactMaxGeneration);NextImpactAt=Now;
            if(Generation==0) KnockSource=IsValid(Source)?TWeakObjectPtr<AASCharacter>(Source):((LastHitter.IsValid()&&Now-LastHitTime<1.5f)?LastHitter:TWeakObjectPtr<AASCharacter>());
            UE_LOG(LogTemp,Display,TEXT("AshenKnock: launch kind %d strength %.0f generation %d: %.0f cm/s out, %.0f up, ~%.0f cm over %.2f s%s"),Kind,Launch,KnockGeneration,F.Horizontal,F.Vertical,F.Distance,F.Seconds,KnockSource.IsValid()?TEXT(""):TEXT(" (no hero to credit)"));
        }
    }
    if(Type==1&&Now>=StunImmuneUntil) {StunUntil=Now+.4f;StunImmuneUntil=Now+2.f;EndWindup();}
    if(Type==2&&Now>=RootImmuneUntil) {RootUntil=Now+.8f;RootImmuneUntil=Now+3.f;}
}

void AASEnemy::ImpactHurt(AASEnemy* Victim,float Strength,int32 Generation) {
    AASCharacter* Hero=KnockSource.Get();
    if(!HasAuthority()||!IsValid(Victim)||Victim->Health<=0||!IsValid(Hero)) return;
    const float Multiplier=Hero->Metamorphosed(ASPartC::Metamorphosis::Pinball)?ASPartC::PinballImpactMultiplier:1.f; // C-6 Pinball
    const float Damage=ASPartC::ImpactDamage(Strength,Generation,Multiplier)*Hero->DamageScale(false); // 0.24.2: impacts grow with the hero's damage like every other source
    if(Damage<=0.f) return;
    if(auto* Player=Hero->GetPlayerState<AASPlayerState>()) Player->RunImpacts=FMath::Min(Player->RunImpacts+1,100000);
    Hero->SecondaryDamage(Victim,Damage,EASHitKind::Physical);
}
void AASEnemy::TickKnock(float Now,float Delta) {
    auto* Move=GetCharacterMovement();
    if(bSkidFriction&&!Launched(Now)) {bSkidFriction=false;if(Move) Move->GroundFriction=BaseFriction;} // the skid is over: the walker's own friction (TickGround sets its braking every tick)
    if((Flags&ASEnemyRules::FlagLaunched)&&!Launched(Now)) {Flags&=uint8(0xFFu&~ASEnemyRules::FlagLaunched);ForceNetUpdate();}
    if(!bKnocked) return;
    auto* S=AS::State(this);
    if(!S||!Move||Health<=0||!Launched(Now)) {bKnocked=false;return;}
    const FVector V=GetVelocity(),Flat(V.X,V.Y,0.f),Previous=KnockVelocity;
    const float Speed=Flat.Size(),PreviousSpeed=Previous.Size();
    KnockVelocity=Flat;
    if(Now<NextImpactAt) return;
    const FVector Heading=PreviousSpeed>1.f?Previous/PreviousSpeed:Flat.GetSafeNormal();
    const float Strength=ASPartC::ContactStrength(KnockStrength,KnockSpeed,FMath::Max(Speed,PreviousSpeed));
    if(Heading.IsNearlyZero()||Strength<=0.f) {if(FMath::Max(Speed,PreviousSpeed)<ASPartC::ImpactMinSpeed) bKnocked=false;return;} // too slow to hurt anything any more
    const FVector Here=GetActorLocation();
    const float Radius=GetCapsuleComponent()->GetScaledCapsuleRadius(),Half=GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    // 1. Another enemy in the way (capsules block each other, so the contact is at about the sum of the radii; only bodies ahead of the flight count).
    const auto Enemies=S->Enemies;
    for(auto W:Enemies) if(auto* N=W.Get()) if(N!=this&&N->Health>0&&!N->bTrainingDummy&&!N->SpawnProtected()) {
        const FVector To=N->GetActorLocation()-Here;const float Reach=Radius+N->GetCapsuleComponent()->GetScaledCapsuleRadius()+ASPartC::ImpactContactSlack;
        if(To.SizeSquared2D()<Reach*Reach&&FMath::Abs(To.Z)<Half+N->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()&&FVector::DotProduct(To.GetSafeNormal2D(),Heading)>.2f
            &&ASPartC::Closing(FVector::DotProduct(FVector(V.X,V.Y,0.f)-FVector(N->GetVelocity().X,N->GetVelocity().Y,0.f),To.GetSafeNormal2D()))) {PileInto(N,Strength,Heading);return;} // bodies flung side by side do not collide
    }
    // 2. A wall ahead: sweep a slightly shrunk capsule (it cannot touch the floor) along the flight, one tick and a bit ahead. World geometry, cover proxies and props count.
    FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    const FCollisionQueryParams Params(SCENE_QUERY_STAT(ASKnockSweep),false,this);
    const float Ahead=FMath::Max(Speed,PreviousSpeed)*FMath::Clamp(Delta,0.f,.1f)*1.5f+14.f;
    FHitResult Hit;
    if(GetWorld()->SweepSingleByObjectType(Hit,Here,Here+Heading*Ahead,FQuat::Identity,Objects,FCollisionShape::MakeCapsule(Radius*.92f,Half*.6f),Params)&&Hit.bBlockingHit&&!Hit.bStartPenetrating
        &&FMath::Abs(Hit.ImpactNormal.Z)<.5f&&FVector::DotProduct(-Hit.ImpactNormal.GetSafeNormal2D(),Heading)>.3f) {WallSlam(Hit.ImpactNormal,Hit.ImpactPoint,Strength,Hit.GetActor());return;}
    // 3. Stopped short between two ticks (the engine resolved the contact before the sweep saw it): if a wall is right there along the old heading, that was a slam.
    if(PreviousSpeed>=ASPartC::ImpactMinSpeed&&Speed<PreviousSpeed*.4f) {
        FHitResult Near;
        if(GetWorld()->LineTraceSingleByObjectType(Near,Here,Here+Heading*(Radius+40.f),Objects,Params)&&FMath::Abs(Near.ImpactNormal.Z)<.5f) WallSlam(Near.ImpactNormal,Near.ImpactPoint,ASPartC::ContactStrength(KnockStrength,KnockSpeed,PreviousSpeed),Near.GetActor());
    }
}
void AASEnemy::WallSlam(const FVector& Normal,const FVector& At,float Strength,AActor* Struck) {
    auto* S=AS::State(this);const float Now=AS::Time(this);
    if(!HasAuthority()||!S||Health<=0||Strength<=0.f) return;
    NextImpactAt=Now+ASPartC::ImpactCooldown;bKnocked=false;
    const FVector Wall=FVector(Normal.X,Normal.Y,0.f).GetSafeNormal();
    // Dust and grit burst off the wall where the body struck, with the thud of a heavy blow.
    UE_LOG(LogTemp,Display,TEXT("AshenKnock: wall slam kind %d strength %.0f generation %d at %s against %s"),Kind,Strength,KnockGeneration,*At.ToCompactString(),Struck?*Struck->GetName():TEXT("?"));
    S->Effect(11,At+Wall*12.f,Wall,FLinearColor(.62f,.56f,.50f),1.7f);
    S->Effect(5,At+Wall*18.f,Wall,FLinearColor(1.f,.92f,.78f),110.f);
    if(auto* Prop=Cast<AASBreakable>(Struck)) if(AASCharacter* Hero=KnockSource.Get()) Prop->Strike(Hero,ASPartC::ImpactDamage(Strength,KnockGeneration)); // a launched body smashes crates and crystals too
    ImpactHurt(this,Strength,KnockGeneration);
    if(!IsValid(this)||Health<=0) return;
    // It rebounds off the wall a little, then is dazed for a moment (a window to punish it).
    const FVector V=KnockVelocity.IsNearlyZero()?GetVelocity():KnockVelocity;const FVector Flat(V.X,V.Y,0.f);
    const FVector Bounce=Wall.IsNearlyZero()?-Flat*ASPartC::SlamBounce:(Flat-2.f*FVector::DotProduct(Flat,Wall)*Wall)*ASPartC::SlamBounce;
    LaunchCharacter(Bounce+FVector(0.f,0.f,ASPartC::SlamBounceLift),true,true);
    KnockUntil=FMath::Max(KnockUntil,Now+2.f*ASPartC::SlamBounceLift/ASPartC::LaunchGravity+.15f);StunUntil=FMath::Max(StunUntil,Now+ASPartC::SlamDaze);
    BeginHitReaction(At,true);ForceNetUpdate();
}
void AASEnemy::PileInto(AASEnemy* Other,float Strength,const FVector& Heading) {
    auto* S=AS::State(this);const float Now=AS::Time(this);
    if(!HasAuthority()||!S||!IsValid(Other)) return;
    NextImpactAt=Now+ASPartC::ImpactCooldown;bKnocked=false;
    const FVector Between=(GetActorLocation()+Other->GetActorLocation())*.5f;
    UE_LOG(LogTemp,Display,TEXT("AshenKnock: pile-up kind %d into kind %d strength %.0f generation %d at %s"),Kind,Other->Kind,Strength,KnockGeneration,*Between.ToCompactString());
    S->Effect(11,Between,Heading,FLinearColor(.75f,.70f,.62f),1.2f);S->Effect(5,Between+FVector(0,0,40),FVector::UpVector,FLinearColor(1.f,.95f,.8f),100.f);
    // enemy into enemy: both are hurt (the one that was struck more than the projectile), the struck one is thrown on along the flight, and the projectile keeps little of its speed
    ImpactHurt(Other,Strength,KnockGeneration);ImpactHurt(this,Strength*.5f,KnockGeneration);
    if(IsValid(Other)&&Other->Health>0&&KnockGeneration<ASPartC::ImpactMaxGeneration) {Other->KnockSource=KnockSource;Other->Control(0,Heading,ASPartC::ThrownOnStrength(Strength),KnockGeneration+1);}
    if(IsValid(this)&&Health>0) if(auto* Move=GetCharacterMovement()) {const FVector V=Move->Velocity;Move->Velocity=FVector(V.X*ASPartC::PileCarry,V.Y*ASPartC::PileCarry,V.Z);KnockVelocity*=ASPartC::PileCarry;}
}
void AASEnemy::NotifyHit(UPrimitiveComponent* MyComp,AActor* Other,UPrimitiveComponent* OtherComp,bool bSelfMoved,FVector HitLocation,FVector HitNormal,FVector NormalImpulse,const FHitResult& Hit) {
    Super::NotifyHit(MyComp,Other,OtherComp,bSelfMoved,HitLocation,HitNormal,NormalImpulse,Hit);
    // A second opinion on the sweep in TickKnock: the engine reports a contact while a launched body moves. Side-on only (not the floor it lands on); heroes do not count.
    if(!HasAuthority()||!bKnocked||Health<=0||!IsValid(Other)||FMath::Abs(HitNormal.Z)>.5f) return;
    const float Now=AS::Time(this);if(Now<NextImpactAt) return;
    const FVector Heading=KnockVelocity.GetSafeNormal2D();if(Heading.IsNearlyZero()||FVector::DotProduct(-FVector(HitNormal.X,HitNormal.Y,0.f).GetSafeNormal(),Heading)<.3f) return;
    const float Strength=ASPartC::ContactStrength(KnockStrength,KnockSpeed,KnockVelocity.Size());if(Strength<=0.f) return;
    if(auto* Struck=Cast<AASEnemy>(Other)) {
        const FVector Toward=(Struck->GetActorLocation()-GetActorLocation()).GetSafeNormal2D();
        if(Struck->Health>0&&!Struck->bTrainingDummy&&!Struck->SpawnProtected()&&ASPartC::Closing(FVector::DotProduct(KnockVelocity-FVector(Struck->GetVelocity().X,Struck->GetVelocity().Y,0.f),Toward))) PileInto(Struck,Strength,Heading);
        return;
    }
    if(Other->IsA<ACharacter>()) return;
    WallSlam(HitNormal,HitLocation,Strength,Other);
}

bool AASEnemy::SpawnProtected() const {return Boss()&&AS::Time(this)<SpawnProtectedUntil;}
void AASEnemy::BeginHitReaction(const FVector& SourceLocation,bool Heavy) {
    if(!HasAuthority()||SpawnProtected()) return;
    HitReactionTime=AS::Time(this);HitReactionStrength=Heavy?1.f:.45f;
    HitReactionDirection=(GetActorLocation()-SourceLocation).GetSafeNormal2D();ForceNetUpdate();
}
void AASEnemy::RegisterCoopElement(EASHitKind Type,AASCharacter* Source,float Power) {
    if(!HasAuthority()||!IsValid(Source)||SpawnProtected()||Health<=0||bTrainingDummy) return;
    auto* State=AS::State(this);if(!State||State->Phase!=EASPhase::Combat) return;
    const int32 Element=int32(Type);if(Element!=2&&Element!=3&&Element!=4&&Element!=8) return;
    const float Now=State->SimTime;int32 Trigger=-1;const AASCharacter* Partner=nullptr;
    for(int32 I=0;I<9;++I) if(SynergySources[I].IsValid()&&SynergySources[I].Get()!=Source&&Now-SynergyTimes[I]<=3.f) {
        Trigger=ASExpedition::Synergy(I,Element);if(Trigger>=0) {Partner=SynergySources[I].Get();break;}
    }
    SynergySources[Element]=Source;SynergyTimes[Element]=Now;
    if(Trigger<0||Now<NextSynergy) return;
    NextSynergy=Now+3.f;for(int32 I=0;I<9;++I) {SynergySources[I].Reset();SynergyTimes[I]=-100.f;}
    const TCHAR* Names[]={TEXT("SYNERGY: Lightning Blast"),TEXT("SYNERGY: Stormbreak"),TEXT("SYNERGY: Wildfire")};
    const FLinearColor Colors[]={FLinearColor(.86f,.74f,1.f),FLinearColor(.12f,1,1),FLinearColor(1,.35f,.05f)}; // 0.24: Lightning Blast is violet-white (lightning), not pale blue; Stormbreak keeps frost's cyan
    const FVector At=GetActorLocation();State->Effect(8,At,FVector::UpVector,Colors[Trigger],470.f);
    for(auto P:State->PlayerArray) if(auto* Player=Cast<AASPlayerState>(P)) if(auto* NotifyController=Cast<AASPlayerController>(Player->GetOwner())) {NotifyController->Notify(Names[Trigger]);NotifyController->SynergyCue(Trigger);}
    const auto Targets=State->Enemies;
    for(auto W:Targets) if(auto* Target=W.Get()) if(!Target->SpawnProtected()&&Target->Health>0&&FVector::DistSquared2D(At,Target->GetActorLocation())<470.f*470.f&&AS::ClearSight(GetWorld(),At,Target->GetActorLocation())) {
        Target->BeginHitReaction(Source->GetActorLocation(),true);
        if(Trigger==1) Target->SlowUntil=FMath::Max(Target->SlowUntil,Now+2.f);
        const float SynergyAmount=ASExpedition::SynergyDamage(Trigger,Target->MaxHealth,Power,Target->Boss())*ASBuild::SynergyDamageMultiplier(Source->TeamworkRank(),Partner?Partner->TeamworkRank():0);const float SynergyBefore=Target->Health;
        const bool SynergyKilled=Target->Hit(SynergyAmount,Source);
        const float SynergyDealt=SynergyKilled?SynergyBefore:FMath::Max(0.f,SynergyBefore-Target->Health);
        if(SynergyDealt>0) for(auto P:State->PlayerArray) if(auto* Player=Cast<AASPlayerState>(P)) if(auto* RecipientPC=Cast<AASPlayerController>(Player->GetOwner())) RecipientPC->SynergyNumber(Target->GetActorLocation()+FVector(0,0,80),SynergyDealt,Trigger);
    }
}

void AASEnemy::ConfineFlight(float CapsuleHalfHeight) {
    // 0.24: fliers have no wall or floor collision, so they are held inside the arena by explicit limits (sides, floor, ceiling). Before this a flying boss could orbit
    // outside the walls whenever its target stood near an edge, and nothing ever rescued a flier that ended up below the map. Authority only: clients follow replication.
    auto* S=AS::State(this);if(!S||!HasAuthority()) return;
    const FVector Here=GetActorLocation();FVector P=Here;
    P=ConfineToMap(P,true);
    P.Z=ASEnemyRules::ClampFlightHeight(P.Z,0.f,CapsuleHalfHeight);
    if(P.Equals(Here,1.f)) return;
    SetActorLocation(P,false,nullptr,ETeleportType::None);
    if(auto* Move=GetCharacterMovement()) { // stop pushing against the limit: drop the part of the velocity that points back out
        const FVector Out=(Here-P).GetSafeNormal();
        if(!Out.IsNearlyZero()) {const float Into=FVector::DotProduct(Move->Velocity,Out);if(Into>0.f) Move->Velocity-=Out*Into;}
    }
}
void AASEnemy::Molt() {
    // The flying boss's half-health scream has finished: it bursts out hatchlings (small Gloomwings) in a ring around its wings. Each one appears with its own shard burst
    // and the boss with a larger one, so the spawn is visible and loud. Spawning respects the enemy cap (SpawnMinion refuses past it).
    auto* S=AS::State(this);auto* GM=GetWorld()?GetWorld()->GetAuthGameMode<AASGameMode>():nullptr;if(!HasAuthority()||!S||!GM) return;
    const FVector Here=GetActorLocation();
    S->Effect(11,Here,FVector::UpVector,FLinearColor(.8f,.25f,.9f),3.f); // boss-sized burst (tier 3)
    const int32 Count=ASEnemyRules::MoltHatchlings(S->PlayerArray.Num());
    for(int32 I=0;I<Count;++I) {
        const float Angle=UE_TWO_PI*float(I)/float(Count)+float(GetUniqueID()%7)*.3f;
        FVector At=Here+FVector(FMath::Cos(Angle),FMath::Sin(Angle),0.f)*(Boss()?330.f:200.f)+FVector(0,0,FMath::Sin(Angle*2.f)*40.f);
        At=ConfineToMap(At,true);
        At.Z=ASEnemyRules::ClampFlightHeight(At.Z,0.f,83.f);
        if(GM->SpawnMinion(At,this,ASExpedition::FlierKind,ASEnemyRules::HatchlingHealthScale,true)) S->Effect(11,At,FVector::UpVector,FLinearColor(.7f,.2f,.9f),2.f); // elite-sized burst
    }
}
void AASEnemy::TickFlier(float Delta,AASCharacter* Target,float DistanceSquared) {
    auto* S=AS::State(this);auto* Move=GetCharacterMovement();if(!S||!Move||!Target) return;
    if(Move->MovementMode!=MOVE_Flying) Move->SetMovementMode(MOVE_Flying);
    const bool Big=Boss(); // the flying boss: same loop, bigger numbers
    const float Hover=Big?ASExpedition::FlyingBossHover:ASExpedition::FlierHover;
    const float OrbitMax=Big?ASExpedition::FlyingBossOrbitRadius:ASExpedition::FlierOrbitRadius,OrbitMin=Big?ASExpedition::FlyingBossOrbitMin:ASExpedition::FlierOrbitMin,OrbitCycle=Big?ASExpedition::FlyingBossOrbitCycle:ASExpedition::FlierOrbitCycle;
    const float DiveSpeed=Big?ASExpedition::FlyingBossDiveSpeed:ASExpedition::FlierDiveSpeed,DiveSeconds=Big?ASExpedition::FlyingBossDiveSeconds:ASExpedition::FlierDiveSeconds;
    const float Windup=Big?ASExpedition::FlyingBossWindup:ASExpedition::FlierWindup,ContactRange=Big?ASExpedition::FlyingBossContactRange:170.f;
    const float ContactDamage=Big?ASExpedition::FlyingBossContactDamage:ASExpedition::FlierContactDamage;
    const float Half=GetCapsuleComponent()?GetCapsuleComponent()->GetScaledCapsuleHalfHeight():83.f;
    ConfineFlight(Half); // first thing every tick: whatever happened last frame, the flier is back inside the arena before it decides anything
    const float Now=S->SimTime;const FVector Here=GetActorLocation(),There=Target->GetActorLocation();
    if(Launched(Now)) {Move->Velocity*=FMath::Exp(-4.f*Delta);return;} // 0.24.2: knocked back through the air, slowing; it steers again once the knock is over
    const float FloorZ=There.Z-88.f; // the target's feet: fliers hover relative to the fight, not to the terrain under them
    const float LowestZ=ASEnemyRules::ClampFlightHeight(-1.e9f,0.f,Half); // the lowest the flier's centre may ever be: its capsule stays off the floor
    Move->MaxFlySpeed=(Big?ASExpedition::FlyingBossFlySpeed:330.f)*ASArsenal::EnemySpeedScale(Big)*(S->bEndless?ASExpedition::AngerSpeed(S->EndlessWave):1.f);
    if(DiveUntil>Now) {
        // Dive: fast, straight through where the player was; one contact hit. The Gloomwing dives down onto the player; the big boss flies it LEVEL at its hover height (its body is
        // far too tall to dive onto the floor), so a pass is a low sweep the player can duck, dash or hit. It ends on arrival, on timeout, or near the floor.
        Move->Velocity=DiveDirection*DiveSpeed;
        if(FVector::Dist(Here,There)<ContactRange&&Now>LastContact&&Here.Z>FloorZ-60.f) {LastContact=Now+.6f;Target->Hurt(ContactDamage,this);}
        const bool Arrived=Big?FVector::Dist2D(Here,DiveTarget)<80.f:FVector::Dist(Here,DiveTarget)<80.f;
        if(Arrived||(!Big&&Here.Z<=FMath::Max(LowestZ,FloorZ+70.f))) {DiveUntil=0.f;Ability=uint8(ASEnemyRules::AbilityNone);SetAIState(ASEnemyRules::StateApproach);}
        return;
    }
    if(bWinding) {
        Move->StopMovementImmediately();SetActorRotation(FRotator(0.f,(There-Here).Rotation().Yaw,0.f));
        if(Now>=WindupEnd) {
            const uint8 Done=Ability;Ability=uint8(ASEnemyRules::AbilityNone);Flags&=uint8(0xFFu&~ASEnemyRules::FlagPackCall);SetAIState(ASEnemyRules::StateApproach);
            if(Done==ASEnemyRules::AbilityMolt) {Molt();NextAttack=Now+2.5f;ForceNetUpdate();return;}
            if(Big&&(++DiveCycle%2)==0) { // every second attack of the flying boss is its normal boss attack (blast fields or a shot ring), not a dive
                CommitAttack();NextAttack=Now+3.f;ForceNetUpdate();return;
            }
            DiveTarget=There;
            FVector Toward=Big?FVector(There.X-Here.X,There.Y-Here.Y,0.f):((There-FVector(0,0,20.f))-Here);
            DiveDirection=Toward.GetSafeNormal();if(DiveDirection.IsNearlyZero()) DiveDirection=GetActorForwardVector();
            DiveUntil=Now+DiveSeconds;NextAttack=Now+(Big?3.f:ASExpedition::FlierAttackDelay(FMath::FRand()));Ability=uint8(ASEnemyRules::AbilityDive);SetAIState(ASEnemyRules::StateAttack);ForceNetUpdate();
        }
        return;
    }
    if(Big&&ASEnemyRules::MoltDue(bMolted,Health,MaxHealth)) { // at half health the boss rears back and screams before it releases its hatchlings (once per fight)
        bMolted=true;Ability=uint8(ASEnemyRules::AbilityMolt);Aim=(There-Here).GetSafeNormal2D();
        WindupStart=Now;WindupEnd=Now+ASEnemyRules::MoltWindupSeconds;SetAIState(ASEnemyRules::StateWindup);ForceNetUpdate();return;
    }
    if(Now>=NextAttack&&DistanceSquared<FMath::Square(1100.f+(Big?400.f:0.f))) {
        Aim=(There-Here).GetSafeNormal2D();Ability=uint8(ASEnemyRules::AbilityDive);WindupStart=Now;WindupEnd=Now+Windup;
        if(!Big) {if(!bPackFollower) CallPack(There);bPackFollower=false;} // the first of a pack to dive calls the others, who then follow one after another
        SetAIState(ASEnemyRules::StateWindup);ForceNetUpdate();return;
    }
    // Circle the target at a breathing radius and the hover height, with a slow bob. The radius swings between OrbitMin (a sweep inside melee reach) and OrbitMax.
    if(OrbitAngle==0.f) {OrbitSign=(GetUniqueID()&1)?1.f:-1.f;OrbitAngle=float(GetUniqueID()%628)/100.f+.01f;}
    OrbitAngle+=Delta*(Big?.5f:.9f)*OrbitSign;
    const float Radius=ASEnemyRules::OrbitRadius(Now+float(GetUniqueID()%7),OrbitMin,OrbitMax,OrbitCycle);
    FVector Orbit=There+FVector(FMath::Cos(OrbitAngle),FMath::Sin(OrbitAngle),0.f)*Radius;
    Orbit=ConfineToMap(Orbit,true); // never steer to a point outside the map
    const float Bob=FMath::Sin(Now*2.3f+float(GetUniqueID()%7))*18.f;
    const FVector Delta3=FVector(Orbit.X,Orbit.Y,ASEnemyRules::ClampFlightHeight(FloorZ+Hover+Bob,0.f,Half))-Here;
    AddMovementInput(Delta3.GetSafeNormal(),FMath::Clamp(Delta3.Size()/200.f,.2f,1.f),true);
    SetActorRotation(FRotator(0.f,(There-Here).Rotation().Yaw,0.f));
}

// ---- Debug: `as.ringaudit` in the in-game console ----
// Lists every visible ring, arc, sector and flat disc mesh in the world with its owner, place and colour. Run it the moment a stray circle is on screen and send the log lines
// (search for "AshenRingAudit"): the owner and mesh name identify exactly what draws it. Read-only; changes nothing.
static void ASRunRingAudit(UWorld* World) {
    if(!World) return;
    int32 Count=0;
    for(TActorIterator<AActor> It(World);It;++It) {
        TArray<UStaticMeshComponent*> Parts;It->GetComponents<UStaticMeshComponent>(Parts);
        for(const UStaticMeshComponent* Part:Parts) {
            if(!Part||!Part->IsVisible()||!Part->GetStaticMesh()) continue;
            const FString MeshName=Part->GetStaticMesh()->GetName();const FVector Scale=Part->GetComponentScale();
            const bool Flat=MeshName==TEXT("SM_Ring")||MeshName==TEXT("SM_StrikeSector")||MeshName==TEXT("SM_SwingArc")||MeshName==TEXT("SM_Slash")||(MeshName==TEXT("Cylinder")&&Scale.Z<.1f);
            if(!Flat) continue;
            FLinearColor Tint(0,0,0,0);if(auto* M=Cast<UMaterialInstanceDynamic>(Part->GetMaterial(0))) Tint=M->K2_GetVectorParameterValue(TEXT("Tint"));
            const FVector P=Part->GetComponentLocation();
            UE_LOG(LogTemp,Display,TEXT("AshenRingAudit: %s (%s) / %s / %s at (%.0f,%.0f,%.0f) scale %.2f tint (%.2f,%.2f,%.2f)"),*It->GetClass()->GetName(),*It->GetName(),*Part->GetName(),*MeshName,P.X,P.Y,P.Z,Scale.X,Tint.R,Tint.G,Tint.B);
            ++Count;
        }
    }
    UE_LOG(LogTemp,Display,TEXT("AshenRingAudit: %d flat circle/arc meshes visible"),Count);
}
static FAutoConsoleCommandWithWorld ASRingAuditCommand(TEXT("as.ringaudit"),TEXT("Lists every visible ring, arc, sector and flat disc mesh (owner, place, colour) to the log so a stray circle can be traced."),FConsoleCommandWithWorldDelegate::CreateStatic(&ASRunRingAudit));
