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
#include "AnimationRuntime.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarASSkeletalHero(TEXT("as.hero.skeletal"),1,TEXT("1: heroes are the Unreal Mannequin (0.24.2 Phase F) when its assets are present on this machine; 0: the 21-part armour rig. Applied at the next look refresh."),ECVF_Default);

namespace ASHeroLook {
const FName UpperBody(TEXT("UpperBody")),FullBody(TEXT("DefaultSlot"));
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
    static const Attack Table[9]={
        {{TEXT("Aurora_Primary_Attack_Fast_A"),TEXT("Aurora_Primary_Attack_Fast_B"),TEXT("Aurora_Primary_Attack_Fast_C")},{false,false,true}},   // Cinder Blade (the third lunges)
        {{TEXT("Grux_PrimaryAttack_LA"),TEXT("Grux_PrimaryAttack_RA"),TEXT("terra_Primary_Melee_C_slow")},{false,false,true}},                 // Hoarfrost Maul (across, overhead, the planted overhead smash)
        {{TEXT("Sparrow_Primary_Fire_Fast"),TEXT("Sparrow_Primary_Fire_Fast"),TEXT("Sparrow_Primary_Fire_Med")},{false,false,false}},         // Ash Bow
        {{TEXT("gideon_Primary_Attack_A_Medium"),TEXT("gideon_Primary_Attack_B_Medium"),TEXT("gideon_Primary_Attack_C_Medium")},{false,false,false}}, // Spiral Glaive
        {{TEXT("gideon_Primary_Attack_B_Medium"),TEXT("gideon_Primary_Attack_C_Medium"),TEXT("gideon_Primary_Attack_A_Medium")},{false,false,false}}, // Ember Staff
        {{TEXT("Kwang_PrimaryAttack_A_Slow"),TEXT("Kwang_PrimaryAttack_B_Slow"),TEXT("Kwang_PrimaryAttack_C_Slow")},{false,false,false}},      // Rift Scythe
        {{TEXT("Murdock_Fire_Fast"),TEXT("Murdock_Fire_Fast"),TEXT("Murdock_Fire_Fast")},{false,false,false}},                                 // Cinder Scattergun
        {{TEXT("TwinBlast_DoubleShot_Fire_Rt_Fast"),TEXT("TwinBlast_DoubleShot_Fire_Rt_Fast"),TEXT("TwinBlast_DoubleShot_Fire_Rt_Fast")},{false,false,false}}, // Tempest Rotary
        {{TEXT("Wraith_Ability_Q_Fire"),TEXT("Wraith_Ability_Q_Fire"),TEXT("Wraith_Ability_Q_Fire")},{false,false,false}},                     // Rift Mortar
    };
    return Table[FMath::Clamp(Weapon,0,8)];
}
// The stance each weapon is carried in between moves, looped on the upper body (the legs keep their own walk, run and jump). TUNABLE: swap a clip by name.
const TCHAR* StanceFor(int32 Weapon) {
    static const TCHAR* Table[9]={TEXT("Kwang_Idle"),TEXT("Grux_TravelMode_Idle"),TEXT("Sparrow_idle"),TEXT("Yin_Idle_Combat"),TEXT("Yin_Idle_Combat"),TEXT("Yin_Idle_Combat"),
        TEXT("Murdock_Idle"),TEXT("Murdock_Idle"),TEXT("Wraith_Idle_Combat")};
    return Table[FMath::Clamp(Weapon,0,8)];
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
    const FMatrix Axes=Weapon==2||Weapon>=6?FRotationMatrix::MakeFromZX(Along,Knuckles):FRotationMatrix::MakeFromZY(Along,Knuckles);
    Bone=Ref.GetBoneName(Hand);Out=FTransform(Axes.ToQuat(),Centre).GetRelativeTransform(Wrist);
    return true;
}
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
    if(Body->GetSkeletalMeshAsset()!=Model) Body->SetSkeletalMesh(Model);
    if(Body->GetAnimClass()!=Anim) Body->SetAnimInstanceClass(Anim);
    if(SkelStanceWeapon!=Build.Weapon) { // a new weapon: its own stance takes over at the next frame
        if(UAnimInstance* Current=Body->GetAnimInstance()) if(SkelStance.IsValid()) Current->Montage_Stop(.2f,SkelStance.Get());
        SkelStance.Reset();SkelStanceWeapon=-1;
    }
    Body->SetHiddenInGame(false);Body->SetVisibility(true);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetRelativeLocationAndRotation(FVector(0.f,0.f,-GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),FRotator(0.f,-90.f,0.f)); // the Mannequin faces +Y: turned to face the actor's +X
    Body->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones; // the cape and the weapon read its bones every frame
    for(auto& Part:Parts) if(Part) Part->SetVisibility(false);
    // the weapon in the fist (the bow in the left), the spiral charge in the left hand; the part rig's bow string and nocked arrow are not drawn on the Mannequin
    FName Hand;FTransform Grip;
    if(ASHeroLook::GripInHand(Body,Build.Weapon==2,Build.Weapon,Hand,Grip)) {
        WeaponMesh->AttachToComponent(Body,FAttachmentTransformRules::SnapToTargetNotIncludingScale,Hand);WeaponMesh->SetRelativeTransform(Grip);WeaponMesh->SetVisibility(true);
    } else WeaponMesh->SetVisibility(false); // a mesh without hand bones: better no weapon than one at the feet
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

UAnimMontage* AASCharacter::PlayHeroClip(const TCHAR* Name,FName Slot,float Seconds,float BlendIn,float BlendOut) {
    if(!bSkeletalHero||GetNetMode()==NM_DedicatedServer) return nullptr;
    UAnimInstance* Anim=GetMesh()?GetMesh()->GetAnimInstance():nullptr;UAnimSequenceBase* Clip=ASHeroLook::Clip(Name);
    if(!Anim||!Clip) return nullptr;
    const float Length=FMath::Max(.05f,Clip->GetPlayLength());
    const float Rate=FMath::Clamp(Seconds>0.f?Length/Seconds:1.f,.35f,4.f);
    return Anim->PlaySlotAnimationAsDynamicMontage(Clip,Slot,BlendIn,BlendOut,Rate,1,-1.f,0.f);
}

void AASCharacter::PlayAttackClip() {
    if(!bSkeletalHero) return;
    const auto& Set=ASHeroLook::AttackFor(Build.Weapon);
    const int32 Step=ASMelee::IsMelee(Build.Weapon)?FMath::Clamp(MeleeStep,0,2):int32(AttackSequence%3);
    // a melee clip is stretched over the whole swing (its blow then lands about where the swing's does); a shot is a quick kick, never shorter than the clip wants to be
    const float Seconds=ASMelee::IsMelee(Build.Weapon)?FMath::Max(.18f,AttackPoseDuration*1.35f):FMath::Max(.16f,AttackPoseDuration*2.2f);
    PlayHeroClip(Set.Clip[Step],Set.FullBody[Step]?ASHeroLook::FullBody:ASHeroLook::UpperBody,Seconds,.05f,ASMelee::IsMelee(Build.Weapon)?.18f:.10f);
}

void AASCharacter::AnimateSkeletal(float Delta,bool Dashing,bool Airborne) {
    (void)Airborne;
    USkeletalMeshComponent* Body=GetMesh();if(!Body) return;
    PoseAge+=Delta;
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
    // between moves the weapon is carried in its stance: a long loop on the upper body; any move takes the slots over (one slot group) and the stance eases back in after it
    if(!bDowned) if(UAnimInstance* Anim=Body->GetAnimInstance()) if(!Anim->GetCurrentActiveMontage()) if(UAnimSequenceBase* Clip=ASHeroLook::Clip(ASHeroLook::StanceFor(Build.Weapon))) {
        SkelStance=Anim->PlaySlotAnimationAsDynamicMontage(Clip,ASHeroLook::UpperBody,.25f,.12f,1.f,1000,-1.f,0.f);SkelStanceWeapon=Build.Weapon;
    }
}
