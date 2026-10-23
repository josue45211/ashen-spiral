#include "ASSpell.h"
#include "ASCharacter.h"
#include "ASEnemy.h"
#include "ASGameState.h"
#include "ASGameInstance.h"
#include "ASWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
// 0.26 Phase 2: the show of the team skills (rules in ASTeamRules.h, the server's side in ASTeamAbilities.cpp). Everything here is cosmetic and local to each machine: the server sends one cue per
// moment (AASCharacter::TeamCue), and the long-lived looks (the tether's beam, a Twin Collapse's open call) read the state that replicates on the heroes, so a partner sees what you see.
namespace {
const FLinearColor BrandGold(1.f,.78f,.22f),EmberOrange(1.f,.52f,.14f),TwinViolet(.72f,.50f,1.f);
// The unit Cube is 100 cm long: lay one between two points as a bar (Thick in cube scale: .03 is 3 cm).
void Bar(UStaticMeshComponent* M,const FVector& A,const FVector& B,float Thick) {
    if(!M) return;
    const FVector D=B-A;const float L=float(D.Size());
    if(L<1.f) {M->SetVisibility(false);return;}
    M->SetVisibility(true);M->SetWorldLocation((A+B)*.5f);M->SetWorldRotation(D.Rotation());M->SetWorldScale3D(FVector(L/100.f,Thick,Thick));
}
bool TeamLog() {static IConsoleVariable* Variable=IConsoleManager::Get().FindConsoleVariable(TEXT("as.team.log"));return Variable&&Variable->GetInt()!=0;} // as.team.log (ASTeamAbilities.cpp)
float Noise(float Seed,float Step) {const float V=FMath::Sin(Seed*12.9898f+Step*78.233f)*43758.5453f;return V-FMath::FloorToFloat(V)-.5f;}
// A jagged line of bars from A to B (bars[First..First+Count-1]), re-rolled 24 times a second: lightning that does not look like a wire.
void Jag(TArray<UStaticMeshComponent*>& Bars,int32 First,int32 Count,const FVector& A,const FVector& B,float Seed,float Age,float Thick,float Amplitude) {
    const FVector D=B-A;const float L=float(D.Size());if(L<1.f||Count<1) return;
    const FVector Dir=D/L;const FVector Side=FVector::CrossProduct(Dir,FVector::UpVector).GetSafeNormal(),Up=FVector::CrossProduct(Side,Dir).GetSafeNormal();
    const float Step=FMath::FloorToFloat(Age*24.f),Amp=FMath::Min(Amplitude,L*.06f);FVector Prev=A;
    for(int32 I=1;I<=Count;++I) {
        const float T=float(I)/float(Count);FVector P=A+D*T;
        if(I<Count) {const float Taper=FMath::Sin(T*UE_PI);P+=Side*Noise(Seed+I,Step)*2.f*Amp*Taper+Up*Noise(Seed+I+40.f,Step)*Amp*Taper;}
        Bar(Bars[First+I-1],Prev,P,Thick);Prev=P;
    }
}
}

void AASClawFx::SetupTether(AASCharacter* Hero,FLinearColor Color) {
    if(!Hero||Active>=30) {Destroy();return;}
    ++Active;bCounted=true;Kind=Look::Tether;Holder=Hero;Tint=Color;Life=ASTeam::TetherSeconds+2.f;Seed=FMath::FRand()*100.f;Emission=5.5f;
    Glow=ASAssets::Glow(this,FMath::Lerp(Color,FLinearColor::White,.12f),Emission);Matte=ASAssets::Glow(this,Color,2.2f);
    FBit Halo;Halo.Mesh=AddBit(TEXT("Cube"),Matte);Bits.Add(Halo);                                              // 0: the fat soft glow along the whole line
    for(int32 I=0;I<9;++I) {FBit B;B.Mesh=AddBit(TEXT("Cube"),Glow);Bits.Add(B);}                              // 1-9: the jagged core
    for(int32 I=0;I<2;++I) {FBit B;B.Mesh=AddBit(TEXT("Sphere"),Glow);B.Mesh->SetRelativeScale3D(FVector(.14f));Bits.Add(B);} // 10-11: two bright pulses running along it
    FBit Pillar;Pillar.Mesh=AddBit(TEXT("Cube"),Glow);Pillar.Mesh->SetVisibility(false);Bits.Add(Pillar);       // 12: an ember stake's column of light (alone)
    FBit Foot;Foot.Mesh=AddBit(TEXT("SM_Ring"),nullptr);ASAssets::StyleGlowBand(Foot.Mesh,TEXT("SM_Ring"),Color,4.5f,.44f,.52f,0.f);Foot.Mesh->SetVisibility(false);Bits.Add(Foot); // 13: a ring on the floor: the stake's, or the holder's feet
    FBit Foot2;Foot2.Mesh=AddBit(TEXT("SM_Ring"),nullptr);ASAssets::StyleGlowBand(Foot2.Mesh,TEXT("SM_Ring"),Color,4.5f,.44f,.52f,0.f);Foot2.Mesh->SetVisibility(false);Bits.Add(Foot2);   // 14: the partner's feet (paired)
}
void AASClawFx::SetupArc(const FVector& From,const FVector& To,FLinearColor Color,float Seconds) {
    if(Active>=30) {Destroy();return;}
    ++Active;bCounted=true;Kind=Look::Arc;PointA=From;PointB=To;Tint=Color;Life=FMath::Max(.06f,Seconds);Seed=FMath::FRand()*100.f;Emission=18.f;
    Glow=ASAssets::Glow(this,FMath::Lerp(Color,FLinearColor::White,.55f),Emission);
    for(int32 I=0;I<6;++I) {FBit B;B.Mesh=AddBit(TEXT("Cube"),Glow);Bits.Add(B);}
}
void AASClawFx::SetupCall(AASCharacter* Hero,FLinearColor Color) {
    if(!Hero||Active>=30) {Destroy();return;}
    ++Active;bCounted=true;Kind=Look::Call;Holder=Hero;Tint=Color;Life=ASTeam::TwinWindow+1.f;Emission=4.5f;
    Glow=ASAssets::Glow(this,FMath::Lerp(Color,FLinearColor::White,.15f),6.5f);
    FBit Ring;Ring.Mesh=AddBit(TEXT("SM_Ring"),nullptr);ASAssets::StyleGlowBand(Ring.Mesh,TEXT("SM_Ring"),Color,Emission,.44f,.52f,0.f);Bits.Add(Ring);   // 0: the ring on the floor, closing as the window does
    for(int32 I=0;I<4;++I) {FBit B;B.Mesh=AddBit(TEXT("Cube"),Glow);B.Mesh->SetRelativeScale3D(FVector(.09f));Bits.Add(B);}                                      // 1-4: orbiting diamonds
}
void AASClawFx::SetupColumn(const FVector& At,FLinearColor Color,float Width,float Height,float Seconds) {
    if(Active>=30) {Destroy();return;}
    ++Active;bCounted=true;Kind=Look::Column;PointA=At;PointB=FVector(Height,0.f,0.f);Radius=Width;Tint=Color;Life=FMath::Max(.1f,Seconds);Emission=7.f;
    Glow=ASAssets::Glow(this,FMath::Lerp(Color,FLinearColor::White,.25f),Emission);
    FBit Body;Body.Mesh=AddBit(TEXT("Cube"),Glow);Bits.Add(Body);                                      // 0: the column
    FBit Heart;Heart.Mesh=AddBit(TEXT("Cube"),Glow);Bits.Add(Heart);                                   // 1: its thinner, brighter heart
}
void AASClawFx::SetupConverge(const FVector& Mid,const FVector& A,const FVector& B,float Seconds,float BlastRadius,FLinearColor Color) {
    if(Active>=30) {Destroy();return;}
    ++Active;bCounted=true;Kind=Look::Converge;PointMid=Mid;PointA=A;PointB=B;Tint=Color;Life=FMath::Max(.2f,Seconds);Radius=BlastRadius;Emission=7.f;
    Glow=ASAssets::Glow(this,FMath::Lerp(Color,FLinearColor::White,.18f),Emission);
    for(int32 I=0;I<12;++I) {FBit Orb;Orb.Mesh=AddBit(TEXT("Sphere"),Glow);Orb.Mesh->SetRelativeScale3D(FVector(.01f));Bits.Add(Orb);}                         // 0-5: orb A and its trail; 6-11: orb B and its
    FBit Ring;Ring.Mesh=AddBit(TEXT("SM_Ring"),nullptr);ASAssets::StyleGlowBand(Ring.Mesh,TEXT("SM_Ring"),Color,4.5f,.44f,.52f,0.f);Ring.Mesh->SetRelativeScale3D(FVector(Radius/50.f,Radius/50.f,1.f));Bits.Add(Ring); // 12: the coming blast's edge, closing in
    FBit Core;Core.Mesh=AddBit(TEXT("Sphere"),Glow);Core.Mesh->SetRelativeScale3D(FVector(.01f));Bits.Add(Core);                                                   // 13: the core, growing
}

FString AASClawFx::DebugText() const {
    static const TCHAR* Names[]={TEXT("rake"),TEXT("sparks"),TEXT("dust"),TEXT("ring"),TEXT("bond"),TEXT("tether"),TEXT("arc"),TEXT("call"),TEXT("converge"),TEXT("column")};
    FString First;if(Bits.Num()>0&&Bits[0].Mesh) {const auto* M=Bits[0].Mesh.Get();First=FString::Printf(TEXT(" | bit0 world %s scale %s visible %d mesh %s material %s"),*M->GetComponentLocation().ToCompactString(),*M->GetComponentScale().ToCompactString(),M->IsVisible()?1:0,*GetNameSafe(M->GetStaticMesh()),*GetNameSafe(M->GetMaterial(0)));}
    return FString::Printf(TEXT("%s age %.2f of %.2f at %s, %d bits, holder %s%s"),Names[FMath::Clamp(int32(Kind),0,9)],Age,Life,*GetActorLocation().ToCompactString(),Bits.Num(),*GetNameSafe(Holder.Get()),*First);
}
void AASClawFx::TickTeamLook(float Delta) {
    (void)Delta;const auto* S=AS::State(this);const float Now=S?S->SimTime:0.f;
    TArray<UStaticMeshComponent*> Meshes;Meshes.Reserve(Bits.Num());for(const FBit& B:Bits) Meshes.Add(B.Mesh);
    const int32 Needed=Kind==Look::Tether?15:(Kind==Look::Arc?6:(Kind==Look::Call?5:(Kind==Look::Converge?14:2)));
    if(Meshes.Num()<Needed) {Destroy();return;} // a look that could not finish its setup draws nothing rather than reaching past its bits
    for(const UStaticMeshComponent* M:Meshes) if(!M) {Destroy();return;}
    if(Kind==Look::Tether) {
        const AASCharacter* H=Holder.Get();const AASCharacter* P=H?H->LinkPartner.Get():nullptr;
        const bool Pair=H&&H->LinkMode==ASTeam::LinkPair,Anchor=H&&H->LinkMode==ASTeam::LinkAnchor;
        const bool Holds=H&&!H->bDowned&&Now<H->LinkUntil&&((Pair&&P&&!P->bDowned)||Anchor);
        if(Holds) {
            PointA=H->GetActorLocation()+FVector(0,0,40);PointB=Pair?P->GetActorLocation()+FVector(0,0,40):FVector(H->LinkAnchor)+FVector(0,0,50);EndedAt=-1.f;
        } else if(EndedAt<0.f) EndedAt=Age;
        const float Fade=EndedAt<0.f?1.f:FMath::Clamp(1.f-(Age-EndedAt)/.30f,0.f,1.f);
        if(Fade<=0.f||Age>Life+4.f) {Destroy();return;}
        const float Left=Holds?FMath::Clamp((H->LinkUntil-Now)/ASTeam::TetherSeconds,0.f,1.f):0.f;
        const float Flicker=.85f+.15f*FMath::Sin(Age*37.f),Dying=Left<.25f&&Holds?(.5f+.5f*FMath::Sin(Age*28.f)):1.f; // the last quarter of the time the line gutters
        Jag(Meshes,1,9,PointA,PointB,Seed,Age,.016f*Fade,20.f);
        Bar(Meshes[0],PointA,PointB,.040f*Fade*(.8f+.2f*FMath::Sin(Age*9.f)));
        for(int32 I=0;I<2;++I) {const float T=FMath::Frac(Age*.9f+I*.5f);Meshes[10+I]->SetRelativeScale3D(FVector(.16f*Fade));Meshes[10+I]->SetWorldLocation(FMath::Lerp(PointA,PointB,T));}
        Meshes[12]->SetVisibility(Anchor&&Fade>0.f);Meshes[13]->SetVisibility(Fade>0.f&&(Anchor||Pair));Meshes[14]->SetVisibility(Pair&&Fade>0.f);
        const bool Close=H&&H->LinkBuffActive(); // the rings under the two heroes are bright while the damage bonus holds, dim when they have drifted apart
        const float RingGlow=(Close||Anchor)?1.f:.35f;
        if(Anchor) { // the stake: a column of light and a ring on the floor under it
            const FVector Foot=FVector(H?FVector(H->LinkAnchor):PointB);
            Bar(Meshes[12],Foot-FVector(0,0,10),Foot+FVector(0,0,170),.04f*Fade);Meshes[13]->SetWorldLocation(Foot-FVector(0,0,6));Meshes[13]->SetWorldRotation(FRotator::ZeroRotator);Meshes[13]->SetWorldScale3D(FVector(1.6f*Fade+.2f,1.6f*Fade+.2f,1.f));
        } else if(Pair&&H&&P) { // the heroes: a ring under each, breathing
            const float Breath=1.f+.06f*FMath::Sin(Age*7.f);const float HalfH=H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-4.f;
            Meshes[13]->SetWorldLocation(H->GetActorLocation()-FVector(0,0,HalfH));Meshes[13]->SetWorldRotation(FRotator::ZeroRotator);Meshes[13]->SetWorldScale3D(FVector(1.5f*Breath*Fade,1.5f*Breath*Fade,1.f));
            Meshes[14]->SetWorldLocation(P->GetActorLocation()-FVector(0,0,HalfH));Meshes[14]->SetWorldRotation(FRotator::ZeroRotator);Meshes[14]->SetWorldScale3D(FVector(1.5f*Breath*Fade,1.5f*Breath*Fade,1.f));
        }
        for(int32 I=13;I<=14;++I) if(auto* M=Cast<UMaterialInstanceDynamic>(Meshes[I]->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),4.5f*RingGlow*Fade*Dying);
        if(Glow) Glow->SetScalarParameterValue(TEXT("Emission"),Emission*Fade*Flicker*Dying);
        if(Matte) Matte->SetScalarParameterValue(TEXT("Emission"),2.2f*Fade*Dying);
        return;
    }
    if(Kind==Look::Arc) {
        if(Age>=Life) {Destroy();return;}
        const float Fade=1.f-Age/Life;Jag(Meshes,0,6,PointA,PointB,Seed,Age,.03f*Fade+.004f,18.f);
        if(Glow) Glow->SetScalarParameterValue(TEXT("Emission"),Emission*Fade);
        return;
    }
    if(Kind==Look::Call) {
        const AASCharacter* H=Holder.Get();const bool Open=H&&!H->bDowned&&H->TwinCallUntil>Now;
        if(!Open&&EndedAt<0.f) EndedAt=Age;
        const float Fade=EndedAt<0.f?1.f:FMath::Clamp(1.f-(Age-EndedAt)/.2f,0.f,1.f);
        if(!H||Fade<=0.f||Age>Life+1.f) {Destroy();return;}
        const float Left=Open?FMath::Clamp((H->TwinCallUntil-Now)/ASTeam::TwinWindow,0.f,1.f):0.f;
        const FVector Feet=H->GetActorLocation()-FVector(0,0,H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-4.f);
        const float R=70.f+130.f*Left; // the ring closes on the caster as the window runs out: the partner sees how long is left
        Meshes[0]->SetWorldLocation(Feet);Meshes[0]->SetWorldRotation(FRotator::ZeroRotator);Meshes[0]->SetWorldScale3D(FVector(R/50.f*Fade,R/50.f*Fade,1.f));
        if(auto* M=Cast<UMaterialInstanceDynamic>(Meshes[0]->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),Emission*Fade*(.7f+.5f*FMath::Sin(Age*14.f)));
        for(int32 I=0;I<4;++I) {
            const float A=Age*3.4f+I*UE_HALF_PI;
            Meshes[1+I]->SetWorldLocation(Feet+FVector(FMath::Cos(A)*(R*.72f),FMath::Sin(A)*(R*.72f),95.f+8.f*FMath::Sin(Age*9.f+I)));
            Meshes[1+I]->SetWorldRotation(FRotator(Age*180.f,Age*240.f+I*40.f,45.f));Meshes[1+I]->SetWorldScale3D(FVector(.10f*Fade));
        }
        if(Glow) Glow->SetScalarParameterValue(TEXT("Emission"),6.5f*Fade);
        return;
    }
    if(Kind==Look::Column) { // it shoots up in the first fifth of its life, then thins away
        if(Age>=Life) {Destroy();return;}
        const float T=Age/Life,Up=FMath::Min(1.f,T/.2f),Rise=1.f-FMath::Square(1.f-Up),Height=PointB.X*Rise,Fade=FMath::Square(1.f-T),Width=Radius*(.35f+.65f*(1.f-T));
        Bar(Meshes[0],PointA,PointA+FVector(0,0,Height),Width/100.f);Bar(Meshes[1],PointA,PointA+FVector(0,0,Height*1.05f),Width*.38f/100.f);
        if(Glow) Glow->SetScalarParameterValue(TEXT("Emission"),Emission*Fade);
        return;
    }
    // converge
    if(Age>=Life) {Destroy();return;}
    const float T=FMath::Clamp(Age/Life,0.f,1.f),Ease=T*T*(3.f-2.f*T);
    for(int32 Orb=0;Orb<2;++Orb) {
        const FVector From=(Orb==0?PointA:PointB)+FVector(0,0,60.f),To=PointMid+FVector(0,0,80.f);
        for(int32 K=0;K<6;++K) {
            const float Back=FMath::Max(0.f,Ease-K*.075f);const FVector P=FMath::Lerp(From,To,Back)+FVector(0,0,FMath::Sin(Back*UE_PI)*60.f); // a rising arc, the trail behind it
            const float Size=(K==0?.40f:.30f-.045f*K)*(.3f+.7f*FMath::Min(1.f,Age/.1f));
            Meshes[Orb*6+K]->SetWorldLocation(P);Meshes[Orb*6+K]->SetWorldScale3D(FVector(FMath::Max(.02f,Size)));
        }
    }
    const float Close=1.f-T*.88f;
    Meshes[12]->SetWorldLocation(PointMid+FVector(0,0,6));Meshes[12]->SetWorldRotation(FRotator::ZeroRotator);Meshes[12]->SetWorldScale3D(FVector(Radius/50.f*Close,Radius/50.f*Close,1.f));
    if(auto* M=Cast<UMaterialInstanceDynamic>(Meshes[12]->GetMaterial(0))) M->SetScalarParameterValue(TEXT("Emission"),4.5f*(.4f+.9f*T));
    Meshes[13]->SetWorldLocation(PointMid+FVector(0,0,70.f));Meshes[13]->SetWorldScale3D(FVector(.12f+.85f*T*T));
    if(Glow) Glow->SetScalarParameterValue(TEXT("Emission"),Emission*(.6f+.8f*T));
}

// ---------------------------------------------------------------- the cues
// One cue per moment, from the server, on every machine (AASCharacter::TeamCue): the sounds, the bursts, the camera. The words, the banner and the big number are the controller's (SynergyCue,
// SynergyNumber); the hit-stop is the host's. At is where it happens; To a second point or a number; Size a radius or a time (see each cue).
void AASCharacter::TeamCue_Implementation(uint8 Kind,FVector_NetQuantize At,FVector_NetQuantize To,float Size) {
    if(GetNetMode()==NM_DedicatedServer) return;
    UWorld* World=GetWorld();auto* S=AS::State(this);if(!World||!S||!FMath::IsFinite(Size)) return;
    const FVector Point(At),Other(To);
    if(TeamLog()) UE_LOG(LogTemp,Display,TEXT("AshenTeam: cue %d reached %s (%s machine) at %s, size %.0f (%d effects alive)"),int32(Kind),*GetName(),HasAuthority()?TEXT("the host"):TEXT("a client"),*Point.ToCompactString(),Size,AASClawFx::Active);
    const auto Fx=[World]()->AASClawFx* {return World->SpawnActor<AASClawFx>();};
    // the local player's own camera: stronger the nearer the moment is to its hero
    const auto Punch=[this](const FVector& Where,float Fov,float Shake) {
        if(APawn* Local=UGameplayStatics::GetPlayerPawn(this,0)) if(auto* Hero=Cast<AASCharacter>(Local)) {
            const float Falloff=FMath::Clamp(1.f-float(FVector::Dist(Local->GetActorLocation(),Where))/2200.f,.3f,1.f);Hero->TeamPunch(Fov*Falloff,Shake*Falloff);
        }
    };
    const auto Hum2D=[this](const TCHAR* Name,float Volume,float Pitch) { // a call, a tether, a collapse: both players hear it wherever they stand
        if(auto* Cue=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Audio/%s.%s"),Name,Name),nullptr,LOAD_NoWarn|LOAD_Quiet)) UGameplayStatics::PlaySound2D(this,Cue,Volume,Pitch);
    };
    switch(Kind) {
    case ASTeam::CueBrandSet: { // At the enemy, Size its radius, To.X 1 for a boss
        ASClawFx::Sound(this,TEXT("S_BrandSet"),1,Point,.9f,.97f,1.03f);
        if(auto* Ring=Fx()) Ring->SetupRing(Point,BrandGold,Size+110.f,.42f,.14f);
        if(auto* Sparks=Fx()) Sparks->SetupSparks(Point+FVector(0,0,40),Point+FVector(0,0,260),BrandGold,.5f);
        Punch(Point,-1.2f,.4f);
        break;}
    case ASTeam::CueBrandTick: { // To.X the quarter just passed (1-3), Size the percent
        const float Quarter=FMath::Clamp(float(Other.X),1.f,3.f);
        ASClawFx::Sound(this,TEXT("S_BrandTick"),1,Point,.55f+.1f*Quarter,.92f+.10f*Quarter,.95f+.10f*Quarter);
        if(auto* Ring=Fx()) Ring->SetupRing(Point,BrandGold,90.f+30.f*Quarter,.24f);
        break;}
    case ASTeam::CueBrandPop:
    case ASTeam::CueBrandDeathPop: { // Size the splash radius
        const bool Died=Kind==ASTeam::CueBrandDeathPop;const float R=FMath::Max(120.f,Size)*(Died?.8f:1.f);
        ASClawFx::Sound(this,TEXT("S_BrandPop"),1,Point,Died?.8f:1.f,Died?.88f:.98f,Died?.94f:1.04f);
        if(auto* Ring=Fx()) Ring->SetupRing(Point,BrandGold,R,.5f,.12f);
        if(auto* Ring=Fx()) Ring->SetupRing(Point,FLinearColor(1.f,.95f,.8f),R*.6f,.32f,.14f);
        if(auto* Column=Fx()) Column->SetupColumn(Point,BrandGold,Died?44.f:70.f,Died?320.f:520.f,.42f);
        for(int32 I=0;I<(Died?2:3);++I) if(auto* Sparks=Fx()) Sparks->SetupSparks(Point+FVector(0,0,30),Point+FVector(FMath::FRandRange(-90.f,90.f),FMath::FRandRange(-90.f,90.f),260.f),FLinearColor::LerpUsingHSV(BrandGold,FLinearColor::White,.3f),Died?.6f:1.f);
        S->Effect_Implementation(8,Point+FVector(0,0,20),FVector::UpVector,BrandGold,R);
        if(!Died) S->Effect_Implementation(11,Point+FVector(0,0,40),FVector::UpVector,BrandGold,1.5f);
        Punch(Point,Died?-2.f:-5.f,Died?1.f:2.4f);
        break;}
    case ASTeam::CueBrandFizzle:
        ASClawFx::Sound(this,TEXT("S_BrandFizzle"),1,Point,.5f,.95f,1.05f);
        if(auto* Ring=Fx()) Ring->SetupRing(Point,BrandGold*.6f,100.f,.5f);
        break;
    case ASTeam::CueTetherOn: { // At this hero, To its partner or its stake, Size 1 when paired
        const FVector Mid=(Point+Other)*.5f;
        Hum2D(TEXT("S_TetherOn"),.85f,1.f);
        ASClawFx::Sound(this,TEXT("S_TetherOn"),1,Mid,.5f,1.f,1.f);
        for(const FVector& End:{Point,Other}) {
            if(auto* Ring=Fx()) Ring->SetupRing(End,EmberOrange,150.f,.38f,.14f);
            if(auto* Sparks=Fx()) Sparks->SetupSparks(End+FVector(0,0,50),Mid+FVector(0,0,200),EmberOrange,.7f);
        }
        if(auto* Beam=Fx()) Beam->SetupTether(this,EmberOrange);
        Punch(Point,-2.f,.6f);
        break;}
    case ASTeam::CueTetherZap: { // At the enemy, To the nearest point of the line
        ASClawFx::Sound(this,TEXT("S_TetherZap"),3,Point,.34f,.94f,1.1f);
        if(auto* Arc=Fx()) Arc->SetupArc(Other,Point+FVector(0,0,20),EmberOrange,.14f);
        if(auto* Sparks=Fx()) Sparks->SetupSparks(Point+FVector(0,0,30),Other,EmberOrange,.35f);
        break;}
    case ASTeam::CueTetherEnd: { // At this hero, To the other end, Size 1 when it snapped
        const bool Snapped=Size>.5f;
        ASClawFx::Sound(this,TEXT("S_TetherEnd"),1,(Point+Other)*.5f,Snapped?.7f:.45f,Snapped?.9f:1.f,Snapped?.95f:1.05f);
        for(const FVector& End:{Point,Other}) if(auto* Sparks=Fx()) Sparks->SetupSparks(End+FVector(0,0,50),(Point+Other)*.5f+FVector(0,0,120),EmberOrange,Snapped?.9f:.4f);
        break;}
    case ASTeam::CueTwinCall: { // At the caller, Size the window in seconds
        Hum2D(TEXT("S_TwinCall"),.8f,1.f);
        if(auto* Ring=Fx()) Ring->SetupRing(Point,TwinViolet,230.f,.4f);
        if(auto* Call=Fx()) Call->SetupCall(this,TwinViolet);
        break;}
    case ASTeam::CueTwinLesser: { // At the caster, Size the radius
        ASClawFx::Sound(this,TEXT("S_TwinLesser"),1,Point,.8f,.97f,1.03f);
        if(auto* Ring=Fx()) Ring->SetupRing(Point,TwinViolet,Size,.36f,.11f);
        if(auto* Dust=Fx()) Dust->SetupDust(Point-FVector(0,0,70),Size*.5f,.5f);
        if(auto* Sparks=Fx()) Sparks->SetupSparks(Point,Point+FVector(0,0,240),TwinViolet,.5f);
        Punch(Point,-1.5f,.8f);
        break;}
    case ASTeam::CueTwinConverge: { // At the midpoint, To the other hero's place, Size the delay (the blast's radius is ASTeam::TwinRadius)
        Hum2D(TEXT("S_TwinConverge"),.9f,1.f);
        if(auto* Pull=Fx()) Pull->SetupConverge(Point,GetActorLocation(),Other,FMath::Max(.2f,Size),ASTeam::TwinRadius,TwinViolet);
        break;}
    case ASTeam::CueTwinBlast: { // At the midpoint, Size the radius
        const float R=FMath::Max(200.f,Size);
        Hum2D(TEXT("S_TwinBlast"),1.f,1.f);ASClawFx::Sound(this,TEXT("S_TwinBlast"),1,Point,.8f,.98f,1.02f);
        if(auto* Ring=Fx()) Ring->SetupRing(Point,TwinViolet,R*.45f,.30f,.16f);
        if(auto* Ring=Fx()) Ring->SetupRing(Point,FLinearColor(.92f,.85f,1.f),R*.75f,.46f,.12f);
        if(auto* Ring=Fx()) Ring->SetupRing(Point,TwinViolet,R,.62f,.10f);
        if(auto* Column=Fx()) Column->SetupColumn(Point,TwinViolet,150.f,900.f,.7f);
        for(int32 I=0;I<4;++I) if(auto* Sparks=Fx()) Sparks->SetupSparks(Point+FVector(0,0,40),Point+FVector(FMath::FRandRange(-140.f,140.f),FMath::FRandRange(-140.f,140.f),300.f),FLinearColor::LerpUsingHSV(TwinViolet,FLinearColor::White,.35f),1.f);
        if(auto* Dust=Fx()) Dust->SetupDust(Point-FVector(0,0,40),R*.45f,1.f);
        S->Effect_Implementation(8,Point+FVector(0,0,20),FVector::UpVector,TwinViolet,R);S->Effect_Implementation(11,Point+FVector(0,0,50),FVector::UpVector,TwinViolet,2.2f);
        Punch(Point,-9.f,3.4f);
        break;}
    default:break;
    }
}
void AASCharacter::TeamPunch(float FovDegrees,float Shake) {
    if(!IsLocallyControlled()||!FMath::IsFinite(FovDegrees)||!FMath::IsFinite(Shake)) return;
    if(const auto* GI=GetGameInstance<UASGameInstance>()) if(GI->bReduceMotion) return;
    if(FMath::Abs(FovDegrees)>FMath::Abs(TeamFovNow)) TeamFovNow=FovDegrees;
    if(Shake>HammerShakeStrength*FMath::Square(FMath::Max(0.f,1.f-HammerShakeAge/.28f))) {HammerShakeAge=0;HammerShakeStrength=Shake;bTeamShake=true;}
}
