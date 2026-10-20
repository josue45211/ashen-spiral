#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "ASBuildHeroAnimCommandlet.generated.h"
// 0.24.2 Phase F: builds /Game/Characters/Hero/ABP_AshenHero (see the .cpp). Editor only.
UCLASS() class UASBuildHeroAnimCommandlet : public UCommandlet {
    GENERATED_BODY()
public:
    virtual int32 Main(const FString& Params) override;
};
