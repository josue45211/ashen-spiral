#pragma once
#include "CoreMinimal.h"
#include "ASRules.h"
#include "ASBuildRules.h"
#include "ASAdventureRules.h"
#include "ASCareerRules.h"
#include "ASPresentationRules.h"
#include "ASData.generated.h"
UENUM() enum class EASHitKind : uint8 { Physical, Spiral, Burn, Poison, Frost, Received, Shield, Bleed, Shock };
UENUM() enum class EASPhase : uint8 { Hub, Countdown, Combat, Reward, Defeat, Victory };
USTRUCT() struct FASBuild {
    GENERATED_BODY()
    UPROPERTY() int32 Weapon=0;
    UPROPERTY() int32 Style=0;
    UPROPERTY() int32 Ability=0;
    UPROPERTY() TArray<int32> Ranks;
    UPROPERTY() int32 RelicPool=8;
    UPROPERTY() int32 StylePool=6;
    UPROPERTY() int32 TalentMask=0;
    FASBuild() { Ranks.Init(0,ASRules::HearthCount); }
    int32 Rank(int32 I) const { return Ranks.IsValidIndex(I)?Ranks[I]:0; }
    bool Talent(int32 I) const { return (TalentMask&(1<<I))!=0; }
    void Sanitize();
};
USTRUCT() struct FASEndlessRecord {
    GENERATED_BODY()
    UPROPERTY() FString RunId;
    UPROPERTY() int32 Wave=0;
    UPROPERTY() float Damage=0;
    UPROPERTY() int32 Kills=0;
    UPROPERTY() float DamageTaken=0;
    UPROPERTY() float PartnerDamage=0;
    UPROPERTY() int32 PartnerKills=0;
    UPROPERTY() float PartnerDamageTaken=0;
    UPROPERTY() FString EndedUtc;
    void Sanitize();
};
USTRUCT() struct FASProfileData {
    GENERATED_BODY()
    UPROPERTY() int32 Schema=1;
    UPROPERTY() int32 MigrationVersion=0;
    UPROPERTY() int32 RewardedLevel=1;
    UPROPERTY() int32 TitleChoice=-1;
    UPROPERTY() TArray<int32> RewardedTiers;
    UPROPERTY() int32 ColorChoice=-1;
    UPROPERTY() int32 Experience=0;
    UPROPERTY() TArray<int32> Career;
    UPROPERTY() FString ProfileId;
    UPROPERTY() int32 Ash=0;
    UPROPERTY() int32 RelicEmbers=0;
    UPROPERTY() int32 Tablets=0;
    UPROPERTY() int32 SkillPoints=0;
    UPROPERTY() int32 OwnedWeapons=1;
    UPROPERTY() int32 BestRoom=0;
    UPROPERTY() int32 Wins=0;
    UPROPERTY() int32 Deaths=0;
    UPROPERTY() int32 BestEndlessWave=0;
    UPROPERTY() int32 EndlessChoices=0;
    UPROPERTY() TArray<FASEndlessRecord> EndlessRecords;
    UPROPERTY() TArray<int32> Materials;
    UPROPERTY() bool bRefinementBought=false;
    UPROPERTY() FASBuild Build;
    UPROPERTY() TArray<FString> Receipts;
    FASProfileData() { Materials.Init(0,5);Career.Init(0,ASCareer::Count); }
    void Sanitize();
};
USTRUCT() struct FASReceipt {
    GENERATED_BODY()
    UPROPERTY() int32 Experience=0;
    UPROPERTY() TArray<int32> Career;
    UPROPERTY() FString Id;
    // Every economic receipt is bound to the owning character, never the host slot.
    UPROPERTY() FString ProfileId;
    UPROPERTY() bool bEndless=false;
    UPROPERTY() int32 EndlessWave=0;
    UPROPERTY() bool bRunEnded=false;
    UPROPERTY() FASEndlessRecord EndlessRecord;
    UPROPERTY() int32 AshCost=0;
    UPROPERTY() int32 ScrapCost=0;
    UPROPERTY() int32 Scrap=0;
    UPROPERTY() int32 Ash=0;
    UPROPERTY() int32 Embers=0;
    UPROPERTY() int32 Tablets=0;
    UPROPERTY() int32 Points=0;
    UPROPERTY() int32 Room=0;
    UPROPERTY() int32 Ward=0;
    UPROPERTY() int32 Material=0;
    UPROPERTY() int32 Cores=0;
    UPROPERTY() bool bWin=false;
    UPROPERTY() bool bDeath=false;
};
struct FASWeapon { const TCHAR* Name; const TCHAR* Desc; int32 Cost; float Rate; };
struct FASRelic { const TCHAR* Id; const TCHAR* Name; const TCHAR* Desc; };
struct FASStyle { const TCHAR* Name; const TCHAR* Desc; };
struct FASHearth { const TCHAR* Id; const TCHAR* Branch; const TCHAR* Name; const TCHAR* Desc; int32 Cost; int32 MaxRank; };
struct FASRunUpgrade {const TCHAR* Name;const TCHAR* Desc;};
struct FASTalent { const TCHAR* Name; const TCHAR* Desc; int32 Branch; int32 Cost; int32 Required; };
namespace ASData {
    extern const FASWeapon Weapons[ASRules::WeaponCount];
    extern const FASRelic Relics[24];
    extern const FASStyle Styles[10];
    extern const FASStyle Abilities[ASAdventure::AbilityCount];
    extern const FASHearth Hearth[ASRules::HearthCount];
    extern const FASRunUpgrade RunUpgrades[ASRules::RunUpgradeCount];
    extern const FASTalent Talents[12];
    extern const TCHAR* Biomes[5];
    extern const TCHAR* Bosses[4];
    extern const TCHAR* Materials[5];
    extern const TCHAR* ChestKinds[6];
    EASHitKind WeaponElement(int32 Weapon);
    FString CareerTitle(int32 Choice,int32 Tier);
    FLinearColor HitColor(EASHitKind Kind);
    const TCHAR* ElementName(EASHitKind Kind);
    FLinearColor PlayerColor(int32 Index);
    FLinearColor ArmorColor(int32 Choice,int32 PlayerIndex);
    FLinearColor MapPlayerColor(int32 PlayerIndex);
    FLinearColor CrosshairColor(int32 Choice);
    extern const TCHAR* ArmorColorNames[8];
    extern const TCHAR* CrosshairColorNames[8];
    extern const TCHAR* CrosshairStyles[4];
    FLinearColor BiomeColor(int32 Index);
    constexpr int32 Protocol=2300;
}
