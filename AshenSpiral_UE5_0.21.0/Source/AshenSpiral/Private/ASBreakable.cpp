#include "ASBreakable.h"
#include "ASWorld.h"
#include "ASGameState.h"
#include "ASCharacter.h"
#include "ASEnemy.h"
#include "ASSpell.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
AASBreakable::AASBreakable() {
    bReplicates=true;bAlwaysRelevant=true;Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Breakable"));RootComponent=Visual;
}
void AASBreakable::Spawn(UWorld* World,const FVector& At,int32 Type) {
    const auto* S=AS::State(World);if(!S||World->GetNetMode()==NM_Client||S->Breakables.Num()>=24) return;
    if(World->OverlapBlockingTestByChannel(At+FVector(0,0,62),FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeBox(FVector(48,48,60)))) return;
    const FTransform T(At);auto* A=World->SpawnActorDeferred<AASBreakable>(StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(A) {A->Kind=FMath::Clamp(Type,0,1);UGameplayStatics::FinishSpawningActor(A,T);}
}
void AASBreakable::BeginPlay() {Super::BeginPlay();if(auto* S=AS::State(this)) S->Breakables.AddUnique(this);RefreshLook();}
void AASBreakable::EndPlay(const EEndPlayReason::Type Reason) {if(auto* S=AS::State(this)) S->Breakables.Remove(this);Super::EndPlay(Reason);}
void AASBreakable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AASBreakable,Kind);DOREPLIFETIME(AASBreakable,Health);}
void AASBreakable::RefreshLook() {
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
    if(Kind==0) {
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
