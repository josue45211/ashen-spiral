// 0.24.2 (Edison 08:30: "keep the visual polish going"; the 0.25 plan's held creatures, cleared once AI-made art was accepted): a ground boss is the Warrior Titan - a
// stylised armoured warrior (Fab) - instead of the 21-part armour rig. The Titan came with walk and run clips only, so it borrows the heroes': Tools/build_boss_model.py
// retargets the Mannequin's skeleton onto the Titan's and duplicates the hero animation blueprint (locomotion, jumps, the full-body slot) and the boss clips onto it. Every
// clip is chosen on every machine from state that was already replicated (the ability and its wind-up times, the death), so nothing new crosses the network.
// The armour rig stays as the fallback: as.boss.skeletal 0, the flying boss (it keeps its wings), or a machine without the Titan plays exactly as before.
#include "ASEnemy.h"
#include "ASEnemyRules.h"
#include "ASEnemyAbilities.h"
#include "ASPresentationRules.h"
#include "ASExpeditionRules.h"
#include "ASData.h"
#include "ASGameState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarASSkeletalBoss(TEXT("as.boss.skeletal"),1,TEXT("1: ground bosses are the Warrior Titan (0.24.2) when it is imported on this machine; 2: and the elite brutes too, smaller, with their maul in hand; 0: the armour rig. Applied at the next look refresh."),ECVF_Default);
// where an elite's maul sits in the Titan's right hand: "x y z pitch yaw roll" (cm, degrees), relative to the RightHand bone. TUNABLE (read every tick so it can be tuned in a running game).
static TAutoConsoleVariable<FString> CVarASEliteHold(TEXT("as.elite.hold"),TEXT("0 0 0 0 0 90"),TEXT("The elite Titan's maul in its hand: x y z pitch yaw roll, relative to the RightHand bone."),ECVF_Default);

namespace ASBossLook {
const FName FullBody(TEXT("DefaultSlot"));
constexpr float TitanHeight=160.f;   // SK_Titan stands 160 cm in its T-pose; it is scaled to the boss's capsule
constexpr float TitanFill=.96f;      // ... a little under the capsule's full height (the capsule's round ends)
UAnimSequenceBase* Clip(const TCHAR* Name) {
    static TMap<FString,TWeakObjectPtr<UAnimSequenceBase>> Cache;
    if(const TWeakObjectPtr<UAnimSequenceBase>* Hit=Cache.Find(Name)) if(Hit->IsValid()) return Hit->Get();
    const FString Path=FString::Printf(TEXT("/Game/Characters/Titan/Anims/Titan_%s.Titan_%s"),Name,Name);
    UAnimSequenceBase* Found=LoadObject<UAnimSequenceBase>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet);
    Cache.Add(Name,Found);return Found;
}
// What a boss plays as it winds an ability up (the clip runs over the wind-up and the committed motion, so its blow lands about when the ability's does). TUNABLE.
const TCHAR* ClipFor(int32 Ability,int32 Pattern) {
    namespace ER=ASEnemyRules;
    switch(Ability) {
    case ER::AbilityBossSlam: case ER::AbilityBossToll: case ER::AbilityPound: return TEXT("terra_Primary_Melee_C_slow");               // an overhead smash into the floor
    case ER::AbilityBossRoots: return TEXT("steel_Steel_Ability_GroundSmash_End");                                                       // a fist driven into the ground
    case ER::AbilityBossMarks: case ER::AbilityBossSeeds: case ER::AbilityBossBloom: return TEXT("gideon_RMB_Cast");                    // a summoning cast
    case ER::AbilityBossRing: return TEXT("Wraith_Ability_Q_Fire");                                                                      // a volley thrown out
    case ER::AbilityBossGrasp: return TEXT("gideon_Primary_Attack_A_Medium");                                                            // a pulling sweep
    case ER::AbilityBossPhase: case ER::AbilityEnrage: return TEXT("Grux_DoublePain");                                                   // the half-health fury, an elite's rage roar
    case ER::AbilityGlacial: return TEXT("terra_Primary_Melee_C_slow");                                                                  // an elite's mutated slam
    case ER::AbilityCinders: return TEXT("Wraith_Ability_Q_Fire");                                                                       // fire pools thrown out in a line
    case ER::AbilityStorm: case ER::AbilitySeeds: return TEXT("gideon_RMB_Cast");                                                       // a call on the sky, a toss of seed bombs
    case ER::AbilityBossCharge: return TEXT("Grux_PrimaryAttack_RA");                                                                    // the blow at the end of the stampede
    default: return Pattern%2?TEXT("Grux_PrimaryAttack_RA"):TEXT("Grux_PrimaryAttack_LA");                                               // the older boss attack: alternating swings
    }
}
// An elite wears its mutation as a steady cast over the Titan's colours (a pulse of light over the textures turned it to glass). TUNABLE.
FLinearColor MutationTint(int32 Mutation) {
    if(Mutation==ASEnemyRules::MutationNone) return FLinearColor::White;
    float R,G,B;ASEnemyRules::MutationColor(Mutation,R,G,B);
    return FMath::Lerp(FLinearColor::White,FLinearColor(R*1.5f,G*1.5f,B*1.5f,1.f),.42f);
}
// Each ward's boss wears its own cast over the Titan's brown and gold (the four would otherwise be one figure). TUNABLE.
FLinearColor WardTint(int32 Ward) {
    const FLinearColor Base=ASData::BiomeColor(Ward);
    return FMath::Lerp(FLinearColor::White,FLinearColor(Base.R*1.6f,Base.G*1.6f,Base.B*1.6f,1.f),.30f);
}
}

void AASEnemy::SetupSkeletalBoss() {
    USkeletalMeshComponent* Model=GetMesh();if(!Model) return;
    const int32 Mode=CVarASSkeletalBoss.GetValueOnGameThread();
    const bool Elite=Mode>=2&&Kind==ASEnemyRules::Brute&&!bTrainingDummy;                       // the elite brute: a Titan too, smaller, with its maul in hand
    const bool Want=Mode!=0&&(Boss()&&!bAirborne||Elite)&&GetNetMode()!=NM_DedicatedServer;
    USkeletalMesh* Titan=Want?LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Titan/SK_Titan.SK_Titan"),nullptr,LOAD_NoWarn|LOAD_Quiet):nullptr;
    UClass* Anim=Want?LoadObject<UClass>(nullptr,TEXT("/Game/Characters/Titan/Anims/Titan_ABP_AshenHero.Titan_ABP_AshenHero_C"),nullptr,LOAD_NoWarn|LOAD_Quiet):nullptr;
    bSkeletalBoss=Titan&&Anim;
    if(!bSkeletalBoss) {
        Model->SetHiddenInGame(true);
        if(ArmorWeapon&&SkelWeaponParent.IsValid()&&ArmorWeapon->GetAttachParent()==Model) { // the Titan was switched off: the maul goes back to the armour rig's hand
            ArmorWeapon->AttachToComponent(SkelWeaponParent.Get(),FAttachmentTransformRules::KeepRelativeTransform);ArmorWeapon->SetRelativeTransform(SkelWeaponRel);
        }
        return;
    }
    if(Model->GetSkeletalMeshAsset()!=Titan) Model->SetSkeletalMesh(Titan);
    if(Model->GetAnimClass()!=Anim) Model->SetAnimInstanceClass(Anim);
    const float HalfHeight=GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    Model->SetHiddenInGame(false);Model->SetVisibility(true);Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Model->SetRelativeLocationAndRotation(FVector(0.f,0.f,-HalfHeight),FRotator(0.f,-90.f,0.f)); // like the Mannequin it faces +Y: turned to face the actor's +X
    // a client's network smoothing owns this component's relative transform (it re-applies the offset the character cached when it was spawned, every frame): the Titan's own offset is
    // what it must keep, or it floats half a capsule up and faces sideways on every machine but the host
    CacheInitialMeshOffset(FVector(0.f,0.f,-HalfHeight),FRotator(0.f,-90.f,0.f));
    Model->SetRelativeScale3D(FVector(2.f*HalfHeight*ASBossLook::TitanFill/ASBossLook::TitanHeight));
    Model->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones; // the glint reads its hand
    if(!SkelMaterial||Model->GetMaterial(0)!=SkelMaterial) SkelMaterial=Model->CreateDynamicMaterialInstance(0);
    if(SkelMaterial) {SkelMaterial->SetVectorParameterValue(TEXT("Tint"),Elite?ASBossLook::MutationTint(Mutation):ASBossLook::WardTint(Ward));SkelMaterial->SetVectorParameterValue(TEXT("Glow"),FLinearColor::Black);}
    for(auto& Part:ArmorParts) if(Part) Part->SetVisibility(false);
    for(auto* Piece:{ArmorShield.Get(),Body.Get(),Eye.Get()}) if(Piece) Piece->SetVisibility(false);
    if(ArmorWeapon) {
        if(Elite) { // the maul in the Titan's right fist (it inherits the Titan's scale, so a 131 cm maul becomes about 220 cm in a 270 cm elite's hand)
            if(ArmorWeapon->GetAttachParent()!=Model) {
                SkelWeaponParent=ArmorWeapon->GetAttachParent();SkelWeaponRel=ArmorWeapon->GetRelativeTransform();
                ArmorWeapon->AttachToComponent(Model,FAttachmentTransformRules::SnapToTargetNotIncludingScale,TEXT("RightHand"));
            }
            ArmorWeapon->SetVisibility(true);
        } else ArmorWeapon->SetVisibility(false);
    }
}

UAnimMontage* AASEnemy::PlayBossClip(const TCHAR* Name,float Seconds,float BlendIn,float BlendOut,const TCHAR* Slot) {
    if(!bSkeletalBoss||GetNetMode()==NM_DedicatedServer) return nullptr;
    UAnimInstance* Anim=GetMesh()?GetMesh()->GetAnimInstance():nullptr;UAnimSequenceBase* Clip=ASBossLook::Clip(Name);
    if(!Anim||!Clip) return nullptr;
    const float Rate=FMath::Clamp(Seconds>0.f?FMath::Max(.05f,Clip->GetPlayLength())/Seconds:1.f,.35f,3.f);
    return Anim->PlaySlotAnimationAsDynamicMontage(Clip,Slot?FName(Slot):ASBossLook::FullBody,BlendIn,BlendOut,Rate,1,-1.f,0.f);
}

void AASEnemy::AnimateSkeletalBoss(float Delta) {
    namespace ER=ASEnemyRules;
    (void)Delta;
    // a new wind-up (a new ability, or the same one again: its start time changes) plays its clip over the wind-up and the committed motion after it
    const bool Telegraph=(AIState==ER::StateWindup||AIState==ER::StateSpecial)&&Ability!=ER::AbilityNone;
    if(Telegraph&&WindupStart!=SkelWindupSeen) {
        SkelWindupSeen=WindupStart;
        const float Seconds=FMath::Max(.5f,(WindupEnd-WindupStart)+ER::SpecFor(Ability).Attack+.35f);
        PlayBossClip(ASBossLook::ClipFor(Ability,SkelPattern++),Seconds);
    }
    // an elite flinches under a blow (over its legs, and never in the middle of a wind-up); a boss does not
    if(Kind==ER::Brute&&HitReactionTime!=SkelHitSeen) {
        SkelHitSeen=HitReactionTime;
        if(!Telegraph&&AS::Time(this)-HitReactionTime<.3f) PlayBossClip(TEXT("TwinBlast_HitReact_Front"),.5f,.04f,.18f,TEXT("UpperBody"));
    }
    if(ArmorWeapon&&ArmorWeapon->GetAttachParent()==GetMesh()) { // the maul's hold, tuned in a running game with as.elite.hold
        TArray<FString> Parts;CVarASEliteHold.GetValueOnGameThread().ParseIntoArrayWS(Parts);
        const auto At=[&](int32 I) {return Parts.IsValidIndex(I)?FCString::Atof(*Parts[I]):0.f;};
        ArmorWeapon->SetRelativeLocationAndRotation(FVector(At(0),At(1),At(2)),FRotator(At(3),At(4),At(5)));ArmorWeapon->SetRelativeScale3D(FVector(1.f));
    }
}

bool AASEnemy::AnimateSkeletalDeath(float Since) {
    if(!bSkeletalBoss) return false;
    USkeletalMeshComponent* Model=GetMesh();if(!Model) return false;
    if(!bSkelDeathPlayed) {
        bSkelDeathPlayed=true;
        const FVector Push=GetActorTransform().InverseTransformVectorNoScale(HitReactionDirection); // pushed back by the killing blow: falls back; otherwise forward
        if(UAnimMontage* Fall=PlayBossClip(Push.X<0.f?TEXT("MM_Death_Back_01"):TEXT("MM_Death_Front_01"),1.4f,.1f,.3f)) Fall->bEnableAutoBlendOut=false; // it stays down
    }
    // the fall, then the same shrink-away and flare as every enemy (ASPresentation::EnemyDeathAt)
    const auto Look=ASPresentation::EnemyDeathAt(Since);
    const float HalfHeight=GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    Model->SetRelativeScale3D(FVector(2.f*HalfHeight*ASBossLook::TitanFill/ASBossLook::TitanHeight*FMath::Max(.02f,Look.Scale)));
    if(SkelMaterial) SkelMaterial->SetVectorParameterValue(TEXT("Glow"),FLinearColor(1.f,.35f,.12f)*(Look.Glow*2.f));
    return true;
}
