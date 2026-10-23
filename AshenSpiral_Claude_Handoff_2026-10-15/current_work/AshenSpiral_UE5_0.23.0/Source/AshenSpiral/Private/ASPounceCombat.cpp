#include "ASCharacter.h"
#include "ASMovementComponent.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "ASEnemy.h"
#include "ASBreakable.h"
#include "ASPlayerController.h"
#include "ASGameMode.h"
#include "ASGameInstance.h"
#include "ASSpell.h"
#include "Math/RotationMatrix.h"
#include "GameFramework/RootMotionSource.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveVector.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
// 0.26 Twin Claws: the pounce. RMB leaps onto the enemy under the crosshair and latches there. See ASClawRules.h for the numbers and ASCharacter.h for the state.
//   server, RequestPounce:   picks the enemy along the crosshair's ray, finds where to land beside it (FindPounceLanding), starts the crouch (state 1)
//   server, TickPounce:      1 -> 2 after the crouch: PounceMotion launches the leap on the server and the owner; 2 -> 3 when it has had its time: the arrival strike; 3 -> 0 on a release
//   server + owner:          TickPounceHoming keeps the leap's root-motion target on the moving enemy; UASMovementComponent::CalcVelocity holds the latch (PounceGlueVelocity)
//   from the crouch until a moment after the latch lets go the movement component tells the server to trust the wielder's own position (PounceNetAuthority), so a guest's leap is never corrected
static TAutoConsoleVariable<int32> CVarASPounceLog(TEXT("as.pounce.log"),0,TEXT("1: log every pounce decision (target, landing, release reason) as AshenPounce lines."),ECVF_Default);

bool AASCharacter::PounceLatched() const {
    return Build.Weapon==ASRules::Claws&&PounceState==3&&!bDowned&&!bPounceDashBroke&&IsValid(PounceTarget)&&PounceTarget->Health>0;
}
bool AASCharacter::PounceNetAuthority() const {
    return Build.Weapon==ASRules::Claws&&(PounceState!=0||AS::Time(this)<PounceReleasedAt+.45f);
}
bool AASCharacter::PounceGlueVelocity(FVector& Out) const {
    if(!PounceLatched()) return false;
    const AASEnemy* E=PounceTarget;const FVector T=E->GetActorLocation(),Me=GetActorLocation(),TV=E->GetVelocity();
    const float Ring=ASClaws::RingRadius(GetCapsuleComponent()->GetScaledCapsuleRadius(),E->GetCapsuleComponent()->GetScaledCapsuleRadius());
    const auto Point=ASClaws::RingPoint({float(T.X),float(T.Y)},{float(PounceOutward.X),float(PounceOutward.Y)},Ring);
    const auto V=ASClaws::GlueVelocity({float(Me.X),float(Me.Y)},Point,{float(TV.X),float(TV.Y)});
    Out=FVector(V.X,V.Y,0.f);return true;
}
void AASCharacter::FaceRotation(FRotator NewControlRotation,float DeltaTime) {
    // A pouncing wielder faces its target (the camera stays free): the crouch and the leap toward the target, the latch on it. Every machine runs this inside the movement it predicts or replays.
    if(Build.Weapon==ASRules::Claws&&PounceState!=0&&!bDowned&&!bPounceDashBroke&&IsValid(PounceTarget)) {
        const FVector To=(PounceTarget->GetActorLocation()-GetActorLocation()).GetSafeNormal2D();
        if(!To.IsNearlyZero()) {SetActorRotation(FRotator(0.f,To.Rotation().Yaw,0.f));return;}
    }
    Super::FaceRotation(NewControlRotation,DeltaTime);
}

// ---------------------------------------------------------------- choosing and landing
AASEnemy* AASCharacter::PickPounceTarget(const FVector& Origin,const FVector& Direction) const {
    const auto* S=AS::State(this);if(!S) return nullptr;
    std::vector<ASClaws::Candidate> List;std::vector<AASEnemy*> Owners;
    for(auto W:S->Enemies) if(auto* E=W.Get()) if(E->Health>0&&CanDamage(E)) {
        const FVector P=E->GetActorLocation();
        ASClaws::Candidate C;C.X=float(P.X);C.Y=float(P.Y);C.Z=float(P.Z);C.Radius=E->GetCapsuleComponent()->GetScaledCapsuleRadius()+(E->Flies()?30.f:0.f);
        C.Valid=AS::ClearSight(GetWorld(),GetActorLocation()+FVector(0,0,40),P); // an enemy behind a wall is not a target
        List.push_back(C);Owners.push_back(E);
    }
    const float O[3]={float(Origin.X),float(Origin.Y),float(Origin.Z)},D[3]={float(Direction.X),float(Direction.Y),float(Direction.Z)};
    const FVector Me=GetActorLocation();const float Self[3]={float(Me.X),float(Me.Y),float(Me.Z)};
    const int Index=ASClaws::PickTarget(O,D,Self,List);
    return Index>=0?Owners[size_t(Index)]:nullptr;
}
// A spot beside the target, on the floor, where the wielder fits: tried on the side facing it first, then around the ring. The spot must have a floor within a step of the target's footing (a flier's
// is the floor under it), room for the capsule (no pillar or wall) and a clear line from where the wielder is.
bool AASCharacter::FindPounceLanding(const AASEnemy* E,const FVector& From,FVector& Landing,FVector& Outward) const {
    UWorld* W=GetWorld();if(!W||!IsValid(E)) return false;
    const float Half=GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),Radius=GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float Ring=ASClaws::RingRadius(Radius,E->GetCapsuleComponent()->GetScaledCapsuleRadius());
    const FVector T=E->GetActorLocation();
    const float TargetFeet=T.Z-E->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FVector2D Base(From.X-T.X,From.Y-T.Y);if(!Base.Normalize()) Base=FVector2D(1.f,0.f);
    static const float Angles[]={0.f,35.f,-35.f,70.f,-70.f,105.f,-105.f,140.f,-140.f,180.f};
    FCollisionQueryParams Query(SCENE_QUERY_STAT(PounceLanding),false,this);
    const FCollisionShape Capsule=FCollisionShape::MakeCapsule(Radius,FMath::Max(Radius,Half-3.f));
    float BestDistance=TNumericLimits<float>::Max();bool Found=false;
    for(const float Angle:Angles) {
        const FVector2D Dir=Base.GetRotated(Angle);
        const float X=T.X+Dir.X*Ring,Y=T.Y+Dir.Y*Ring;
        FHitResult Floor;
        const FVector Top(X,Y,FMath::Max(From.Z,TargetFeet+Half)+120.f),Bottom(X,Y,TargetFeet-(E->Flies()?500.f:110.f));
        if(!W->LineTraceSingleByObjectType(Floor,Top,Bottom,FCollisionObjectQueryParams(ECC_WorldStatic),Query)) continue;
        if(!E->Flies()&&FMath::Abs(Floor.ImpactPoint.Z-TargetFeet)>90.f) continue;                        // a ledge or a pit: not the target's footing
        const FVector Centre(X,Y,Floor.ImpactPoint.Z+Half+1.f);
        if(W->OverlapAnyTestByObjectType(Centre,FQuat::Identity,FCollisionObjectQueryParams(ECC_WorldStatic),Capsule,Query)) continue; // in a wall, a pillar, a prop
        if(Angle!=0.f&&!AS::ClearSight(W,From+FVector(0,0,40),Centre+FVector(0,0,40))) continue;
        const float D=float(FVector::DistSquared2D(From,Centre));
        if(D<BestDistance) {BestDistance=D;Landing=Centre;Outward=FVector(Dir.X,Dir.Y,0.f);Found=true;}
    }
    return Found;
}

// ---------------------------------------------------------------- the request, the machine
void AASCharacter::RequestPounce_Implementation(FVector_NetQuantize Origin,FVector_NetQuantizeNormal Direction) {
    auto* S=AS::State(this);auto* M=Movement();
    if(!S||Build.Weapon!=ASRules::Claws||!CanUseWeapons()||bReviving||bDowned||PounceState!=0||S->SimTime<NextHammerLeap||M->DashRemaining>0||M->MovementMode==MOVE_Custom) return;
    if(Origin.ContainsNaN()||Direction.ContainsNaN()||Direction.IsNearlyZero()) return;
    AASEnemy* Target=PickPounceTarget(Origin,Direction);
    FVector Landing=GetActorLocation(),Outward=GetActorForwardVector();bool Whiff=false;
    const bool Picked=Target!=nullptr,Landable=Picked&&FindPounceLanding(Target,GetActorLocation(),Landing,Outward);
    if(CVarASPounceLog.GetValueOnGameThread()) UE_LOG(LogTemp,Display,TEXT("AshenPounce: crosshair ray from %s along %s picked %s (%.0f cm from the wielder, kind %d); a spot beside it: %s"),*FVector(Origin).ToCompactString(),*FVector(Direction).ToCompactString(),*GetNameSafe(Target),Picked?float(FVector::Dist(GetActorLocation(),Target->GetActorLocation())):0.f,Picked?int32(Target->Kind):-1,Picked?(Landable?TEXT("found"):TEXT("NONE (a wall, a pit or a crowd all round)")):TEXT("-"));
    if(Landable) {}
    else {
        // nothing under the crosshair (or nowhere to stand beside it): a lunge along the aim, short of walls, onto the floor
        Target=nullptr;Whiff=true;
        FVector Dir=FVector(Direction.X,Direction.Y,0.f).GetSafeNormal();if(Dir.IsNearlyZero()) Dir=GetActorForwardVector();
        const FVector From=GetActorLocation();FVector To=From+Dir*ASClaws::WhiffDistance;
        FHitResult Wall;FCollisionQueryParams Query(SCENE_QUERY_STAT(PounceWhiff),false,this);
        if(GetWorld()->SweepSingleByObjectType(Wall,From,To,FQuat::Identity,FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(),GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-4.f),Query)) To=From+Dir*FMath::Max(0.f,Wall.Distance-10.f);
        FHitResult Floor;
        if(GetWorld()->LineTraceSingleByObjectType(Floor,To+FVector(0,0,160),To-FVector(0,0,400),FCollisionObjectQueryParams(ECC_WorldStatic),Query)) To.Z=Floor.ImpactPoint.Z+GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+1.f;
        else To.Z=From.Z;
        Landing=To;Outward=-Dir;
    }
    CancelMelee(true);bCharging=false;StopJumping();
    PounceTarget=Target;PounceLanding=Landing;PounceOutward=Outward;bPounceDashBroke=false;
    PounceState=1;PounceStateAt=S->SimTime;NextHammerLeap=S->SimTime+ASClaws::Cooldown(Whiff);
    PendingMeleePhase=int32(S->Phase);PendingMeleeGeneration=S->Arena?S->Arena->Config.Epoch:-1;
    M->StopMovementImmediately();
    if(CVarASPounceLog.GetValueOnGameThread()) UE_LOG(LogTemp,Display,TEXT("AshenPounce: request by %s target=%s whiff=%d landing=%s cooldown %.1fs"),*GetName(),*GetNameSafe(Target),int32(Whiff),*Landing.ToString(),ASClaws::Cooldown(Whiff));
    PounceCue(0,GetActorLocation(),FVector(Outward*-1.f));ForceNetUpdate();
}
void AASCharacter::PounceMotion_Implementation(bool Launch,FVector_NetQuantize Destination,FVector_NetQuantizeNormal Outward,float Seconds,float Arc) {
    auto* M=Movement();M->RemoveRootMotionSource(FName(TEXT("PounceLeap")));
    if(!Launch) {M->RefreshDashCollision();return;}
    if(!HasAuthority()&&!IsLocallyControlled()) return; // proxies get the replicated movement, not a second local force
    if(bDowned||Build.Weapon!=ASRules::Claws||!FMath::IsFinite(Seconds)||Seconds<.05f||!FMath::IsFinite(Arc)||FVector(Destination).ContainsNaN()) return;
    if(!FVector(Outward).IsNearlyZero()) PounceOutward=FVector(Outward).GetSafeNormal2D(); // the side of the target the server chose, which the latch keeps
    // the rise: a parabola of Arc cm over the leap, as the root motion's path offset (the straight line between the two points does the rest)
    if(!PounceArc) PounceArc=NewObject<UCurveVector>(this);
    for(auto& Curve:PounceArc->FloatCurves) Curve.Reset();
    constexpr int32 Samples=12;
    for(int32 I=0;I<=Samples;++I) {const float T=float(I)/Samples;PounceArc->FloatCurves[2].AddKey(T,4.f*Arc*T*(1.f-T));}
    auto Leap=MakeShared<FRootMotionSource_MoveToDynamicForce>();
    Leap->InstanceName=FName(TEXT("PounceLeap"));Leap->AccumulateMode=ERootMotionAccumulateMode::Override;Leap->Priority=700;Leap->Duration=Seconds;
    Leap->StartLocation=GetActorLocation();Leap->InitialTargetLocation=Leap->TargetLocation=FVector(Destination);Leap->bRestrictSpeedToExpected=false;
    Leap->PathOffsetCurve=PounceArc;
    Leap->FinishVelocityParams.Mode=ERootMotionFinishVelocityMode::SetVelocity;Leap->FinishVelocityParams.SetVelocity=FVector::ZeroVector;
    M->StopMovementImmediately();M->SetMovementMode(MOVE_Falling);M->ApplyRootMotionSource(Leap);M->RefreshDashCollision();
}
// The leap homes on a moving enemy: its landing point is the ring point around the enemy as this machine sees it now (the server's truth; the owner's smoothed view), at the floor height
// found when the leap began. Runs on the server and the owner, the two machines that apply the root motion.
void AASCharacter::TickPounceHoming() {
    if(PounceState==0) {bPounceDashBroke=false;return;}
    if(PounceState!=2||!IsValid(PounceTarget)||(!HasAuthority()&&!IsLocallyControlled())) return;
    auto* M=Movement();const TSharedPtr<FRootMotionSource> Source=M->GetRootMotionSource(FName(TEXT("PounceLeap")));if(!Source.IsValid()) return;
    const FVector T=PounceTarget->GetActorLocation();
    const float Ring=ASClaws::RingRadius(GetCapsuleComponent()->GetScaledCapsuleRadius(),PounceTarget->GetCapsuleComponent()->GetScaledCapsuleRadius());
    const auto P=ASClaws::RingPoint({float(T.X),float(T.Y)},{float(PounceOutward.X),float(PounceOutward.Y)},Ring);
    auto* Leap=static_cast<FRootMotionSource_MoveToDynamicForce*>(Source.Get());
    Leap->SetTargetLocation(FVector(P.X,P.Y,Leap->TargetLocation.Z));
}
void AASCharacter::CancelPounce(ASClaws::Release Why) {
    Movement()->RemoveRootMotionSource(FName(TEXT("PounceLeap")));Movement()->RefreshDashCollision();
    if(!HasAuthority()||PounceState==0) return; // only the server changes the replicated state; a client's copy would be overwritten, or worse, left behind
    const uint8 Before=PounceState;AASEnemy* WasTarget=PounceTarget;
    PounceState=0;PounceTarget=nullptr;
    auto* S=AS::State(this);PounceReleasedAt=S?S->SimTime:0.f;
    if(CVarASPounceLog.GetValueOnGameThread()) UE_LOG(LogTemp,Display,TEXT("AshenPounce: %s let go from state %d, reason %d, target %s"),*GetName(),int32(Before),int32(Why),*GetNameSafe(WasTarget));
    PounceMotion(false,FVector::ZeroVector,FVector::ForwardVector,0.f,0.f);
    // cue 4 a release, 5 by dash, 6 the target died (the wielder finished it)
    PounceCue(Why==ASClaws::Release::Dashed?5:(Why==ASClaws::Release::TargetDead?6:4),GetActorLocation(),GetActorForwardVector());
    ForceNetUpdate();
}
void AASCharacter::BeginLatch(float Now) {
    PounceMotion(false,FVector::ZeroVector,FVector::ForwardVector,0.f,0.f); // the leap's root motion ends on every machine
    AASEnemy* T=PounceTarget;
    if(!IsValid(T)||T->Health<=0||!CanDamage(T)) { // a lunge at nothing, or the target is gone: land and recover
        PounceState=0;PounceTarget=nullptr;PounceReleasedAt=Now;NextAttack=FMath::Max(NextAttack,Now+.12f);
        PounceCue(3,GetActorLocation(),GetActorForwardVector());ForceNetUpdate();return;
    }
    PounceState=3;PounceStateAt=Now;PounceOffRing=PounceAirborneFor=0.f;PounceLastTarget=T->GetActorLocation();
    const FVector Toward=(T->GetActorLocation()-GetActorLocation()).GetSafeNormal2D();
    const bool Critical=(CombatBenefits()&&bNextCritical)||FMath::FRand()<ASRules::CriticalChance(Rank(19),RunRank(5));if(CombatBenefits()) bNextCritical=false;
    PounceCue(2,T->GetActorLocation(),Toward);
    if(CVarASPounceLog.GetValueOnGameThread()) UE_LOG(LogTemp,Display,TEXT("AshenPounce: %s latched onto %s (hero %s, target %s)"),*GetName(),*GetNameSafe(T),*GetActorLocation().ToString(),*T->GetActorLocation().ToString()); // before the blow: it may kill the target
    HitEnemy(T,ASArsenal::Damage(Build.Weapon,PrimaryDamageBonus())*ASClaws::StrikeDamageScale,Critical,ASData::WeaponElement(Build.Weapon),true); // the arrival strike
    ForceNetUpdate();
}
void AASCharacter::TickPounce(float Now,float Delta) {
    auto* S=AS::State(this);auto* M=Movement();
    if(PounceState==0) return;
    if(Build.Weapon!=ASRules::Claws||bDowned||!S||!CanUseWeapons()||bReviving||!S->Arena||!ASMelee::ContextValid(PendingMeleePhase,int32(S->Phase),PendingMeleeGeneration,S->Arena->Config.Epoch)) {CancelPounce(ASClaws::Release::Interrupted);return;}
    if(PounceState==1) {
        if(Now<PounceStateAt+ASClaws::Windup) return;
        // the leap: toward the target's ring as it is now (it may have moved during the crouch), else the planned spot
        FVector Landing=PounceLanding,Outward=PounceOutward;
        AASEnemy* T=PounceTarget;
        if(IsValid(T)&&T->Health>0&&CanDamage(T)) {FVector L,O;if(FindPounceLanding(T,GetActorLocation(),L,O)) {Landing=L;Outward=O;}}
        else T=nullptr;
        PounceTarget=T;PounceOutward=Outward;PounceLanding=Landing;
        const FVector From=GetActorLocation();const float Distance=float(FVector::Dist(From,Landing));
        const float Seconds=ASClaws::FlightSeconds(Distance),Arc=ASClaws::ArcHeight(Distance)+FMath::Max(0.f,float(Landing.Z-From.Z)); // clear a lip as high as the landing
        PounceState=2;PounceStateAt=Now;PounceFlightSeconds=PounceFlight=Seconds;
        InvulnerableUntil=FMath::Max(InvulnerableUntil,Now+Seconds+ASClaws::FlightProtection);
        PounceMotion(true,Landing,Outward,Seconds,Arc);PounceCue(1,From,(Landing-From).GetSafeNormal());
        if(CVarASPounceLog.GetValueOnGameThread()) UE_LOG(LogTemp,Display,TEXT("AshenPounce: %s leaps %.0f cm in %.2fs (arc %.0f) to %s"),*GetName(),Distance,Seconds,Arc,*Landing.ToString());
        ForceNetUpdate();return;
    }
    if(PounceState==2) {
        if(Now>=PounceStateAt+PounceFlightSeconds+.02f||(Now>PounceStateAt+.1f&&FVector::DistSquared(GetActorLocation(),PounceLanding)<30.f*30.f)) BeginLatch(Now);
        else if(Now>PounceStateAt+PounceFlightSeconds+2.f) CancelPounce(ASClaws::Release::Lost); // never a stuck leap
        return;
    }
    // state 3: latched
    AASEnemy* T=PounceTarget;ASClaws::LatchFrame F;F.Elapsed=Now-PounceStateAt;
    F.TargetAlive=IsValid(T)&&T->Health>0;F.TargetTouchable=F.TargetAlive&&CanDamage(T);
    if(F.TargetAlive) {
        const FVector TP=T->GetActorLocation();F.TargetJump=float(FVector::Dist(TP,PounceLastTarget));PounceLastTarget=TP;F.TargetLaunched=T->Launched(Now);
        const float Ring=ASClaws::RingRadius(GetCapsuleComponent()->GetScaledCapsuleRadius(),T->GetCapsuleComponent()->GetScaledCapsuleRadius());
        const auto P=ASClaws::RingPoint({float(TP.X),float(TP.Y)},{float(PounceOutward.X),float(PounceOutward.Y)},Ring);
        const FVector Me=GetActorLocation();
        PounceOffRing=std::hypot(Me.X-P.X,Me.Y-P.Y)>ASClaws::GlueLoseDistance?PounceOffRing+Delta:0.f;
    }
    PounceAirborneFor=M->IsMovingOnGround()?0.f:PounceAirborneFor+Delta;
    F.OffRingSeconds=PounceOffRing;F.AirborneSeconds=PounceAirborneFor;F.Dashed=M->DashRemaining>0;
    const auto Why=ASClaws::ReleaseReason(F);
    if(Why!=ASClaws::Release::None) CancelPounce(Why);
}
// The pounce's show, on every machine from the server's cue: 0 the crouch, 1 the launch, 2 the strike on the target (At is the target), 3 landed on nothing, 4 let go, 5 let go by a dash, 6 the
// target died under the claws. Sounds are the claw set (Tools/generate_claw_audio.py); the camera pieces belong to the wielder's own machine only.
void AASCharacter::PounceCue_Implementation(uint8 Kind,FVector_NetQuantize At,FVector_NetQuantizeNormal Direction) {
    if(GetNetMode()==NM_DedicatedServer) return;
    UWorld* World=GetWorld();auto* S=AS::State(this);if(!World||!S) return;
    const FLinearColor Blood=ASData::HitColor(EASHitKind::Bleed);const FVector Point(At),Feet=GetActorLocation()-FVector(0,0,GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-3.f);
    const auto Spawn=[World]()->AASClawFx* {return World->SpawnActor<AASClawFx>();};
    const bool Mine=IsLocallyControlled()&&!bDowned;
    if(Kind==0) {
        ASClawFx::Sound(this,TEXT("S_PounceCoil"),1,GetActorLocation(),.6f,.96f,1.04f);
        if(auto* Dust=Spawn()) Dust->SetupDust(Feet,70.f,.2f);
    } else if(Kind==1) {
        ASClawFx::Sound(this,TEXT("S_PounceLaunch"),1,GetActorLocation(),.8f,.96f,1.05f);
        if(auto* Dust=Spawn()) Dust->SetupDust(Feet,130.f,.9f);
        if(auto* Ring=Spawn()) Ring->SetupRing(Feet,FLinearColor(.9f,.8f,.65f),190.f,.34f);
        if(Mine) {HammerShakeAge=0;HammerShakeStrength=.8f;}
    } else if(Kind==2) {
        // the arrival: both diagonals of the rake across the body, a big spray of sparks, a ring on the floor, the heaviest sound, and for the wielder a hard shake and a hit-stop
        ASClawFx::Sound(this,TEXT("S_PounceStrike"),1,Point,.95f,.97f,1.04f);
        const FVector Chest=Point+FVector(0,0,18),From=GetActorLocation()+FVector(0,0,40);
        for(int32 Step=0;Step<2;++Step) if(auto* Rake=Spawn()) Rake->SetupRake(Chest,From,Blood,Step,1.f);
        if(auto* Sparks=Spawn()) Sparks->SetupSparks(Chest,From,Blood,1.f);
        if(auto* Ring=Spawn()) Ring->SetupRing(Point-FVector(0,0,60),Blood,230.f,.4f);
        if(auto* Dust=Spawn()) Dust->SetupDust(Feet,110.f,.6f);
        if(auto* Bond=Spawn()) Bond->SetupBond(this,Blood); // lives while this hero holds on
        if(Mine) {HammerShakeAge=0;HammerShakeStrength=2.6f;}
        if(auto* Viewer=World->GetFirstPlayerController<AASPlayerController>()) if(const auto* WielderState=GetPlayerState<AASPlayerState>()) if(WielderState->PlayerIndex>=0&&WielderState->PlayerIndex<8) { // the word, in gold over the wielder's head for both players (a partner's trait banner floats the same way)
            Viewer->RewardText(FString::Printf(TEXT("@%d|POUNCE"),WielderState->PlayerIndex),FLinearColor(1.f,.78f,.22f),false);
            if(CVarASPounceLog.GetValueOnGameThread()) UE_LOG(LogTemp,Display,TEXT("AshenPounce: the strike cue reached %s (%s machine): word over player %d"),*GetName(),HasAuthority()?TEXT("the host"):TEXT("a client"),WielderState->PlayerIndex);
        }
        if(HasAuthority()) if(auto* GM=World->GetAuthGameMode<AASGameMode>()) GM->ClearBeat(.06f,ASClaws::HitStop*1.3f); // the host's hit-stop: a blink of stillness on the arrival
    } else if(Kind==3) {
        ASClawFx::Sound(this,TEXT("S_PounceLand"),1,GetActorLocation(),.7f,.97f,1.04f);
        if(auto* Dust=Spawn()) Dust->SetupDust(Feet,120.f,.8f);
        if(auto* Ring=Spawn()) Ring->SetupRing(Feet,FLinearColor(.9f,.8f,.65f),150.f,.3f);
        if(Mine) {HammerShakeAge=0;HammerShakeStrength=.9f;}
    } else {
        ASClawFx::Sound(this,TEXT("S_PounceRelease"),1,GetActorLocation(),Kind==6?.35f:.5f,Kind==6?1.12f:.96f,Kind==6?1.2f:1.04f);
        if(auto* Dust=Spawn()) Dust->SetupDust(Feet,60.f,.15f);
    }
}
// How far the pounce pushes this camera's field of view (degrees; the controller adds it): a small push in on the crouch, a rush wider in flight, a hard punch in on the arrival that settles to
// a held push while latched. Eased on the way (ClawFovNow); nothing for a player who asked for reduced motion.
float AASCharacter::ClawFovTarget(float Now) const {
    if(Build.Weapon!=ASRules::Claws||bDowned||!IsLocallyControlled()) return 0.f;
    if(const auto* GI=GetGameInstance<UASGameInstance>()) if(GI->bReduceMotion) return 0.f;
    const auto Ease=[](float T) {T=FMath::Clamp(T,0.f,1.f);return T*T*(3.f-2.f*T);};
    if(PounceState==1) return -3.5f*Ease((Now-PounceStateAt)/ASClaws::Windup);
    if(PounceState==2) return 7.f*Ease((Now-PounceStateAt)/FMath::Max(.08f,PounceFlight*.5f));
    if(PounceState==3) return -2.5f-9.f*FMath::Exp(-(Now-PounceStateAt)/.10f);
    return 0.f;
}
