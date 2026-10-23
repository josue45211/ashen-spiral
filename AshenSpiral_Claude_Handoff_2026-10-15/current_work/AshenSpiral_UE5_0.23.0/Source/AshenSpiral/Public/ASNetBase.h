#pragma once
// 0.24.2: a character standing on a component that a client cannot name replicates that component as its movement base. The arena's floors, cover and ruins are built at run time
// (NewObject components of AASWorld), so they have no stable network name: the client cannot resolve the base, and UE then skips SimulateMovement for that proxy altogether (the
// engine's CharacterMovementComponent.cpp: "If base is not resolved on the client, we should not try to simulate at all"), so every enemy and the other player's hero is shown without
// smoothing or prediction, jumping from one net update to the next. The host log of a real co-op playtest showed 12.7k "FNetGUIDCache::SupportsObject ... NOT Supported" warnings
// for exactly this. Nothing here moves with its floor (the cover's collision proxies are fixed), so the REPLICATED copy of the base is dropped when no client could resolve it; the
// server's own movement keeps its base, and a base a client can name (a stable-named component, another pawn's capsule) is kept.
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/MovementBaseInterface.h"
#include "HAL/IConsoleManager.h"
namespace ASNetBase {
// as.net.dropbase 0 switches the fix off (to compare against the old behaviour, or if it ever misbehaves); it is read by the server when it replicates. Defined in ASCharacter.cpp so it
// exists from startup (a function-local static registers only when first called, after a script has already tried to set it).
extern TAutoConsoleVariable<int32> CVarDropBase;
inline bool Enabled() {return CVarDropBase.GetValueOnGameThread()!=0;}
inline void DropUnresolvable(FBasedMovementInfo& Replicated) {
    if(!Enabled()) return;
    const UObject* Base=Replicated.MovementBaseInterfaceData.GetMovementBaseObject();
    if(!Replicated.bServerHasBaseComponent&&!Base) return;       // no base: nothing to drop
    if(Base&&Base->IsNameStableForNetworking()) return;           // a client can resolve it: it stays
    Replicated.MovementBaseInterfaceData.Clear();Replicated.bServerHasBaseComponent=false;Replicated.BoneName=NAME_None;Replicated.bRelativeRotation=false;Replicated.BaseID=0;
}
}
