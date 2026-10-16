#include "ASCharacter.h"
#include "ASGameState.h"
#include "ASPlayerController.h"
#include "ASSpell.h"
#include "ASEnemy.h"
#include "ASWorld.h"
#include "ASBreakable.h"
#include "Engine/World.h"

void AASCharacter::CastRune_Implementation() {
    auto* S=AS::State(this);if(!S||!CanUseWeapons()||bReviving||S->SimTime<NextRune) return;
    const int32 Ability=FMath::Clamp(Build.Ability,0,ASAdventure::AbilityCount-1);
    const bool Nova=TraitValue(ASBuild::Trait::RuneNova)>0;
    const bool Active=S->Phase==EASPhase::Hub||CombatBenefits();const float Cost=ASAdventure::AbilityCost(Ability);
    if(Active&&Focus<Cost) {if(auto* PC=Cast<AASPlayerController>(GetController())) PC->Notify(TEXT("Not enough Focus. It regenerates during combat and in the lobby."));return;}
    if(Active) Focus-=Cost;
    NextRune=S->SimTime+ASAdventure::AbilityCooldown(Ability,SpiralCooldown()/3.3f);
    const FVector Start=MuzzleLocation(),Dir=(AimPoint-Start).GetSafeNormal();FVector End=Start+Dir*1500.f;
    FHitResult Hit;FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query;Query.AddIgnoredActor(this);for(auto W:S->Heroes) if(auto* Ally=W.Get()) Query.AddIgnoredActor(Ally);
    if(GetWorld()->SweepSingleByObjectType(Hit,Start,End,FQuat::Identity,Objects,FCollisionShape::MakeSphere(12.f),Query)) End=Hit.ImpactPoint;
    if(Ability==0) {
        if(Active) if(auto* B=Cast<AASBreakable>(Hit.GetActor())) B->Strike(this,48.f*DamageScale(false));
        S->Effect(10,Start,Dir,FLinearColor(.25f,.65f,1),FVector::Distance(Start,End));
        if(Nova) {if(Active) Area(End,300.f,48.f);S->Effect(6,End,FVector::UpVector,FLinearColor(.25f,.65f,1),300.f*AreaScale());}
        else if(Active) if(auto* E=Cast<AASEnemy>(Hit.GetActor())) HitEnemy(E,48.f*(E->MarkUntil>S->SimTime?1.25f:1.f),false,EASHitKind::Spiral,false,false);
    } else if(Ability==1||Ability==4) {
        const float Radius=(Ability==1?650.f:ASAdventure::NovaAreaRadius(330.f,Nova))*AreaScale();const auto Enemies=S->Enemies;
        const FVector NovaPoint=Start+Dir*FMath::Min(650.f,float(FVector::Distance(Start,End)));
        if(Ability==1&&Nova) S->Effect(6,NovaPoint,FVector::UpVector,ASData::HitColor(EASHitKind::Frost),300.f*AreaScale());
        if(Ability==1) {
            for(float Angle:{-60.f,-30.f,0.f,30.f,60.f}) S->Effect(9,Start,Dir.GetSafeNormal2D().RotateAngleAxis(Angle,FVector::UpVector),ASData::HitColor(EASHitKind::Frost),Radius);
        } else S->Effect(6,GetActorLocation()-FVector(0,0,65),Dir,ASData::HitColor(EASHitKind::Physical),Radius);
        if(Active) for(auto W:Enemies) if(auto* E=W.Get()) if(CanDamage(E)&&FMath::Abs(GetActorLocation().Z-E->GetActorLocation().Z)<250.f) {
            const bool InBase=FVector::DistSquared2D(GetActorLocation(),E->GetActorLocation())<Radius*Radius&&(Ability==4||FVector::DotProduct(Dir.GetSafeNormal2D(),(E->GetActorLocation()-GetActorLocation()).GetSafeNormal2D())>=.5f)&&AS::ClearSight(GetWorld(),Start,E->GetActorLocation());
            const bool InNova=Ability==1&&Nova&&FVector::DistSquared2D(NovaPoint,E->GetActorLocation())<FMath::Square(300.f*AreaScale())&&AS::ClearSight(GetWorld(),NovaPoint,E->GetActorLocation());
            if(!InBase&&!InNova) continue; // Each target is struck once even in the overlap.
            HitEnemy(E,Ability==1?24.f:18.f,false,Ability==1?EASHitKind::Frost:EASHitKind::Physical,false,false);
            if(IsValid(E)&&E->Health>0) {
                if(Ability==1) {E->ApplyElement(EASHitKind::Frost,this,DamageScale(false));E->ApplyElement(EASHitKind::Frost,this,DamageScale(false));}
                else E->Control(1,E->GetActorLocation()-GetActorLocation(),450.f);
            }
        }
        if(Ability==4&&Active) {Shield=ASRules::GrantShield(Shield,22.f,MaxHealth);WarCryUntil=S->SimTime+4.f;}
    } else if(Ability==2) {
        FVector Origin=End;TSet<TWeakObjectPtr<AASEnemy>> Visited;AASEnemy* Target=Cast<AASEnemy>(Hit.GetActor());
        for(int32 I=0;I<4;++I) {
            if(!Target) {
                float Best=FMath::Square((I==0?230.f:620.f)*AreaScale());
                for(auto W:S->Enemies) if(auto* E=W.Get()) if(CanDamage(E)&&E->Health>0&&!Visited.Contains(E)) {
                    const float D=FVector::DistSquared(Origin,E->GetActorLocation());
                    if(D<Best&&AS::ClearSight(GetWorld(),Origin,E->GetActorLocation())) {Best=D;Target=E;}
                }
            }
            if(!Target) break;
            const FVector At=Target->GetActorLocation();Visited.Add(Target);
            S->Effect(9,I==0?Start:Origin,(At-(I==0?Start:Origin)).GetSafeNormal(),ASData::HitColor(EASHitKind::Shock),FVector::Distance(I==0?Start:Origin,At));
            if(Active) HitEnemy(Target,38.f*FMath::Pow(.76f,float(I))*((Target->StatusBits&6)?1.3f:1.f),false,EASHitKind::Shock,false,false);
            if(Nova) {
                S->Effect(6,At,FVector::UpVector,ASData::HitColor(EASHitKind::Shock),300.f*AreaScale());
                const auto Neighbors=S->Enemies;
                if(Active) for(auto W:Neighbors) if(auto* E=W.Get()) if(CanDamage(E)&&E->Health>0&&!Visited.Contains(E)&&FVector::DistSquared(At,E->GetActorLocation())<FMath::Square(300.f*AreaScale())&&AS::ClearSight(GetWorld(),At,E->GetActorLocation())) {
                    Visited.Add(E);HitEnemy(E,38.f*FMath::Pow(.76f,float(I))*((E->StatusBits&6)?1.3f:1.f),false,EASHitKind::Shock,false,false);
                }
            }
            Origin=At;Target=nullptr;
        }
        if(Visited.IsEmpty()) S->Effect(9,Start,Dir,ASData::HitColor(EASHitKind::Shock),FVector::Distance(Start,End));
    } else {
        FVector Place=End;const FVector Flat=(End-GetActorLocation()).GetSafeNormal2D();
        if(FVector::DistSquared2D(GetActorLocation(),Place)>1000.f*1000.f) Place=GetActorLocation()+Flat*1000.f;
        FHitResult Wall,Ground;FCollisionObjectQueryParams Static(ECC_WorldStatic);
        if(GetWorld()->LineTraceSingleByObjectType(Wall,Start,Place,Static)) Place=Wall.ImpactPoint+Wall.ImpactNormal*35.f;
        if(GetWorld()->LineTraceSingleByObjectType(Ground,Place+FVector(0,0,160),Place-FVector(0,0,600),Static)) Place=Ground.ImpactPoint+FVector(0,0,12);
        else Place=GetActorLocation()-FVector(0,0,76);
        AASField::Spawn(GetWorld(),Ability==3?8:9,Place,ASAdventure::NovaAreaRadius(Ability==3?320.f:330.f,Nova)*AreaScale(),Ability==3?7.f:68.f,Ability==3?.1f:1.f,Ability==3?4.f:1.5f,this);
    }
    ForceNetUpdate();
}
