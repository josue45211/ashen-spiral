#include "ASRouteGuide.h"
#include "ASWorld.h"
#include "ASCharacter.h"
#include "ASPlayerController.h"
#include "ASGameState.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
AASRouteGuide::AASRouteGuide() {
    PrimaryActorTick.bCanEverTick=true;bReplicates=false;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    for(int32 I=0;I<7;++I) {
        auto* Mesh=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("RouteChevron%d"),I));
        Mesh->SetupAttachment(RootComponent);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);Chevrons.Add(Mesh);
    }
}
void AASRouteGuide::BeginPlay() {
    Super::BeginPlay();
    for(int32 I=0;I<Chevrons.Num();++I) {
        ASAssets::Style(Chevrons[I],TEXT("SM_RouteArrow"),FLinearColor(1.f,.64f,.16f),.35f,I==0?2.8f:1.3f);
        Chevrons[I]->SetVisibility(false);
    }
}
void AASRouteGuide::Tick(float Delta) {
    Super::Tick(Delta);JumpAhead=false;for(auto Mesh:Chevrons) Mesh->SetVisibility(false);
    auto* PC=Cast<AASPlayerController>(GetOwner());auto* State=AS::State(this);
    auto* Hero=PC?Cast<AASCharacter>(PC->GetPawn()):nullptr;
    if(!PC||!PC->IsLocalController()||!Hero||!State||!State->Arena||PC->MenuOpen()||Hero->bDowned||State->Frozen()
        ||(State->Phase!=EASPhase::Reward&&State->Phase!=EASPhase::Victory)) {Route.Reset();return;}
    auto* Arena=State->Arena.Get();const FVector From=Hero->GetActorLocation();
    // Stop inside interaction reach, before the chest's physical collision.
    if(Arena->AtExit(From)) {Route.Reset();return;}
    const FVector Goal=Arena->ExitPoint();
    const float Now=GetWorld()->GetTimeSeconds();
    if(!Hero->GetCharacterMovement()->IsFalling()&&(Route.Num()<2||Now>=RepathAt||Generation!=Arena->Config.Generation||FVector::DistSquared2D(From,PlannedFrom)>200.f*200.f)) {
        Arena->GuideRoute(From,Goal,Route,JumpLegs,280.f);Generation=Arena->Config.Generation;PlannedFrom=From;RepathAt=Now+.30f;
    }
    if(Route.Num()<2) return;
    // Find a still-clear forward waypoint so the leading arrow follows movement
    // every frame even between route searches, including backing up at a corner.
    int32 First=1;while(First+1<Route.Num()&&FVector::DistSquared2D(From,Route[First])<80.f*80.f&&!(JumpLegs.IsValidIndex(First+1)&&JumpLegs[First+1])&&Arena->CanWalkDirectly(From,Route[First+1])) ++First;
    const FVector Ground(From.X,From.Y,0);
    if(Hero->GetCharacterMovement()->IsFalling()) for(int32 Node=1;Node<Route.Num();++Node) if(JumpLegs.IsValidIndex(Node)&&JumpLegs[Node]) {
        const FVector Leg=Route[Node]-Route[Node-1];const float T=FVector::DotProduct(Ground-Route[Node-1],Leg)/FMath::Max(1.f,float(Leg.SizeSquared()));
        if(T>=0&&T<=1.05f&&FVector::DistSquared2D(Ground,Route[Node-1]+Leg*T)<180.f*180.f) {First=Node;break;}
    }
    const bool LeadingJump=JumpLegs.IsValidIndex(First)&&JumpLegs[First];
    if(!LeadingJump&&!Arena->CanWalkDirectly(Ground,Route[First])) {RepathAt=0;return;}
    for(int32 Node=First;Node<FMath::Min(First+3,Route.Num());++Node) if(JumpLegs.IsValidIndex(Node)&&JumpLegs[Node]&&FVector::DistSquared2D(Ground,Route[Node-1])<500.f*500.f) JumpAhead=true;
    for(int32 I=0;I<Chevrons.Num();++I) {
        float Remaining=I==0?155.f:300.f+float(I-1)*135.f;FVector Begin=Ground,Point,Direction;bool Found=false;float ArcHeight=0;
        for(int32 Node=First;Node<Route.Num();++Node) {
            const FVector Leg=Route[Node]-Begin;const float Length=Leg.Size2D();
            if(Length>1.f&&Remaining<=Length) {Direction=Leg.GetSafeNormal2D();Point=Begin+Direction*Remaining;
                if(JumpLegs.IsValidIndex(Node)&&JumpLegs[Node]) {const float T=FMath::Clamp(float(FVector::Dist2D(Route[Node-1],Point)/FMath::Max(1.0,FVector::Dist2D(Route[Node-1],Route[Node]))),0.f,1.f);ArcHeight=560.f*T*(1.f-T);}
                Found=true;break;}
            Remaining-=Length;Begin=Route[Node];
        }
        if(!Found) break;
        // A raised, thick arrow near the character; smaller marks reveal turns ahead.
        Point.Z=ArcHeight+(I==0?108.f+FMath::Sin(Now*3.f)*4.f:18.f);
        Chevrons[I]->SetWorldLocation(Point);Chevrons[I]->SetWorldRotation(Direction.Rotation());
        Chevrons[I]->SetWorldScale3D(FVector(I==0?.72f:.30f));Chevrons[I]->SetVisibility(true);
    }
}
