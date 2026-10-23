// 0.24.2 Phase F: the hero as the Unreal Mannequin (Josue after the 0.24.1 playtest: "use base UE base look for our character but remap things to flow smoothly").
// P1 is Manny and P2 is Quinn (the template's two bodies, so the players tell each other apart at a glance). ABP_AshenHero (built by the AshenSpiralTools commandlet from the
// template's ABP_Unarmed) does the locomotion - strafing while the hero faces its aim, jumps, falls, landings, foot IK - and the moves are clips played on its slots: the weapon's
// attacks on the upper body (the legs keep running), dashes, the maul's leap, heavy blows and the downed fall on the whole body. Every clip is chosen on every machine from state that
// was already replicated (the attack counter and combo step, the dash, the leap state, the downed flag, the hit and cast multicasts), so nothing new crosses the network.
// The 21-part armour rig stays as the fallback: as.hero.skeletal 0, or a machine without the Mannequin content (Tools/import_mannequin.ps1) or the hero clips
// (Tools/build_hero_animation.py), plays exactly as before.
#include "ASCharacter.h"
#include "ASMovementComponent.h"
#include "ASGameState.h"
#include "ASMeleeRules.h"
#include "ASHammerRules.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AnimationRuntime.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarASSkeletalHero(TEXT("as.hero.skeletal"),1,TEXT("1: heroes are the Unreal Mannequin (0.24.2 Phase F) when its assets are present on this machine; 0: the 21-part armour rig. Applied at the next look refresh."),ECVF_Default);
#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<FString> CVarASStanceClip(TEXT("as.hero.stance"),TEXT(""),TEXT("Dev: the clip every hero loops as its stance instead of its weapon's own (a name under /Game/Characters/Hero/Anims, or a full asset path); empty: the weapon's own. For judging stances by screenshot."),ECVF_Default);
#endif

namespace ASHeroLook {
const FName UpperBody(TEXT("UpperBody")),FullBody(TEXT("DefaultSlot"));
constexpr float PaintStrength=.42f; // the player's colour as the Mannequin's "Paint Tint" (M_Mannequin): a deep coat of paint, not a glow (.6 read as bright plastic in-engine)
UAnimSequenceBase* Clip(const TCHAR* Name) {
    static TMap<FString,TWeakObjectPtr<UAnimSequenceBase>> Cache;
    if(const TWeakObjectPtr<UAnimSequenceBase>* Hit=Cache.Find(Name)) if(Hit->IsValid()) return Hit->Get();
    const FString Path=FString(Name).StartsWith(TEXT("/"))?FString(Name):FString::Printf(TEXT("/Game/Characters/Hero/Anims/%s.%s"),Name,Name);
    UAnimSequenceBase* Found=LoadObject<UAnimSequenceBase>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet);
    Cache.Add(Name,Found);return Found;
}
// What each weapon's attack plays, by combo step (melee) or in turn (shots and casts), and on which slot. TUNABLE: swap a clip by name (Tools/build_hero_animation.py imports them).
struct Attack {const TCHAR* Clip[3];bool FullBody[3];};
const Attack& AttackFor(int32 Weapon) {
    static const Attack Table[ASRules::WeaponCount]={
        {{TEXT("Aurora_Primary_Attack_Fast_A"),TEXT("Aurora_Primary_Attack_Fast_B"),TEXT("Aurora_Primary_Attack_Fast_C")},{false,false,true}},   // Cinder Blade (the third lunges)
        {{TEXT("Grux_PrimaryAttack_LA"),TEXT("Grux_PrimaryAttack_RA"),TEXT("terra_Primary_Melee_C_slow")},{false,false,true}},                 // Hoarfrost Maul (across, overhead, the planted overhead smash)
        {{TEXT("Sparrow_Primary_Fire_Fast"),TEXT("Sparrow_Primary_Fire_Fast"),TEXT("Sparrow_Primary_Fire_Med")},{false,false,false}},         // Ash Bow
        {{TEXT("gideon_Primary_Attack_A_Medium"),TEXT("gideon_Primary_Attack_B_Medium"),TEXT("gideon_Primary_Attack_C_Medium")},{false,false,false}}, // Spiral Glaive
        {{TEXT("gideon_Primary_Attack_B_Medium"),TEXT("gideon_Primary_Attack_C_Medium"),TEXT("gideon_Primary_Attack_A_Medium")},{false,false,false}}, // Ember Staff
        {{TEXT("Kwang_PrimaryAttack_A_Slow"),TEXT("Kwang_PrimaryAttack_B_Slow"),TEXT("Kwang_PrimaryAttack_C_Slow")},{false,false,false}},      // Rift Scythe
        {{TEXT("Murdock_Fire_Fast"),TEXT("Murdock_Fire_Fast"),TEXT("Murdock_Fire_Fast")},{false,false,false}},                                 // Cinder Scattergun
        {{TEXT("TwinBlast_DoubleShot_Fire_Rt_Fast"),TEXT("TwinBlast_DoubleShot_Fire_Rt_Fast"),TEXT("TwinBlast_DoubleShot_Fire_Rt_Fast")},{false,false,false}}, // Tempest Rotary
        {{TEXT("Wraith_Ability_Q_Fire"),TEXT("Wraith_Ability_Q_Fire"),TEXT("Wraith_Ability_Q_Fire")},{false,false,false}},                     // Rift Mortar
        {{TEXT("Khaimera_Melee_A_Fast"),TEXT("Khaimera_Melee_B_Fast"),TEXT("Khaimera_Melee_A_Fast")},{false,false,false}},                       // Twin Claws (two steps; Khaimera's fast claw chain)
    };
    return Table[FMath::Clamp(Weapon,0,ASRules::WeaponCount-1)];
}
// The part of a step's clip that plays (seconds of the clip; To 0 = to its end). The claws cut Khaimera's chain before it bends the wielder double (look_claw_poses.txt: A and B are
// hunched from about .28 s on; the strike is the sweep at .07-.14 s, which the stretch lands on the swing's impact).
void AttackSpan(int32 Weapon,int32 Step,float& From,float& To) {
    From=To=0.f;
    if(Weapon==ASRules::Claws) {if(Step==0) {From=0.f;To=.30f;} else {From=.02f;To=.32f;}}
}
// The stance each weapon is carried in between moves, looped on the upper body (the legs keep their own walk, run and jump). TUNABLE: swap a clip by name.
const TCHAR* StanceFor(int32 Weapon) {
    // 0.26 G2 (the 0.24.2 playtest: the ready stance "reads as a hunchback - cowering, exhausted"): the stance also plays on the arms and torso while running, so it is how the hero looks most of the
    // time. Chosen from contact sheets of every candidate clip on every weapon (Tools/DevScripts/look_stances.txt, look_stances2.txt): the blade's Kwang_Idle and the maul's Grux_TravelMode_Idle
    // were bent over at the waist with the head down; TwinBlast_Idle is an upright guard with the sword forward, and Steel's combat idle an upright two-handed carry, the hammer (or scythe)
    // shouldered with both hands on the shaft. The others were already upright and stay.
    static const TCHAR* Table[ASRules::WeaponCount]={TEXT("TwinBlast_Idle"),TEXT("steel_Steel_Idle_Combat"),TEXT("Sparrow_idle"),TEXT("Yin_Idle_Combat"),TEXT("Yin_Idle_Combat"),TEXT("steel_Steel_Idle_Combat"),
        TEXT("Murdock_Idle"),TEXT("Murdock_Idle"),TEXT("Wraith_Idle_Combat"),TEXT("Kallari_Idle_Combat")}; // the claws: Kallari's stalking guard, the most upright of the claw-bearing idles tried (look_claw_stances.txt)
    return Table[FMath::Clamp(Weapon,0,ASRules::WeaponCount-1)];
}
FString StanceName(int32 Weapon) {
#if !UE_BUILD_SHIPPING
    const FString Override=CVarASStanceClip.GetValueOnGameThread();if(!Override.IsEmpty()) return Override;
#endif
    return StanceFor(Weapon);
}
// Where a weapon sits in the fist, in the hand bone's own frame, worked out from the mesh's reference skeleton, so it fits Manny and Quinn alike without relying on the bones'
// axis conventions (the template meshes have no weapon bones of their own): the handle runs through the fist from the little finger to the index finger, a blade's edges (a
// maul's faces) line up with the knuckles, a gun's barrel and a bow's belly point the way the knuckles do, and the handle sits in the curled fingers just inside the knuckle line.
constexpr float GripDepth=3.2f,GripBack=1.f; // cm from the knuckle line toward the palm, and back toward the wrist. TUNABLE, checked by screenshot.
bool GripInHand(const USkeletalMeshComponent* Body,bool Left,int32 Weapon,FName& Bone,FTransform& Out) {
    const USkeletalMesh* Model=Body?Body->GetSkeletalMeshAsset():nullptr;if(!Model) return false;
    const FReferenceSkeleton& Ref=Model->GetRefSkeleton();const TCHAR* Side=Left?TEXT("l"):TEXT("r");
    auto Find=[&](const TCHAR* Name) {return Ref.FindBoneIndex(FName(*FString::Printf(TEXT("%s_%s"),Name,Side)));};
    const int32 Hand=Find(TEXT("hand")),First=Find(TEXT("index_01")),Little=Find(TEXT("pinky_01")),Middle=Find(TEXT("middle_01")),Thumb=Find(TEXT("thumb_01"));
    if(Hand==INDEX_NONE||First==INDEX_NONE||Little==INDEX_NONE||Middle==INDEX_NONE||Thumb==INDEX_NONE) return false;
    auto At=[&](int32 B) {return FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref,B);};
    const FTransform Wrist=At(Hand);
    const FVector W=Wrist.GetLocation(),I=At(First).GetLocation(),P=At(Little).GetLocation(),M=At(Middle).GetLocation(),T=At(Thumb).GetLocation();
    const FVector Along=(I-P).GetSafeNormal();                                                            // through the fist, out past the thumb
    FVector Knuckles=M-W;Knuckles=(Knuckles-Along*FVector::DotProduct(Knuckles,Along)).GetSafeNormal();    // from the wrist to the knuckles
    if(Along.IsNearlyZero()||Knuckles.IsNearlyZero()) return false;
    FVector Palm=FVector::CrossProduct(Along,Knuckles);if(FVector::DotProduct(Palm,T-W)<0.f) Palm=-Palm;  // the thumb's root sits on the palm side
    const FVector Centre=(I+P)*.5f+Palm*GripDepth-Knuckles*GripBack;
    // the meshes (Tools/generate_weapon_art.py) hold the handle on +Z at the origin; a blade's edges and a maul's faces lie on Y; a gun's barrel and a bow's belly on +X
    const FMatrix Axes=Weapon==2||ASRules::IsGun(Weapon)||Weapon==ASRules::Claws?FRotationMatrix::MakeFromZX(Along,Knuckles):FRotationMatrix::MakeFromZY(Along,Knuckles); // the claws' blades, like a barrel, point the way the knuckles do
    Bone=Ref.GetBoneName(Hand);Out=FTransform(Axes.ToQuat(),Centre).GetRelativeTransform(Wrist);
    return true;
}
// 0.26 G4 (the 0.24.2 playtest: "hammer held with TWO hands, both on the hilt the whole time, the hands repositioning through the swing; hammer model BIGGER"). TwoHandedGrip says whether the left
// hand is held on the weapon and where: a point on its handle in the weapon's own units (the meshes hold the handle on +Z with the right fist at the origin; the maul's grip runs -13..13 and its
// shaft on to 97, so the left hand goes above the right, a good lever's length away). The ABP's two-bone IK (ASBuildHeroAnimCommandlet.cpp) takes the left hand there from the animated right
// hand each frame, so it stays on the hilt through every swing, leap and stance. WeaponScale: how much bigger a weapon is drawn than its mesh (the maul 1.35). TUNABLE.
bool TwoHandedGrip(int32 Weapon,FVector& OnHandle) {if(Weapon==1) {OnHandle=FVector(0.f,0.f,24.f);return true;}return false;}
float WeaponScale(int32 Weapon) {return Weapon==1?1.35f:1.f;}
}

void AASCharacter::SetupSkeletalHero() {
    USkeletalMeshComponent* Body=GetMesh();if(!Body) return;
    const bool Want=CVarASSkeletalHero.GetValueOnGameThread()!=0&&GetNetMode()!=NM_DedicatedServer;
    USkeletalMesh* Model=Want?LoadObject<USkeletalMesh>(nullptr,ColorIndex==1?TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"):TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"),nullptr,LOAD_NoWarn|LOAD_Quiet):nullptr;
    UClass* Anim=Want?LoadObject<UClass>(nullptr,TEXT("/Game/Characters/Hero/ABP_AshenHero.ABP_AshenHero_C"),nullptr,LOAD_NoWarn|LOAD_Quiet):nullptr;
    const bool Was=bSkeletalHero;bSkeletalHero=Model&&Anim;
    if(!bSkeletalHero) {
        Body->SetHiddenInGame(true);
        if(Was) {for(auto& Part:Parts) if(Part) Part->SetVisibility(true);} // back to the rig (RefreshLook has just restyled it)
        return;
    }
    if(Body->GetSkeletalMeshAsset()!=Model) {Body->SetSkeletalMesh(Model);Body->EmptyOverrideMaterials();} // the other body's paint does not carry over
    if(Body->GetAnimClass()!=Anim) Body->SetAnimInstanceClass(Anim);
    // the player's colour (Settings > Character color; the default is the player's own) is the Mannequin's paint, as it was the rig's visor and core; dark red while downed. TUNABLE.
    const FLinearColor Paint=bDowned?FLinearColor(.3f,.02f,.025f):OwnColor()*ASHeroLook::PaintStrength;
    for(int32 Slot=0;Slot<Body->GetNumMaterials();++Slot) {
        UMaterialInstanceDynamic* Coat=Cast<UMaterialInstanceDynamic>(Body->GetMaterial(Slot));
        if(!Coat) Coat=Body->CreateDynamicMaterialInstance(Slot);
        if(Coat) Coat->SetVectorParameterValue(TEXT("Paint Tint"),Paint);
    }
    if(SkelStanceWeapon!=Build.Weapon) { // a new weapon: its own stance takes over at the next frame
        if(UAnimInstance* Current=Body->GetAnimInstance()) if(SkelStance.IsValid()) Current->Montage_Stop(.2f,SkelStance.Get());
        SkelStance.Reset();SkelStanceWeapon=-1;
    }
    Body->SetHiddenInGame(false);Body->SetVisibility(true);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    const FVector Stand(0.f,0.f,-GetCapsuleComponent()->GetScaledCapsuleHalfHeight());const FRotator Face(0.f,-90.f,0.f); // the Mannequin faces +Y: turned to face the actor's +X
    Body->SetRelativeLocationAndRotation(Stand,Face);
    // 0.26 G1: the engine rebuilds a proxy's mesh transform every frame from the offset it cached when the actor was created (CharacterMovementComponent::SmoothClientPosition_UpdateVisuals:
    // smoothing offset + GetBaseTranslationOffset). That was the empty rig's (0,0,0), so the OTHER player's hero was drawn 88 cm too high and turned 90 degrees on every machine (a listen
    // host's view of its guest included) while one's own hero, which is never smoothed, stood right. The Titan's setup does the same (ASEnemySkeletal.cpp).
    CacheInitialMeshOffset(Stand,Face);
    Body->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones; // the weapon and the dash afterimages read its bones every frame
    for(auto& Part:Parts) if(Part) Part->SetVisibility(false);
    // the weapon in the fist (the bow in the left), the spiral charge in the left hand; the part rig's bow string and nocked arrow are not drawn on the Mannequin
    FName Hand;FTransform Grip;
    if(ASHeroLook::GripInHand(Body,Build.Weapon==2,Build.Weapon,Hand,Grip)) {
        Grip.SetScale3D(FVector(ASHeroLook::WeaponScale(Build.Weapon))); // about the fist (the mesh's origin): the hammer grows both ways along its handle
        WeaponMesh->AttachToComponent(Body,FAttachmentTransformRules::SnapToTargetNotIncludingScale,Hand);WeaponMesh->SetRelativeTransform(Grip);WeaponMesh->SetVisibility(true);
    } else WeaponMesh->SetVisibility(false); // a mesh without hand bones: better no weapon than one at the feet
    // Twin Claws: the second claw in the left fist (the same mesh: the hand frame makes it symmetric)
    FName OffHand;FTransform OffGrip;
    if(Build.Weapon==ASRules::Claws&&ASHeroLook::GripInHand(Body,true,Build.Weapon,OffHand,OffGrip)) {
        OffGrip.SetScale3D(FVector(ASHeroLook::WeaponScale(Build.Weapon)));
        OffhandMesh->AttachToComponent(Body,FAttachmentTransformRules::SnapToTargetNotIncludingScale,OffHand);OffhandMesh->SetRelativeTransform(OffGrip);OffhandMesh->SetVisibility(true);
    } else OffhandMesh->SetVisibility(false);
    ChargeMesh->AttachToComponent(Body,FAttachmentTransformRules::SnapToTargetNotIncludingScale,FName(TEXT("hand_l")));
    ArrowMesh->SetVisibility(false);for(auto& String:BowStrings) if(String) String->SetVisibility(false);
    // no cape: the base Unreal look Josue asked for (the part rig keeps its cloth)
    for(auto& Piece:FlowingCape) if(Piece) Piece->SetVisibility(false);
}

bool AASCharacter::SkeletalMovePlaying() const {
    const UAnimInstance* Anim=GetMesh()?GetMesh()->GetAnimInstance():nullptr;if(!Anim) return false;
    const UAnimMontage* Active=Anim->GetCurrentActiveMontage();
    return Active&&Active!=SkelStance.Get();
}

// Seconds: how long the whole clip is stretched over (0: its own length); the part from StartAt to StopAt (clip seconds; StopAt<=0: the end) is what plays, over Seconds*(that part / the clip).
UAnimMontage* AASCharacter::PlayHeroClip(const TCHAR* Name,FName Slot,float Seconds,float BlendIn,float BlendOut,float StartAt,float StopAt) {
    if(!bSkeletalHero||GetNetMode()==NM_DedicatedServer) return nullptr;
    UAnimInstance* Anim=GetMesh()?GetMesh()->GetAnimInstance():nullptr;UAnimSequenceBase* Clip=ASHeroLook::Clip(Name);
    if(!Anim||!Clip) return nullptr;
    const float Length=FMath::Max(.05f,Clip->GetPlayLength());
    const float From=FMath::Clamp(StartAt,0.f,Length-.02f),To=StopAt>From+.02f?FMath::Min(StopAt,Length):Length;
    const float Rate=FMath::Clamp(Seconds>0.f?(To-From)/Seconds:1.f,.35f,4.f); // Seconds is the time the played part takes
    UAnimMontage* Played=Anim->PlaySlotAnimationAsDynamicMontage(Clip,Slot,BlendIn,BlendOut,Rate,1,-1.f,From);
    if(Played&&To<Length-.001f) { // a part of the clip: its blend out starts when the part ends
        FTimerHandle Cut;const TWeakObjectPtr<UAnimMontage> Weak=Played;
        GetWorldTimerManager().SetTimer(Cut,FTimerDelegate::CreateWeakLambda(this,[this,Weak,BlendOut]{
            UAnimInstance* A=GetMesh()?GetMesh()->GetAnimInstance():nullptr;if(A&&Weak.IsValid()) A->Montage_Stop(BlendOut,Weak.Get());}),FMath::Max(.02f,(To-From)/Rate-BlendOut),false);
    }
    return Played;
}

void AASCharacter::DevPlayClip(const FString& Name,bool bFullBody,float Seconds,float HoldAt) {
    if(HoldAt<0.f) {PlayHeroClip(*Name,bFullBody?ASHeroLook::FullBody:ASHeroLook::UpperBody,Seconds,.05f,.1f);return;}
    // the pose at HoldAt seconds into the clip, held (a play rate of nearly nothing) for Seconds: a still of an exact moment of a clip
    UAnimInstance* Anim=GetMesh()?GetMesh()->GetAnimInstance():nullptr;UAnimSequenceBase* Clip=ASHeroLook::Clip(*Name);if(!Anim||!Clip) return;
    if(UAnimMontage* M=Anim->PlaySlotAnimationAsDynamicMontage(Clip,bFullBody?ASHeroLook::FullBody:ASHeroLook::UpperBody,.02f,.1f,.0001f,1,-1.f,FMath::Clamp(HoldAt,0.f,Clip->GetPlayLength()-.01f))) M->bEnableAutoBlendOut=false;
    FTimerHandle Release;GetWorldTimerManager().SetTimer(Release,FTimerDelegate::CreateWeakLambda(this,[this]{if(UAnimInstance* A=GetMesh()?GetMesh()->GetAnimInstance():nullptr) A->StopAllMontages(.1f);}),FMath::Max(.2f,Seconds),false);
}

void AASCharacter::PlayAttackClip() {
    if(!bSkeletalHero) return;
    const auto& Set=ASHeroLook::AttackFor(Build.Weapon);
    const int32 Step=ASMelee::IsMelee(Build.Weapon)?FMath::Clamp(MeleeStep,0,2):int32(AttackSequence%3);
    // a melee clip is stretched over the whole swing (its blow then lands about where the swing's does); a shot is a quick kick, never shorter than the clip wants to be
    float Seconds=ASMelee::IsMelee(Build.Weapon)?FMath::Max(.18f,AttackPoseDuration*1.35f):FMath::Max(.16f,AttackPoseDuration*2.2f);
    float From=0.f,To=0.f;ASHeroLook::AttackSpan(Build.Weapon,Step,From,To);
    if(To>From) Seconds=FMath::Max(.12f,AttackPoseDuration*1.1f); // a cut clip is stretched over the swing itself (its blow near the swing's impact), not over the swing and its follow-through
    PlayHeroClip(Set.Clip[Step],Set.FullBody[Step]?ASHeroLook::FullBody:ASHeroLook::UpperBody,Seconds,.05f,ASMelee::IsMelee(Build.Weapon)?.18f:.10f,From,To);
}

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarASFoleyLog(TEXT("as.foley.log"),0,TEXT("Dev: 1 logs every footstep a hero's feet make (\"AshenStep:\": which foot, when, how high the bone was)."),ECVF_Default);
#endif
// 0.26 G6: the steps follow the Mannequin's own feet. The 0.25 steps were timed from the old part rig's gait phase, which since Phase F has nothing to do with where the feet are, so
// they drifted against the stride ("they do not match the steps"). Every machine reads every hero's foot bones itself, so the steps still cost no network traffic.
void AASCharacter::SkeletalFootsteps(bool Dashing,bool Airborne) {
    USkeletalMeshComponent* Body=GetMesh();if(!Body||GetNetMode()==NM_DedicatedServer) return;
    static const FName Bones[2]={FName(TEXT("foot_l")),FName(TEXT("foot_r"))};
    const float Now=AS::Time(this),Speed=GetVelocity().Size2D(),Bottom=GetActorLocation().Z-GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    for(int32 Side=0;Side<2;++Side) {
        if(!Body->DoesSocketExist(Bones[Side])) continue;
        const float Height=Body->GetSocketLocation(Bones[Side]).Z-Bottom;
        if(!ASFoley::FootStruck(SkelFeet[Side],Height,Now)) continue;
        if(!ASFoley::Audible(Airborne,Dashing,bDowned,Speed)||LandingAge<ASFoley::LandingQuiet) continue;
        PlayFootstep(Speed);
#if !UE_BUILD_SHIPPING
        if(CVarASFoleyLog.GetValueOnGameThread()!=0) UE_LOG(LogTemp,Display,TEXT("AshenStep: %s foot at %.2f, bone %.1f cm above the capsule bottom, speed %.0f cm/s, %s"),Side?TEXT("right"):TEXT("left"),Now,Height,Speed,IsLocallyControlled()?TEXT("this machine's own hero"):TEXT("the other player's hero"));
#endif
    }
}

// 0.26 G4: tells the animation blueprint how much to hold the left hand on the weapon (TwoHandAlpha, eased in and out so a weapon change or a fall does not snap the arm) and where, as a point in the
// right hand bone's space (LeftGripOffset: the weapon sits on that bone at WeaponMesh's relative transform, so a point on its handle is that transform applied to it). Both are variables of ABP_AshenHero,
// set by name; a machine whose ABP predates the IK has no such variables and simply keeps the arm as animated. as.hero.twohand 0 switches it off to compare.
#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarASTwoHand(TEXT("as.hero.twohand"),1,TEXT("Dev: 1 holds the left hand on two-handed weapons with IK (0.26 G4); 0 leaves the arm as animated."),ECVF_Default);
#endif
void AASCharacter::DriveTwoHandGrip(float Delta) {
    UAnimInstance* Anim=GetMesh()?GetMesh()->GetAnimInstance():nullptr;if(!Anim||!WeaponMesh) return;
    FVector OnHandle=FVector::ZeroVector;bool Wants=!bDowned&&WeaponMesh->IsVisible()&&ASHeroLook::TwoHandedGrip(Build.Weapon,OnHandle);
#if !UE_BUILD_SHIPPING
    if(CVarASTwoHand.GetValueOnGameThread()==0) Wants=false;
#endif
    SkelTwoHand=FMath::FInterpTo(SkelTwoHand,Wants?1.f:0.f,Delta,10.f);
    const UClass* Class=Anim->GetClass();
    if(FFloatProperty* Alpha=CastField<FFloatProperty>(Class->FindPropertyByName(TEXT("TwoHandAlpha")))) Alpha->SetPropertyValue_InContainer(Anim,SkelTwoHand);
    if(FStructProperty* Grip=CastField<FStructProperty>(Class->FindPropertyByName(TEXT("LeftGripOffset")))) if(Grip->Struct==TBaseStructure<FVector>::Get())
        *Grip->ContainerPtrToValuePtr<FVector>(Anim)=WeaponMesh->GetRelativeTransform().TransformPosition(OnHandle);
}

void AASCharacter::AnimateSkeletal(float Delta,bool Dashing,bool Airborne) {
    USkeletalMeshComponent* Body=GetMesh();if(!Body) return;
    PoseAge+=Delta;
    SkeletalFootsteps(Dashing,Airborne);
    // a dash: a directional evade over the dash, on the whole body
    const bool DashNow=!bDowned&&(Dashing||bDashActive);
    if(DashNow&&!bSkelDashSeen) {
        const FVector Local=GetActorTransform().InverseTransformVectorNoScale(GetVelocity().IsNearlyZero()?GetActorForwardVector():GetVelocity()).GetSafeNormal2D();
        const TCHAR* Evade=FMath::Abs(Local.X)>=FMath::Abs(Local.Y)?(Local.X>=0.f?TEXT("TwinBlast_Evade_Fwd"):TEXT("TwinBlast_Evade_Bwd")):(Local.Y>=0.f?TEXT("TwinBlast_Evade_Right"):TEXT("TwinBlast_Evade_Left"));
        PlayHeroClip(Evade,ASHeroLook::FullBody,FMath::Max(.22f,DashDuration()*1.6f),.04f,.16f);
    }
    bSkelDashSeen=DashNow;
    // the maul's leap: crouch, the flight, the landing smash
    if(Build.Weapon==1&&HammerLeapState!=SkelLeapSeen) {
        if(HammerLeapState==1) PlayHeroClip(TEXT("steel_Steel_Ability_GroundSmah_Start"),ASHeroLook::FullBody,ASHammer::Startup,.05f,.05f);
        else if(HammerLeapState==2) PlayHeroClip(TEXT("steel_Steel_Ability_GroundSmash_Loop"),ASHeroLook::FullBody,ASHammer::Flight,.05f,.05f);
        else if(HammerLeapState==3) PlayHeroClip(TEXT("steel_Steel_Ability_GroundSmash_End"),ASHeroLook::FullBody,.55f,.03f,.2f);
        SkelLeapSeen=HammerLeapState;
    }
    // Twin Claws' pounce: the coil, the leap, the rake. Khaimera's clips, cut to the part each moment wants (look_claw_poses.txt): Jump_Start's first .22 s is a deep crouch with the claws low and forward;
    // R_Ability from .08 to .52 s is arms up, then a dive into a crouch on the ground (the leap lands in it); from .85 to 1.45 s the crouch rises into a wide claw sweep (the arrival strike).
    if(Build.Weapon==ASRules::Claws&&PounceState!=SkelPounceSeen) {
        if(PounceState==1) PlayHeroClip(TEXT("Khaimera_Jump_Start"),ASHeroLook::FullBody,ASClaws::Windup,.03f,.02f,0.f,.22f);
        else if(PounceState==2) PlayHeroClip(TEXT("Khaimera_R_Ability"),ASHeroLook::FullBody,FMath::Max(.12f,PounceFlight),.03f,.02f,.08f,.52f);
        else if(PounceState==3) PlayHeroClip(TEXT("Khaimera_R_Ability"),ASHeroLook::FullBody,.30f,.02f,.18f,.85f,1.45f);
        else if(SkelPounceSeen==2&&!DashNow&&!bDowned) PlayHeroClip(TEXT("Khaimera_Jump_Recovery"),ASHeroLook::FullBody,.4f,.03f,.2f,.0f,.6f); // a lunge that landed on nothing: the crouching landing
        SkelPounceSeen=PounceState;
    } else if(Build.Weapon!=ASRules::Claws) SkelPounceSeen=0;
    // downed: the Mannequin's own death fall, held at its last frame until revived
    if(bDowned!=bSkelDownedSeen) {
        bSkelDownedSeen=bDowned;
        if(UAnimInstance* Anim=Body->GetAnimInstance()) {
            if(bDowned) {if(UAnimMontage* Fall=PlayHeroClip(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01"),ASHeroLook::FullBody,1.1f,.08f,.25f)) Fall->bEnableAutoBlendOut=false;} // it ends lying down and stays there
            else Anim->StopAllMontages(.3f);
        }
    }
    // a reload: the rifle's reload over the reload time, on the arms
    if(ReloadStart>0.f&&ReloadStart!=SkelReloadSeen&&IsReloading()) {SkelReloadSeen=ReloadStart;PlayHeroClip(TEXT("/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Reload.MM_Rifle_Reload"),ASHeroLook::UpperBody,FMath::Max(.3f,ReloadEnd-ReloadStart),.1f,.15f);}
    DriveTwoHandGrip(Delta); // 0.26 G4: the left hand on a two-handed weapon, through whatever the slots are playing
    // between moves the weapon is carried in its stance: a long loop on the upper body; any move takes the slots over (one slot group) and the stance eases back in after it
    const FString Stance=ASHeroLook::StanceName(Build.Weapon);
    if(Stance!=SkelStanceClip) { // the stance clip changed (a new weapon is handled in SetupSkeletalHero; the as.hero.stance dev variable changes it live): the old loop eases out
        if(UAnimInstance* Current=Body->GetAnimInstance()) if(SkelStance.IsValid()) Current->Montage_Stop(.15f,SkelStance.Get());
        SkelStance.Reset();SkelStanceWeapon=-1;
    }
    if(!bDowned) if(UAnimInstance* Anim=Body->GetAnimInstance()) if(!Anim->GetCurrentActiveMontage()) if(UAnimSequenceBase* Clip=ASHeroLook::Clip(*Stance)) {
        SkelStance=Anim->PlaySlotAnimationAsDynamicMontage(Clip,ASHeroLook::UpperBody,.25f,.12f,1.f,1000,-1.f,0.f);SkelStanceWeapon=Build.Weapon;SkelStanceClip=Stance;
    }
}
