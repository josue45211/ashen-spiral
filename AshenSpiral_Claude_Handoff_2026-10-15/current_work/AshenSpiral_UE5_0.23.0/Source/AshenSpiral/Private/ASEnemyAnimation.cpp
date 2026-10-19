#include "ASEnemy.h"
#include "ASEnemyRules.h"
#include "ASEnemyPose.h"
#include "ASEnemyDeathTracks.h"
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
#include "Materials/MaterialInstanceDynamic.h"
// ---- 0.24 Tier 0 animation: the armour rig is posed by ASEnemyPose (pure rules, tested as plain C++). The gait, the current ability's track (anticipation, strike,
// recovery), the release gesture and the hit recoil go in; a transform for every part comes out. Tracks are data (ASEnemyPose.h) and Tools/preview_enemy_poses.py draws them
// from the real meshes. The track time comes from replicated fields only (Ability, AIState, WindupStart/End, AttackReleaseTime), so every machine poses identically.
float AASEnemy::PoseTime(float Now) {
    namespace ER=ASEnemyRules;
    if(Ability!=ER::AbilityNone) PoseAbility=Ability; // remembered: the replicated ability resets when the recovery ends, but the pose may still be settling
    if(PoseAbility==ER::AbilityNone) return -1.f;
    const bool Telegraphing=AIState==ER::StateWindup||AIState==ER::StateSpecial;
    const float U=ASEnemyPose::AbilityU(ER::SpecFor(PoseAbility),Telegraphing,Now,WindupStart,WindupEnd,AttackReleaseTime);
    if(U<0.f) PoseAbility=ER::AbilityNone;
    return U;
}
void AASEnemy::AnimateArmor(float Delta,float Windup) {
    namespace EP=ASEnemyPose;
    if(bTrainingDummy||Kind==5||Kind==9||Kind==ASExpedition::FlierKind) return; // the leaper and broodcaller (AnimateBlob) and the Gloomwing have no armour parts to pose
    const float Now=AS::Time(this);
    // Replicated attack release and hit reaction: readable, brief, cosmetic only.
    const float Release=ASAnimation::ReleaseGesture(Now-AttackReleaseTime);
    const float Recoil=ASAnimation::Reaction(Now-HitReactionTime,HitReactionStrength);
    const FVector Push=GetActorTransform().InverseTransformVectorNoScale(HitReactionDirection);
    const float HalfHeight=GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    if(Flies()) { // 0.24: the armoured flying boss no longer walks on air (the gait followed its sideways orbit velocity: a moonwalk); its own flight pose, plus the gesture layer
        AnimateFlightPose(Delta,Windup);
        ArmorParts[0]->AddRelativeRotation(FRotator(Release*14.f-Push.X*Recoil*18.f,0,Push.Y*Recoil*12.f));
        ArmorParts[1]->AddRelativeRotation(FRotator(-Recoil*8.f,0,0));
        ArmorFigure->SetRelativeLocation(FVector(Push.X*Recoil*12.f+Release*6.f,Push.Y*Recoil*12.f,-HalfHeight));
        return;
    }
    // Gait. Enemies strafe and back away while facing the target, so the feet swing along the direction of motion in the enemy's own frame (forward walks as before,
    // sideways side-steps, backwards steps back).
    const FVector Velocity=GetVelocity(),LocalVelocity=GetActorTransform().InverseTransformVectorNoScale(Velocity);
    const float Speed=Velocity.Size2D(),Weight=FMath::Clamp(Speed/400.f,0.f,1.f);
    GaitPhase+=Delta*Speed/190.f*UE_TWO_PI;
    EP::RigInput In;
    In.GaitPhase=GaitPhase;In.Weight=Weight;In.Step=FMath::Sin(GaitPhase)*Weight;
    if(Speed>5.f) {In.HeadingX=LocalVelocity.X/Speed;In.HeadingY=LocalVelocity.Y/Speed;}
    In.Gun=Kind==8;In.Release=Release;In.Recoil=Recoil;In.PushX=Push.X;In.PushY=Push.Y;
    In.Seconds=Now;In.Speed=Weight;In.Turn=0.f;In.Flare=FMath::Clamp(Windup*.5f+Release*.6f,0.f,1.f); // the cape billows with speed and flares as an attack winds up and lands
    float D[EP::ChannelCount];EP::ClearChannels(D);
    const float U=PoseTime(Now);
    if(U>=0.f) EP::Evaluate(EP::TrackFor(PoseAbility),U,D);
    EP::RigOut Out;EP::Solve(In,D,Out);
    ApplyRig(Out,HalfHeight,1.f);
}
// Puts a solved pose on the components: every armour part, the weapon and the shield, the figure's own transform (the death's Fade shrinks the whole figure), and the glow.
void AASEnemy::ApplyRig(const ASEnemyPose::RigOut& Out,float HalfHeight,float Fade) {
    namespace EP=ASEnemyPose;
    auto Place=[](UStaticMeshComponent* Part,const EP::Xf& X) {
        if(!Part) return;
        Part->SetRelativeLocationAndRotation(FVector(X.Loc.X,X.Loc.Y,X.Loc.Z),FQuat(X.Rot.X,X.Rot.Y,X.Rot.Z,X.Rot.W));
        const FVector Scale(X.Scale.X,X.Scale.Y,X.Scale.Z);
        if(!Part->GetRelativeScale3D().Equals(Scale,.001)) Part->SetRelativeScale3D(Scale); // most parts never change scale
    };
    for(int32 I=0;I<EP::Weapon&&I<ArmorParts.Num();++I) Place(ArmorParts[I],Out.Part[I]);
    Place(ArmorWeapon,Out.Part[EP::Weapon]);Place(ArmorShield,Out.Part[EP::Shield]);
    const float Size=(Elite()?BodyScale:.90f)*Fade; // the figure's own size, as RefreshLook set it
    ArmorFigure->SetRelativeLocationAndRotation(FVector(Out.FigureOffset.X,Out.FigureOffset.Y,-HalfHeight),FQuat(Out.FigureRot.X,Out.FigureRot.Y,Out.FigureRot.Z,Out.FigureRot.W));
    ArmorFigure->SetRelativeScale3D(FVector(.82f*Size*Out.FigureScale.X,.70f*Size*Out.FigureScale.Y,Size*Out.FigureScale.Z));
    if(FMath::Abs(Out.Glow-AppliedGlow)>.02f&&ArmorParts.Num()>EP::Core) { // the visor and the chest core brighten as an attack winds up (and flare as it dies)
        AppliedGlow=Out.Glow;
        for(int32 Slot:{int32(EP::Visor),int32(EP::Core)}) if(auto* M=Cast<UMaterialInstanceDynamic>(ArmorParts[Slot]->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),4.f+Out.Glow*4.f);
    }
}
// The leaper and the broodcaller are one body: the same ability tracks squash, stretch and tip it (and swell its glow).
void AASEnemy::AnimateBlob() {
    namespace EP=ASEnemyPose;
    if(bTrainingDummy||!Body) return;
    float D[EP::ChannelCount];EP::ClearChannels(D);
    const float U=PoseTime(AS::Time(this));
    if(U>=0.f) EP::Evaluate(EP::TrackFor(PoseAbility),U,D);
    EP::BlobOut Out;EP::SolveBlob(D,Out);
    const float HalfHeight=GetCapsuleComponent()?GetCapsuleComponent()->GetScaledCapsuleHalfHeight():83.f;
    Body->SetRelativeLocationAndRotation(FVector(Out.Offset.X,Out.Offset.Y,-HalfHeight+Out.Offset.Z),FQuat(Out.Rot.X,Out.Rot.Y,Out.Rot.Z,Out.Rot.W));
    Body->SetRelativeScale3D(FVector(BodyScale*Out.Scale.X,BodyScale*Out.Scale.Y,BodyScale*Out.Scale.Z));
    if(FMath::Abs(Out.Glow-AppliedGlow)>.02f) {AppliedGlow=Out.Glow;if(auto* M=Cast<UMaterialInstanceDynamic>(Body->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),Out.Glow*2.5f);}
}

void AASEnemy::AnimateDeath() {
    namespace EP=ASEnemyPose;
    const float Now=AS::Time(this);
    if(DeathSeenAt<0.f) {
        DeathSeenAt=Now;
        const FVector Push=GetActorTransform().InverseTransformVectorNoScale(HitReactionDirection); // which way the killing blow pushed it, in its own frame
        DeathVariant=EP::DeathVariantFor(Push.X,Push.Y,GetUniqueID());
        for(auto* Piece:{Tell.Get(),TellFill.Get(),Beam.Get(),Eye.Get(),Crown.Get(),WingL.Get(),WingR.Get(),WingBoneL.Get(),WingBoneR.Get()}) if(Piece) Piece->SetVisibility(false);
    }
    const auto Look=ASPresentation::EnemyDeathAt(Now-DeathSeenAt);
    // 0.24: an armoured walker plays a collapse retargeted from the Mannequin's own death animations (ASEnemyDeathTracks.h) over EnemyDeathSeconds, then shrinks away as before.
    if(!bTrainingDummy&&!Flies()&&ASEnemyRules::KitFor(Kind).AnimSet==ASEnemyRules::AnimHumanoid&&ArmorParts.Num()>EP::Core) {
        float D[EP::ChannelCount];
        EP::Evaluate(EP::DeathTrack(DeathVariant<0?0:DeathVariant),FMath::Clamp((Now-DeathSeenAt)/ASPresentation::EnemyDeathSeconds,0.f,1.f),D);
        EP::RigInput In;In.Seconds=Now;In.Gun=Kind==8;
        EP::RigOut Out;EP::Solve(In,D,Out);Out.Glow=Look.Glow*3.f;
        ApplyRig(Out,GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),FMath::Max(.02f,Look.Scale));
        return;
    }
    // Topples backward and shrinks away with a brief glow, over EnemyDeathSeconds.
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
