#include "ASGameMode.h"
#include "ASPlayerController.h"
#include "ASGameState.h"
#include "ASCharacter.h"
#include "ASMovementComponent.h"
#include "ASEnemy.h"
#include "ASSpell.h"
#include "ASBreakable.h"
#include "ASWorld.h"
#include "ASHUD.h"
#include "ASRules.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameSession.h"

namespace {
void CaptureTraits(AASCharacter* H,AASPlayerState* PS,FASReconnect& R) {
    R.PendingExperience=PS->PendingExperience;R.PendingCareer=PS->PendingCareer;R.RoomDamaged=PS->bRoomDamaged;R.RoomEligible=PS->bRoomEligible;
    R.Ammo=H->Ammo;R.ReloadStart=H->ReloadStart;R.ReloadEnd=H->ReloadEnd;R.NextAttack=H->NextAttack;
    R.TraitRanks=H->TraitRanks;R.EvolutionMask=H->EvolutionMask;R.LastUpgrade=H->LastUpgrade;R.Rerolls=H->Rerolls;R.CurseStacks=H->CurseStacks;
    R.Focus=H->Focus;R.NextRune=H->NextRune;R.ConduitHits=H->ConduitHits;R.PhoenixUsed=H->bPhoenixUsed;R.GoldRemainder=H->GoldRemainder;
    R.RoomBuffUntil=H->RoomBuffUntil;R.KillBuffUntil=H->KillBuffUntil;R.DashCharges=H->Movement()->DashCharges;R.DashRecharge=H->Movement()->DashRecharge;R.AirDashesUsed=H->Movement()->AirDashesUsed;
    R.WagerVote=PS->bWagerVote;R.ShrineUsed=PS->bShrineUsed;R.WarCryUntil=H->WarCryUntil;R.ReceiptQueue=PS->ReceiptQueue;
}
}
AASGameMode::AASGameMode() {
    PrimaryActorTick.bCanEverTick=true;
    DefaultPawnClass=AASCharacter::StaticClass();PlayerControllerClass=AASPlayerController::StaticClass();
    GameStateClass=AASGameState::StaticClass();PlayerStateClass=AASPlayerState::StaticClass();HUDClass=AASHUD::StaticClass();
}
void AASGameMode::BeginPlay() {
    Super::BeginPlay();auto* S=AS::State(this);if(!S) return;
    if(GameSession) GameSession->MaxPlayers=2;
    S->SessionCode=UGameplayStatics::ParseOption(OptionsString,TEXT("Session")).Left(12).ToUpper();
    if(S->SessionCode.IsEmpty()) S->SessionCode=FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(6).ToUpper();
    S->Arena=GetWorld()->SpawnActor<AASWorld>();S->Arena->Configure(-1,1103);SpawnPracticeDummy();
    for(auto W:S->Heroes) if(auto* H=W.Get()) {H->SetActorLocation(S->Arena->SpawnPoint(H->ColorIndex));H->LastSafe=H->GetActorLocation();}
}
void AASGameMode::SpawnPracticeDummy() {
    auto* S=AS::State(this);if(!S||!S->Arena||S->Phase!=EASPhase::Hub) return;
    const FTransform T(FRotator(0,-130,0),S->Arena->DummyPoint());
    auto* E=GetWorld()->SpawnActorDeferred<AASEnemy>(AASEnemy::StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(E) {E->bTrainingDummy=true;E->MaxHealth=10000;E->Health=E->MaxHealth;UGameplayStatics::FinishSpawningActor(E,T);}
}
bool AASGameMode::Host(const AASPlayerController* PC) const {
    const auto* PS=PC?PC->GetPlayerState<AASPlayerState>():nullptr;
    return PS && PS->PlayerIndex==0 && PC->IsLocalController();
}
void AASGameMode::PreLogin(const FString& Options,const FString& Address,const FUniqueNetIdRepl& UniqueId,FString& Error) {
    Super::PreLogin(Options,Address,UniqueId,Error);if(!Error.IsEmpty()) return;
    auto* S=AS::State(this);if(!S) return;
    if(S->PlayerArray.Num()>=2) {Error=TEXT("This expedition supports two players.");return;}
    bool EditorTest=false;
#if WITH_EDITOR
    EditorTest=GetWorld()->WorldType==EWorldType::PIE;
#endif
    const FString ID=UGameplayStatics::ParseOption(Options,TEXT("Profile"));
    if(!EditorTest) {
        if(UGameplayStatics::GetIntOption(Options,TEXT("Protocol"),0)!=ASData::Protocol) {Error=TEXT("Both players need Ashen Spiral UE 0.21.0 (protocol 2100).");return;}
        if(UGameplayStatics::ParseOption(Options,TEXT("Code")).ToUpper()!=S->SessionCode) {Error=TEXT("Session code does not match the host.");return;}
        if(S->Phase!=EASPhase::Hub && !Disconnected.Contains(ID)) {Error=TEXT("An expedition is already running. Join in the hub.");return;}
    }
}
FString AASGameMode::InitNewPlayer(APlayerController* PC,const FUniqueNetIdRepl& UniqueId,const FString& Options,const FString& Portal) {
    const FString Error=Super::InitNewPlayer(PC,UniqueId,Options,Portal);
    if(auto* PS=PC->GetPlayerState<AASPlayerState>()) PS->ProfileId=UGameplayStatics::ParseOption(Options,TEXT("Profile")).Left(32);
    return Error;
}
void AASGameMode::PostLogin(APlayerController* NewPlayer) {
    if(auto* PS=NewPlayer->GetPlayerState<AASPlayerState>()) {
        PS->PlayerIndex=NewPlayer->IsLocalController()?0:1;
        if(auto* S=AS::State(this)) for(auto P:S->PlayerArray) {
            auto* Other=Cast<AASPlayerState>(P);
            if(Other && Other!=PS && Other->PlayerIndex==PS->PlayerIndex) PS->PlayerIndex=1-PS->PlayerIndex;
        }
    }
    Super::PostLogin(NewPlayer);
    if(auto* PC=Cast<AASPlayerController>(NewPlayer)) PC->RequestProfile();
}
void AASGameMode::RestartPlayer(AController* Player) {
    if(!Player || Player->GetPawn()) return;
    auto* S=AS::State(this);auto* PS=Player->GetPlayerState<AASPlayerState>();
    const FVector Position=(S&&S->Arena)?S->Arena->SpawnPoint(PS?PS->PlayerIndex:0):FVector(0,0,100);
    FActorSpawnParameters Params;Params.Owner=Player;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* H=GetWorld()->SpawnActor<AASCharacter>(DefaultPawnClass,Position,FRotator::ZeroRotator,Params);
    if(H) {H->ColorIndex=PS?PS->PlayerIndex:0;H->RefreshLook();Player->Possess(H);}
}
void AASGameMode::ProfileArrived(AASPlayerController* PC,const FString& ID,const FASBuild& InBuild,int32 Experience,int32 ColorChoice) {
    auto* S=AS::State(this);auto* PS=PC?PC->GetPlayerState<AASPlayerState>():nullptr;if(!S||!PS) return;
    bool Valid=ID.Len()==32;for(TCHAR C:ID) if(!FChar::IsHexDigit(C)) Valid=false;
    if(!Valid) {PC->Notify(TEXT("Invalid profile ID. Restart with a writable save folder."));return;}
    for(auto P:S->PlayerArray) if(auto* Other=Cast<AASPlayerState>(P)) if(Other!=PS && Other->bProfileReady && Other->ProfileId==ID) {
        PC->Notify(TEXT("Duplicate profile. Use a different -ProfileSlot for two local clients."));return;
    }
    if(PS->bProfileReady && S->Phase!=EASPhase::Hub) return; // No mid-run loadout injection.
    if(!PS->ProfileId.IsEmpty() && PS->ProfileId!=ID) {PC->Notify(TEXT("Reconnect profile differs from the profile used to join."));return;}
    PS->ColorChoice=FMath::Clamp(ColorChoice,-1,7);PS->CharacterLevel=ASCareer::Level(Experience);PS->ProfileId=ID;PS->Build=InBuild;PS->Build.Sanitize();PS->bProfileReady=true;PS->bReady=false;
    RestartPlayer(PC);auto* H=Cast<AASCharacter>(PC->GetPawn());if(!H) return;
    H->ColorIndex=PS->PlayerIndex;H->ApplyBuild(PS->Build,true);
    if(FASReconnect* Saved=Disconnected.Find(ID)) {
        PS->PendingExperience=Saved->PendingExperience;PS->PendingCareer=Saved->PendingCareer;PS->bRoomDamaged=Saved->RoomDamaged;PS->bRoomEligible=Saved->RoomEligible;
        PS->Build=Saved->Build;PS->PlayerIndex=Saved->PlayerIndex;PS->Offers=Saved->Offers;PS->bRewardClaimed=Saved->Claimed;
        H->ColorIndex=Saved->PlayerIndex;H->ApplyBuild(Saved->Build,true);H->SetActorLocation(Saved->Position);
        H->Health=Saved->Health;H->MaxHealth=Saved->MaxHealth;H->Shield=Saved->Shield;H->RelicMask=Saved->Relics;
        H->RunRanks=Saved->RunRanks;H->RunRanks.SetNumZeroed(ASRules::RunUpgradeCount);
        for(int32& Rank:H->RunRanks) Rank=ASRules::UpgradeRank(Rank);
        H->TraitRanks=Saved->TraitRanks;H->TraitRanks.SetNumZeroed(ASBuild::Count);
        for(int32 I=0;I<ASBuild::Count;++I) H->TraitRanks[I]=FMath::Clamp(H->TraitRanks[I],0,ASBuild::Catalog[I].Max);
        H->Ammo=FMath::Clamp(Saved->Ammo,0,H->MagazineCapacity());H->ReloadStart=ASArsenal::UsesMagazine(H->Build.Weapon)?Saved->ReloadStart:0;H->ReloadEnd=ASArsenal::UsesMagazine(H->Build.Weapon)?Saved->ReloadEnd:0;H->NextAttack=Saved->NextAttack;
        H->EvolutionMask=Saved->EvolutionMask;H->LastUpgrade=Saved->LastUpgrade;H->Rerolls=Saved->Rerolls;H->CurseStacks=Saved->CurseStacks;
        H->Focus=Saved->Focus;H->NextRune=Saved->NextRune;H->ConduitHits=Saved->ConduitHits;H->bPhoenixUsed=Saved->PhoenixUsed;H->GoldRemainder=Saved->GoldRemainder;
        H->RoomBuffUntil=Saved->RoomBuffUntil;H->KillBuffUntil=Saved->KillBuffUntil;H->Movement()->DashCharges=Saved->DashCharges;
        H->Movement()->KnownDashCapacity=H->MaxDashCharges();H->Movement()->DashRecharge=Saved->DashRecharge;H->Movement()->AirDashesUsed=FMath::Clamp(Saved->AirDashesUsed,0,H->MaxDashCharges());
        PS->bWagerVote=Saved->WagerVote;PS->bShrineUsed=Saved->ShrineUsed;H->WarCryUntil=Saved->WarCryUntil;PS->ReceiptQueue=Saved->ReceiptQueue;
        H->NextSpiral=Saved->NextSpiral;H->bDowned=H->Health<=0;H->bMercyUsed=Saved->Mercy;H->bSecondWindUsed=Saved->SecondWind;
        if(!Saved->Receipt.Id.IsEmpty()) SendReceipt(PS,Saved->Receipt);
        Disconnected.Remove(ID);S->bConnectionHold=false;S->ReconnectRemaining=0;PC->Notify(TEXT("Rejoined the expedition."));
    } else if(S->Phase!=EASPhase::Hub) {
        PS->bProfileReady=false;PC->Notify(TEXT("No matching reconnect snapshot. Return to the hub before joining."));
    }
    if(S->Phase==EASPhase::Hub) PS->bRoomEligible=false;
    H->RefreshLook();PS->ForceNetUpdate();
}
void AASGameMode::Logout(AController* Exiting) {
    auto* S=AS::State(this);auto* PS=Exiting?Exiting->GetPlayerState<AASPlayerState>():nullptr;auto* H=Exiting?Cast<AASCharacter>(Exiting->GetPawn()):nullptr;
    if(S&&PS&&PS->bProfileReady&&S->Phase!=EASPhase::Hub&&S->PlayerArray.Num()>1) {
        FASReconnect Saved;
        if(const auto* Previous=LiveSnapshots.Find(PS->ProfileId)) Saved=*Previous;
        if(H) {
            Saved.Build=H->Build;Saved.Position=H->GetActorLocation();Saved.Health=H->Health;Saved.MaxHealth=H->MaxHealth;
            Saved.Shield=H->Shield;Saved.NextSpiral=H->NextSpiral;Saved.Relics=H->RelicMask;
            Saved.RunRanks=H->RunRanks;CaptureTraits(H,PS,Saved);
            Saved.Mercy=H->bMercyUsed;Saved.SecondWind=H->bSecondWindUsed;
        }
        Saved.PendingExperience=PS->PendingExperience;Saved.PendingCareer=PS->PendingCareer;Saved.RoomDamaged=PS->bRoomDamaged;Saved.RoomEligible=PS->bRoomEligible;Saved.ReceiptQueue=PS->ReceiptQueue;
        Saved.PlayerIndex=PS->PlayerIndex;Saved.Claimed=PS->bRewardClaimed;Saved.Offers=PS->Offers;
        Saved.Receipt=PS->bSavePending?PS->PendingReceipt:FASReceipt();
        Disconnected.Add(PS->ProfileId,Saved);S->bConnectionHold=true;S->ReconnectRemaining=90;ReconnectDeadline=GetWorld()->GetTimeSeconds()+90;
    }
    if(PS) LiveSnapshots.Remove(PS->ProfileId);
    if(H) H->Destroy();Super::Logout(Exiting);
}
void AASGameMode::AbandonReconnect() {if(auto* S=AS::State(this)) {S->bConnectionHold=false;S->ReconnectRemaining=0;}Disconnected.Empty();}
bool AASGameMode::AllReady() const {
    auto* S=AS::State(this);if(!S||S->PlayerArray.IsEmpty()) return false;
    for(auto P:S->PlayerArray) {auto* PS=Cast<AASPlayerState>(P);if(!PS||!PS->bProfileReady||PS->bSavePending||(S->PlayerArray.Num()>1&&!PS->bReady)) return false;}
    return true;
}
void AASGameMode::BeginRun(AASPlayerController* PC,int32 Seed) {
    auto* S=AS::State(this);if(!S||S->Phase!=EASPhase::Hub||!Host(PC)) return;
    if(!AllReady()) {PC->Notify(TEXT("Both players must load a profile and mark Ready before departure."));return;}
    RunSeed=Seed;Random.Initialize(Seed);S->RunId=FGuid::NewGuid().ToString(EGuidFormats::Digits);S->Cinders=0;S->Challenge=0;bEnding=false;
    S->bPartyPaused=false;AbandonReconnect();
    for(auto W:S->Heroes) if(auto* H=W.Get()) {
        auto* PS=H->GetPlayerState<AASPlayerState>();if(!PS) continue;
        H->ApplyBuild(PS->Build,true);S->Cinders+=H->Rank(13)*10;
    }
    EnterRoom(0);
}
void AASGameMode::ClearCombat(bool CarryWells) {
    auto* S=AS::State(this);if(!S) return;
    // Copy weak arrays before Destroy: EndPlay unregisters actors from the originals.
    auto Enemies=S->Enemies;for(auto W:Enemies) if(auto* E=W.Get()) E->Destroy();
    auto Projectiles=S->Projectiles;for(auto W:Projectiles) if(auto* P=W.Get()) P->Destroy();
    auto Wells=S->Wells;for(auto W:Wells) if(auto* G=W.Get()) if(!CarryWells||!G->Hero||!G->Hero->Relic(4)) G->Destroy();
    auto Breakables=S->Breakables;for(auto W:Breakables) if(auto* B=W.Get()) B->Destroy();
    auto Pickups=S->Pickups;for(auto W:Pickups) if(auto* P=W.Get()) P->Destroy();
    for(TActorIterator<AASField> It(GetWorld());It;++It) It->Destroy();
    S->LivingEnemies=0;S->PendingSpawns=0;S->SpawnWarnings.Empty();Queued=0;
}
void AASGameMode::EnterRoom(int32 Room) {
    auto* S=AS::State(this);if(!S||!S->Arena) return;
    struct FCarried {TWeakObjectPtr<AASCharacter> Hero;bool Perfect;};TArray<FCarried> Carried;int32 PerPlayer[2]={0,0};
    for(auto W:S->Wells) if(auto* G=W.Get()) if(G->Hero && !G->bCosmetic&&!G->bPractice&&G->Hero->Relic(4)) {
        const int32 OwnerIndex=FMath::Clamp(G->Hero->ColorIndex,0,1);
        if(PerPlayer[OwnerIndex]<2) {Carried.Add({G->Hero.Get(),G->bPerfect});++PerPlayer[OwnerIndex];}
    }
    int32 Voters=0;for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) {if(PS->bWagerVote) ++Voters;}
    S->Challenge=Room>0?ASBuild::ChallengeTier(S->Challenge,S->PlayerArray.Num(),Voters):0;S->VaultMask=0;S->bVaultClaimed=false;
    bAdvancePending=false;S->bCindersBanked=false;S->BankedAsh=0;RoomStarted=S->SimTime;ClearCombat();S->Room=FMath::Clamp(Room,0,31);S->Wave=0;S->Phase=EASPhase::Countdown;S->bPartyPaused=false;
    const uint32 RoomSeed=uint32(RunSeed)+uint32(Room)*104729u;S->Arena->Configure(Room,int32(RoomSeed));
    for(auto W:S->Heroes) if(auto* H=W.Get()) {
        H->ResetInput();H->ResetMagazine();H->bDowned=false;H->Health=FMath::Max(H->Health,H->MaxHealth*.35f);H->ReviveProgress=0;
        H->SetActorLocation(S->Arena->SpawnPoint(H->ColorIndex));H->LastSafe=H->GetActorLocation();H->Movement()->StopMovementImmediately();
        H->Movement()->AirDashesUsed=0;H->Movement()->bAirDashing=false;H->Movement()->DashRemaining=0;
        H->Movement()->SetMovementMode(MOVE_Walking);H->InvulnerableUntil=S->SimTime+3;H->WarCryUntil=0;
        if(auto* PS=H->GetPlayerState<AASPlayerState>()) {PS->Offers.Empty();PS->bRewardClaimed=false;PS->bShrineUsed=false;PS->bRoomDamaged=false;PS->bRoomEligible=true;}
    }
    for(int32 I=1;I<int32(S->Arena->Layout.Cells.size());++I) if(I!=S->Arena->Layout.Exit&&!S->Arena->IsVaultCell(I)) {
        const FVector Center=S->Arena->CellPosition(I);
        AASBreakable::Spawn(GetWorld(),Center+FVector(210,190,4),I%3==0?1:0);
        if(I%3==0) AASBreakable::Spawn(GetWorld(),Center+FVector(350,180,4),1);
        FVector Start,End;
        if(I%4==1&&S->Arena->WallTrapPath(I,Start,End)) AASField::Spawn(GetWorld(),6,Start,28.f,ASAdventure::TrapDamage(ASRules::Ward(Room)),0,999999,nullptr,End);
        if(ASRules::Ward(Room)>0&&I%4==2&&!S->Arena->IsVaultCell(I)) AASField::Spawn(GetWorld(),7,Center+FVector(-220,-200,12),65.f,ASAdventure::TrapDamage(ASRules::Ward(Room)),0,999999,nullptr,Center+FVector(220,-200,12));
    }
    // Spikes occupy optional side clearings, never the spawn or mandatory door center.
    for(int32 I=2;I<int32(S->Arena->Layout.Cells.size());I+=5) if(I!=S->Arena->Layout.Exit&&!S->Arena->IsVaultCell(I)) {
        AASField::Spawn(GetWorld(),2,S->Arena->CellPosition(I)+FVector(250,-250,15),150,18,0,999999);
    }
    int32 SideTrapIndex=0;
    for(int32 I=3;I<int32(S->Arena->Layout.Cells.size());I+=6) if(I!=S->Arena->Layout.Exit&&!S->Arena->IsVaultCell(I)) {
        const bool Flame=ASRules::SideTrapKind(SideTrapIndex++)==4;
        AASField::Spawn(GetWorld(),Flame?4:5,S->Arena->CellPosition(I)+FVector(-240,180,12),Flame?185.f:210.f,Flame?15.f:18.f,0,999999);
    }
    if(S->Arena->HasVault()) for(int32 I=0;I<2;++I) {
        auto* F=AASField::Spawn(GetWorld(),I==0?4:5,S->Arena->VaultPoint()+FVector(I==0?-230:230,0,12),170,16,0,999999);
        if(F) {F->bVault=true;F->ForceNetUpdate();}
    }
    S->Arena->RefreshNavigation();
    QueueWave();
    for(auto W:S->Heroes) if(auto* H=W.Get()) H->EnterRoomTraits(S->CountdownEnd);
    int32 CarryIndex=0;for(const auto& C:Carried) if(auto* H=C.Hero.Get()) AASWell::Plant(H,H->GetActorLocation()+FVector(200,120*(CarryIndex++%2?1:-1),-85),C.Perfect,.7f,FMath::Max(0.f,S->CountdownEnd-S->SimTime)+3.f,true);
    S->ForceNetUpdate();
}
void AASGameMode::QueueWave() {
    auto* S=AS::State(this);if(!S||!S->Arena) return;
    const int32 Players=S->PlayerArray.Num();
    Queued=ASRules::Boss(S->Room)?1:ASRules::WaveEnemies(S->Room,S->Wave,Players);
    S->PendingSpawns=Queued;S->SpawnWarnings.Empty();
    FVector Center=S->Arena->SpawnPoint(0);for(auto W:S->Heroes) if(auto* H=W.Get()) if(!H->bDowned) {Center=H->GetActorLocation();break;}
    for(int32 I=0;I<Queued;++I) {
        FVector P=S->Arena->EnemySpawn(Center,Random);
        for(int32 Try=0;Try<16;++Try) {
            bool Clear=true;
            for(const FVector& Other:S->SpawnWarnings) if(FVector::DistSquared2D(P,Other)<190*190) Clear=false;
            for(auto W:S->Heroes) if(auto* H=W.Get()) if(FVector::DistSquared2D(P,H->GetActorLocation())<550*550) Clear=false;
            if(Clear) break;P=S->Arena->EnemySpawn(Center,Random);
        }
        S->SpawnWarnings.Add(P);
    }
    S->Phase=EASPhase::Countdown;S->CountdownEnd=S->SimTime+3.f;NextSpawn=S->CountdownEnd;
}
void AASGameMode::Tick(float Delta) {
    Super::Tick(Delta);auto* S=AS::State(this);if(!S||!S->Arena) return;
    const float RealNow=GetWorld()->GetTimeSeconds();
    // PlayerController teardown can destroy the pawn before Logout. Keep a small
    // authority-side snapshot independently of the pawn's lifetime.
    if(S->Phase!=EASPhase::Hub && RealNow>=NextSnapshot) {
        NextSnapshot=RealNow+.2f;
        for(auto W:S->Heroes) if(auto* H=W.Get()) if(auto* PS=H->GetPlayerState<AASPlayerState>()) if(PS->bProfileReady) {
            FASReconnect Saved;Saved.Build=H->Build;Saved.Position=H->GetActorLocation();Saved.Health=H->Health;Saved.MaxHealth=H->MaxHealth;
            Saved.Shield=H->Shield;Saved.NextSpiral=H->NextSpiral;Saved.Relics=H->RelicMask;Saved.PlayerIndex=PS->PlayerIndex;
            Saved.RunRanks=H->RunRanks;CaptureTraits(H,PS,Saved);
            Saved.Mercy=H->bMercyUsed;Saved.SecondWind=H->bSecondWindUsed;Saved.Claimed=PS->bRewardClaimed;Saved.Offers=PS->Offers;
            if(PS->bSavePending) Saved.Receipt=PS->PendingReceipt;LiveSnapshots.Add(PS->ProfileId,Saved);
        }
    }
    if(RealNow>=NextReceiptRetry) {
        NextReceiptRetry=RealNow+3;
        for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) if(PS->bSavePending) if(auto* PC=Cast<AASPlayerController>(PS->GetOwner())) PC->ReceiveReceipt(PS->PendingReceipt);
    }
    if(S->bConnectionHold) {S->ReconnectRemaining=FMath::Max(0.f,ReconnectDeadline-RealNow);if(S->ReconnectRemaining<=0) AbandonReconnect();}
    if(bHubPending) {
        bool Waiting=false;for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) Waiting|=PS->bSavePending;
        if(!Waiting) {bHubPending=false;BackToHub();return;}
    }
    if(bAdvancePending&&S->Phase==EASPhase::Reward&&!S->Frozen()) {
        bool Waiting=false;for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) Waiting|=PS->bSavePending||!PS->bRewardClaimed;
        if(!Waiting) {bAdvancePending=false;EnterRoom(S->Room+1);return;}
    }
    if(S->Frozen()) return;
    if(S->Phase==EASPhase::Countdown && S->SimTime>=S->CountdownEnd) S->Phase=EASPhase::Combat;
    if(!S->Fighting()) return;
    bool Alive=false;int32 Loaded=0;for(auto W:S->Heroes) if(auto* H=W.Get()) {
        if(auto* PS=H->GetPlayerState<AASPlayerState>()) if(PS->bProfileReady) {++Loaded;if(!H->bDowned) Alive=true;}
    }
    if(Loaded>0&&!Alive) {EndRun(false);return;}
    if(S->Phase!=EASPhase::Combat) return;
    if(Queued>0 && S->SimTime>=NextSpawn && S->Enemies.Num()<ASRules::MaxEnemies) {
        const FVector Warning=S->SpawnWarnings.IsEmpty()?S->Arena->CellPosition(0)+FVector(400,400,100):S->SpawnWarnings[0];
        FVector At;if(!S->Arena->FreeEnemyPoint(Warning,At)) {NextSpawn=S->SimTime+.25f;return;}
        auto* E=GetWorld()->SpawnActorDeferred<AASEnemy>(AASEnemy::StaticClass(),FTransform(At),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
        if(E) {
            E->Ward=ASRules::Ward(S->Room);E->Theme=ASAdventure::Biome(S->Room);const int32 Pool[]={0,1,2,5,6,7,8,9};
            E->Kind=ASRules::Boss(S->Room)?4:((ASRules::Elite(S->Room)&&S->Wave==2&&Queued==1)?3:Pool[Random.RandRange(0,FMath::Min(7,3+S->Room/2))]);
            if(!E->Elite()&&S->Room>=6&&Random.FRand()<.45f) {const int32 Themes[5][3]={{1,6,8},{0,5,7},{2,6,8},{0,5,9},{5,6,8}};E->Kind=Themes[E->Theme][Random.RandRange(0,2)];}
            E->MaxHealth=(E->Boss()?1100.f+450.f*E->Ward:(E->Elite()?300.f:70.f)+E->Ward*22.f)*(S->PlayerArray.Num()==2?1.45f:1.f);
            E->MaxHealth*=ASBuild::ChallengeHealth(S->Challenge)*(E->Kind==6?1.8f:(E->Kind==9?1.5f:(E->Kind==5?.85f:1.f)));
            E->Health=E->MaxHealth;UGameplayStatics::FinishSpawningActor(E,FTransform(At));E->RefreshLook();
            S->Effect(3,At,FVector::ForwardVector,ASData::BiomeColor(E->Theme),1.3f);
        }
        --Queued;if(!S->SpawnWarnings.IsEmpty()) S->SpawnWarnings.RemoveAt(0);S->PendingSpawns=Queued;NextSpawn=S->SimTime+.23f;
    }
    S->Enemies.RemoveAll([](const auto& W){return !W.IsValid();});S->LivingEnemies=S->Enemies.Num();
    if(Queued==0&&S->LivingEnemies==0) {
        if(S->Wave+1<ASRules::WaveCount(S->Room)) {++S->Wave;QueueWave();}else FinishRoom();
    }
}
void AASGameMode::EnemyKilled(AASEnemy* E,AASCharacter* Killer) {
    auto* S=AS::State(this);if(!S||!E||E->bTrainingDummy||S->Phase!=EASPhase::Combat) return;++Kills;
    if(IsValid(Killer)) if(auto* PS=Killer->GetPlayerState<AASPlayerState>()) {
        PS->Record(ASCareer::Kills,1,E->Boss()?100:(E->Elite()?25:5));
        if(E->Boss()) PS->Record(ASCareer::Bosses);else if(E->Elite()) PS->Record(ASCareer::Elites);
        if(ASCareer::MultiStatus(E->StatusBits)) PS->Record(ASCareer::Elemental);
    }
    const int32 Base=E->Boss()?30:(E->Elite()?8:1);
    const int32 Gold=IsValid(Killer)?ASBuild::CreditKills(Base,Killer->TraitValue(ASBuild::Trait::Greed),Killer->GoldRemainder):Base;
    S->Cinders=ASRules::Credit(S->Cinders,Gold);if(IsValid(Killer)) Killer->OnKill();
    const float DropChance=.12f+(IsValid(Killer)?.06f*Killer->TraitValue(ASBuild::Trait::Fortune):0.f);
    if(Random.FRand()<DropChance) AASPickup::Spawn(GetWorld(),E->GetActorLocation(),Random.FRand()<.3f?1:0,12.f);
    if(Random.FRand()<.25f) AASPickup::Spawn(GetWorld(),E->GetActorLocation()+FVector(40,0,0),2,1.f);
    S->Effect(6,E->GetActorLocation(),FVector::ForwardVector,ASData::BiomeColor(E->Theme),E->Boss()?2.f:.65f);
}
void AASGameMode::SendReceipt(AASPlayerState* PS,const FASReceipt& R) {
    if(!PS) return;
    if(PS->bSavePending) {
        if(PS->PendingReceipt.Id==R.Id) return;
        for(const auto& Waiting:PS->ReceiptQueue) if(Waiting.Id==R.Id) return;
        PS->ReceiptQueue.Add(R);return;
    }
    PS->PendingReceipt=R;PS->bSavePending=true;PS->ForceNetUpdate();
    if(auto* PC=Cast<AASPlayerController>(PS->GetOwner())) PC->ReceiveReceipt(R);
}
void AASGameMode::FinishRoom() {
    auto* S=AS::State(this);if(!S) return;S->Phase=EASPhase::Reward;
    S->ChestKind=(ASRules::Boss(S->Room)||ASRules::Elite(S->Room))?0:S->Room%6;
    auto Projectiles=S->Projectiles;for(auto W:Projectiles) if(auto* P=W.Get()) P->Destroy();
    for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) {
        PS->Offers.Empty();PS->bRewardClaimed=false;
        AASCharacter* Hero=nullptr;
        for(auto W:S->Heroes) if(auto* H=W.Get()) if(H->GetPlayerState<AASPlayerState>()==PS) {Hero=H;break;}
        if(!ASRules::Final(S->Room)) RollOffers(PS,Hero);else PS->bRewardClaimed=true;
        FASReceipt R;R.Id=S->RunId+FString::Printf(TEXT(":room%d:"),S->Room)+PS->ProfileId;
        R.Room=S->Room+1;R.Ward=ASRules::Ward(S->Room);R.Ash=3+(ASRules::Boss(S->Room)?20:0)+S->Challenge*3;R.Material=3+(ASRules::Boss(S->Room)?6:0);if(PS->Build.Talent(8)) R.Material=FMath::CeilToInt(R.Material*1.2f);
        R.Embers=(S->Room%3==2)?1:0;R.Tablets=ASRules::Boss(S->Room)?1:0;R.Points=ASRules::Boss(S->Room)?1:0;R.Cores=ASRules::Boss(S->Room)?1:0;
        PS->Record(ASCareer::Rooms,1,40);
        if(PS->bRoomEligible&&!PS->bRoomDamaged) PS->Record(ASCareer::Flawless);
        if(PS->bRoomEligible&&S->SimTime-RoomStarted<=90.f) PS->Record(ASCareer::Swift);
        PS->DrainCareer(R);SendReceipt(PS,R);
    }
    S->Cinders=ASRules::Credit(S->Cinders,S->Challenge*15);
    if(S->Arena) S->Effect(7,S->Arena->ExitPoint()+FVector(0,0,100),FVector::ForwardVector,FLinearColor(1,.6f,.12f),2.f);
    if(ASRules::Final(S->Room)) {BankCinders();EndRun(true);}
    S->ForceNetUpdate();
}
void AASGameMode::Reward(AASPlayerController* PC,int32 Index) {
    auto* S=AS::State(this);auto* PS=PC?PC->GetPlayerState<AASPlayerState>():nullptr;auto* H=PC?Cast<AASCharacter>(PC->GetPawn()):nullptr;
    if(!S||!PS||!H||S->Phase!=EASPhase::Reward||S->Frozen()||PS->bRewardClaimed||!PS->Offers.IsValidIndex(Index)||!S->Arena) return;
    if(H->bDowned) {PC->Notify(TEXT("Ask your partner to hold Revive nearby, then claim your upgrade."));return;}
    if(FVector::Dist2D(H->GetActorLocation(),S->Arena->ExitPoint())>400) {PC->Notify(TEXT("Approach the gold chest to claim your reward."));return;}
    const int32 Choice=PS->Offers[Index];
    const bool StatUpgrade=ASRules::IsRunUpgradeOffer(Choice)||ASBuild::IsTrait(Choice);
    if(StatUpgrade&&!H->CanApplyChoice(Choice)) {PC->Notify(TEXT("That upgrade is already at its maximum rank or effect."));return;}
    if(!StatUpgrade&&(Choice<0||Choice>=(S->ChestKind==2?PS->Build.StylePool:PS->Build.RelicPool))) return;
    if(S->ChestKind==4&&Index>0&&!ASRules::Spend(S->Cinders,25)) {PC->Notify(TEXT("That shop choice costs 25 shared Cinders. The first is free."));return;}
    if(StatUpgrade) H->ApplyChoice(Choice);
    else if(S->ChestKind==2) H->Build.Style=Choice;
    else {if(H->Relic(Choice)) H->Shield=FMath::Min(H->MaxHealth,H->Shield+15);H->RelicMask|=1<<Choice;}
    if(S->ChestKind==1) S->Cinders=ASRules::Credit(S->Cinders,40);
    if(S->ChestKind==3) H->Heal(H->MaxHealth*.35f);
    if(S->ChestKind==4&&Index==0) S->Cinders=ASRules::Credit(S->Cinders,10);
    if(S->ChestKind==5) {H->Heal(15);H->Shield=FMath::Min(H->MaxHealth,H->Shield+10);}
    H->RefreshLook();H->ForceNetUpdate();PS->bRewardClaimed=true;PS->ForceNetUpdate();
    PC->Notify(TEXT("Reward claimed. When both players have chosen, either player uses E at the chest to continue."));
}
void AASGameMode::Interact(AASPlayerController* PC,int32 Target) {
    auto* S=AS::State(this);auto* H=PC?Cast<AASCharacter>(PC->GetPawn()):nullptr;
    if(!S||!S->Arena||!H||S->Frozen()) return;
    if(S->Phase==EASPhase::Victory) {
        const auto* PS=PC->GetPlayerState<AASPlayerState>();const FVector Portal=S->Arena->InteractionPoint(9);
        if(Target!=9||H->bDowned||!PS||!PS->bProfileReady||!ASAdventure::InReach(FVector::DistSquared2D(H->GetActorLocation(),Portal),H->GetActorLocation().Z-Portal.Z)) return;
        if(!AS::ClearSight(GetWorld(),H->GetActorLocation(),Portal)) return;
        BackToHub();return;
    }
    if(InteractVault(PC,Target)) return;
    if(S->Phase!=EASPhase::Reward) return;
    if(H->bDowned) return;
    const auto* Requester=PC->GetPlayerState<AASPlayerState>();if(!Requester||!Requester->bProfileReady) return;
    if(FVector::Dist2D(H->GetActorLocation(),S->Arena->ExitPoint())>400) return;
    for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) if(!PS->bRewardClaimed||PS->bSavePending) {
        PC->Notify(PS->bSavePending?TEXT("Waiting for a verified save. Check that both save folders are writable."):TEXT("Each player must claim their own reward."));return;
    }
    BankCinders();
    // Receipts are acknowledged before loading the next chamber, including the conversion.
    bool Saving=false;for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) Saving|=PS->bSavePending;
    if(Saving) {bAdvancePending=true;PC->Notify(TEXT("Banking leftover Cinders. The next chamber opens when both saves are confirmed."));return;}
    EnterRoom(S->Room+1);
}
void AASGameMode::BankCinders() {
    auto* S=AS::State(this);if(!S||S->bCindersBanked) return;S->bCindersBanked=true;
    const int32 Ash=ASCareer::Cashout(S->Cinders);S->Cinders-=ASCareer::CashoutCost(S->Cinders);S->BankedAsh=Ash;
    if(Ash>0) for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) {
        FASReceipt R;R.Id=S->RunId+FString::Printf(TEXT(":bank%d:"),S->Room)+PS->ProfileId;R.Ash=Ash;R.Room=S->Room+1;SendReceipt(PS,R);
        if(auto* PC=Cast<AASPlayerController>(PS->GetOwner())) PC->Notify(FString::Printf(TEXT("Banked %d leftover Cinders: +%d permanent Ash per player."),Ash*10,Ash));
    }
    S->ForceNetUpdate();
}
void AASGameMode::EndRun(bool Victory) {
    if(bEnding) return;bEnding=true;auto* S=AS::State(this);if(!S) return;
    ClearCombat();S->Phase=Victory?EASPhase::Victory:EASPhase::Defeat;S->bPartyPaused=false;
    for(auto W:S->Heroes) if(auto* H=W.Get()) {
        H->ResetInput();if(Victory&&H->bDowned) {H->bDowned=false;H->Health=FMath::Max(1.f,H->Health);H->ReviveProgress=0;H->RefreshLook();H->ForceNetUpdate();}
    }
    for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) {
        FASReceipt R;R.Id=S->RunId+(Victory?TEXT(":victory:"):TEXT(":defeat:"))+PS->ProfileId;
        R.Room=Victory?32:S->Room;R.Ward=ASRules::Ward(S->Room);R.Ash=Victory?80:(5+(PS->Build.Talent(10)?8:0));
        R.Cores=Victory?2:0;R.Points=Victory?1:0;R.bWin=Victory;R.bDeath=!Victory;if(Victory) PS->Record(ASCareer::Wins,1,150);PS->DrainCareer(R);SendReceipt(PS,R);
    }
}
void AASGameMode::BackToHub() {
    auto* S=AS::State(this);if(!S||!S->Arena) return;
    // Freeze before draining progress so no late projectile or kill can create a
    // second return receipt while the first is awaiting its durable-save ack.
    S->bPartyPaused=true;for(auto W:S->Heroes) if(auto* H=W.Get()) H->ResetInput();
    for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) {
        bool Pending=PS->PendingExperience>0;for(int32 V:PS->PendingCareer) Pending|=V>0;
        if(Pending) {FASReceipt R;R.Id=S->RunId+TEXT(":return:")+PS->ProfileId;R.Room=FMath::Clamp(S->Room,0,32);PS->DrainCareer(R);SendReceipt(PS,R);}
    }
    for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) if(PS->bSavePending) {bHubPending=true;return;}
    bHubPending=false;bAdvancePending=false;ClearCombat();AbandonReconnect();LiveSnapshots.Empty();S->Phase=EASPhase::Hub;S->Room=0;S->Wave=0;S->Cinders=0;S->Challenge=0;S->VaultMask=0;S->bVaultClaimed=false;S->bPartyPaused=false;bEnding=false;
    S->Arena->Configure(-1,1103);SpawnPracticeDummy();
    for(auto W:S->Heroes) if(auto* H=W.Get()) {
        if(auto* PS=H->GetPlayerState<AASPlayerState>()) {PS->bReady=false;PS->bRewardClaimed=false;PS->Offers.Empty();H->ApplyBuild(PS->Build,true);}
        H->ResetInput();H->SetActorLocation(S->Arena->SpawnPoint(H->ColorIndex));H->Movement()->StopMovementImmediately();H->Movement()->SetMovementMode(MOVE_Walking);
    }
}

void AASGameMode::RollOffers(AASPlayerState* PS,AASCharacter* H) {
    if(!PS||!H) return;auto* S=AS::State(this);if(!S) return;
    PS->Offers.Empty();for(int I:ASBuild::Offers(H->UpgradeContext(),uint32(Random.RandRange(1,MAX_int32)),PS->Build.Rank(12)>0?4:3)) PS->Offers.Add(I);
    TArray<int32> Pool;for(int32 I=0;I<(S->ChestKind==2?PS->Build.StylePool:PS->Build.RelicPool);++I) if(S->ChestKind==2?I!=H->Build.Style:!H->Relic(I)) Pool.Add(I);
    if(!Pool.IsEmpty()) PS->Offers.Add(Pool[Random.RandRange(0,Pool.Num()-1)]);
    PS->ForceNetUpdate();PS->OnOffersChanged();
}
void AASGameMode::Reroll(AASPlayerController* PC) {
    auto* S=AS::State(this);auto* H=PC?Cast<AASCharacter>(PC->GetPawn()):nullptr;auto* PS=PC?PC->GetPlayerState<AASPlayerState>():nullptr;
    if(!S||!S->Arena||!H||!PS||S->Phase!=EASPhase::Reward||S->Frozen()||H->bDowned||PS->bRewardClaimed||H->Rerolls<=0||FVector::Dist2D(H->GetActorLocation(),S->Arena->ExitPoint())>400.f) return;
    --H->Rerolls;H->ForceNetUpdate();RollOffers(PS,H);PC->Notify(TEXT("New personal choices rolled. Your partner's choices were preserved."));
}
void AASGameMode::ToggleWager(AASPlayerController* PC,bool Enabled) {
    auto* S=AS::State(this);auto* PS=PC?PC->GetPlayerState<AASPlayerState>():nullptr;
    if(!S||!PS||S->Frozen()||S->Phase!=EASPhase::Reward||ASRules::Final(S->Room)) return;
    PS->bWagerVote=Enabled;PS->ForceNetUpdate();PC->Notify(PS->bWagerVote?TEXT("Stakes enabled until unchecked. All players must opt in; harder clears improve rare reward odds."):TEXT("Stakes disabled. The next chamber returns to normal difficulty and reward odds."));
}

void AASGameMode::SpawnMinion(const FVector& Near) {
    auto* S=AS::State(this);if(!S||!S->Arena||S->Phase!=EASPhase::Combat||S->Enemies.Num()+Queued>=ASRules::MaxEnemies) return;
    FVector At;if(!S->Arena->FreeEnemyPoint(Near,At)) return;const FTransform T(At);
    auto* E=GetWorld()->SpawnActorDeferred<AASEnemy>(AASEnemy::StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if(E) {E->Kind=0;E->Ward=ASRules::Ward(S->Room);E->Theme=ASAdventure::Biome(S->Room);E->MaxHealth=(45.f+15.f*E->Ward)*ASBuild::ChallengeHealth(S->Challenge)*(S->PlayerArray.Num()==2?1.45f:1.f);E->Health=E->MaxHealth;UGameplayStatics::FinishSpawningActor(E,T);S->Effect(3,At,FVector::UpVector,FLinearColor(.2f,1,.4f),100);}
}
bool AASGameMode::InteractVault(AASPlayerController* PC,int32 Target) {
    auto* S=AS::State(this);auto* H=PC?Cast<AASCharacter>(PC->GetPawn()):nullptr;
    if(!S||!H||!S->Arena||H->bDowned||!S->Arena->HasVault()||(S->Phase!=EASPhase::Combat&&S->Phase!=EASPhase::Reward&&S->Phase!=EASPhase::Countdown)) return false;
    const int32 Part=Target>=201&&Target<=203?Target-200:S->Arena->VaultInteraction(H->GetActorLocation());if(Part==0) return false;
    const FVector Point=S->Arena->VaultPoint(Part);
    if(!ASAdventure::InReach(FVector::DistSquared2D(H->GetActorLocation(),Point),H->GetActorLocation().Z-Point.Z)) {PC->Notify(TEXT("Move within 3.2 metres of the sigil or cache."));return true;}
    if(!AS::ClearSight(GetWorld(),H->GetActorLocation(),Point+FVector(0,0,100))) {PC->Notify(TEXT("Move around the obstacle to reach the vault."));return true;}
    if(Part<3) {
        S->VaultMask|=1<<(Part-1);S->ForceNetUpdate();
        PC->Notify(S->VaultMask==3?TEXT("Both sigils lit. Vault traps disabled; collect the cache!"):TEXT("Sigil lit. Activate the other sigil to disarm the vault."));return true;
    }
    if(S->bVaultClaimed) {PC->Notify(TEXT("This vault cache has already been collected by your party."));return true;}
    if(S->VaultMask!=3) {PC->Notify(TEXT("Activate both vault sigils to unlock this cache."));return true;}
    S->bVaultClaimed=true;S->Cinders=ASRules::Credit(S->Cinders,20);S->ForceNetUpdate();
    for(auto P:S->PlayerArray) if(auto* PS=Cast<AASPlayerState>(P)) {
        FASReceipt R;R.Id=S->RunId+FString::Printf(TEXT(":vault%d:"),S->Room)+PS->ProfileId;R.Room=S->Room;R.Ward=ASRules::Ward(S->Room);R.Ash=2;R.Material=1;R.Experience=15;R.Career.Init(0,ASCareer::Count);R.Career[ASCareer::Vaults]=1;SendReceipt(PS,R);
        if(auto* C=Cast<AASPlayerController>(PS->GetOwner())) C->Notify(TEXT("Rune vault recovered: +20 shared Cinders, +2 Ash and +1 ward material per player."));
    }
    S->Effect(7,S->Arena->VaultPoint(3)+FVector(0,0,90),FVector::UpVector,FLinearColor(.2f,.7f,1),250);return true;
}

void AASGameMode::UseShrine(AASPlayerController* PC) {
    auto* S=AS::State(this);auto* H=PC?Cast<AASCharacter>(PC->GetPawn()):nullptr;auto* PS=PC?PC->GetPlayerState<AASPlayerState>():nullptr;
    if(!S||!H||!PS||!PS->bProfileReady||!S->Arena||!S->Arena->HasShrine()||H->bDowned||S->Frozen()||PS->bShrineUsed||!(S->Phase==EASPhase::Reward||S->Phase==EASPhase::Combat)) return;
    const FVector At=S->Arena->ShrinePoint();
    if(!ASAdventure::InReach(FVector::DistSquared2D(H->GetActorLocation(),At),H->GetActorLocation().Z-At.Z)||!AS::ClearSight(GetWorld(),H->GetActorLocation(),At+FVector(0,0,210))) {PC->Notify(TEXT("Stand next to the Ember Bargain shrine with a clear path."));return;}
    if(!ASAdventure::CanSacrifice(H->Health,H->MaxHealth)) {PC->Notify(TEXT("Not enough health for this offering."));return;}
    const auto C=H->UpgradeContext();TArray<int32> Choices;
    for(int32 I=100;I<106;++I) if(H->CanApplyChoice(I)) Choices.Add(I);
    for(int32 I=0;I<ASBuild::Count;++I) if(I!=int32(ASBuild::Trait::BloodPact)&&!ASBuild::Catalog[I].Instant&&H->CanApplyChoice(ASBuild::OfferBase+I)) for(int32 N=0;N<FMath::Max(1,ASBuild::Weight(C,ASBuild::OfferBase+I)/10);++N) Choices.Add(ASBuild::OfferBase+I);
    if(Choices.IsEmpty()) {PC->Notify(TEXT("Your eligible upgrades are already at their caps. No health spent."));return;}
    const int32 Choice=Choices[Random.RandRange(0,Choices.Num()-1)];const float Cost=ASAdventure::ShrineCost(H->MaxHealth);
    if(!H->ApplyChoice(Choice)) return;
    H->Health=FMath::Max(1.f,H->Health-Cost);PS->bShrineUsed=true;H->LastDamageAt=S->SimTime;H->ForceNetUpdate();PS->ForceNetUpdate();
    const FString Name=ASBuild::IsTrait(Choice)?FString(UTF8_TO_TCHAR(ASBuild::Catalog[Choice-ASBuild::OfferBase].Name)):FString(ASData::RunUpgrades[Choice-100].Name);
    PC->Notify(FString::Printf(TEXT("Ember bargain: %s gained for %.0f HP."),*Name,Cost));S->Effect(7,At,FVector::UpVector,FLinearColor(1,.6f,.16f),200);
}
