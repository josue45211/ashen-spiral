#pragma once
#include "ASRules.h"
#include "ASExpeditionRules.h"

// Run-only choices. Stable IDs leave the original six stat IDs untouched.
namespace ASBuild {
enum class Trait : int {
    Sharpened,Deadeye,Melee,Ranged,Magic,Burn,Poison,Bleed,Shock,Frost,
    Pierce,Ricochet,Split,Splash,EliteHunter,BossHunter,Opener,Leech,Reaper,
    Mend,Armor,Defiance,Barrier,Evasion,Thorns,Phoenix,Regrowth,Grace,
    LongDash,DashCharge,ChillAura,Knockback,Stun,Root,Gravity,Repulse,
    Greed,Fortune,Reroll,MendingCoin,RoomBlessing,DashStrike,Chain,Stillness,
    Momentum,Berserk,KillFlow,EchoShot,Volley,Area,Duration,ManaReserve,
    ManaRegen,RuneNova,Orbit,Sense,Vengeance,FullHeal=Vengeance,Cleanse,BloodPact,Refine,
    Mystery,Duplicate,Conduit,Magazine,Reload,Precision,Attunement,Teamwork,WideArc,Count
};
constexpr int Count=int(Trait::Count),OfferBase=200;
constexpr int MeleeWeapons=(1<<0)|(1<<1)|(1<<5)|(1<<ASRules::Claws),AllWeapons=ASRules::WeaponMask;
// Original evolution bits stay stable; new utility traits do not evolve.
constexpr int EvolutionLimit=int(Trait::Magazine);
static_assert(EvolutionLimit<=64,"Legacy evolution flags must fit in uint64.");
enum Tag : int {Weapon=1,Element=2,Projectile=4,Guard=8,Dash=16,Spell=32,Echo=64,Loot=128};
struct Definition {const char* Name;const char* Description;int Max,Category,Tags,Rarity,WeaponMask;bool Instant;};
constexpr float VengeanceProtection=3.f,VengeanceFinisherMultiplier=5.f;
constexpr float PhoenixProtection=2.f,PhoenixHealthFraction=.35f,PhoenixNovaRadius=520.f,PhoenixNovaDamage=80.f;
// Categories: 0 damage, 1 survival, 2 modifier. Rarity: 0 common, 1 rare, 2 legendary.
// Weapon masks (a weapon's id is its bit): blade/maul/scythe/claws = MeleeWeapons (547), bow/glaive=12, staff=16, guns=448; AllWeapons is every weapon there is (1023 since Twin Claws).
inline constexpr std::array<Definition,Count> Catalog{{
 {"Honed Edge","+4 base damage per rank; firearms normalize this bonus by fire rate for equal added burst DPS.",5,0,Weapon,0,AllWeapons,false},
 {"Deadeye","+20 percentage points of critical damage per rank (base 180%).",3,0,Weapon,0,AllWeapons,false},
 {"Brawler","+12% melee weapon damage per rank.",5,0,Weapon,0,MeleeWeapons,false},
 {"Marksman","+12% bow/glaive/firearm weapon damage per rank.",5,0,Weapon|Projectile,0,460,false},
 {"Arcanist","+12% staff and skill damage per rank.",5,0,Spell,0,AllWeapons,false},
 {"Kindling Touch","18% chance/rank to apply burn (54% before evolution). If the weapon already burns, a proc raises its burn power by 30%.",3,2,Element|Weapon,0,AllWeapons,false},
 {"Venom Touch","18% chance/rank to apply poison (54% before evolution). Native poison gains an extra stack and 30% power on a proc.",3,2,Element|Weapon,0,AllWeapons,false},
 {"Serrated Touch","18% chance per rank to bleed on a primary hit. Bleed: 4 DPS/stack, 2 stacks, 3 sec.",3,2,Element|Weapon,0,AllWeapons,false},
 {"Storm Touch","18% chance per rank to shock for 20% weapon damage and arc to one nearby foe. 0.35 sec limit.",3,2,Element|Echo,0,AllWeapons,false},
 {"Winter Touch","18% chance per rank to add frost on a primary hit; bosses only chill.",3,2,Element|Weapon,0,AllWeapons,false},
 {"Drill Shot","Primary projectiles pierce one additional enemy per rank.",3,2,Projectile,0,220,false},
 {"Ricochet","Primary projectiles ricochet to one extra enemy or off a wall per rank.",2,2,Projectile,1,220,false},
 {"Forked Flight","18% chance per rank to split on impact into two 40%-damage shots. Splits cannot split again.",2,2,Projectile|Echo,1,220,false},
 {"Impact Bloom","Primary hits splash nearby enemies for 15% dealt damage per rank. 0.25 sec limit; no recursive procs.",3,2,Weapon|Echo,1,AllWeapons,false},
 {"Elite Hunter","+15% damage against elites per rank.",3,0,Weapon,0,AllWeapons,false},
 {"Giant Slayer","+15% damage against bosses per rank.",3,0,Weapon,0,AllWeapons,false},
 {"First Blood","+20% damage against an enemy above 99% health per rank.",3,0,Weapon,0,AllWeapons,false},
 {"Leech","Heal 1.5% of primary-hit damage per rank, capped at 3 HP per 0.25 sec.",3,1,Weapon|Guard,1,AllWeapons,false},
 {"Reaper's Rest","Heal 1.5 HP per rank when you kill a real enemy.",3,1,Guard|Loot,0,AllWeapons,false},
 {"Field Dressing","Immediately restore 35% maximum health. Can be chosen up to five times per run.",5,1,Guard,0,AllWeapons,true},
 {"Ironweave","Reduce incoming damage by 5% per rank. Combined reductions cap at 65%.",5,1,Guard,0,AllWeapons,false},
 {"Defiance","After taking damage, gain 10% damage reduction per rank for 3 sec.",3,1,Guard,0,AllWeapons,false},
 {"Renewing Barrier","+12 regenerating shield capacity per rank; restores 2 shield/sec/rank after 5 sec without a hit in combat.",3,1,Guard,1,AllWeapons,false},
 {"Elusive","4% evasion chance per rank, capped at 20% before evolution.",5,1,Guard|Dash,0,AllWeapons,false},
 {"Briar Mail","Return 4 damage per rank to the attacker on a damaging hit; 0.3 sec internal cooldown.",4,1,Guard|Echo,0,AllWeapons,false},
 {"Phoenix Oath","Survive one lethal hit this run at 35% health with 2 sec of protection; erupt in a 520 cm nova for 80 base damage and burn.",1,1,Guard,2,AllWeapons,false},
 {"Regrowth","Regenerate 1 HP/sec per rank in combat after 3 sec without damage.",3,1,Guard,1,AllWeapons,false},
 {"Afterimage","+0.08 sec invulnerability after taking damage per rank.",3,1,Guard|Dash,0,AllWeapons,false},
 {"Longstride","Dash distance +12% per rank; dash invulnerability +0.03 sec per rank.",3,2,Dash,0,AllWeapons,false},
 {"Second Step","+1 dash charge per rank, up to 3 charges. Charges refill in sequence; air dashes per landing match total charge capacity and each still spends a charge.",2,2,Dash,1,AllWeapons,false},
 {"Winter Mantle","Chill nearby enemies within 320 + 50 cm/rank. Normal enemies slow 35%; bosses 25%.",3,2,Guard|Element,1,AllWeapons,false},
 {"Heavy Impact","Primary hits knock normal enemies back with 120 force/rank; elites receive half force, bosses resist.",3,2,Weapon,0,AllWeapons,false},
 {"Concussion","8% stun chance per rank on primary hits: 0.4 sec, 2 sec immunity. Bosses resist.",3,2,Weapon|Guard,1,AllWeapons,false},
 {"Bramble Bind","10% root chance per rank on primary hits: 0.8 sec, 3 sec immunity. Bosses resist.",3,2,Element|Guard,1,AllWeapons,false},
 {"Gravity Hook","Primary hits pull nearby normal enemies toward the impact. 0.6 sec limit.",2,2,Spell|Echo,1,AllWeapons,false},
 {"Repulsor","Dashing pushes nearby normal enemies away. Radius 280 + 40 cm/rank.",3,2,Dash|Guard,0,AllWeapons,false},
 {"Prospector","+20% Cinders from kills per rank, using fractional carry so small kills benefit.",3,2,Loot,0,AllWeapons,false},
 {"Fortune","+6 percentage points to health/shield drop chance per rank.",3,2,Loot|Guard,0,AllWeapons,false},
 {"Loaded Dice","Gain one personal reward reroll immediately. Up to three picks per run.",3,2,Loot,0,AllWeapons,true},
 {"Mending Coin","Picking up a Cinder shard restores 1 HP per rank during combat.",3,1,Loot|Guard,0,AllWeapons,false},
 {"Vanguard","Next and later rooms begin with +25% damage for the first 10 combat seconds. A flat bonus: extra ranks add nothing.",1,0,Weapon|Spell,0,AllWeapons,false},
 {"Razor Dash","Dash through enemies for 12 damage/rank, once per enemy per dash.",3,2,Dash|Weapon,0,AllWeapons,false},
 {"Chain Lightning","Primary hits chain to one additional enemy per rank at 30% dealt damage, once per 0.35 sec.",3,2,Echo|Element,1,AllWeapons,false},
 {"Patient Hunter","After standing still for 1.2 sec, gain +12% damage per rank.",3,0,Weapon,0,AllWeapons,false},
 {"Momentum","+8% damage per rank while moving faster than a walk.",3,0,Weapon|Dash,0,AllWeapons,false},
 {"Last Ember","+15% damage per rank below 40% health.",3,0,Weapon|Guard,1,AllWeapons,false},
 {"Battle Rhythm","Kills reduce Spiral cooldown by 0.25 sec/rank and grant +5% damage/rank for 2 sec.",3,2,Weapon|Spell,1,AllWeapons,false},
 {"Echo Shot","Fire one additional projectile at 45% damage. Extra shots do not create more splits or proc chains.",1,2,Projectile|Echo,1,476,false},
 {"Fan of Sparks","Fire one extra spread projectile per rank at 35% damage each.",2,2,Projectile|Echo,1,476,false},
 {"Expanding Force","+15% explosion, Spiral and aura radius per rank.",3,2,Spell|Echo,0,AllWeapons,false},
 {"Lingering Affliction","+20% burn, poison and bleed duration per rank. Hard crowd-control durations remain capped.",3,2,Element,0,AllWeapons,false},
 {"Deep Reservoir","+10 maximum Focus per rank; immediately restore that amount. Rune Lance uses 25 Focus.",3,2,Spell,0,AllWeapons,false},
 {"Flowing Runes","+2 Focus regeneration/sec per rank during combat or lobby practice.",3,2,Spell,0,AllWeapons,false},
 {"Rune Nova","Evolve any active skill: Lance and Storm impacts gain a 300 cm blast; Winter Fan gains a 300 cm impact bloom. War Cry, Venom Bloom and Cinder Mine gain 300 cm base radius. Damage per hit is unchanged; area upgrades apply once.",1,2,Spell|Echo,1,AllWeapons,false},
 {"Orbiting Ember","Gain an orbiting wisp per rank (max 3), each dealing 4 damage every 0.5 sec to a nearby enemy.",3,2,Echo|Spell,1,AllWeapons,false},
 {"Vault Sense","Show trap positions and timing on the HUD and minimap, plus the optional vault cache.",1,2,Loot|Guard,0,AllWeapons,false},
 {"Vengeance Oath","Survive one lethal hit this run at 1 HP with 3 sec of protection; your next three-hit combo finisher deals 5x damage.",1,1,Guard|Weapon,2,AllWeapons,false},
 {"Cleanse","Remove all Blood Pact curse stacks (+10% incoming damage each). The traded maximum health stays traded.",1,1,Guard,1,AllWeapons,true},
 {"Blood Pact","Trade 15% maximum health for +45% damage. Gain a removable curse: +10% incoming damage.",1,0,Weapon|Spell,2,AllWeapons,false},
 {"Emberforge Evolution","Evolve one owned non-instant trait, granting one effective rank above its normal value. Two evolutions per run.",2,2,Echo|Spell,2,AllWeapons,true},
 {"Mystery Relic","Gain one random, unowned relic from your unlocked pool. One pick per run.",1,2,Loot|Echo,1,AllWeapons,true},
 {"Echo of Choice","Repeat your most recent repeatable stat/trait upgrade if eligible. One pick per run.",1,2,Echo,1,AllWeapons,true},
 {"Battle Conduit","Every 6 primary hits restores 12 Focus and shortens Spiral cooldown by 0.3 sec. Counts at most once per 0.15 sec.",1,2,Weapon|Spell,1,AllWeapons,false},
 {"Deep Magazine","Add 20% of base ammunition/charge capacity per rank, rounding each rank up to whole rounds. Five ranks. Every pick adds at least one round; applies to ranged weapons only. Melee has no reload. Cannot evolve.",5,2,Weapon,0,476,false},
 {"Quick Hands","+15% reload speed per rank. Reload time = base time / (1 + 0.15 x rank). Ranged weapons only. Five ranks; cannot evolve.",5,2,Weapon,0,476,false},
 {"Tight Grouping","15% less spread per rank, up to 75%. Tightens pellets, rotary fire, Echo Shot, Fan of Sparks and Forked Flight. Cannot evolve.",5,2,Projectile,0,476,false},
 // 0.23.2 (TUNABLE): elemental damage, and a larger synergy-damage buff that only appears in co-op.
 {"Attunement","+12% burn, poison and bleed damage per rank. Cannot evolve.",5,0,Element,0,AllWeapons,false},
 {"Teamwork","+20% synergy attack damage per rank, for you and for synergies you trigger with a partner. Co-op only. Cannot evolve.",5,0,Element|Echo,1,AllWeapons,false},
 // 0.23.3 (TUNABLE): melee reach, so swings and slams catch enemies further out. Sword, hammer and scythe only.
 {"Wide Arc","+20% melee swing reach per rank, for sword, hammer and scythe. Cannot evolve.",5,0,Weapon,0,MeleeWeapons,false}
}};
inline constexpr std::array<const char*,Count> Briefs{{
 "Boost weapon damage",
 "Stronger critical hits",
 "Stronger melee attacks",
 "Stronger ranged attacks",
 "Empower staff and skills",
 "Ignite enemies on hit",
 "Poison enemies on hit",
 "Make enemies bleed",
 "Shock and arc on hit",
 "Chill enemies on hit",
 "Shots pass through enemies",
 "Shots bounce to new targets",
 "Hits split into extra shots",
 "Hits splash nearby enemies",
 "Deal more damage to elites",
 "Deal more damage to bosses",
 "Hit untouched foes harder",
 "Weapon hits restore health",
 "Kills restore health",
 "Restore health now",
 "Take less damage",
 "Gain protection after a hit",
 "Gain a regenerating shield",
 "Chance to dodge incoming hits",
 "Reflect damage at attackers",
 "Survive lethal; erupt in a burn nova",
 "Regenerate after avoiding damage",
 "Longer protection after a hit",
 "Dash farther and safer",
 "Gain an extra dash charge",
 "Slow enemies around you",
 "Knock enemies back",
 "Chance to stun on hit",
 "Chance to root on hit",
 "Pull foes toward your hits",
 "Dash pushes enemies away",
 "Earn more Cinders from kills",
 "Find more health and shields",
 "Gain a reward reroll",
 "Cinder pickups restore health",
 "Begin chambers with bonus damage",
 "Damage foes along your dash",
 "Hits arc to nearby foes",
 "Deal more damage standing still",
 "Deal more damage while moving",
 "Deal more damage at low health",
 "Kills boost damage and cooldowns",
 "Fire a bonus echo shot",
 "Fire extra spread shots",
 "Larger blasts and auras",
 "Make damage effects last longer",
 "Increase your Focus capacity",
 "Regenerate Focus faster",
 "Give your active skill area impact",
 "Summon a damaging orbiting wisp",
 "Retired reward",
 "Survive lethal; empower your next finisher",
 "Remove Blood Pact curses",
 "Trade maximum health for power",
 "Evolve an owned trait",
 "Discover a random unowned relic",
 "Repeat your previous upgrade",
 "Hits restore Focus and cooldown",
 "More attacks per reload",
 "Reload and recharge faster",
 "Tighter pellets and bonus shots",
 "Stronger burn, poison and bleed",
 "Stronger synergy attacks",
 "Melee swings reach further"
}};
inline int Index(Trait T) {return int(T);}
inline bool IsTrait(int Offer) {return Offer>=OfferBase&&Offer<OfferBase+Count;}
inline bool Evolved(std::uint64_t Mask,int I) {return I>=0&&I<EvolutionLimit&&(Mask&(std::uint64_t(1)<<I));}
inline int Rank(const std::array<int,Count>& R,Trait T) {return std::clamp(R[Index(T)],0,Catalog[Index(T)].Max);}
inline float ProcChance(int RankValue,float PerRank) {return std::clamp(RankValue*PerRank,0.f,.75f);}
inline float CriticalMultiplier(int RankValue) {return std::min(2.6f,1.8f+.2f*std::clamp(RankValue,0,4));}
inline float DamageReduction(int Armor,int Defiance,bool Recent) {return std::min(.65f,.05f*std::max(0,Armor)+(Recent?.10f*std::max(0,Defiance):0.f));}
inline float DashDuration(int RankValue) {return .24f*(1.f+.12f*std::clamp(RankValue,0,4));}
inline float StatusDuration(int RankValue) {return 1.f+.20f*std::clamp(RankValue,0,4);}
inline int MaxDashCharges(int RankValue) {return std::clamp(1+RankValue,1,3);}
// 0.23.2 TUNABLE: the synergy buff per rank is deliberately larger than the elemental one (teamwork incentive).
constexpr float AttunementPerRank=.12f,TeamworkPerRank=.20f;
inline float ElementalDamageMultiplier(int RankValue) {return 1.f+AttunementPerRank*float(std::clamp(RankValue,0,10000));}
// Vanguard (0.23.5): FLAT +25% damage for the first 10 combat seconds of each room, whatever the rank. TUNABLE.
constexpr float VanguardBonusAmount=.25f,VanguardSeconds=10.f;
inline float VanguardBonus(float RankValue) {return RankValue>0.f?VanguardBonusAmount:0.f;}
constexpr float WideArcPerRank=.20f; // TUNABLE (0.23.4): was .10, barely noticeable; rank 5 doubles melee reach
inline float MeleeReachMultiplier(int RankValue) {return 1.f+WideArcPerRank*float(std::clamp(RankValue,0,10000));}
inline float SynergyDamageMultiplier(int RankA,int RankB) {return 1.f+TeamworkPerRank*float(std::clamp(RankA,0,10000)+std::clamp(RankB,0,10000));}
inline int CreditKills(int Base,int RankValue,float& Remainder) {
    const float Total=std::max(0,Base)*(1.f+.2f*std::clamp(RankValue,0,4))+Remainder;
    const int Whole=int(std::floor(Total+.00001f));Remainder=std::clamp(Total-Whole,0.f,.99999f);return Whole;
}
struct Context {
    std::array<int,ASRules::RunUpgradeCount> Stats{};
    std::array<int,Count> Traits{};
    std::uint64_t Evolutions=0;
    int Weapon=0,Room=0,Challenge=0,Last=-1,Curse=0,PermanentCooldown=0,PermanentCritical=0;
    int Difficulty=0,EndlessWave=0;
    bool Hurt=false,MissingRelic=true,Endless=false,Coop=false;
};
inline bool CanEvolve(const Context& C,int I) {
    return I>=0&&I<EvolutionLimit&&I!=Index(Trait::Sense)&&I!=Index(Trait::DashCharge)&&!Catalog[I].Instant&&Catalog[I].Max>1&&C.Traits[I]>0&&!Evolved(C.Evolutions,I);
}
inline bool ScalesEndlessly(int I) {
    if(I<0||I>=Count||Catalog[I].Instant||Catalog[I].Max<=1) return false;
    // Fixed movement and projectile-count ceilings are retained in endless mode.
    const Trait T=Trait(I);
    return T!=Trait::Sense&&T!=Trait::DashCharge&&T!=Trait::LongDash&&T!=Trait::Precision&&
        T!=Trait::Magazine&&T!=Trait::Reload&&T!=Trait::Deadeye&&T!=Trait::Evasion&&T!=Trait::Grace&&
        T!=Trait::Pierce&&T!=Trait::Ricochet&&T!=Trait::Split&&T!=Trait::Volley&&T!=Trait::Chain&&
        T!=Trait::Orbit&&T!=Trait::Burn&&T!=Trait::Poison&&T!=Trait::Bleed&&T!=Trait::Shock&&
        T!=Trait::Frost&&T!=Trait::Stun&&T!=Trait::Root&&T!=Trait::Duration;
}
inline bool CanChoose(const Context& C,int Offer,bool AllowSpecial=true) {
    if(ASRules::IsRunUpgradeOffer(Offer)) {
        const int I=Offer-ASRules::RunUpgradeOfferBase;
        if(C.Endless&&(I==1||I==2||I==3)) return C.Stats[I]<10000;
        return ASRules::CanUpgradeRunStat(I,C.Stats[I],I==0?C.PermanentCooldown:(I==5?C.PermanentCritical:0));
    }
    if(!IsTrait(Offer)) return false;
    const int I=Offer-OfferBase;const auto& D=Catalog[I];
    if(C.Weapon<0||C.Weapon>=ASRules::WeaponCount||!(D.WeaponMask&(1<<C.Weapon))||C.Traits[I]>=(C.Endless&&ScalesEndlessly(I)?10000:D.Max)) return false;
    const auto T=Trait(I);
    if(T==Trait::Sense) return false; // Retired reward ID; preserve all other IDs.
    if(T==Trait::Mend&&!C.Hurt) return false;
    if(T==Trait::Cleanse&&C.Curse<=0) return false;
    if(T==Trait::Teamwork&&!C.Coop) return false; // synergies need two players
    if(T==Trait::Mystery&&!C.MissingRelic) return false;
    if(T==Trait::Refine) {if(!AllowSpecial) return false;for(int J=0;J<Count;++J) if(CanEvolve(C,J)) return true;return false;}
    if(T==Trait::Duplicate) {
        if(!AllowSpecial||(!ASRules::IsRunUpgradeOffer(C.Last)&&(!IsTrait(C.Last)||Catalog[C.Last-OfferBase].Instant))) return false;
        return CanChoose(C,C.Last,false);
    }
    return true;
}
inline int Category(int Offer) {
    if(IsTrait(Offer)) return Catalog[Offer-OfferBase].Category;
    return Offer==103?1:(Offer==100||Offer==104?2:0);
}
inline int Tags(int Offer) {
    if(IsTrait(Offer)) return Catalog[Offer-OfferBase].Tags;
    return Offer==100?Spell:(Offer==103?Guard:(Offer==104?Dash:Weapon));
}
inline int Weight(const Context& C,int Offer) {
    int W=IsTrait(Offer)?(Catalog[Offer-OfferBase].Rarity==2?5:(Catalog[Offer-OfferBase].Rarity==1?14:30)):38;
    const int Tag=Tags(Offer);int Affinity=0;
    for(int I=0;I<Count;++I) if(C.Traits[I]>0&&(Catalog[I].Tags&Tag)) Affinity+=std::min(C.Traits[I],3);
    for(int I=0;I<ASRules::RunUpgradeCount;++I) if(C.Stats[I]>0&&(Tags(100+I)&Tag)) Affinity+=std::min(C.Stats[I],3);
    W+=W*std::min(Affinity,8)/8; // At most double; off-build options never disappear.
    if(C.Room<2&&(Offer==100||Offer==101)) W*=3;
    // Reward quality is earned by clearing the current challenge, never by toggling a future vote.
    const int Rarity=IsTrait(Offer)?Catalog[Offer-OfferBase].Rarity:0;
    const int Bonus=(Rarity==2?20:(Rarity==1?12:0))*std::clamp(C.Challenge,0,3);
    const int Quality=Rarity>0?25*std::clamp(C.Difficulty,0,2):0;
    return ((W*(100+Bonus)+99)/100*(100+Quality)+99)/100;
}
// TUNABLE: percent chance that an Endless chest carries one exclusive trait; rises slowly with the wave.
inline int ExclusiveChance(int Wave) {return Wave<3?0:std::min(25,8+Wave/5);}
inline std::vector<int> Offers(const Context& C,std::uint32_t Seed,int CountWanted=3) {
    ASRules::Random R(Seed);std::vector<int> Pool,Result;
    for(int I=0;I<ASRules::RunUpgradeCount;++I) if(CanChoose(C,100+I)) Pool.push_back(100+I);
    for(int I=0;I<Count;++I) if(CanChoose(C,OfferBase+I)) Pool.push_back(OfferBase+I);
    for(int Slot=0;Slot<std::clamp(CountWanted,0,5)&&!Pool.empty();++Slot) {
        std::vector<int> Candidates;for(int V:Pool) if(Slot>=3||Category(V)==Slot) Candidates.push_back(V);
        // Endless sidegrades (TUNABLE): after wave 15 the extra chest slots lean to sustain/utility/mobility,
        // so at most about half of a five-choice chest is offence.
        if(C.Endless&&C.EndlessWave>15&&Slot>=3) {
            std::vector<int> Side;for(int V:Candidates) if(Category(V)!=0) Side.push_back(V);
            if(!Side.empty()) Candidates=Side;
        }
        if(Candidates.empty()) Candidates=Pool;
        int Sum=0;for(int V:Candidates) Sum+=Weight(C,V);int Pick=R.Range(Sum),Chosen=Candidates.back();
        for(int V:Candidates) {Pick-=Weight(C,V);if(Pick<0) {Chosen=V;break;}}
        Result.push_back(Chosen);Pool.erase(std::find(Pool.begin(),Pool.end(),Chosen));
    }
    // Endless-exclusive stacking traits replace the last choice from time to time (never in normal runs).
    if(C.Endless&&!Result.empty()&&R.Range(100)<ExclusiveChance(C.EndlessWave))
        Result.back()=ASExpedition::ExclusiveBase+R.Range(ASExpedition::ExclusiveCount);
    return Result;
}
inline void RefillDash(int& Charges,float& Timer,float Delta,int Capacity,float Recharge) {
    Capacity=std::clamp(Capacity,1,3);Charges=std::clamp(Charges,0,Capacity);
    if(Charges==Capacity) {Timer=0;return;}
    if(!std::isfinite(Delta)||Delta<0||!std::isfinite(Recharge)||Recharge<=0) return;
    if(!std::isfinite(Timer)||Timer<=0) Timer=Recharge;
    Timer-=Delta;
    while(Timer<=0&&Charges<Capacity) {++Charges;if(Charges<Capacity) Timer+=Recharge;}
    if(Charges==Capacity) Timer=0;
}
inline int ChallengeTier(int Previous,int Players,int Votes) {return Players>0&&Votes==Players?std::clamp(std::clamp(Previous,0,3)+1,1,3):0;}
inline float ChallengeHealth(int Tier) {return 1.f+.20f*std::clamp(Tier,0,3);}
inline float ChallengeDamage(int Tier) {return 1.f+.12f*std::clamp(Tier,0,3);}
inline float TrapPhase(float Now,float Offset,float Period=6.f) {return std::fmod(std::max(0.f,Now+Offset),Period);}
inline bool FlameActive(float Phase) {return Phase>=2.f&&Phase<4.f;}
inline bool BladeActive(float Phase) {return Phase>=1.5f&&Phase<4.5f;}
inline bool SweptCircle(float AX,float AY,float BX,float BY,float X,float Y,float Radius) {
    const float DX=BX-AX,DY=BY-AY,Length=DX*DX+DY*DY;
    const float T=Length>0?std::clamp(((X-AX)*DX+(Y-AY)*DY)/Length,0.f,1.f):0.f;
    const float EX=X-AX-T*DX,EY=Y-AY-T*DY;return EX*EX+EY*EY<=Radius*Radius;
}
inline bool BladeContact(float Time,float X,float Y,float Radius) {
    const float A=Time*3.14159265358979323846f;
    for(int I=0;I<2;++I) {
        const float D=A+I*1.57079632679489661923f,DX=std::cos(D)*Radius,DY=std::sin(D)*Radius;
        if(SweptCircle(-DX,-DY,DX,DY,X,Y,50.f)) return true;
    }
    return false;
}
inline float RefillFocus(float Current,float Delta,int Reserve,int Recovery) {
    const float Max=60.f+10.f*std::clamp(Reserve,0,4);
    if(!std::isfinite(Current)) Current=0;
    if(!std::isfinite(Delta)||Delta<0) Delta=0;
    return std::clamp(Current+Delta*(5.f+2.f*std::clamp(Recovery,0,4)),0.f,Max);
}
}
