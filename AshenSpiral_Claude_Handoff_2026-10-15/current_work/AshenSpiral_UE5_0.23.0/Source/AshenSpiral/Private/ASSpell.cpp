#include "ASSpell.h"
#include "ASArsenalRules.h"
#include "ASCharacter.h"
#include "ASMovementComponent.h"
#include "ASGameState.h"
#include "ASWorld.h"
#include "ASData.h"
#include "ASRules.h"
#include "ASEnemy.h"
#include "ASEnemyRules.h"
#include "ASBreakable.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "ASPresentationRules.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
AASProjectile::AASProjectile() {
    bReplicates=true;SetReplicateMovement(true);SetNetUpdateFrequency(25.f);PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Projectile"));Visual->SetupAttachment(RootComponent);Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Trail=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LuminousTrail"));Trail->SetupAttachment(RootComponent);
}
AASProjectile* AASProjectile::Fire(UWorld* W,const FVector& P,const FVector& D,AASCharacter* H,int32 Type,float Amount,bool Enemy,bool Critical,bool Secondary) {
    auto* S=AS::State(W);if(!W||!S||W->GetNetMode()==NM_Client||S->Projectiles.Num()>=ASRules::MaxProjectiles) return nullptr;
    const FTransform T(D.Rotation(),P);auto* A=W->SpawnActorDeferred<AASProjectile>(StaticClass(),T,H,H,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!A) return nullptr;A->Hero=H;A->Weapon=Type;A->bEnemy=Enemy;A->Damage=Amount;A->bCritical=Critical;A->bSecondary=Secondary;
    A->bPractice=H&&S->Phase==EASPhase::Hub;A->bCosmetic=!Enemy&&S->Phase!=EASPhase::Hub&&S->Phase!=EASPhase::Combat;
    A->Velocity=D.GetSafeNormal()*(Enemy?(Type==8?2300.f:870.f):(Type>=6?ASArsenal::Speed(Type):(Type==3?1600.f:2400.f)));
    A->Bounces=H&&!Secondary?(H->Relic(20)?1:0)+H->TraitValue(ASBuild::Trait::Ricochet):0;
    A->Pierces=H&&!Secondary?H->TraitValue(ASBuild::Trait::Pierce):0;
    if(!Enemy&&Type==ASArsenal::Launcher) {A->Bounces=0;A->Pierces=0;}
    UGameplayStatics::FinishSpawningActor(A,T);return A;
}
void AASProjectile::BeginPlay() {
    Super::BeginPlay();if(auto* S=AS::State(this)) S->Projectiles.AddUnique(this);Expires=AS::Time(this)+(!bEnemy&&Weapon>=6?ASArsenal::Lifetime(Weapon):4.f);
    const FLinearColor C=bEnemy?(Weapon==8?FLinearColor(1,.12f,.55f):FLinearColor(1,.14f,.04f)):(Hero?ASData::HitColor(ASData::WeaponElement(Weapon)):FLinearColor(1,.6f,.2f));
    ASAssets::Style(Visual,!bEnemy&&Weapon==2?TEXT("SM_Arrow"):(Weapon==3&&!bEnemy?TEXT("SM_Slash"):TEXT("Sphere")),C,0,5.f);
    Visual->SetRelativeScale3D(!bEnemy&&Weapon==2?FVector(.95f):(bEnemy?FVector(.29f):FVector(.58f,.24f,.24f)));
    ASAssets::Style(Trail,TEXT("Cube"),C,0,7.f);Trail->SetRelativeLocation(FVector(-85,0,0));Trail->SetRelativeScale3D(FVector(1.7f,.075f,.075f));
    if(!bEnemy&&Weapon>=6) {
        Visual->SetRelativeScale3D(Weapon==ASArsenal::Launcher?FVector(.60f,.20f,.20f):FVector(.18f,.05f,.05f));
        Trail->SetRelativeLocation(FVector(-35,0,0));Trail->SetRelativeScale3D(Weapon==ASArsenal::Launcher?FVector(.9f,.06f,.06f):FVector(.5f,.025f,.025f));
    }
}
void AASProjectile::Detonate(const FVector& Position,AASEnemy* DirectTarget) {
    auto* State=AS::State(this);if(!State||bEnemy||Weapon!=ASArsenal::Launcher) return;
    const float Radius=240.f*(IsValid(Hero)?Hero->AreaScale():1.f);
    State->Effect(6,Position,FVector::UpVector,FLinearColor(1,.35f,.07f),Radius);
    if(!bCosmetic&&IsValid(Hero)) {
        bool ProcAvailable=!bSecondary;const auto Targets=State->Enemies;
        // Apply a direct hit first so exactly one target owns this rocket's proc budget.
        if(IsValid(DirectTarget)&&Hero->CanDamage(DirectTarget)) {
            Hero->HitEnemy(DirectTarget,Damage,bCritical,ASData::WeaponElement(Weapon),true,ProcAvailable);ProcAvailable=false;
        }
        for(auto Weak:Targets) if(auto* Enemy=Weak.Get()) if(Enemy!=DirectTarget&&Hero->CanDamage(Enemy)&&Enemy->Health>0) {
            const float Distance=FVector::Dist(Position,Enemy->GetActorLocation());
            if(Distance<=Radius&&AS::ClearSight(GetWorld(),Position,Enemy->GetActorLocation())) {
                Hero->HitEnemy(Enemy,Damage*ASArsenal::BlastFalloff(Distance,Radius),bCritical,ASData::WeaponElement(Weapon),true,ProcAvailable);ProcAvailable=false;
            }
        }
        const auto Props=State->Breakables;
        for(auto Weak:Props) if(auto* Prop=Weak.Get()) {
            const float Distance=FVector::Dist(Position,Prop->GetActorLocation());
            if(Distance<=Radius&&Prop->VisibleFrom(Position)) Prop->Strike(Hero,Damage*ASArsenal::BlastFalloff(Distance,Radius)*Hero->DamageScale(true));
        }
    }
    Destroy();
}
void AASProjectile::EndPlay(const EEndPlayReason::Type R) {if(auto* S=AS::State(this)) S->Projectiles.Remove(this);Super::EndPlay(R);}
void AASProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASProjectile,Velocity);DOREPLIFETIME(AASProjectile,bHoming);DOREPLIFETIME(AASProjectile,Hero);DOREPLIFETIME(AASProjectile,bEnemy);
    DOREPLIFETIME(AASProjectile,Weapon);DOREPLIFETIME(AASProjectile,Damage);DOREPLIFETIME(AASProjectile,bCritical);DOREPLIFETIME(AASProjectile,bCosmetic);DOREPLIFETIME(AASProjectile,bPractice);DOREPLIFETIME(AASProjectile,bSecondary);
}
void AASProjectile::Tick(float Delta) {
    Super::Tick(Delta);auto* S=AS::State(this);if(!S||S->Frozen()) return;
    if(bHoming&&Visual) Visual->SetRelativeScale3D(FVector(.85f));
    if(!HasAuthority()) {SetActorLocation(GetActorLocation()+Velocity*Delta);SetActorRotation(Velocity.Rotation());return;}
    if(!bCosmetic&&S->Phase!=(bPractice?EASPhase::Hub:EASPhase::Combat)) {Destroy();return;}
    if(S->SimTime>Expires) {if(!bEnemy&&Weapon==ASArsenal::Launcher) Detonate(GetActorLocation());else Destroy();return;}
    if(!bCosmetic&&!bEnemy&&!bSecondary&&IsValid(Hero)) for(auto W:S->Wells) if(W.IsValid()&&W->CanAffectGameplay()&&FVector::DistSquared2D(GetActorLocation(),W->GetActorLocation())<FMath::Square(W->Radius)) {
        if(Weapon==3) {W->OrbitUntil=S->SimTime+2.5f;Destroy();return;}
        break;
    }
    if(bHoming&&HomingTarget.IsValid()&&!HomingTarget->bDowned) {
        // Limited turn rate: walking cannot shake it, a dash can.
        const FVector Cur=Velocity.GetSafeNormal(),Want=(HomingTarget->GetActorLocation()-GetActorLocation()).GetSafeNormal();
        const float Angle=FMath::Acos(FMath::Clamp(FVector::DotProduct(Cur,Want),-1.f,1.f)),Step=FMath::Min(Angle,FMath::DegreesToRadians(75.f)*Delta);
        if(Angle>KINDA_SMALL_NUMBER) {const FVector Axis=FVector::CrossProduct(Cur,Want).GetSafeNormal();if(!Axis.IsNearlyZero()) Velocity=Cur.RotateAngleAxisRad(Step,Axis)*Velocity.Size();}
    }
    const FVector Start=GetActorLocation(),End=Start+Velocity*Delta;
    const float FrameTravel=FVector::Dist(Start,End);
    FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Params;Params.AddIgnoredActor(this);if(Hero) Params.AddIgnoredActor(Hero);if(EnemySource.IsValid()) Params.AddIgnoredActor(EnemySource.Get());
    for(auto A:HitActors) if(A.IsValid()) Params.AddIgnoredActor(A.Get());
    // Re-query after every penetrated target, including on a long frame. Never
    // skip a second pawn because a trace ended at the first blocking contact.
    for(int32 Pass=0;Pass<8;++Pass) {
        FHitResult Hit;if(!GetWorld()->SweepSingleByObjectType(Hit,Start,End,FQuat::Identity,Objects,FCollisionShape::MakeSphere(9.f),Params)) break;
        AActor* Actor=Hit.GetActor();
        if(auto* H=Cast<AASCharacter>(Actor)) {
            if(bEnemy) {H->Hurt(Damage,EnemySource.Get(),true);Destroy();return;}
            Params.AddIgnoredActor(H);continue;
        }
        if(!bEnemy&&Weapon==ASArsenal::Launcher) {Detonate(Hit.ImpactPoint+Hit.ImpactNormal*12.f,Cast<AASEnemy>(Actor));return;}
        if(auto* E=Cast<AASEnemy>(Actor)) {
            Params.AddIgnoredActor(E);HitActors.Add(E);if(bEnemy) continue;
            const FVector Point=Hit.ImpactPoint;
            if(!bCosmetic&&IsValid(Hero)&&Hero->CanDamage(E)) {
                if(Weapon==4) E->MarkUntil=S->SimTime+3;
                const float Falloff=Weapon==ASArsenal::Scattergun?ASArsenal::ScatterFalloff(Travelled+FrameTravel*Hit.Time):1.f;
                Hero->HitEnemy(E,Damage*Falloff,bCritical,ASData::WeaponElement(Weapon),true,!bSecondary);
                if(!bSecondary&&!bSplit&&FMath::FRand()<ASBuild::ProcChance(Hero->TraitValue(ASBuild::Trait::Split),.18f)) {
                    bSplit=true;for(float A:{-24.f,24.f}) if(auto* Child=Fire(GetWorld(),Point+Velocity.GetSafeNormal()*20,Velocity.RotateAngleAxis(A*Hero->SpreadScale(),FVector::UpVector),Hero,Weapon,Damage*.4f,false,bCritical,true)) Child->HitActors=HitActors;
                }
            }
            if(Pierces>0) {--Pierces;continue;}
            if(Bounces>0&&IsValid(Hero)) {
                AASEnemy* Next=nullptr;float Distance=750*750;
                for(auto W:S->Enemies) if(auto* N=W.Get()) if(!HitActors.Contains(N)&&Hero->CanDamage(N)&&N->Health>0) {float D=FVector::DistSquared(Point,N->GetActorLocation());if(D<Distance&&AS::ClearSight(GetWorld(),Point,N->GetActorLocation())) {Next=N;Distance=D;}}
                if(Next) {--Bounces;Velocity=(Next->GetActorLocation()-Point).GetSafeNormal()*Velocity.Size();SetActorLocation(Point+Velocity.GetSafeNormal()*14);SetActorRotation(Velocity.Rotation());return;}
            }
            Destroy();return;
        }
        if(auto* Prop=Cast<AASBreakable>(Actor)) {if(!bCosmetic&&!bEnemy&&IsValid(Hero)) Prop->Strike(Hero,Damage*Hero->DamageScale(true));Destroy();return;}
        if(Bounces>0) {--Bounces;Velocity=FMath::GetReflectionVector(Velocity,Hit.ImpactNormal);SetActorLocation(Hit.ImpactPoint+Hit.ImpactNormal*14);SetActorRotation(Velocity.Rotation());return;}
        Destroy();return;
    }
    Travelled+=FrameTravel;SetActorLocation(End);SetActorRotation(Velocity.Rotation());
}
AASWell::AASWell() {
    bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;SetReplicateMovement(true);
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Ring=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OuterRing"));Ring->SetupAttachment(RootComponent);
    Inner=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Orbit"));Inner->SetupAttachment(RootComponent);
    Core=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Singularity"));Core->SetupAttachment(RootComponent);
}
AASWell* AASWell::Plant(AASCharacter* H,const FVector& P,bool Perfect,float Scale,float LifeOverride,bool CombatCarry) {
    if(!IsValid(H)||!H->HasAuthority()) return nullptr;auto* S=AS::State(H);if(!S) return nullptr;
    if(S->Wells.Num()>=ASRules::MaxWells && S->Wells[0].IsValid()) S->Wells[0]->Collapse();
    const FTransform T(FRotator::ZeroRotator,P);auto* W=H->GetWorld()->SpawnActorDeferred<AASWell>(StaticClass(),T,H,H,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!W) return nullptr;W->bPractice=S->Phase==EASPhase::Hub;W->bCosmetic=!CombatCarry&&!W->bPractice&&S->Phase!=EASPhase::Combat;W->Hero=H;W->bPerfect=Perfect;W->Radius=((Perfect?330.f:450.f)+H->Rank(4)*25)*(H->Build.Style==7?.8f:1.f)*Scale*H->AreaScale();
    float Life=4+H->Rank(5)*.25f+(H->Build.Style==7?2.f:(H->Build.Style==1?-1.f:0.f));if(H->Build.Weapon==0) Life*=.75f;
    W->Expires=S->SimTime+(LifeOverride>0?LifeOverride:Life);W->OrbitUntil=H->Relic(11)||H->Build.Style==3?W->Expires:0;
    UGameplayStatics::FinishSpawningActor(W,T);return W;
}
void AASWell::BeginPlay() {Super::BeginPlay();if(auto* S=AS::State(this)) S->Wells.AddUnique(this);}
void AASWell::EndPlay(const EEndPlayReason::Type R) {if(auto* S=AS::State(this)) S->Wells.Remove(this);Super::EndPlay(R);}
void AASWell::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASWell,Hero);DOREPLIFETIME(AASWell,Radius);DOREPLIFETIME(AASWell,Expires);
    DOREPLIFETIME(AASWell,bPerfect);DOREPLIFETIME(AASWell,bInverted);DOREPLIFETIME(AASWell,bCosmetic);DOREPLIFETIME(AASWell,bPractice);
}
bool AASWell::CanAffectGameplay() const {
    const auto* S=AS::State(this);
    return S&&!S->Frozen()&&IsValid(Hero)&&ASRules::CanDamageTarget(int(S->Phase),bPractice,bCosmetic);
}
void AASWell::Tick(float Delta) {
    Super::Tick(Delta);auto* S=AS::State(this);if(!S||S->Frozen()) return;
    if(!bStyled && IsValid(Hero)) {
        auto C=ASData::PlayerColor(Hero->ColorIndex);ASAssets::Style(Ring,TEXT("SM_Ring"),C,0,bPerfect?7:3);
        ASAssets::Style(Inner,TEXT("SM_Slash"),C,0,6);ASAssets::Style(Core,TEXT("Sphere"),C*.12f,1,.8f);bStyled=true;
    }
    Ring->SetRelativeScale3D(FVector(Radius/50,Radius/50,2));Inner->SetRelativeScale3D(FVector(Radius/85,Radius/85,1));
    Inner->SetRelativeRotation(FRotator(10,S->SimTime*(bInverted?-150:150),0));Core->SetRelativeLocation(FVector(0,0,45));
    Core->SetRelativeScale3D(FVector(.45f+.06f*FMath::Sin(S->SimTime*9)));
    if(!HasAuthority()) return;
    if(!IsValid(Hero)) {Destroy();return;}
    if(S->SimTime>=Expires) {Collapse();return;}
    if(bPractice&&S->Phase!=EASPhase::Hub) {Destroy();return;}
    if(!CanAffectGameplay()) return;
    const bool Pulse=S->SimTime>=NextTick;if(Pulse) NextTick=S->SimTime+.25f;
    const auto Enemies=S->Enemies;
    for(auto E:Enemies) if(E.IsValid()&&Hero->CanDamage(E.Get())&&E->Health>0 && FVector::DistSquared2D(GetActorLocation(),E->GetActorLocation())<FMath::Square(Radius) && AS::ClearSight(GetWorld(),GetActorLocation()+FVector(0,0,40),E->GetActorLocation())) {
        E->PulledUntil=S->SimTime+.35f;
        if(!E->Elite()&&!E->bTrainingDummy) {
            FVector Dir=(GetActorLocation()-E->GetActorLocation()).GetSafeNormal2D();
            float Pull=(bPerfect?420.f:265.f)+Hero->Rank(7)*45+(Hero->Build.Style==1?130.f:0.f);
            E->SetActorLocation(E->GetActorLocation()+Dir*Pull*Delta*(bInverted?-1.f:1.f),true);
        }
        if(Hero->Relic(14)||Hero->Build.Style==2||(Hero->Build.Weapon==4&&bPerfect)) E->SlowUntil=S->SimTime+.4f;
        if(Pulse) {
            Hero->HitEnemy(E.Get(),(bPerfect?9:4)+(OrbitUntil>S->SimTime?5:0)+(Hero->Relic(0)?3:0)+(Hero->Build.Weapon==4&&E->MarkUntil>S->SimTime?3:0));
            if(E.IsValid()&&E->Health>0&&Hero->Relic(0)) E->ApplyElement(EASHitKind::Burn,Hero,.60f*Hero->DamageScale(false));
        }
    }
    if(!Hero->CombatBenefits()) return;
    for(auto H:S->Heroes) if(H.IsValid()&&!H->bDowned&&H->Movement()->DashRemaining>0&&!Crossed.Contains(H)&&FVector::DistSquared2D(GetActorLocation(),H->GetActorLocation())<FMath::Square(Radius)) {
        Crossed.Add(H);if(H->Relic(1)) H->NextSpiral=0;if(H->Relic(15)) H->Area(H->GetActorLocation(),340,28);
        if(H->Build.Weapon==5) {H->Area(GetActorLocation(),520,65);H->InvulnerableUntil=FMath::Max(H->InvulnerableUntil,S->SimTime+.45f);}
        if(H.Get()!=Hero.Get()) {
            H->Movement()->DashRemaining=FMath::Max(.16f,H->Movement()->DashRemaining);
            if(H->Relic(23)||Hero->Relic(23)) {H->Shield=ASRules::GrantShield(H->Shield,12.f,60.f);Hero->Shield=ASRules::GrantShield(Hero->Shield,12.f,60.f);}
            if(H->Rank(15)>0&&S->SimTime>=H->NextAllyWellHeal) {H->Heal(3);H->NextAllyWellHeal=S->SimTime+3;}
        }
    }
}
void AASWell::Collapse() {
    if(!HasAuthority()||bCollapsed) return;bCollapsed=true;auto* S=AS::State(this);
    if(CanAffectGameplay()) {
        float R=Radius*(Hero->Build.Style==8?1.35f:1.f);Hero->Area(GetActorLocation(),R/Hero->AreaScale(),bPerfect?72:35);
        if(Hero->CombatBenefits()) Hero->Heal(Hero->Rank(2)+(Hero->Relic(2)?5:0)+(Hero->Build.Style==4?3:0));
        S->Effect(6,GetActorLocation(),FVector::ForwardVector,ASData::PlayerColor(Hero->ColorIndex),R);
        if(Hero->CombatBenefits()&&Hero->Relic(7)) for(int32 I=0;I<6;++I) AASProjectile::Fire(GetWorld(),GetActorLocation()+FVector(0,0,60),FRotator(0,I*60,0).Vector(),Hero,2,16);
        if(Hero->CombatBenefits()&&Hero->Relic(6)&&bPerfect) AASField::Spawn(GetWorld(),3,GetActorLocation(),R/Hero->AreaScale(),35,.35f,.6f,Hero);
        if(Hero->CombatBenefits()&&Hero->Relic(12)) for(auto W:S->Wells) if(W.IsValid()&&W->CanAffectGameplay()&&W.Get()!=this&&FVector::DistSquared2D(GetActorLocation(),W->GetActorLocation())<900*900) W->Expires=FMath::Max(S->SimTime+.05f,W->Expires-1);
        if(Hero->CombatBenefits()&&Hero->Relic(21)) for(auto P:S->Pickups) if(P.IsValid()&&FVector::DistSquared2D(GetActorLocation(),P->GetActorLocation())<FMath::Square(R*2)) P->SetActorLocation(Hero->GetActorLocation());
    }
    Destroy();
}
AASField::AASField() {
    bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;SetReplicateMovement(true);
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));Ring=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TellRing"));Ring->SetupAttachment(RootComponent);
    for(int32 I=0;I<9;++I) {auto* C=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Spike%d"),I));C->SetupAttachment(RootComponent);Spikes.Add(C);}
}
AASField* AASField::Spawn(UWorld* W,int32 Type,const FVector& At,float R,float D,float Delay,float Life,AASCharacter* H,const FVector& EndPoint) {
    if(!W||W->GetNetMode()==NM_Client) return nullptr;
    if(auto* S=AS::State(W)) if(S->Fields.Num()>=64) return nullptr;
    FTransform T(FRotator::ZeroRotator,At);auto* A=W->SpawnActorDeferred<AASField>(StaticClass(),T,H,H,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!A) return nullptr;A->Kind=Type;if(auto* State=AS::State(W)) A->Tier=ASRules::Ward(State->Room);A->Radius=R;A->Damage=D;A->FireAt=AS::Time(W)+Delay;A->Expires=AS::Time(W)+Life;A->Hero=H;A->End=EndPoint;
    if(auto* S=AS::State(W)) {A->bPractice=H&&S->Phase==EASPhase::Hub;A->bCosmetic=H&&!A->bPractice&&S->Phase!=EASPhase::Combat;}
    UGameplayStatics::FinishSpawningActor(A,T);return A;
}
void AASField::BeginPlay() {
    Super::BeginPlay();if(auto* S=AS::State(this)) S->Fields.AddUnique(this);
    const FLinearColor C=Kind==8?FLinearColor(.22f,1,.25f):(Kind==5||Kind==7?FLinearColor(1,.25f,.35f):FLinearColor(1,.3f,.035f));
    ASAssets::Style(Ring,TEXT("SM_Ring"),C,0,5);Ring->SetRelativeScale3D(FVector(Radius/50,Radius/50,1));
    for(int32 I=0;I<9;++I) {
        ASAssets::Style(Spikes[I],(Kind==5||Kind==7)?(I==0?TEXT("SM_SawDisc"):TEXT("Cube")):(Kind==6?(I==0?TEXT("SM_WallNozzle"):TEXT("Cube")):TEXT("Cone")),Kind==4?FLinearColor(1,.25f,.02f):FLinearColor(.3f,.28f,.25f),.7f,Kind==4?5:0);
        Spikes[I]->SetVisibility(Kind==2);
    }
}
void AASField::EndPlay(const EEndPlayReason::Type R) {if(auto* S=AS::State(this)) S->Fields.Remove(this);Super::EndPlay(R);}
float AASField::Cycle() const {return ASBuild::TrapPhase(AS::Time(this)*ASAdventure::TrapSpeed(Tier),GetActorLocation().X*.0003f+GetActorLocation().Y*.0002f+30.f);}
bool AASField::Disabled() const {const auto* S=AS::State(this);return bVault&&S&&S->VaultMask==3;}
void AASField::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASField,Tier);DOREPLIFETIME(AASField,Kind);DOREPLIFETIME(AASField,Radius);DOREPLIFETIME(AASField,Damage);
    DOREPLIFETIME(AASField,FireAt);DOREPLIFETIME(AASField,Expires);DOREPLIFETIME(AASField,End);DOREPLIFETIME(AASField,Hero);DOREPLIFETIME(AASField,bCosmetic);DOREPLIFETIME(AASField,bPractice);DOREPLIFETIME(AASField,bVault);
}
void AASField::Tick(float Delta) {
    Super::Tick(Delta);auto* S=AS::State(this);if(!S||S->Frozen()) return;
    // Echo stores base radius; draw and damage each apply the owner area scale once.
    const float DisplayRadius=Kind==3&&IsValid(Hero)?Radius*Hero->AreaScale():Radius;
    const float Now=S->SimTime;bool Active=Now>=FireAt;
    if(Kind==6) {
        const float Phase=Cycle();Active=Phase>=2.f&&Phase<3.4f;const bool Warning=Phase>=1.f&&Phase<2.f;
        const FVector V=End-GetActorLocation();Ring->SetVisibility(false);
        Spikes[0]->SetVisibility(true);Spikes[0]->SetRelativeRotation(V.Rotation());
        Spikes[1]->SetVisibility(Active||Warning);Spikes[1]->SetRelativeLocation(V*.5f);Spikes[1]->SetRelativeRotation(V.Rotation());
        Spikes[1]->SetRelativeScale3D(FVector(V.Size()/100.f,Active?.56f:.05f,Active?.56f:.05f));
        if(auto* M=Cast<UMaterialInstanceDynamic>(Spikes[1]->GetMaterial(0))) {M->SetVectorParameterValue(TEXT("Tint"),Warning?FLinearColor(1,.8f,.08f):FLinearColor(1,.15f,.015f));M->SetScalarParameterValue(TEXT("Emission"),4.f);}
    } else if(Kind==7) {
        Active=true;const FVector V=End-GetActorLocation();const float T=ASAdventure::Shuttle(Now,ASAdventure::TrapSpeed(Tier));
        Ring->SetVisibility(true);Ring->SetRelativeLocation(V*T);Ring->SetRelativeScale3D(FVector(DisplayRadius/50.f,DisplayRadius/50.f,1));
        Spikes[0]->SetVisibility(true);Spikes[0]->SetRelativeLocation(V*T+FVector(0,0,24));Spikes[0]->SetRelativeScale3D(FVector(DisplayRadius/63.f));Spikes[0]->SetRelativeRotation(FRotator(0,Now*400.f,0));
        Spikes[1]->SetVisibility(true);Spikes[1]->SetRelativeLocation(V*.5f-FVector(0,0,9));Spikes[1]->SetRelativeRotation(V.Rotation());Spikes[1]->SetRelativeScale3D(FVector(V.Size()/100.f,.09f,.08f));
    } else if(Kind==4||Kind==5) {
        const float Phase=Cycle();Active=!Disabled()&&(Kind==4?ASBuild::FlameActive(Phase):ASBuild::BladeActive(Phase));
        const bool Warning=!Disabled()&&Phase>=(Kind==4?1.f:.5f)&&Phase<(Kind==4?2.f:1.5f);
        Ring->SetVisibility(Active||Warning);Ring->SetRelativeScale3D(FVector(DisplayRadius/50,DisplayRadius/50,1));
        if(auto* M=Cast<UMaterialInstanceDynamic>(Ring->GetMaterial(0))) M->SetVectorParameterValue(TEXT("Tint"),Warning?FLinearColor(1,.75f,.05f):FLinearColor(1,.12f,.04f));
        if(Kind==4) for(int32 I=0;I<9;++I) {
            Spikes[I]->SetVisibility(Active);const float A=I*UE_TWO_PI/9;
            Spikes[I]->SetRelativeLocation(FVector(FMath::Cos(A)*DisplayRadius*.45f,FMath::Sin(A)*DisplayRadius*.45f,65.f));
            Spikes[I]->SetRelativeScale3D(FVector(.4f,.4f,1.3f+.35f*FMath::Sin(Now*21+I)));
        }
        else {
            Spikes[0]->SetVisibility(true);Spikes[0]->SetRelativeLocation(FVector(0,0,18));Spikes[0]->SetRelativeRotation(FRotator(0,Active?Now*240:0,0));Spikes[0]->SetRelativeScale3D(FVector(.6f));
            for(int32 I=1;I<9;++I) {Spikes[I]->SetVisibility(I<=2&&!Disabled());if(I>2) continue;
                Spikes[I]->SetRelativeLocation(FVector(0,0,35));Spikes[I]->SetRelativeRotation(FRotator(0,Now*ASAdventure::TrapSpeed(Tier)*(Active?180:0)+(I-1)*90,0));
                Spikes[I]->SetRelativeScale3D(FVector(DisplayRadius/50,Active?.14f:.07f,.08f));}
        }
    } else if(Kind==2) {
        const float Phase=FMath::Fmod(Now+GetActorLocation().X*.0003f+GetActorLocation().Y*.0002f+30,5.4f);
        const float Height=Phase<4?0:(Phase<4.15f?(Phase-4)/.15f:(Phase<4.55f?1:FMath::Max(0.f,1-(Phase-4.55f)/.35f)));
        Active=Phase>=4.15f&&Phase<=4.55f;Ring->SetVisibility(Phase>=3.2f&&Phase<=4.9f);
        for(int32 I=0;I<9;++I) {Spikes[I]->SetRelativeLocation(FVector((I%3-1)*48,(I/3-1)*48,Height*65-3));Spikes[I]->SetRelativeScale3D(FVector(.20f,.20f,FMath::Max(.01f,Height*1.3f)));}
    } else if(Kind==1) {
        const FVector V=End-GetActorLocation();Ring->SetRelativeLocation(V*.5f);Ring->SetRelativeRotation(FRotator(0,V.Rotation().Yaw,0));Ring->SetRelativeScale3D(FVector(V.Size()/100,DisplayRadius/50,1));
    } else Ring->SetRelativeScale3D(FVector(DisplayRadius/50*(Kind==3?1.f:(.92f+.08f*FMath::Sin(Now*18))),DisplayRadius/50,1));
    if(!HasAuthority()) return;
    if(Now>=Expires) {Destroy();return;}
    if(bCosmetic||Disabled()||S->Phase!=(bPractice?EASPhase::Hub:EASPhase::Combat)) return;
    if((Kind==8||Kind==9)&&IsValid(Hero)&&Active&&Now>=NextTick&&!bFired) {
        NextTick=Now+.6f;if(Kind==9) bFired=true;
        const auto List=S->Enemies;
        for(auto W:List) if(auto* E=W.Get()) if(Hero->CanDamage(E)&&FVector::DistSquared2D(GetActorLocation(),E->GetActorLocation())<Radius*Radius&&AS::ClearSight(GetWorld(),GetActorLocation()+FVector(0,0,45),E->GetActorLocation())) {
            float Power=Damage;if(Kind==8&&(E->StatusBits&16)) Power*=1.3f;if(Kind==9&&E->PulledUntil>Now) Power*=1.25f;
            Hero->HitEnemy(E,Power,false,Kind==8?EASHitKind::Poison:EASHitKind::Burn,false,false);
            if(IsValid(E)&&E->Health>0) E->ApplyElement(Kind==8?EASHitKind::Poison:EASHitKind::Burn,Hero,Hero->DamageScale(false));
        }
        if(Kind==9) {S->Effect(6,GetActorLocation(),FVector::UpVector,FLinearColor(1,.3f,.05f),Radius);Ring->SetVisibility(false);}
    } else if((Kind==6||Kind==7)&&Active) {
        NextTick=Now+.06f;
        const FVector Travel=End-GetActorLocation();
        const FVector Previous=GetActorLocation()+Travel*ASAdventure::Shuttle(Now-Delta,ASAdventure::TrapSpeed(Tier));
        const FVector Current=GetActorLocation()+Travel*ASAdventure::Shuttle(Now,ASAdventure::TrapSpeed(Tier));
        for(auto W:S->Heroes) if(auto* H=W.Get()) if(!H->bDowned) {
            const FVector At=H->GetActorLocation();
            const bool Contact=Kind==6?FVector::DistSquared(At,FMath::ClosestPointOnSegment(At,GetActorLocation(),End))<62.f*62.f:
                (FMath::Abs(At.Z-Current.Z)<150.f&&ASBuild::SweptCircle(Previous.X,Previous.Y,Current.X,Current.Y,At.X,At.Y,Radius+34.f));
            if(Contact&&AS::ClearSight(GetWorld(),Kind==6?GetActorLocation():Current+FVector(0,0,40),At)) H->Hurt(Damage);
        }
    } else if(Kind==1 && IsValid(Hero) && Now>=NextTick) {
        NextTick=Now+.25f;const auto List=S->Enemies;
        for(auto E:List) if(E.IsValid()&&Hero->CanDamage(E.Get())&&FMath::Abs(E->GetActorLocation().Z-GetActorLocation().Z)<200.f&&FVector::DistSquared2D(E->GetActorLocation(),FMath::ClosestPointOnSegment(E->GetActorLocation(),GetActorLocation(),End))<FMath::Square(Radius)&&AS::ClearSight(GetWorld(),FMath::ClosestPointOnSegment(E->GetActorLocation(),GetActorLocation(),End)+FVector(0,0,45),E->GetActorLocation())) {
            Hero->HitEnemy(E.Get(),Damage);
            if(E.IsValid()&&E->Health>0) E->ApplyElement(EASHitKind::Burn,Hero,Hero->DamageScale(false));
        }
    } else if(Kind==3 && Active && !bFired) {bFired=true;if(IsValid(Hero)) Hero->Area(GetActorLocation(),Radius,Damage);}
    else if((Kind==0 && Active&&!bFired)||((Kind==2||Kind==4||Kind==5) && Active&&Now>=NextTick)) {
        NextTick=Now+(Kind==5?.08f:.5f);bFired=true;
        for(auto H:S->Heroes) if(H.IsValid()&&!H->bDowned&&FVector::DistSquared2D(H->GetActorLocation(),GetActorLocation())<FMath::Square(Radius)&&AS::ClearSight(GetWorld(),GetActorLocation()+FVector(0,0,60),H->GetActorLocation())) {
            if((Kind==2||Kind==5)&&H->GetActorLocation().Z-GetActorLocation().Z>160) continue; // Jump over low mechanical traps.
            if(Kind==5) {
                const FVector Relative=H->GetActorLocation()-GetActorLocation();
                if(!ASBuild::BladeContact(Now*ASAdventure::TrapSpeed(Tier),Relative.X,Relative.Y,Radius)) continue;
            }
            H->Hurt(Damage,EnemySource.Get(),Kind==0);
        }
        if(Kind==0) S->Effect(6,GetActorLocation(),FVector::ForwardVector,FLinearColor(1,.1f,.05f),Radius);
    }
}
AASPickup::AASPickup() {bReplicates=true;SetReplicateMovement(true);PrimaryActorTick.bCanEverTick=true;PickupRoot=CreateDefaultSubobject<USceneComponent>(TEXT("PickupRoot"));RootComponent=PickupRoot;Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pickup"));Visual->SetupAttachment(PickupRoot);Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
AASPickup* AASPickup::Spawn(UWorld* W,const FVector& At,int32 Type,float Value) {
    auto* S=AS::State(W);if(!W||!S||W->GetNetMode()==NM_Client||S->Pickups.Num()>=70) return nullptr;
    const FTransform T(At);auto* P=W->SpawnActorDeferred<AASPickup>(StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(P) {P->Kind=Type;P->Amount=Value;UGameplayStatics::FinishSpawningActor(P,T);}return P;
}
void AASPickup::RefreshLook() {
    ASAssets::Style(Visual,Kind==0?TEXT("SM_HealthPickup"):(Kind==1?TEXT("SM_ShieldPickup"):TEXT("SM_Crystal")),Kind==0?FLinearColor(.1f,1,.35f):(Kind==1?ASData::HitColor(EASHitKind::Shield):FLinearColor(1,.75f,.12f)),0,4);
    Visual->SetRelativeScale3D(Kind==2?FVector(.25f):FVector(1));
}
void AASPickup::BeginPlay() {Super::BeginPlay();if(auto* S=AS::State(this)) S->Pickups.AddUnique(this);Born=AS::Time(this);Expires=Born+90;RefreshLook();}
void AASPickup::EndPlay(const EEndPlayReason::Type R) {if(auto* S=AS::State(this)) S->Pickups.Remove(this);Super::EndPlay(R);}
void AASPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASPickup,Kind);DOREPLIFETIME(AASPickup,Amount);}
void AASPickup::Tick(float Delta) {
    Super::Tick(Delta);auto* S=AS::State(this);if(!S||S->Frozen()) return;
    // Toss, bounce and hover are local and cosmetic; the server only moves the pickup toward a player who needs it.
    const float Age=S->SimTime-Born;
    Visual->SetRelativeLocation(FVector(0,0,ASPresentation::PickupHeight(Age)));Visual->AddLocalRotation(FRotator(0,Delta*(65.f+160.f*FMath::Exp(-Age*2.f)),0));
    Visual->SetRelativeScale3D((Kind==2?FVector(.25f):FVector(1))*ASPresentation::PickupPulse(Age));
    if(!HasAuthority()) return;
    if(S->SimTime>=Expires) {Destroy();return;}
    if(Age>.55f) {
        AASCharacter* Puller=nullptr;float Nearest=ASPresentation::PickupMagnetRadius;
        for(auto W:S->Heroes) if(auto* H=W.Get()) if(!H->bDowned&&!(Kind==0&&H->Health>=H->MaxHealth-.1f)&&!(Kind==1&&H->Shield>=H->MaxHealth)) {
            const float D=FVector::Dist(H->GetActorLocation(),GetActorLocation());if(D<Nearest) {Nearest=D;Puller=H;}
        }
        if(Puller) {const FVector To=Puller->GetActorLocation()-GetActorLocation();SetActorLocation(GetActorLocation()+To.GetSafeNormal()*FMath::Min(To.Size(),ASPresentation::PickupMagnetSpeed(Nearest)*Delta));}
    }
    for(auto W:S->Heroes) if(auto* H=W.Get()) if(!H->bDowned&&FVector::DistSquared(H->GetActorLocation(),GetActorLocation())<170*170) {
        if(Kind==0&&H->Health>=H->MaxHealth-.1f) continue;
        if(Kind==1&&H->Shield>=H->MaxHealth) continue;
        S->Effect(uint8(12+FMath::Clamp(Kind,0,2)),H->GetActorLocation(),FVector::UpVector,Kind==0?FLinearColor(.1f,1,.35f):(Kind==1?ASData::HitColor(EASHitKind::Shield):FLinearColor(1,.75f,.12f)),Kind==2?60.f:140.f);
        if(Kind==0) H->Heal(Amount);
        else if(Kind==1) H->Shield=FMath::Min(H->MaxHealth,H->Shield+Amount);
        else {S->Cinders=ASRules::Credit(S->Cinders,FMath::RoundToInt(Amount));H->OnCinderPickup();}
        H->ForceNetUpdate();Destroy();return;
    }
}
int32 AASDashGhost::Active=0;
AASDashGhost::AASDashGhost() {PrimaryActorTick.bCanEverTick=true;Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ghost"));RootComponent=Visual;Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);Visual->SetCastShadow(false);}
void AASDashGhost::EndPlay(const EEndPlayReason::Type Reason) {Super::EndPlay(Reason);if(bCounted) {Active=FMath::Max(0,Active-1);bCounted=false;}}
void AASDashGhost::Setup(const FVector& At,float Yaw,FLinearColor Color) {
    if(Active>=ASPresentation::DashGhostCap) {Destroy();return;}
    ++Active;bCounted=true;SetActorLocation(At);SetActorRotation(FRotator(0,Yaw,0));
    ASAssets::Style(Visual,TEXT("Sphere"),Color,0,5);Visual->SetRelativeScale3D(FVector(1.3f,.5f,1.5f));
}
void AASDashGhost::SetupFromCharacter(const AASCharacter* Source,FLinearColor Color) {
    if(!Source||Active>=ASPresentation::DashGhostCap) {Destroy();return;}
    ++Active;bCounted=true;
    SetActorLocation(Source->GetActorLocation());
    TArray<UStaticMeshComponent*> Meshes;Source->CollectTrailMeshes(Meshes);
    if(Meshes.IsEmpty()) {Destroy();return;}
    // One material shared by every copy keeps the cost to a single dynamic instance per afterimage.
    Shared=ASAssets::Material(this,Color,0.f,3.5f);
    for(auto* Piece:Meshes) {
        if(!Piece||!Piece->GetStaticMesh()||Copies.Num()>=40) continue;
        auto* Copy=NewObject<UStaticMeshComponent>(this);Copy->SetStaticMesh(Piece->GetStaticMesh());
        Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision);Copy->SetGenerateOverlapEvents(false);Copy->SetCastShadow(false);
        Copy->SetMaterial(0,Shared);Copy->SetupAttachment(RootComponent);Copy->RegisterComponent();
        Copy->SetWorldTransform(Piece->GetComponentTransform());Copies.Add(Copy);
    }
    if(Copies.IsEmpty()) Destroy();
}
void AASDashGhost::Tick(float Delta) {
    Super::Tick(Delta);if(auto* S=AS::State(this)) if(S->Frozen()) return;
    Age+=Delta;if(Age>=ASPresentation::DashGhostLife) {Destroy();return;}
    const float Alpha=ASPresentation::DashGhostAlpha(Age);
    if(Copies.IsEmpty()) {
        Visual->SetRelativeScale3D(FVector(1.3f,.5f,1.5f)*ASPresentation::DashGhostShrink(Age));
        if(auto* M=Cast<UMaterialInstanceDynamic>(Visual->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),5.f*Alpha);
        return;
    }
    // The silhouette dims and thins out toward its own centre so the trail dissolves instead of popping out.
    Visual->SetRelativeScale3D(FVector(FMath::Max(.05f,ASPresentation::DashGhostShrink(Age)*(.2f+.8f*FMath::Sqrt(Alpha)))));
    if(Shared) Shared->SetScalarParameterValue(TEXT("Emission"),3.5f*Alpha);
}
int32 AASShardBurst::Active=0;
AASShardBurst::AASShardBurst() {PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));}
void AASShardBurst::EndPlay(const EEndPlayReason::Type Reason) {Super::EndPlay(Reason);if(Shards.Num()>0) Active=FMath::Max(0,Active-1);}
void AASShardBurst::Setup(const FVector& At,FLinearColor Color,float Tier) {
    // Tier: 0 normal, 1 elite, 2 boss. Capped so a swarm wipe cannot flood the scene.
    if(Active>=14) {Destroy();return;}
    const auto Burst=ASPresentation::KillBurstFor(Tier>=1.5f&&Tier<2.5f,Tier>=2.5f);
    SetActorLocation(At);Origin=At;Life=Burst.Life;Size=Burst.Size;
    FRandomStream Rand(int32(At.X*13+At.Y*7+At.Z));
    for(int32 I=0;I<Burst.Shards;++I) {
        auto* M=NewObject<UStaticMeshComponent>(this);M->SetupAttachment(RootComponent);ASAssets::Style(M,TEXT("Cube"),Color,0,7);
        M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCastShadow(false);M->RegisterComponent();
        const FVector Dir=(FVector(Rand.FRandRange(-1,1),Rand.FRandRange(-1,1),0).GetSafeNormal()*Rand.FRandRange(.5f,1.f)+FVector(0,0,Rand.FRandRange(.5f,1.1f))).GetSafeNormal();
        const float Scale=Size*float(Rand.FRandRange(.035f,.075f));const FVector Base(Scale,Scale*Rand.FRandRange(.5f,1.f),Scale*Rand.FRandRange(1.f,2.2f));M->SetRelativeScale3D(Base);
        Shards.Add({M,Dir*Burst.Speed*Rand.FRandRange(.55f,1.f),float(Rand.FRandRange(-720.f,720.f)),Base});
    }
    ++Active;
    if(Burst.StopSeconds>0.f) if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Collapse.S_Collapse"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,At,Tier>=2.5f?.7f:.4f,Tier>=2.5f?.7f:1.1f);
}
void AASShardBurst::Tick(float Delta) {
    Super::Tick(Delta);if(auto* S=AS::State(this)) if(S->Frozen()) return;
    Age+=Delta;if(Age>=Life) {Destroy();return;}
    const float Fade=ASPresentation::ShardScale(Age,Life);
    for(auto& Shard:Shards) if(Shard.Mesh) {
        const FVector Flat=FVector(Shard.Velocity.X,Shard.Velocity.Y,0)*Age;
        Shard.Mesh->SetRelativeLocation(Flat+FVector(0,0,ASPresentation::ShardHeight(Shard.Velocity.Z,Age)));
        Shard.Mesh->AddLocalRotation(FRotator(Shard.Spin*Delta,Shard.Spin*Delta*.6f,0));
        Shard.Mesh->SetRelativeScale3D(Shard.BaseScale*FMath::Max(.01f,Fade));
    }
}
AASVisualEffect::AASVisualEffect() {PrimaryActorTick.bCanEverTick=true;Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flash"));RootComponent=Visual;}
void AASVisualEffect::Setup(uint8 T,const FVector& At,const FVector& D,FLinearColor C,float R) {
    Kind=T;Size=R;SetActorLocation(At);SetActorRotation(D.Rotation());
    if(Kind==17) {
        // 0.24 swing arc: a frontal crescent whose outer edge is exactly the swing's reach (Size). The colour's alpha carries the combo step (0, 1, 2 = finisher).
        // If the mesh has not been imported on this machine yet (Setup not re-run), fall back to the older slash mesh instead of a stray cube.
        // Step 3 is a hostile (enemy) swing: flat, bright, and kept at its own colour instead of the white wash players get.
        const int32 Step=FMath::Clamp(FMath::RoundToInt(C.A),0,3);bFinisherArc=Step>=2;
        const float Tilt=Step==0?24.f:(Step==1?-24.f:0.f); // alternating diagonals, a flat finisher
        SetActorRotation(FRotator(0.f,D.Rotation().Yaw,Tilt));
        FLinearColor Tint=Step==3?FLinearColor(C.R,C.G,C.B,1.f):FMath::Lerp(FLinearColor(C.R,C.G,C.B,1.f),FLinearColor::White,.55f); // 0.24.1: a whiter, hotter core
        ASAssets::Style(Visual,ASAssets::HasMesh(TEXT("SM_SwingArc"))?TEXT("SM_SwingArc"):TEXT("SM_Slash"),Tint,0,14.f);
        Duration=bFinisherArc?.34f:.28f; // 0.24.1: a little longer, so the reach reads
        float Volume=.32f,Pitch=1.f;USoundBase* Delivered=Step==3?AASEnemy::ResolveCue(ASEnemyRules::CueSlashSwing,Volume,Pitch):nullptr; // a hostile swing uses E_SlashSwing once it is delivered
        if(Delivered) UGameplayStatics::PlaySoundAtLocation(this,Delivered,At,Volume,Pitch);
        else if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Attack.S_Attack"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,At,.32f);
        return;
    }
    if(Kind==18) {
        // 0.24 meteor: a fireball that falls from the sky onto its ground warning and lands exactly as the warning fires (Size is the fall time in seconds). Cosmetic only; the
        // blast itself is the field's. The colour is the glow of the fall.
        ASAssets::Style(Visual,TEXT("Sphere"),FLinearColor(C.R,C.G,C.B,1.f),0,10.f);Visual->SetCastShadow(false);
        Duration=FMath::Clamp(R,.2f,3.f);Anchor=At;SetActorLocation(At+FVector(0,0,1500.f));Visual->SetRelativeScale3D(FVector(.9f));
        return;
    }
    if(Kind==20) {
        // 0.24.2 (F7; playtest: the melee warning passed, "add one extra indicator that feels natural"): a weapon GLINT a beat before a blow lands - a four-pointed star that
        // flashes at the weapon's tip, faces the camera and is gone - with a faint high "shing". Size is the star's span in cm. Spawned on every machine from replicated state.
        Visual->SetVisibility(false);Duration=.24f;
        for(int32 I=0;I<2;++I) {
            auto* Bar=NewObject<UStaticMeshComponent>(this);Bar->SetupAttachment(Visual);ASAssets::Style(Bar,TEXT("Cube"),FLinearColor(C.R,C.G,C.B,1.f),0,40.f);
            Bar->SetCollisionEnabled(ECollisionEnabled::NoCollision);Bar->SetCastShadow(false);Bar->SetAbsolute(false,false,true);Bar->RegisterComponent();
            Bar->SetRelativeRotation(FRotator(0.f,0.f,I==0?0.f:90.f));Bar->SetRelativeScale3D(FVector(.01f));GlintBars[I]=Bar;
        }
        if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Hit.S_Hit"))) {
            const APawn* Local=UGameplayStatics::GetPlayerPawn(this,0);const float Gain=ASEnemyRules::CueDistanceGain(Local?float(FVector::Dist(Local->GetActorLocation(),At)):0.f);
            if(Gain>.05f) UGameplayStatics::PlaySoundAtLocation(this,Cue,At,.10f*Gain,2.4f);
        }
        return;
    }
    if(Kind==19) {
        // 0.24 brood egg sound (Size above 0.5: it hatched, otherwise a hero popped it): the egg's own burst is effect 11, this one only plays the cue, at a volume that follows
        // the distance to the local player like every enemy cue.
        Visual->SetVisibility(false);Duration=.05f;
        float Volume=.3f,Pitch=1.f;USoundBase* Cue=AASEnemy::ResolveCue(R>.5f?ASEnemyRules::CueEggHatch:ASEnemyRules::CueEggPop,Volume,Pitch);
        const APawn* Local=UGameplayStatics::GetPlayerPawn(this,0);
        Volume*=ASEnemyRules::CueDistanceGain(Local?float(FVector::Dist(Local->GetActorLocation(),At)):0.f);
        if(Cue&&Volume>=.02f) UGameplayStatics::PlaySoundAtLocation(this,Cue,At,Volume,Pitch);
        return;
    }
    ASAssets::Style(Visual,Kind==15?TEXT("SM_Chest"):(Kind==9||Kind==10?TEXT("Cube"):(Kind==1?TEXT("SM_Slash"):(Kind==2||Kind==5?TEXT("Sphere"):TEXT("SM_Ring")))),C,0,Kind==15?3:6);
    // 0.24.2: a ring flash lies flat on the floor. Callers pass UpVector meaning "facing up", but D.Rotation() pitched the ring 90 degrees, so every one of them stood up as a
    // vertical arch (seen in-engine over every elite's slam, chain, wildfire and lightning flash). A vertical direction now keeps only its yaw.
    if(Kind!=15&&Kind!=9&&Kind!=10&&Kind!=1&&Kind!=2&&Kind!=5) {
        if(FMath::Abs(D.GetSafeNormal().Z)>.7f) SetActorRotation(FRotator(0.f,GetActorRotation().Yaw,0.f));
        FHitResult Floor; // and every ring spreads on the floor under its point, a shockwave ring, instead of hovering at chest height (also seen in-engine)
        if(GetWorld()&&GetWorld()->LineTraceSingleByObjectType(Floor,At+FVector(0,0,20),At-FVector(0,0,320),FCollisionObjectQueryParams(ECC_WorldStatic))) SetActorLocation(Floor.ImpactPoint+FVector(0,0,3));
    }
    Duration=Kind==15?ASPresentation::ChestPopLife:(Kind==8?.32f:(Kind==6?.48f:(Kind>=12?.34f:.24f)));
    const TCHAR* Sound=Kind==1||Kind==2?TEXT("S_Attack"):(Kind==3?TEXT("S_Spiral"):(Kind==4?TEXT("S_Dash"):(Kind==5?TEXT("S_Hit"):(Kind==7?TEXT("S_Reward"):TEXT("S_Collapse")))));
    if(Kind==9||Kind==10) {SetActorLocation(At+D*R*.5f);Duration=.18f;return;}
    if(Kind==8) return;
    if(Kind==16) {
        // Melee hit-confirm: one faint tick per swing that connects (no visual).
        Visual->SetVisibility(false);
        if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Hit.S_Hit"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,At,.08f,1.5f);
        return;
    }
    if(Kind==15) {if(auto* Cue=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/S_Reward.S_Reward"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,At,.45f,.8f);return;}
    if(Kind>=12) {
        // Pickup chime: warm and low for health, bright for shield, a quick tick for cinders. Placeholder assets, distinct pitch.
        const TCHAR* Chime=Kind==13?TEXT("S_Hit"):TEXT("S_Reward");const float Pitch=Kind==12?.85f:(Kind==13?1.6f:1.9f);
        if(auto* Cue=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),Chime,Chime))) UGameplayStatics::PlaySoundAtLocation(this,Cue,At,Kind==14?.22f:.4f,Pitch);
        return;
    }
    if(auto* Cue=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),Sound,Sound))) UGameplayStatics::PlaySoundAtLocation(this,Cue,At,.32f);
}
void AASVisualEffect::Tick(float Delta) {
    Super::Tick(Delta);if(auto* S=AS::State(this)) if(S->Frozen()) return;
    Age+=Delta;if(Age>=Duration) {Destroy();return;}
    float T=Age/Duration;
    if(Kind==17) {
        // The crescent shows the true reach at once (a short ease-out from 90% to 100% of it), holds, then draws in toward the swinger while it dims.
        // The material is opaque, so it is hidden near the end rather than faded.
        const float Pop=FMath::Min(1.f,T*7.f),Ease=1.f-FMath::Square(1.f-Pop);
        const float Collapse=T<.62f?1.f:1.f-.4f*FMath::Square((T-.62f)/.38f);
        const float Radius=Size/50.f*(.9f+.1f*Ease)*Collapse;
        Visual->SetRelativeScale3D(FVector(Radius,Radius,bFinisherArc?2.6f:1.5f)); // 0.24.1: thicker
        if(auto* M=Cast<UMaterialInstanceDynamic>(Visual->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),(bFinisherArc?20.f:14.f)*FMath::Square(1.f-T));
        if(T>.9f) Visual->SetVisibility(false);
        return;
    }
    if(Kind==20) { // the star pops open in the first third, then its rays shrink to nothing; it always faces the local camera
        if(const APlayerController* PC=UGameplayStatics::GetPlayerController(this,0)) if(PC->PlayerCameraManager) SetActorRotation((PC->PlayerCameraManager->GetCameraLocation()-GetActorLocation()).Rotation());
        const float Open=T<.3f?T/.3f:1.f-(T-.3f)/.7f,Span=Size/100.f*FMath::Max(.02f,Open),Thin=.035f*FMath::Max(.2f,Open);
        for(const TObjectPtr<UStaticMeshComponent>& Bar:GlintBars) if(Bar) Bar->SetRelativeScale3D(FVector(Thin,Span,Thin));
        return;
    }
    if(Kind==18) {
        // It speeds up as it falls (T squared), grows a little as it nears the ground and flares in the last fifth of its fall.
        SetActorLocation(Anchor+FVector(0,0,1500.f*(1.f-T*T)));
        const float Flare=FMath::Square(FMath::Max(0.f,T-.8f)/.2f);
        Visual->SetRelativeScale3D(FVector(.9f+.6f*Flare));
        if(auto* M=Cast<UMaterialInstanceDynamic>(Visual->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),10.f+8.f*Flare);
        return;
    }
    float Scale=Size/50.f*(Kind==2||Kind==5?1-T:(Kind==8||Kind>=12?.3f+T*1.8f:.2f+T));
    Visual->SetRelativeScale3D(Kind==15?FVector(Size*ASPresentation::ChestPopScale(Age)):(Kind==9||Kind==10?FVector(Size/100.f,.05f*(1-T),.05f*(1-T)):FVector(Scale,Scale,Kind==2||Kind==5?Scale:1)));
    if(auto* M=Cast<UMaterialInstanceDynamic>(Visual->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),6*(1-T));
}
