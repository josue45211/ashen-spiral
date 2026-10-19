#include "ASBreakable.h"
#include "ASWorld.h"
#include "ASGameState.h"
#include "ASCharacter.h"
#include "ASEnemy.h"
#include "ASSpell.h"
#include "ASGameMode.h"
#include "ASEnemyAbilities.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
AASBreakable::AASBreakable() {
    bReplicates=true;bAlwaysRelevant=true;Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Breakable"));RootComponent=Visual;
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.bStartWithTickEnabled=false; // only brood eggs tick (RefreshLook turns it on for them)
}
void AASBreakable::Spawn(UWorld* World,const FVector& At,int32 Type) {
    const auto* S=AS::State(World);if(!S||World->GetNetMode()==NM_Client||S->Breakables.Num()>=24) return;
    if(World->OverlapBlockingTestByChannel(At+FVector(0,0,62),FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeBox(FVector(48,48,60)))) return;
    const FTransform T(At);auto* A=World->SpawnActorDeferred<AASBreakable>(StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(A) {A->Kind=FMath::Clamp(Type,0,1);UGameplayStatics::FinishSpawningActor(A,T);}
}
void AASBreakable::BeginPlay() {Super::BeginPlay();if(auto* S=AS::State(this)) S->Breakables.AddUnique(this);RefreshLook();}
void AASBreakable::EndPlay(const EEndPlayReason::Type Reason) {if(auto* S=AS::State(this)) S->Breakables.Remove(this);Super::EndPlay(Reason);}
void AASBreakable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASBreakable,Kind);DOREPLIFETIME(AASBreakable,Health);DOREPLIFETIME(AASBreakable,HatchAt);}
AASBreakable* AASBreakable::LayEgg(UWorld* World,const FVector& At,AASEnemy* Mother) {
    auto* S=AS::State(World);if(!S||!World||World->GetNetMode()==NM_Client||S->Phase!=EASPhase::Combat||At.ContainsNaN()) return nullptr;
    int32 Eggs=0;for(auto W:S->Breakables) if(const auto* B=W.Get()) if(B->Kind==2) ++Eggs;
    if(Eggs>=ASEnemyRules::EggMaxLive||S->Breakables.Num()>=32) return nullptr;
    const FTransform T(At);auto* Egg=World->SpawnActorDeferred<AASBreakable>(StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!Egg) return nullptr;
    Egg->Kind=2;Egg->Health=ASEnemyRules::EggHealth;Egg->Mother=Mother;Egg->HatchAt=S->SimTime+ASEnemyRules::EggHatchSeconds;
    UGameplayStatics::FinishSpawningActor(Egg,T);return Egg;
}
// A brood egg pulses faster and brighter as it nears its hatch (every machine, from the replicated hatch time); on the server it hatches into a minion when the time comes, or
// is simply removed if the fight is over.
void AASBreakable::Tick(float Delta) {
    Super::Tick(Delta);
    if(Kind!=2||bBroken) return;
    auto* S=AS::State(this);if(!S||S->Frozen()) return;
    const float Progress=ASEnemyRules::EggProgress(S->SimTime,HatchAt,ASEnemyRules::EggHatchSeconds);
    if(GetNetMode()!=NM_DedicatedServer&&Visual) {
        const float Pulse=1.f+.06f*(1.f+2.f*Progress)*FMath::Sin(S->SimTime*(6.f+14.f*Progress));
        Visual->SetRelativeScale3D(FVector(.55f,.55f,.72f)*Pulse);
        if(auto* M=Cast<UMaterialInstanceDynamic>(Visual->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),2.f+5.f*Progress);
    }
    if(!HasAuthority()) return;
    if(S->Phase!=EASPhase::Combat) {bBroken=true;Destroy();return;}
    if(ASEnemyRules::EggHatchDue(S->SimTime,HatchAt)) Hatch();
}
void AASBreakable::Hatch() {
    auto* S=AS::State(this);if(!S||bBroken) return;
    bBroken=true;const FVector At=GetActorLocation();
    if(auto* GM=GetWorld()->GetAuthGameMode<AASGameMode>()) GM->SpawnMinion(At,Mother.Get()); // the nearest free spot; nothing hatches past the enemy cap
    S->Effect(11,At+FVector(0,0,20),FVector::UpVector,FLinearColor(.2f,1.f,.35f),1.f);S->Effect(19,At,FVector::UpVector,FLinearColor(.2f,1.f,.35f),1.f); // hatch: the shell bursts and the hatch cue plays
    Destroy();
}
void AASBreakable::RefreshLook() {
    SetActorTickEnabled(Kind==2);
    if(Kind==2) { // a brood egg (0.24): green, small, and it does not block walkers (only hits and shots reach it)
        ASAssets::Style(Visual,TEXT("Sphere"),FLinearColor(.18f,.85f,.30f),.1f,2.f);Visual->SetRelativeScale3D(FVector(.55f,.55f,.72f));
        Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Visual->SetCollisionObjectType(ECC_WorldStatic);Visual->SetCollisionResponseToAllChannels(ECR_Block);
        Visual->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);Visual->SetCanEverAffectNavigation(false);Visual->SetCastShadow(false);
        return;
    }
    ASAssets::Style(Visual,Kind==0?TEXT("SM_LootCrystal"):TEXT("SM_EmberCask"),Kind==0?FLinearColor(.1f,.8f,1):FLinearColor(.7f,.2f,.04f),.3f,Kind==0?1.5f:.15f);
    Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Visual->SetCollisionObjectType(ECC_WorldStatic);Visual->SetCollisionResponseToAllChannels(ECR_Block);
    Visual->SetCanEverAffectNavigation(false);
}
bool AASBreakable::VisibleFrom(const FVector& From) const {
    FCollisionQueryParams Query;Query.AddIgnoredActor(this);FHitResult Hit;
    return !GetWorld()->LineTraceSingleByObjectType(Hit,From,GetActorLocation()+FVector(0,0,55),FCollisionObjectQueryParams(ECC_WorldStatic),Query);
}
void AASBreakable::Strike(AASCharacter* Hero,float Amount) {
    auto* S=AS::State(this);if(!HasAuthority()||bBroken||!S||S->Frozen()||S->Phase!=EASPhase::Combat||!IsValid(Hero)||Hero->bDowned||Amount<=0||!FMath::IsFinite(Amount)) return;
    Health-=Amount;if(Health>0) {ForceNetUpdate();return;}
    bBroken=true;Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);const FVector At=GetActorLocation()+FVector(0,0,50);
    if(Kind==2) { // a popped brood egg: nothing hatches, and the shell bursts green
        S->Effect(11,At,FVector::UpVector,FLinearColor(.2f,1.f,.35f),1.f);S->Effect(19,At,FVector::UpVector,FLinearColor(.2f,1.f,.35f),0.f);
    } else if(Kind==0) {
        S->Cinders=ASRules::Credit(S->Cinders,4);AASPickup::Spawn(GetWorld(),At,2,2);
        S->Effect(7,At,FVector::UpVector,FLinearColor(.1f,.8f,1),120);
    } else {
        if(auto* PS=Hero->GetPlayerState<AASPlayerState>()) PS->Record(ASCareer::Casks,1,2);
        S->Effect(6,At,FVector::UpVector,FLinearColor(1,.3f,.03f),320);
        const auto Enemies=S->Enemies;
        for(auto W:Enemies) if(auto* E=W.Get()) if(Hero->CanDamage(E)&&FVector::DistSquared(At,E->GetActorLocation())<320.f*320.f&&AS::ClearSight(GetWorld(),At,E->GetActorLocation())) {
            Hero->SecondaryDamage(E,60.f+ASRules::Ward(S->Room)*12.f,EASHitKind::Burn);
            if(IsValid(E)&&E->Health>0) E->ApplyElement(EASHitKind::Burn,Hero,1.f);
        }
        for(auto W:S->Heroes) if(auto* H=W.Get()) if(FVector::DistSquared(At,H->GetActorLocation())<320.f*320.f&&AS::ClearSight(GetWorld(),At,H->GetActorLocation())) H->Hurt(16.f);
        const auto Others=S->Breakables;
        for(auto W:Others) if(auto* B=W.Get()) if(B!=this&&FVector::DistSquared(At,B->GetActorLocation())<320.f*320.f&&B->VisibleFrom(At)) B->Strike(Hero,40.f);
    }
    Destroy();
}
