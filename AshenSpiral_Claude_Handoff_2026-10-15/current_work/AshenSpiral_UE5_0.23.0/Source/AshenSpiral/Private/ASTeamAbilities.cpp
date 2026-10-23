#include "ASCharacter.h"
#include "ASEnemy.h"
#include "ASGameState.h"
#include "ASGameMode.h"
#include "ASPlayerController.h"
#include "ASMovementComponent.h"
#include "ASSpell.h"
#include "ASWorld.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
// 0.26 Phase 2: the team skills, on the server (rules and numbers in ASTeamRules.h, the show in ASTeamEffects.cpp):
//   Ashen Brand   (Mark & Pop)  CastBrand brands an enemy; AASEnemy::Hit stores what heroes do to it (BrandAfter); a full brand bursts on the next tick (BrandBurst)
//   Ember Tether  (Link)        CastTether binds the hero to its partner (or to a stake); TickTeam lashes whatever the line crosses and snaps it when stretched too far
//   Twin Collapse (Twin Cast)   CastTwin makes a small collapse and opens a call; the partner's cast inside the window joins it: BeginTwinCollapse pulls, FireTwinCollapse detonates
// Alone, each is weaker (ASTeamRules.h). They count as synergies: the Teamwork trait multiplies their damage like the elemental synergies', and a lash is a Shock hit, so it also feeds
// Lightning Blast and Stormbreak with the partner's Burn or Frost.
static TAutoConsoleVariable<int32> CVarASTeamLog(TEXT("as.team.log"),0,TEXT("1: log the team skills' decisions (the brand's fill and burst, the tether's lashes and snap, the Twin Collapse's join) as AshenTeam lines."),ECVF_Default);
#define AS_TEAM_LOG(...) do {if(CVarASTeamLog.GetValueOnGameThread()) UE_LOG(LogTemp,Display,__VA_ARGS__);} while(0)
namespace {
// Every player's controller on this server (the host's own and the guest's).
void ForPlayers(AASGameState* S,const TFunctionRef<void(AASPlayerController*)>& Do) {
    if(!S) return;
    for(auto P:S->PlayerArray) if(auto* Player=Cast<AASPlayerState>(P)) if(auto* PC=Cast<AASPlayerController>(Player->GetOwner())) Do(PC);
}
// The moment of a synergy-class skill, for both players: a toast and the cue (the big word, a camera punch, the sound).
void Announce(AASGameState* S,int32 Trigger,const TCHAR* Toast) {ForPlayers(S,[&](AASPlayerController* PC) {PC->Notify(Toast);PC->SynergyCue(Trigger);});}
void HitStop(UWorld* W,float Scale,float Seconds) {if(W) if(auto* GM=W->GetAuthGameMode<AASGameMode>()) GM->ClearBeat(Scale,Seconds);} // only the host has a game mode: the stop is the host's feel
}

// ---------------------------------------------------------------- Ashen Brand: the enemy's side
void AASEnemy::Brand(AASCharacter* Brander) {
    auto* S=AS::State(this);if(!HasAuthority()||!S||!IsValid(Brander)||Health<=0) return;
    for(auto W:S->Enemies) if(auto* E=W.Get()) if(E!=this&&E->BrandUntil>0.f&&E->BrandOwner.Get()==Brander) E->ClearBrand(true); // one brand per hero: the old one fizzles
    if(BrandUntil>0.f) ClearBrand(false);                                                                                       // a new brand on a branded enemy starts it over
    BrandOwner=Brander;BrandNeed=ASTeam::BrandNeed(MaxHealth,Boss());BrandStored=0.f;BrandFill=0;bBrandQueued=false;BrandUntil=S->SimTime+ASTeam::BrandSeconds;
    if(const auto* PS=Brander->GetPlayerState<AASPlayerState>()) BrandOwnerIndex=uint8(FMath::Clamp(PS->PlayerIndex,0,7));
    ForceNetUpdate();
    AS_TEAM_LOG(TEXT("AshenTeam: %s branded %s (kind %d, %.0f health): it needs %.0f stored, until %.1f"),*Brander->GetName(),*GetName(),Kind,MaxHealth,BrandNeed,BrandUntil);
    Brander->TeamCue(ASTeam::CueBrandSet,GetActorLocation(),FVector(Boss()?1.f:0.f,0.f,0.f),GetCapsuleComponent()->GetScaledCapsuleRadius());
}
void AASEnemy::ClearBrand(bool Fizzled) {
    if(!HasAuthority()||BrandUntil<=0.f) return;
    AASCharacter* Brander=BrandOwner.Get();const FVector At=GetActorLocation();
    BrandUntil=0.f;BrandFill=0;BrandStored=BrandNeed=0.f;bBrandQueued=false;BrandOwner.Reset();ForceNetUpdate();
    if(Fizzled&&IsValid(Brander)) Brander->TeamCue(ASTeam::CueBrandFizzle,At,FVector::ZeroVector,0.f);
}
// What a hit stored. A burst is queued for the next tick, never run inside the hit that filled the brand: the blow's own bookkeeping (its number, its procs, a kill) finishes first.
void AASEnemy::BrandAfter(float Dealt,AASCharacter* Source,bool Killed) {
    auto* S=AS::State(this);if(!HasAuthority()||!S||BrandUntil<=0.f||bBrandQueued||Dealt<=0.f) return;
    AASCharacter* Brander=BrandOwner.Get();
    if(!IsValid(Brander)||S->SimTime>=BrandUntil) {ClearBrand(!Killed&&IsValid(Brander));return;}
    const int32 Before=ASTeam::BrandMilestone(BrandFill);
    BrandStored+=ASTeam::BrandCharge(Dealt,Source==Brander);BrandFill=uint8(ASTeam::BrandFillPercent(BrandStored,BrandNeed));
    AS_TEAM_LOG(TEXT("AshenTeam: %s took %.0f from %s: the brand holds %.0f of %.0f (%d%%)%s"),*GetName(),Dealt,*Source->GetName(),BrandStored,BrandNeed,int32(BrandFill),Killed?TEXT(" - it died"):TEXT(""));
    if(Killed) {if(ASTeam::BrandDeathBursts(BrandStored,BrandNeed)) {bBrandQueued=true;bBrandQueuedDied=true;} else ClearBrand(false);ForceNetUpdate();return;}
    if(BrandStored>=BrandNeed) {bBrandQueued=true;bBrandQueuedDied=false;ForceNetUpdate();return;}
    const int32 After=ASTeam::BrandMilestone(BrandFill);
    if(After>Before) Brander->TeamCue(ASTeam::CueBrandTick,GetActorLocation(),FVector(float(After),0.f,0.f),float(BrandFill));
    ForceNetUpdate();
}
void AASEnemy::TickBrand(float Now) {
    if(BrandUntil<=0.f) return;
    if(const AASCharacter* Brander=BrandOwner.Get()) {if(Brander->bDowned) {ClearBrand(true);return;}} else {ClearBrand(false);return;} // a brand needs its brander up: downed or gone, it lapses
    if(bBrandQueued) {BrandBurst(bBrandQueuedDied);return;}
    if(Now>=BrandUntil) ClearBrand(true);
}
void AASEnemy::BrandBurst(bool Died) {
    auto* S=AS::State(this);AASCharacter* Brander=BrandOwner.Get();
    const float Stored=BrandStored,Max=MaxHealth;const FVector At=GetActorLocation();const bool IsBoss=Boss();
    BrandUntil=0.f;BrandFill=0;BrandStored=BrandNeed=0.f;bBrandQueued=false;BrandOwner.Reset();ForceNetUpdate(); // cleared first: nothing the burst deals can fill or burst this brand again
    if(!S||!IsValid(Brander)) return;
    const AASCharacter* Partner=Brander->FindPartner();
    const float Multiplier=ASBuild::SynergyDamageMultiplier(Brander->TeamworkRank(),Partner?Partner->TeamworkRank():0);
    const float Direct=Died?0.f:ASTeam::BrandBurstDamage(Stored,Max,IsBoss)*Multiplier,Splash=ASTeam::BrandSplashDamage(Stored,Died)*Multiplier;
    AS_TEAM_LOG(TEXT("AshenTeam: the brand on %s bursts%s: %.0f stored, %.0f on it, %.0f on each enemy within %.0f cm"),*GetName(),Died?TEXT(" as it dies"):TEXT(""),Stored,Direct,Splash,ASTeam::BrandSplashRadius);
    Brander->TeamCue(Died?ASTeam::CueBrandDeathPop:ASTeam::CueBrandPop,At,FVector::ZeroVector,ASTeam::BrandSplashRadius);
    Announce(S,ASTeam::SynergyBrand,Died?TEXT("SYNERGY: Brand Burst (as it fell)"):TEXT("SYNERGY: Brand Burst"));
    HitStop(GetWorld(),Died?.2f:.1f,Died?.035f:.06f);
    const auto Strike=[&](AASEnemy* E,float Amount) {
        if(!IsValid(E)||E->Health<=0||!Brander->CanDamage(E)||Amount<=0.f) return;
        E->BeginHitReaction(Brander->GetActorLocation(),true);
        const float Before=E->Health;const bool Killed=E->Hit(Amount,Brander,false);const float Dealt=Killed?Before:FMath::Max(0.f,Before-E->Health);
        if(Dealt>0.f) ForPlayers(S,[&](AASPlayerController* PC) {PC->SynergyNumber(E->GetActorLocation()+FVector(0,0,80),Dealt,ASTeam::SynergyBrand);});
    };
    if(Direct>0.f) Strike(this,Direct);
    const auto List=S->Enemies;
    for(auto W:List) if(auto* E=W.Get()) if(E!=this&&FVector::DistSquared2D(At,E->GetActorLocation())<FMath::Square(ASTeam::BrandSplashRadius)&&AS::ClearSight(GetWorld(),At+FVector(0,0,45),E->GetActorLocation())) Strike(E,Splash);
}

// ---------------------------------------------------------------- the casts
AASCharacter* AASCharacter::FindPartner() const {
    const auto* S=AS::State(this);if(!S) return nullptr;
    for(auto W:S->Heroes) if(AASCharacter* H=W.Get()) if(H!=this&&!H->bDowned&&!H->bEliminated&&H->Health>0) return H;
    return nullptr;
}
bool AASCharacter::LinkBuffActive() const {
    const auto* S=AS::State(this);if(!S||bDowned) return false;const float Now=S->SimTime;
    for(auto W:S->Heroes) if(const AASCharacter* H=W.Get()) {
        if(H->LinkMode!=ASTeam::LinkPair||Now>=H->LinkUntil||!IsValid(H->LinkPartner)) continue;
        if(H!=this&&H->LinkPartner.Get()!=this) continue;
        const AASCharacter* Other=H==this?H->LinkPartner.Get():H;
        if(Other&&!Other->bDowned&&ASTeam::TetherBuff(true,float(FVector::Dist(GetActorLocation(),Other->GetActorLocation())))>0.f) return true;
    }
    return false;
}
void AASCharacter::ClearTeamState() {
    LinkMode=ASTeam::LinkNone;LinkUntil=0.f;LinkPartner=nullptr;LinkAnchor=FVector::ZeroVector;TwinCallUntil=0.f;NextLash=0.f;bTwinPending=false;TwinOther.Reset();
}
// CastRune sends a team skill here: the cost is paid only when the cast has something to do (an unbranded crosshair spends nothing), and only where the skills work (a fight, the lobby dummy).
void AASCharacter::CastTeam(int32 Ability) {
    auto* S=AS::State(this);if(!S) return;
    auto* PC=Cast<AASPlayerController>(GetController());
    if(!(S->Phase==EASPhase::Hub||CombatBenefits())) {if(PC) PC->Notify(TEXT("The team skills work in a fight and on the lobby dummy."));return;}
    const float Cost=ASAdventure::AbilityCost(Ability);
    if(Focus<Cost) {if(PC) PC->Notify(TEXT("Not enough Focus. It regenerates during combat and in the lobby."));return;}
    bool Cast=false;
    switch(Ability) {case ASTeam::Brand:Cast=CastBrand();break;case ASTeam::Tether:Cast=CastTether();break;default:Cast=CastTwin();break;}
    if(!Cast) return;
    AbilityGesture();Focus-=Cost;NextRune=S->SimTime+ASAdventure::AbilityCooldown(Ability,SpiralCooldown()/3.3f);ForceNetUpdate();
}
bool AASCharacter::CastBrand() {
    auto* S=AS::State(this);auto* PC=Cast<AASPlayerController>(GetController());if(!S) return false;
    const FVector Start=MuzzleLocation(),Dir=(AimPoint-Start).GetSafeNormal();FVector End=Start+Dir*ASTeam::BrandReach;
    FHitResult Hit;FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query;Query.AddIgnoredActor(this);for(auto W:S->Heroes) if(auto* Ally=W.Get()) Query.AddIgnoredActor(Ally);
    if(GetWorld()->SweepSingleByObjectType(Hit,Start,End,FQuat::Identity,Objects,FCollisionShape::MakeSphere(14.f),Query)) End=Hit.ImpactPoint;
    AASEnemy* Target=Cast<AASEnemy>(Hit.GetActor());
    if(Target&&(!CanDamage(Target)||Target->Health<=0)) Target=nullptr;
    if(!Target) { // not exactly under the crosshair: the enemy nearest to where the aim ended, in sight
        float Best=FMath::Square(ASTeam::BrandAssist);
        for(auto W:S->Enemies) if(auto* E=W.Get()) if(CanDamage(E)&&E->Health>0) {
            const float D=float(FVector::DistSquared(End,E->GetActorLocation()));
            if(D<Best&&AS::ClearSight(GetWorld(),Start,E->GetActorLocation())) {Best=D;Target=E;}
        }
    }
    if(!Target) {if(PC) PC->Notify(TEXT("Ashen Brand: no enemy under your crosshair."));return false;}
    Target->Brand(this);
    return true;
}
bool AASCharacter::CastTether() {
    auto* S=AS::State(this);auto* PC=Cast<AASPlayerController>(GetController());if(!S) return false;
    const float Now=S->SimTime;
    if(LinkMode!=ASTeam::LinkNone) EndTether(2); // a new tether replaces the old one
    AASCharacter* Partner=FindPartner();
    const bool Pair=Partner&&FVector::Dist(GetActorLocation(),Partner->GetActorLocation())<=ASTeam::TetherFormRange;
    FVector Stake=FVector::ZeroVector;
    if(!Pair) { // alone (or the partner is too far): an ember stake where the crosshair meets the floor, within reach
        FVector Flat=AimPoint-GetActorLocation();Flat.Z=0.f;const float Length=float(Flat.Size());
        const FVector Direction=Length>1.f?Flat/Length:GetActorForwardVector();
        FVector Place=GetActorLocation()+Direction*FMath::Min(Length>1.f?Length:300.f,ASTeam::AnchorPlaceRange);
        FHitResult Wall,Ground;FCollisionObjectQueryParams Static(ECC_WorldStatic);
        if(GetWorld()->LineTraceSingleByObjectType(Wall,GetActorLocation(),Place,Static)) Place=Wall.ImpactPoint-Direction*40.f;
        if(GetWorld()->LineTraceSingleByObjectType(Ground,Place+FVector(0,0,160),Place-FVector(0,0,600),Static)) Place=Ground.ImpactPoint+FVector(0,0,8);else Place=GetActorLocation()-FVector(0,0,GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-8.f);
        Stake=Place;
        if(PC) PC->Notify(Partner?TEXT("Ember Tether: your partner is too far, so it holds to an ember stake instead (weaker)."):TEXT("Ember Tether: alone it holds to an ember stake (a softer lash, no damage bonus)."));
    }
    LinkMode=Pair?ASTeam::LinkPair:ASTeam::LinkAnchor;LinkPartner=Pair?Partner:nullptr;LinkAnchor=Stake;LinkUntil=Now+ASTeam::TetherSeconds;NextLash=Now+.15f;ForceNetUpdate();
    AS_TEAM_LOG(TEXT("AshenTeam: %s tethered %s until %.1f"),*GetName(),Pair?*Partner->GetName():*FString::Printf(TEXT("a stake at %s"),*Stake.ToCompactString()),LinkUntil);
    TeamCue(ASTeam::CueTetherOn,GetActorLocation(),Pair?Partner->GetActorLocation():Stake,Pair?1.f:0.f);
    if(Pair) Announce(S,ASTeam::SynergyTether,TEXT("SYNERGY: Ember Tether"));
    return true;
}
void AASCharacter::EndTether(int32 Why) {
    if(LinkMode==ASTeam::LinkNone) return;
    const FVector A=GetActorLocation(),B=(LinkMode==ASTeam::LinkPair&&IsValid(LinkPartner))?LinkPartner->GetActorLocation():FVector(LinkAnchor);
    AS_TEAM_LOG(TEXT("AshenTeam: %s's tether ends (%s)"),*GetName(),Why==1?TEXT("snapped"):(Why==2?TEXT("replaced"):TEXT("ran out")));
    LinkMode=ASTeam::LinkNone;LinkUntil=0.f;LinkPartner=nullptr;LinkAnchor=FVector::ZeroVector;NextLash=0.f;ForceNetUpdate();
    if(Why!=2) TeamCue(ASTeam::CueTetherEnd,A,B,Why==1?1.f:0.f);
}
bool AASCharacter::CastTwin() {
    auto* S=AS::State(this);auto* PC=Cast<AASPlayerController>(GetController());if(!S) return false;
    const float Now=S->SimTime;AASCharacter* Other=FindPartner();
    LesserCollapse(); // every cast is a collapse of its own
    if(!Other) return true; // alone it is the small collapse and nothing more
    const float Separation=float(FVector::Dist(GetActorLocation(),Other->GetActorLocation()));
    if(Other->Build.Ability==ASTeam::Twin&&ASTeam::TwinJoins(Now,Other->TwinCallUntil,Separation)) {
        AS_TEAM_LOG(TEXT("AshenTeam: %s joins %s's call (%.0f cm apart): the collapse converges"),*GetName(),*Other->GetName(),Separation);
        Other->TwinCallUntil=0.f;TwinCallUntil=0.f;Other->ForceNetUpdate();
        BeginTwinCollapse(Other);
        return true;
    }
    if(Other->Build.Ability==ASTeam::Twin&&Other->TwinCallUntil>0.f&&Now<=Other->TwinCallUntil&&PC) PC->Notify(TEXT("Twin Collapse: too far apart to join (16 m)."));
    TwinCallUntil=Now+ASTeam::TwinWindow;ForceNetUpdate();
    AS_TEAM_LOG(TEXT("AshenTeam: %s calls a Twin Collapse until %.1f (%.0f cm from %s)"),*GetName(),TwinCallUntil,Separation,*Other->GetName());
    TeamCue(ASTeam::CueTwinCall,GetActorLocation(),FVector::ZeroVector,ASTeam::TwinWindow);
    if(Other->Build.Ability==ASTeam::Twin) if(auto* OtherPC=Cast<AASPlayerController>(Other->GetController())) OtherPC->Notify(TEXT("Your partner calls a Twin Collapse: press F now to join!"));
    return true;
}
// The small collapse every cast makes: a stagger and a little damage around the caster, a few points of shield.
void AASCharacter::LesserCollapse() {
    auto* S=AS::State(this);if(!S) return;
    const FVector At=GetActorLocation();const float Radius=ASTeam::TwinLesserRadius*AreaScale();
    AS_TEAM_LOG(TEXT("AshenTeam: %s's small collapse: %.0f damage and a stagger within %.0f cm, +%.0f shield"),*GetName(),ASTeam::TwinLesserDamage,Radius,ASTeam::TwinLesserShield);
    TeamCue(ASTeam::CueTwinLesser,At,FVector::ZeroVector,Radius);
    Shield=ASRules::GrantShield(Shield,ASTeam::TwinLesserShield,MaxHealth);
    const auto List=S->Enemies;
    for(auto W:List) if(auto* E=W.Get()) if(CanDamage(E)&&E->Health>0&&FVector::DistSquared2D(At,E->GetActorLocation())<Radius*Radius&&FMath::Abs(At.Z-E->GetActorLocation().Z)<250.f&&AS::ClearSight(GetWorld(),At,E->GetActorLocation())) {
        HitEnemy(E,ASTeam::TwinLesserDamage,false,EASHitKind::Spiral,false,false);
        if(IsValid(E)&&E->Health>0) E->Control(1,E->GetActorLocation()-At,0.f);
    }
}
void AASCharacter::BeginTwinCollapse(AASCharacter* Other) {
    auto* S=AS::State(this);if(!S||!IsValid(Other)) return;
    const FVector A=GetActorLocation(),B=Other->GetActorLocation();FVector Mid=(A+B)*.5f;FHitResult Ground;
    if(GetWorld()->LineTraceSingleByObjectType(Ground,Mid+FVector(0,0,200),Mid-FVector(0,0,600),FCollisionObjectQueryParams(ECC_WorldStatic))) Mid=Ground.ImpactPoint+FVector(0,0,10);
    TwinPoint=Mid;TwinOther=Other;bTwinPending=true;TwinFireAt=S->SimTime+ASTeam::TwinDelay;NextTwinPull=S->SimTime;
    TeamCue(ASTeam::CueTwinConverge,Mid,B,ASTeam::TwinDelay);
}
void AASCharacter::FireTwinCollapse() {
    auto* S=AS::State(this);bTwinPending=false;if(!S) return;
    AASCharacter* Other=TwinOther.Get();TwinOther.Reset();const FVector Mid=TwinPoint;const float Now=S->SimTime;
    const float Multiplier=ASBuild::SynergyDamageMultiplier(TeamworkRank(),Other?Other->TeamworkRank():0);
    TeamCue(ASTeam::CueTwinBlast,Mid,FVector::ZeroVector,ASTeam::TwinRadius);
    Announce(S,ASTeam::SynergyTwin,TEXT("SYNERGY: Twin Collapse"));
    HitStop(GetWorld(),.08f,.08f);
    int32 Struck=0;const auto List=S->Enemies;
    for(auto W:List) if(auto* E=W.Get()) if(CanDamage(E)&&E->Health>0&&FVector::DistSquared2D(Mid,E->GetActorLocation())<FMath::Square(ASTeam::TwinRadius)&&AS::ClearSight(GetWorld(),Mid+FVector(0,0,45),E->GetActorLocation())) {
        const float Amount=ASTeam::TwinBlastDamage(E->MaxHealth,E->Boss())*Multiplier;
        if(!E->Boss()) {E->StunUntil=FMath::Max(E->StunUntil,Now+ASTeam::TwinStun);E->EndWindup();}
        E->BeginHitReaction(Mid,true);
        const float Before=E->Health;const bool Killed=E->Hit(Amount,this,false);const float Dealt=Killed?Before:FMath::Max(0.f,Before-E->Health);++Struck;
        if(Dealt>0.f) ForPlayers(S,[&](AASPlayerController* PC) {PC->SynergyNumber(E->GetActorLocation()+FVector(0,0,80),Dealt,ASTeam::SynergyTwin);});
    }
    for(AASCharacter* H:{this,Other}) if(IsValid(H)&&!H->bDowned) H->Shield=ASRules::GrantShield(H->Shield,ASTeam::TwinShield,H->MaxHealth);
    AS_TEAM_LOG(TEXT("AshenTeam: the Twin Collapse detonates at %s: %d enemies within %.0f cm, %.0f damage each (x%.2f), a %.1f s stun, +%.0f shield each"),*Mid.ToCompactString(),Struck,ASTeam::TwinRadius,ASTeam::TwinDamage*Multiplier,Multiplier,ASTeam::TwinStun,ASTeam::TwinShield);
}

// ---------------------------------------------------------------- the server's tick: the tether's lash and snap, the Twin Collapse's pull and detonation
void AASCharacter::TickTeam(float Now,float Delta) {
    auto* S=AS::State(this);if(!S||!HasAuthority()) return;
    if(LinkMode!=ASTeam::LinkNone) {
        AASCharacter* Partner=LinkPartner.Get();const bool Pair=LinkMode==ASTeam::LinkPair;
        if(bDowned||Now>=LinkUntil||(Pair&&(!IsValid(Partner)||Partner->bDowned||Partner->bEliminated))) EndTether(0);
        else {
            const FVector EndPoint=Pair?Partner->GetActorLocation():FVector(LinkAnchor);
            if(ASTeam::TetherSnaps(LinkMode,float(FVector::Dist(GetActorLocation(),EndPoint)))) EndTether(1);
            else if(Now>=NextLash&&(S->Phase==EASPhase::Hub||CombatBenefits())) {
                NextLash=Now+ASTeam::LashInterval;
                const FVector A=GetActorLocation()+FVector(0,0,40),B=EndPoint+FVector(0,0,Pair?40.f:50.f);
                const float Ends[2][3]={{float(A.X),float(A.Y),float(A.Z)},{float(B.X),float(B.Y),float(B.Z)}};
                const float Multiplier=ASBuild::SynergyDamageMultiplier(TeamworkRank(),Pair?Partner->TeamworkRank():0);
                int32 Cues=0,Lashed=0;const auto List=S->Enemies;
                for(auto W:List) if(auto* E=W.Get()) if(E->Health>0&&CanDamage(E)) {
                    const FVector P=E->GetActorLocation();const float Body[3]={float(P.X),float(P.Y),float(P.Z)};
                    if(!ASTeam::LashCrosses(Body,E->GetCapsuleComponent()->GetScaledCapsuleRadius(),Ends[0],Ends[1])) continue;
                    HitEnemy(E,ASTeam::LashDamageFor(Pair)*Multiplier,false,EASHitKind::Shock,false,false);++Lashed;
                    if(Cues<ASTeam::LashCuesPerPulse) {++Cues;TeamCue(ASTeam::CueTetherZap,P,FMath::ClosestPointOnSegment(P,A,B),0.f);}
                }
                if(Lashed>0) AS_TEAM_LOG(TEXT("AshenTeam: %s's tether lashes %d enemies for %.0f each (%.0f cm between its ends, damage bonus %s)"),*GetName(),Lashed,ASTeam::LashDamageFor(Pair)*Multiplier,FVector::Dist(A,B),LinkBuffActive()?TEXT("on"):TEXT("off"));
            }
        }
    }
    if(TwinCallUntil>0.f&&(Now>TwinCallUntil||bDowned)) {TwinCallUntil=0.f;ForceNetUpdate();} // an unanswered call lapses
    if(bTwinPending) {
        if(bDowned) bTwinPending=false;
        else if(Now>=TwinFireAt) FireTwinCollapse();
        else { // the pull-in: everything within the blast's reach is dragged toward the midpoint (as a well drags it: elites and bosses hold their ground)
            const auto List=S->Enemies;
            for(auto W:List) if(auto* E=W.Get()) if(E->Health>0&&CanDamage(E)&&FVector::DistSquared2D(TwinPoint,E->GetActorLocation())<FMath::Square(ASTeam::TwinRadius)) {
                E->PulledUntil=Now+.35f;
                if(!E->Elite()&&!E->bTrainingDummy) {
                    const FVector Toward=(TwinPoint-E->GetActorLocation()).GetSafeNormal2D();
                    if(FVector::DistSquared2D(TwinPoint,E->GetActorLocation())>50.f*50.f) E->SetActorLocation(E->GetActorLocation()+Toward*ASTeam::TwinPull*Delta,true);
                }
            }
        }
    }
}
