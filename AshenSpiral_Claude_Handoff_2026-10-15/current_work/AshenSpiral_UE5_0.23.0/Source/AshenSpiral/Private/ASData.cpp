#include "ASData.h"
#include "ASRules.h"
#include "ASArsenalRules.h"
namespace ASData {
const FASWeapon Weapons[ASRules::WeaponCount] = {
    {TEXT("Cinder Blade"), TEXT("Hold Attack for a fast three-hit combo. The third strike lunges forward. No reload; attack speed accelerates the full swing. Slash detonates ash on pulled enemies."), 0, ASArsenal::Rate(0)},
    {TEXT("Grav Hammer"), TEXT("Hold Attack: left swing, right swing, planted frontal smash (2.6x damage). Two hands; no reload. Hit three commits and collapses your nearest well. RMB leaps; G uses Spiral."), 15, ASArsenal::Rate(1)},
    {TEXT("Ash Bow"), TEXT("Accurate arrows pass straight through wells. Splits require a Split upgrade and an enemy hit."), 25, 0.3f},
    {TEXT("Spiral Glaive"), TEXT("Thrown blades orbit a well, cutting enemies inside."), 35, 0.48f},
    {TEXT("Ember Staff"), TEXT("Mark enemies with ash. Wells burn marks; perfect wells slow."), 45, 0.34f},
    {TEXT("Rift Scythe"), TEXT("No reload; attack speed accelerates the full scythe swing. Dash through a well for a wide crescent and extra invulnerability."), 55, 0.4f},
    {TEXT("Cinder Scattergun"), TEXT("Six burning pellets in a tight cone. Devastating nearby; damage falls with distance. One proc-carrying pellet per shot."), 65, ASArsenal::Rate(6)},
    {TEXT("Tempest Rotary"), TEXT("Rapid frost rounds. Sustained fire widens the cone; release briefly to recover accuracy."), 80, ASArsenal::Rate(7)},
    {TEXT("Rift Mortar"), TEXT("Launch an ember rocket that bursts on contact. Splash respects cover, scales with area upgrades, and cannot hurt allies."), 95, ASArsenal::Rate(8)},
};
const FASRelic Relics[24] = {
    {TEXT("ignite"), TEXT("Ashen Kindling"), TEXT("Wells ignite enemies inside.")},
    {TEXT("refund"), TEXT("Borrowed Momentum"), TEXT("Dashing through a well refreshes Spiral once per well.")},
    {TEXT("healpulse"), TEXT("Quiet Collapse"), TEXT("A collapsing well heals its owner for 5 HP.")},
    {TEXT("crit"), TEXT("Faultless Edge"), TEXT("First attack after a perfect well is guaranteed critical.")},
    {TEXT("persist"), TEXT("Lingering Orbit"), TEXT("Up to two wells follow you into the next room for 3 seconds.")},
    {TEXT("attune"), TEXT("Party Attunement"), TEXT("Your wells accelerate allies' attacks while inside.")},
    {TEXT("echo"), TEXT("Second Echo"), TEXT("Perfect wells repeat their collapse after a brief delay.")},
    {TEXT("shards"), TEXT("Shattered Ring"), TEXT("Collapsed wells fire six poison arrows.")},
    {TEXT("twin"), TEXT("Twin Singularity"), TEXT("Perfect casts plant a second, smaller well.")},
    {TEXT("shield"), TEXT("Ash Mantle"), TEXT("Releasing Spiral grants a 10-point shield.")},
    {TEXT("trail"), TEXT("Cinder Wake"), TEXT("Dash trails burn enemies that cross them.")},
    {TEXT("orbit"), TEXT("Orbiting Remains"), TEXT("Wells gain an orbiting blade.")},
    {TEXT("chain"), TEXT("Chain Reaction"), TEXT("Collapse hastens nearby wells by one second.")},
    {TEXT("blood"), TEXT("Blood Gravity"), TEXT("Casting below half HP spends 3 HP for a perfect well.")},
    {TEXT("frost"), TEXT("Still Ash"), TEXT("Wells slow enemies caught inside.")},
    {TEXT("nova"), TEXT("Exit Wound"), TEXT("Dashing out of a well erupts for 28 damage.")},
    {TEXT("reach"), TEXT("Long Spiral"), TEXT("Place your well farther along your aim.")},
    {TEXT("soul"), TEXT("Patient Gravity"), TEXT("Killing a pulled enemy extends its well, capped at 6 seconds.")},
    {TEXT("guard"), TEXT("Inner Sanctuary"), TEXT("Standing in your well absorbs one hit per cast.")},
    {TEXT("focus"), TEXT("Measured Breath"), TEXT("Widen the perfect-release timing window.")},
    {TEXT("rebound"), TEXT("Returning Ash"), TEXT("Your projectiles bounce once.")},
    {TEXT("magnet"), TEXT("Hungry Heart"), TEXT("A collapsing well draws health drops to you.")},
    {TEXT("after"), TEXT("Afterimage"), TEXT("Dash plants a short-lived well at its origin.")},
    {TEXT("link"), TEXT("Linked Landing"), TEXT("Dashing through an ally well shields both players.")},
};
const FASStyle Abilities[ASAdventure::AbilityCount] = {
    {TEXT("Rune Lance"),TEXT("25 Focus / 1.2s. A 48-damage aimed lance; +25% against staff marks. Rune Nova turns the impact into a 300cm area blast. Pairs with Ember Staff.")},
    {TEXT("Winter Fan"),TEXT("30 Focus / 6s. A 650cm forward cone deals 24 damage and builds frost twice. Freeze foes before a hammer slam or line up a bow shot. Bosses resist hard control.")},
    {TEXT("Storm Thread"),TEXT("32 Focus / 5s. Aim at a foe to arc through up to four enemies for 38/29/22/17 damage. +30% against chilled or poisoned targets. Pairs with frost and poison weapons.")},
    {TEXT("Venom Bloom"),TEXT("35 Focus / 7s. Plant a 320cm poison field for 4s. Pulses deal 7 damage and apply poison, or +30% pulse damage against bleeding enemies. Gather with Spiral; finish with ranged attacks.")},
    {TEXT("War Cry"),TEXT("28 Focus / 9s. Stagger nearby foes for 18 damage; gain 22 shield and +20% melee damage for 4s. Pairs with blade, hammer and scythe. No shield or buff between waves.")},
    {TEXT("Cinder Mine"),TEXT("30 Focus / 6s. Place an aimed rune that explodes after 1s for 68 damage in 330cm and ignites survivors. +25% damage to pulled foes. Set a well, then detonate.")}
};
const FASStyle Styles[10] = {
    {TEXT("Gather"), TEXT("Balanced pull and collapse.")},
    {TEXT("Undertow"), TEXT("Stronger pull; shorter duration.")},
    {TEXT("Stillness"), TEXT("Slow enemies inside the well.")},
    {TEXT("Orbit"), TEXT("Every well carries an orbiting blade.")},
    {TEXT("Sanctuary"), TEXT("Collapse heals you for 3 HP.")},
    {TEXT("Recoil"), TEXT("Casting refreshes Dash.")},
    {TEXT("Twin"), TEXT("Main well at full size; second well at 72% size.")},
    {TEXT("Anchor"), TEXT("Long-lived wells with a tighter radius.")},
    {TEXT("Rift"), TEXT("Collapsing wells erupt across a wider area.")},
    {TEXT("Convergence"), TEXT("Perfect wells grant a critical next attack.")},
};
const FASHearth Hearth[ASRules::HearthCount] = {
    {TEXT("hp"), TEXT("Survivor"), TEXT("Ashen Vessel"), TEXT("+8 starting HP per rank."), 8, 3},
    {TEXT("ward"), TEXT("Survivor"), TEXT("Ember Shell"), TEXT("+5 starting shield per rank."), 10, 3},
    {TEXT("healing"), TEXT("Survivor"), TEXT("Gentle Collapse"), TEXT("Your well collapses heal 1 HP per rank."), 12, 3},
    {TEXT("mercy"), TEXT("Survivor"), TEXT("Second Breath"), TEXT("Once per run, fatal damage restores 25% HP."), 25, 1},
    {TEXT("radius"), TEXT("Arcanist"), TEXT("Widening Spiral"), TEXT("+25 cm well radius per rank."), 8, 3},
    {TEXT("duration"), TEXT("Arcanist"), TEXT("Held Ash"), TEXT("+0.25 seconds well duration per rank."), 10, 3},
    {TEXT("perfect"), TEXT("Arcanist"), TEXT("Measured Release"), TEXT("Perfect window widens 0.025s per rank."), 12, 3},
    {TEXT("pull"), TEXT("Arcanist"), TEXT("Weight of Names"), TEXT("+45 cm/s pull speed per rank."), 12, 3},
    {TEXT("dash"), TEXT("Wayfarer"), TEXT("Light Step"), TEXT("Dash recharges 0.06s faster per rank."), 8, 3},
    {TEXT("iframe"), TEXT("Wayfarer"), TEXT("Ash Body"), TEXT("+0.025s dash invulnerability per rank."), 10, 3},
    {TEXT("trail"), TEXT("Wayfarer"), TEXT("Long Wake"), TEXT("Dash trail lasts 0.2s longer per rank."), 12, 3},
    {TEXT("landing"), TEXT("Wayfarer"), TEXT("Safe Landing"), TEXT("Dash grants at least 0.4s invulnerability."), 20, 1},
    {TEXT("choice"), TEXT("Tactician"), TEXT("Another Possibility"), TEXT("Reward chests offer five choices instead of four."), 20, 1},
    {TEXT("cinders"), TEXT("Tactician"), TEXT("Provision"), TEXT("Start with +10 Cinders per rank."), 8, 3},
    {TEXT("charge"), TEXT("Tactician"), TEXT("Ability Cooldown"), TEXT("-10% of the base Spiral cooldown per rank. Total cooldown reduction caps at 65%."), 12, 5},
    {TEXT("ally"), TEXT("Tactician"), TEXT("Shared Momentum"), TEXT("Ally-well launch heals 3 HP, once per 3 seconds."), 20, 1},
    {TEXT("attack_speed"), TEXT("Combat"), TEXT("Attack Speed"), TEXT("+8% attack speed per rank. Applies to every weapon."), 10, 5},
    {TEXT("damage"), TEXT("Combat"), TEXT("Damage"), TEXT("+10% damage per rank, including your abilities."), 12, 5},
    {TEXT("move_speed"), TEXT("Combat"), TEXT("Movement Speed"), TEXT("+5% running speed per rank. Dash distance is unchanged."), 10, 5},
    {TEXT("critical_chance"), TEXT("Combat"), TEXT("Critical Chance"), TEXT("+4 percentage points per rank, from a base 5%. Critical attacks deal 1.8x damage."), 14, 5},
};
const FASRunUpgrade RunUpgrades[ASRules::RunUpgradeCount] = {
    {TEXT("Ability Cooldown"),TEXT("-8% of the base Spiral cooldown this run. Stacks with permanent cooldown upgrades; 65% total cap.")},
    {TEXT("Attack Speed"),TEXT("+12% attack speed this run, for every weapon.")},
    {TEXT("Damage"),TEXT("+15% damage this run, including your abilities.")},
    {TEXT("Maximum Health"),TEXT("+15 maximum health and heal 15 HP immediately.")},
    {TEXT("Movement Speed"),TEXT("+5% running speed this run. Dash distance is unchanged.")},
    {TEXT("Critical Chance"),TEXT("+5 percentage points of critical chance this run, up to 50% total.")},
};

const FASTalent Talents[12]={
 {TEXT("Iron Lungs"),TEXT("Dash recharges 15% faster."),0,1,0},
 {TEXT("Thick Skin"),TEXT("8% damage resistance; 12% below 40% health."),0,1,0},
 {TEXT("Second Wind"),TEXT("One fatal rescue per run at 30% health."),0,2,2},
 {TEXT("Unbroken"),TEXT("Gain a 20% health shield before crossing 25% health; 12s recharge."),0,3,4},
 {TEXT("Rally Step"),TEXT("Dash grants nearby allies 10% damage for 3 seconds."),1,1,0},
 {TEXT("Guardian Link"),TEXT("A heavy hit within 720cm grants the talent holder a 10% health shield; 12s recharge."),1,1,0},
 {TEXT("Coordinated Strike"),TEXT("Alternating ally hits deal +18% damage; +5% solo."),1,2,2},
 {TEXT("Twin Resonance"),TEXT("Heal 2% near your ally every 8s; 1% solo."),1,3,4},
 {TEXT("Lucky Find"),TEXT("Earn 20% more common materials on room clear."),2,1,0},
 {TEXT("Retired talent"),TEXT("Former Treasure Sense. Its skill point is refunded automatically."),2,0,0},
 {TEXT("Efficient Extraction"),TEXT("Failure grants an additional 8 Ash."),2,2,1},
 {TEXT("Spiral Hoarder"),TEXT("Victory grants 30 additional Scrap and 10 Bone Dust."),2,3,3}
};
const TCHAR* Biomes[5]={TEXT("Forgotten Archives"),TEXT("Blood-Soaked Catacombs"),TEXT("Obsidian Gardens"),TEXT("Verdant Reliquary"),TEXT("Luminous Caverns")};
const TCHAR* Bosses[4]={TEXT("The Indexer"),TEXT("The Bellwether"),TEXT("The Horticulturist"),TEXT("The Rootbound Sovereign")};
const TCHAR* Materials[5]={TEXT("Scrap"),TEXT("Bone Dust"),TEXT("Retired"),TEXT("Retired"),TEXT("Spiral Core")};
const TCHAR* ChestKinds[6]={TEXT("Relic"),TEXT("Cinders"),TEXT("Skill"),TEXT("Heal"),TEXT("Shop"),TEXT("Event")};
EASHitKind WeaponElement(int32 W) {
    const EASHitKind Types[]={EASHitKind::Burn,EASHitKind::Frost,EASHitKind::Poison,EASHitKind::Frost,EASHitKind::Burn,EASHitKind::Poison,EASHitKind::Burn,EASHitKind::Frost,EASHitKind::Burn};
    return Types[FMath::Clamp(W,0,ASRules::WeaponCount-1)];
}
FString CareerTitle(int32 Choice,int32 Tier) {
    if(Choice<0||Choice>=ASCareer::Count||Tier<1) return TEXT("Unbound");
    const TCHAR* Ranks[]={TEXT("I"),TEXT("II"),TEXT("III")};return FString::Printf(TEXT("%s %s"),UTF8_TO_TCHAR(ASCareer::Achievements[Choice].Name),Ranks[FMath::Clamp(Tier,1,3)-1]);
}
FLinearColor HitColor(EASHitKind Kind) {
    switch(Kind) {
    case EASHitKind::Burn:return FLinearColor(1.f,.34f,.08f);
    case EASHitKind::Poison:return FLinearColor(.40f,1.f,.22f);
    case EASHitKind::Frost:return FLinearColor(.26f,.82f,1.f);
    case EASHitKind::Bleed:return FLinearColor(1.f,.12f,.4f);
    case EASHitKind::Shock:return FLinearColor(1.f,.9f,.2f);
    case EASHitKind::Spiral:return FLinearColor(.72f,.48f,1.f);
    case EASHitKind::Received:return FLinearColor(1.f,.19f,.23f);
    case EASHitKind::Shield:return FLinearColor(.23f,.72f,1.f);
    default:return FLinearColor(.94f,.96f,1.f);
    }
}
const TCHAR* ElementName(EASHitKind Kind) {
    switch(Kind) {case EASHitKind::Bleed:return TEXT("BLEED");case EASHitKind::Shock:return TEXT("SHOCK");case EASHitKind::Burn:return TEXT("BURN");case EASHitKind::Poison:return TEXT("POISON");case EASHitKind::Frost:return TEXT("FROST");default:return TEXT("PHYSICAL");}
}
const TCHAR* ArmorColorNames[8]={TEXT("EMBER"),TEXT("TEAL"),TEXT("AZURE"),TEXT("EMERALD"),TEXT("VIOLET"),TEXT("ROSE"),TEXT("SILVER"),TEXT("CRIMSON")};
const TCHAR* CrosshairColorNames[8]={TEXT("WHITE"),TEXT("CYAN"),TEXT("LIME"),TEXT("YELLOW"),TEXT("PINK"),TEXT("RED"),TEXT("BLUE"),TEXT("ORANGE")};
const TCHAR* CrosshairStyles[4]={TEXT("CROSS"),TEXT("DOT"),TEXT("RING"),TEXT("CROSS + DOT")};
FLinearColor ArmorColor(int32 Choice,int32 PlayerIndex) {
    if(Choice<0) return PlayerColor(PlayerIndex);
    const FLinearColor Colors[8]={FLinearColor(1,.57f,.15f),FLinearColor(.1f,.85f,.75f),FLinearColor(.12f,.5f,1),FLinearColor(.15f,.9f,.3f),FLinearColor(.65f,.3f,1),FLinearColor(1,.24f,.6f),FLinearColor(.8f,.87f,1),FLinearColor(.85f,.07f,.1f)};
    return Colors[FMath::Clamp(Choice,0,7)];
}
FLinearColor MapPlayerColor(int32 PlayerIndex) {return PlayerIndex==0?FLinearColor(.05f,.7f,1.f):FLinearColor::White;}
FLinearColor CrosshairColor(int32 Choice) {
    const FLinearColor Colors[8]={FLinearColor::White,FLinearColor(0,1,1),FLinearColor(.4f,1,.05f),FLinearColor(1,1,0),FLinearColor(1,.15f,.7f),FLinearColor(1,.1f,.1f),FLinearColor(.2f,.45f,1),FLinearColor(1,.5f,.05f)};
    return Colors[FMath::Clamp(Choice,0,7)];
}
FLinearColor PlayerColor(int32 I) { return I==0?FLinearColor(1.f,.57f,.15f):FLinearColor(.10f,.85f,.75f); }
FLinearColor BiomeColor(int32 I) {
 const FLinearColor Colors[5]={FLinearColor(.22f,.65f,.57f),FLinearColor(.9f,.20f,.12f),FLinearColor(.48f,.30f,1.f),FLinearColor(.20f,.75f,.26f),FLinearColor(.12f,.8f,.95f)};
 return Colors[FMath::Clamp(I,0,4)];
}
}
void FASBuild::Sanitize() {
 // Append new ranks so existing save indices retain their meaning.
 Ranks.SetNumZeroed(ASRules::HearthCount);for(int32 I=0;I<ASRules::HearthCount;++I) Ranks[I]=FMath::Clamp(Ranks[I],0,ASData::Hearth[I].MaxRank);
 Ability=FMath::Clamp(Ability,0,ASAdventure::AbilityCount-1);Weapon=FMath::Clamp(Weapon,0,ASRules::WeaponCount-1);StylePool=FMath::Clamp(StylePool,6,10);Style=FMath::Clamp(Style,0,StylePool-1);
 RelicPool=FMath::Clamp(RelicPool,8,24);TalentMask&=4095&~(1<<9);
}
void FASProfileData::Sanitize() {
 BestEndlessWave=FMath::Clamp(BestEndlessWave,0,10000);EndlessChoices=FMath::Clamp(EndlessChoices,0,2);
 for(auto& Entry:EndlessRecords) Entry.Sanitize();
 EndlessRecords.Sort([](const FASEndlessRecord& A,const FASEndlessRecord& B){return A.Wave>B.Wave;});
 if(EndlessRecords.Num()>10) EndlessRecords.SetNum(10);
 Schema=1;ColorChoice=FMath::Clamp(ColorChoice,-1,7);Materials.SetNumZeroed(5);
 ASRules::RetireUnusedProgress(Build.TalentMask,SkillPoints,Materials[0],Materials[2],Materials[3]);Build.Sanitize();
 MigrationVersion=20;
 Experience=FMath::Clamp(Experience,0,ASCareer::MaxValue);Career.SetNumZeroed(ASCareer::Count);for(int32& N:Career) N=FMath::Clamp(N,0,ASCareer::MaxValue);
 Ash=FMath::Clamp(Ash,0,ASRules::MaxCurrency);RelicEmbers=FMath::Clamp(RelicEmbers,0,1000000);
 Tablets=FMath::Clamp(Tablets,0,1000000);SkillPoints=FMath::Clamp(SkillPoints,0,1000000);
 OwnedWeapons=(OwnedWeapons&ASRules::WeaponMask)|1;BestRoom=FMath::Clamp(BestRoom,0,32);
 for(int32& N:Materials) N=FMath::Clamp(N,0,ASRules::MaxCurrency);
 if(!(OwnedWeapons&(1<<Build.Weapon))) Build.Weapon=0;
 if(ProfileId.Len()!=32) ProfileId=FGuid::NewGuid().ToString(EGuidFormats::Digits);
 // Paid cursors live in the same verified save transaction as balances.
 const auto LevelReward=ASCareer::ClaimLevelReward(ASCareer::Level(Experience),RewardedLevel);
 RelicEmbers=ASRules::Credit(RelicEmbers,LevelReward.Embers);Tablets=ASRules::Credit(Tablets,LevelReward.Tablets);
 RewardedTiers.SetNumZeroed(ASCareer::Count);
 for(int32 I=0;I<ASCareer::Count;++I) {
     const auto Reward=ASCareer::ClaimTierReward(Career[I],I,RewardedTiers[I]);
     RelicEmbers=ASRules::Credit(RelicEmbers,Reward.Embers);Tablets=ASRules::Credit(Tablets,Reward.Tablets);
 }
 if(TitleChoice<0||TitleChoice>=ASCareer::Count||ASCareer::Tier(Career[TitleChoice],TitleChoice)==0) TitleChoice=-1;
}

void FASEndlessRecord::Sanitize() {
    RunId=RunId.Left(64);Wave=FMath::Clamp(Wave,0,10000);Kills=FMath::Clamp(Kills,0,10000000);PartnerKills=FMath::Clamp(PartnerKills,0,10000000);
    for(float* Value:{&Damage,&DamageTaken,&PartnerDamage,&PartnerDamageTaken}) *Value=FMath::IsFinite(*Value)?FMath::Clamp(*Value,0.f,1.e12f):0.f;
    EndedUtc=EndedUtc.Left(64);
}
