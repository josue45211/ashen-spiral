#pragma once
#include <algorithm>
#include <cmath>
// Engine-independent enemy rules (0.24). The kind ids are the long-standing runtime ids (see ASEnemy.cpp RefreshLook); nothing here touches Unreal types,
// so Tests/enemy_024_tests.cpp can exercise it as plain C++. TUNABLE values are marked.
namespace ASEnemyRules {
constexpr int Grunt=0,Archer=1,Duelist=2,Brute=3,BossKind=4,Leaper=5,Bulwark=6,Shaman=7,Sniper=8,Broodcaller=9,Gloomwing=10,KindCount=11;

// ---- Melee swing (kinds 0, 2, 3) ----
// The ground warning and the damage test share these numbers: the warning is a half disc (SM_StrikeSector, +-90 degrees) of StrikeRadius in front of
// the enemy, and a hero is hit only inside exactly that shape. A circle on screen therefore always means "damage lands here". TUNABLE.
constexpr float MeleeRadius=210.f,EliteMeleeRadius=340.f,MeleeDamage=12.f,EliteMeleeDamage=24.f;
inline float StrikeRadius(bool Elite) {return Elite?EliteMeleeRadius:MeleeRadius;}
inline float StrikeDamage(bool Elite) {return Elite?EliteMeleeDamage:MeleeDamage;}
// Distance2D is measured from the enemy's centre; ForwardDot is dot(aim, direction to the hero), both flat.
inline bool StrikeHits(float Distance2D,float ForwardDot,float Radius) {
    return std::isfinite(Distance2D)&&std::isfinite(ForwardDot)&&std::isfinite(Radius)&&Distance2D<Radius&&ForwardDot>0.f;
}

// ---- AI states (0.24) ----
// The replicated byte AASEnemy::AIState. The server runs one small state machine per walker; every machine poses the body from it (a windup, a strike, a recovery, a
// run). Approach and Strafe are the two ways of moving (closing in, or circling / backing away); Special is a long kind-specific behaviour (a parry stance, a roar).
constexpr int StateApproach=0,StateStrafe=1,StateWindup=2,StateAttack=3,StateRecover=4,StateRetreat=5,StateSpecial=6,StateCount=7;

// ---- Sound cues (0.24) ----
// Every telegraph and every release has a cue (readability doctrine: no attack is silent). A cue names the file Edison delivers (Stem, imported from SourceArt/Audio) and an
// existing sound to play at its own pitch until that file exists (Fallback; empty = silent until delivered). The enemy plays cues locally on every machine from the replicated
// state, so they cost no network traffic. Ids are never reused. The manifest for the sound designer is Docs/AUDIO_MANIFEST_0.24.md (Tools/export_audio_manifest.cpp prints it).
constexpr int CueNone=0,CueSlashWind=1,CueSlashSwing=2,CueShoveWind=3,CueShoveBash=4,CueShotDraw=5,CueShotLoose=6,CueBackHop=7,CueParryRaise=8,CueParryBlock=9,CueRiposteSlash=10,
    CuePoundWind=11,CuePoundSlam=12,CueEnrageRoar=13,CuePounceCoil=14,CuePounceLeap=15,CuePounceLand=16,CueBlinkCharge=17,CueBlinkAppear=18,CueChargeWind=19,CueChargeRun=20,
    CueGuardBreak=21,CueBlastWind=22,CueBlastBurst=23,CueMeteorCall=24,CueMeteorFall=25,CueMeteorImpact=26,CueSnipeAim=27,CueSnipeLock=28,CueSnipeFire=29,CueSummonWind=30,
    CueEggLay=31,CueEggHatch=32,CueEggPop=33,CueDiveScreech=34,CuePackCall=35,CueBossRoar=36,CueBossSlam=37,CueBossSummon=38,CueBossPhase=39,CueLandingCrash=40,
    CueMutationFrost=41,CueMutationFire=42,CueMutationStorm=43,CueMutationVenom=44,CueCount=45;
struct CueSpec {const char* Stem;const char* Fallback;float Volume,Pitch;const char* Note;};
inline const CueSpec& CueFor(int Id) {
    static constexpr CueSpec Table[CueCount]={
        {"",                 "",             0.f,  1.f,  "(none)"},
        {"E_SlashWind",      "S_Attack",     .22f, .60f, "a blade drawn back: a low, rising breath and a metal scrape, about 0.4 s"},
        {"E_SlashSwing",     "",             .32f, 1.f,  "the hostile swing itself: a short heavy whoosh with a metal edge (until delivered, the swing arc plays S_Attack)"},
        {"E_ShoveWind",      "S_HammerSwing",.22f, .80f, "a grunt bracing a shield: a short grunt and a leather creak"},
        {"E_ShoveBash",      "S_HammerHit",  .32f, 1.20f,"a shield bash: a dull thud with a rattle"},
        {"E_ShotDraw",       "S_Spiral",     .16f, 1.70f,"a staff charging: a thin rising shimmer, about 0.5 s"},
        {"E_ShotLoose",      "S_Attack",     .22f, 1.60f,"a bolt leaving a staff: a sharp zip"},
        {"E_BackHop",        "S_Dash",       .30f, 1.30f,"a quick back-step and a soft smoke puff"},
        {"E_ParryRaise",     "S_Reward",     .20f, 1.80f,"a blade lifted into guard: a clean steel ring"},
        {"E_ParryBlock",     "S_HammerHit",  .40f, 1.60f,"a blocked blow: a bright metal clang with a gold sparkle tail"},
        {"E_RiposteSlash",   "S_Attack",     .35f, 1.30f,"a fast counter-slash: a hard sharp whoosh"},
        {"E_PoundWind",      "S_HammerLeap", .30f, .55f, "a heavy enemy rearing up: a deep grunt and armour rattle, about 1 s"},
        {"E_PoundSlam",      "S_HammerSmash",.55f, .80f, "a ground-pound: a deep boom with a rolling shockwave tail"},
        {"E_EnrageRoar",     "S_Collapse",   .50f, .55f, "an elite roaring into a rage: a bellow, about 1.2 s"},
        {"E_PounceCoil",     "S_Dash",       .18f, .75f, "a leaper coiling: a low hiss and a joint click"},
        {"E_PounceLeap",     "S_HammerLeap", .26f, 1.40f,"a leaper launching: a whoosh"},
        {"E_PounceLand",     "S_HammerHit",  .28f, .90f, "a leaper landing: a thud and a skitter"},
        {"E_BlinkCharge",    "S_Spiral",     .20f, 2.00f,"a rift building around a leaper: a rising shimmer, about 0.5 s"},
        {"E_BlinkAppear",    "S_Dash",       .35f, 1.80f,"reappearing behind you: a sharp rift snap"},
        {"E_ChargeWind",     "S_HammerLeap", .30f, .60f, "a bulwark digging in to charge: a stamp and a snort, about 1 s"},
        {"E_ChargeRun",      "S_HammerSwing",.20f, .60f, "a heavy charge: pounding steps, a rolling rumble"},
        {"E_GuardBreak",     "S_HammerSmash",.50f, 1.40f,"a shield shattering open: a loud metal crack with sparks"},
        {"E_BlastWind",      "S_Spiral",     .20f, .55f, "a caster gathering fire: a rising hum, about 0.8 s"},
        {"E_BlastBurst",     "S_Collapse",   .35f, 1.00f,"a fire blast: a short fiery whump"},
        {"E_MeteorCall",     "S_Collapse",   .35f, .50f, "calling meteors: a deep rumble that climbs"},
        {"E_MeteorFall",     "S_Dash",       .22f, .50f, "a meteor falling: a long falling whistle, about 0.9 s"},
        {"E_MeteorImpact",   "S_HammerSmash",.45f, .90f, "a meteor landing: a boom with crackling embers"},
        {"E_SnipeAim",       "S_Reward",     .12f, 2.30f,"a sniper tracking you: a thin rising whine, about 1 s"},
        {"E_SnipeLock",      "S_Hit",        .30f, 2.40f,"the aim locking: a sharp click and the whine cuts off"},
        {"E_SnipeFire",      "S_Attack",     .40f, .70f, "a heavy rifle shot: a deep crack with a short echo"},
        {"E_SummonWind",     "S_Collapse",   .25f, 1.50f,"a broodcaller swelling: a wet, rising gurgle, about 1 s"},
        {"E_EggLay",         "S_Hit",        .30f, .60f, "an egg sac dropping: a wet squelch and thump"},
        {"E_EggHatch",       "S_Collapse",   .35f, 1.80f,"an egg hatching: a crack and a squeal"},
        {"E_EggPop",         "S_Hit",        .35f, 1.30f,"an egg popped before it hatches: a quick wet pop"},
        {"E_DiveScreech",    "S_Attack",     .25f, 1.90f,"a flier about to dive: a rising screech, about 0.7 s"},
        {"E_PackCall",       "S_Attack",     .30f, 1.40f,"fliers calling a pack dive: a short chorus of screeches"},
        {"E_BossRoar",       "S_HammerSmash",.50f, .55f, "a boss winding up: a huge growl and armour clatter, about 1 s"},
        {"E_BossSlam",       "S_HammerSmash",.65f, .70f, "a boss slam: a massive boom"},
        {"E_BossSummon",     "S_Collapse",   .45f, .80f, "a boss calling reinforcements: a dark horn"},
        {"E_BossPhase",      "S_Collapse",   .60f, .50f, "a boss entering its second phase: a stinger with a roar, about 1.5 s"},
        {"E_LandingCrash",   "S_HammerSmash",.65f, .60f, "the flying boss crash-landing: a huge impact and falling rubble"},
        {"E_MutationFrost",  "S_Reward",     .25f, 2.20f,"a frostbound elite appearing: icy chimes and a cold exhale"},
        {"E_MutationFire",   "S_Collapse",   .25f, 1.20f,"a burning elite appearing: a gout of flame"},
        {"E_MutationStorm",  "S_Spiral",     .30f, 1.60f,"a shocking elite appearing: a crackle of static"},
        {"E_MutationVenom",  "S_Hit",        .25f, .70f, "a venomous elite appearing: a wet hiss"},
    };
    return Table[std::clamp(Id,0,CueCount-1)];
}
// How loud a cue is for the local player at this distance (cm): full inside 600, falling away to silence at 3200. The cue sounds have no attenuation of their own.
inline float CueDistanceGain(float Distance) {
    if(!std::isfinite(Distance)) return 0.f;
    const float T=std::clamp((std::max(0.f,Distance)-600.f)/2600.f,0.f,1.f);
    return (1.f-T)*(1.f-T);
}

// ---- Abilities (0.24) ----
// The replicated byte AASEnemy::Ability: which attack is being wound up, struck or recovered from (0 = none), so every machine poses the right body and draws the right
// warning. Ids are never reused; kits grow by adding rows. One row of timings and warning shape per ability.
constexpr int AbilityNone=0,AbilityDive=1,AbilityMolt=2,AbilitySlash=3,AbilityShot=4,AbilityPounce=5,AbilityCharge=6,AbilityBlast=7,AbilitySnipe=8,AbilitySummon=9,AbilityBossAttack=10,
    AbilityShove=11,AbilityBackHop=12,AbilityParry=13,AbilityRiposte=14,AbilityPound=15,AbilityEnrage=16,AbilityBlink=17,AbilityBlinkStrike=18,AbilityMeteors=19,
    AbilityGlacial=20,AbilityCinders=21,AbilityStorm=22,AbilitySeeds=23, // 20-23: the elite mutations' signature abilities (ASEnemyAbilities.h)
    AbilityBossSlam=24,AbilityBossMarks=25,AbilityBossRing=26,AbilityBossCharge=27,AbilityBossToll=28,AbilityBossSeeds=29,AbilityBossBloom=30,AbilityBossRoots=31,AbilityBossGrasp=32,AbilityBossPhase=33,
    AbilityCount=34; // 24-33: the ward bosses' abilities (each ward's kit is BossKitFor)
// The replicated byte AASEnemy::Flags (PhaseTwo: a boss that has passed half health and made its roar): bits for what every machine must show about an enemy beyond its state (a tint, a shield, a flash).
constexpr unsigned FlagEnraged=1u,FlagGuardBroken=2u,FlagParryToggle=4u,FlagPackCall=8u,FlagPhaseTwo=16u; // ParryToggle flips on every blocked blow, so each machine plays the block's sound and spark once; PackCall: this Gloomwing's wind-up calls the pack
constexpr int TellNone=0,TellSector=1,TellDisc=2,TellLine=3;  // ground warning shapes: a half disc, a full disc, a strip
constexpr unsigned NeedsClearPath=1u;                        // may only start when a straight walk to the target is clear (leaps and charges)
constexpr unsigned StanceFlag=2u;                            // a long stance or roar: the Special state, with no strike of its own (a parry, an enrage)
constexpr unsigned OneShotFlag=4u;                           // happens once per enemy, triggered by the enemy itself, never chosen by the chooser
constexpr unsigned PhaseTwoOnlyFlag=16u;                       // a boss ability that is only chosen once the boss is in its second phase
constexpr unsigned NoKitCooldownFlag=8u;                     // does not delay the kit's next attack (a reposition, a stance, a roar: the attack it sets up should follow soon)
struct AbilitySpec {
    float Windup;    // seconds of telegraph; 0 = the kit's wind-up
    float Attack;    // seconds of committed motion after the release (a leap, a charge); 0 = the release is instant
    float Recover;   // seconds standing exposed afterwards
    float Cooldown;  // this ability's own cooldown on top of the kit's; 0 = none
    float MinRange,MaxRange; // distance window it may start in; MaxRange 0 = the kit's attack range
    float Weight;    // relative chance among the abilities that are eligible
    int Tell;        // ground warning shape
    float TellA,TellB; // its size: sector / disc radius, or line length and width
    unsigned Flags;
    int CueWindup,CueRelease; // played where the telegraph starts and where the attack lands
};
inline const AbilitySpec& SpecFor(int Id) {
    static constexpr AbilitySpec Table[AbilityCount]={
        // windup attack recover  cd  min  max weight  tell        a    b  flags            wind cue / release cue
        {  0.f,  0.f,  0.f,0.f,0.f,0.f,0.f,TellNone,  0.f,0.f,0u,CueNone,CueNone},                       // 0 none
        { .7f, .55f,  0.f,0.f,0.f,0.f,1.f,TellNone,  0.f,0.f,0u,CueDiveScreech,CueNone},                 // 1 dive (the fliers' own loop uses the ASExpedition::Flier* numbers; these time the pose)
        {1.5f,  0.f,  0.f,0.f,0.f,0.f,1.f,TellNone,  0.f,0.f,0u,CueBossPhase,CueNone},                  // 2 molt
        {  0.f,  0.f, .40f,0.f,0.f,0.f,1.f,TellSector,210.f,0.f,0u,CueSlashWind,CueSlashSwing},          // 3 slash (half disc of StrikeRadius; the elite's is wider)
        {  0.f,  0.f, .36f,0.f,0.f,0.f,1.f,TellNone,  0.f,0.f,0u,CueShotDraw,CueShotLoose},              // 4 shot
        {  0.f, .65f, .45f,0.f,0.f,0.f,1.f,TellNone,  0.f,0.f,NeedsClearPath,CuePounceCoil,CuePounceLeap}, // 5 pounce (the leaper's leap)
        {  0.f, .60f, .90f,0.f,0.f,0.f,1.f,TellNone,  0.f,0.f,NeedsClearPath,CueChargeWind,CueChargeRun}, // 6 charge (the bulwark's shield charge and its end blast)
        {  0.f,  0.f, .40f,0.f,0.f,0.f,1.f,TellNone,  0.f,0.f,0u,CueBlastWind,CueBlastBurst},            // 7 blast (targeted ground blasts)
        {  0.f,  0.f, .42f,0.f,0.f,0.f,1.f,TellNone,  0.f,0.f,0u,CueSnipeAim,CueSnipeFire},              // 8 snipe
        {  0.f,  0.f, .40f,0.f,0.f,0.f,1.f,TellNone,  0.f,0.f,0u,CueSummonWind,CueEggLay},              // 9 summon (or a shot, plus the heal tethers)
        {  0.f,  0.f, .60f,0.f,0.f,0.f,1.f,TellNone,  0.f,0.f,0u,CueBossRoar,CueBossSlam},               // 10 boss attack (blast fields, a shot ring, a charge)
        { .60f, 0.f, .50f,3.5f,0.f,230.f,.7f,TellSector,260.f,0.f,0u,CueShoveWind,CueShoveBash},                 // 11 shove (grunt: a shoulder-barge that throws the hero)
        { .50f,.28f, .40f,5.0f,0.f,520.f,3.f,TellNone,  0.f,0.f,NoKitCooldownFlag,CueBackHop,CueNone},                           // 12 back-hop (archer: away from a hero that has closed in)
        { .95f, 0.f, .40f,6.5f,0.f,460.f,1.f,TellNone,  0.f,0.f,StanceFlag|NoKitCooldownFlag,CueParryRaise,CueNone},          // 13 parry (duelist: a guard stance; the block's clang plays from the replicated Flags toggle, not at the stance's end)
        { .28f, 0.f, .50f,0.f, 0.f, 0.f, 0.f,TellSector,260.f,0.f,0u,CueRiposteSlash,CueNone},                    // 14 riposte (the counter after a block; never chosen)
        {1.0f,  0.f, .90f,7.0f,0.f,650.f,1.3f,TellDisc,720.f,0.f,0u,CuePoundWind,CuePoundSlam},                   // 15 ground pound (brute: a shockwave ring)
        {1.2f,  0.f, .50f,1.e9f,0.f,0.f, 0.f,TellNone,  0.f,0.f,StanceFlag|OneShotFlag|NoKitCooldownFlag,CueEnrageRoar,CueNone},   // 16 enrage (brute at half health: a roar)
        { .55f, 0.f, .40f,8.0f,350.f,1100.f,1.2f,TellNone,0.f,0.f,NoKitCooldownFlag,CueBlinkCharge,CueBlinkAppear},             // 17 rift blink (leaper: reappear behind you)
        { .40f, 0.f, .60f,0.f, 0.f, 0.f, 0.f,TellSector,260.f,0.f,0u,CueSlashWind,CueSlashSwing},                 // 18 blink strike (the slash after a blink; never chosen)
        {1.0f,  0.f, .60f,9.0f,0.f,1300.f,1.f,TellNone,  0.f,0.f,0u,CueMeteorCall,CueMeteorFall},                 // 19 meteor rain (shaman)
        {1.0f,  0.f, .90f,8.0f,0.f, 430.f,1.6f,TellDisc,380.f,0.f,0u,CuePoundWind,CuePoundSlam},                  // 20 glacial slam (frostbound elite: a frost zone where it lands)
        { .90f, 0.f, .60f,7.0f,200.f,950.f,1.4f,TellNone,  0.f,0.f,0u,CueBlastWind,CueBlastBurst},                // 21 cinder burst (burning elite: fire pools in a line toward you, each with its own warning)
        {1.0f,  0.f, .60f,8.5f,0.f,1300.f,1.4f,TellNone,  0.f,0.f,0u,CueMeteorCall,CueNone},                      // 22 storm call (shocking elite: lightning on each hero and around, each with its own warning and thunder)
        { .80f, 0.f, .60f,7.0f,250.f,1100.f,1.4f,TellNone, 0.f,0.f,0u,CueSummonWind,CueEggLay},                    // 23 seed bombs (venomous elite: pods that land as venom pools)
        {1.2f,  0.f, .90f,7.0f,0.f, 800.f,1.2f,TellDisc,900.f,0.f,0u,CueBossRoar,CueBossSlam},                    // 24 boss slam (Indexer, Sovereign: a huge shockwave ring)
        {1.1f,  0.f, .70f,6.0f,0.f,1300.f,1.3f,TellNone,  0.f,0.f,0u,CueBossRoar,CueNone},                         // 25 boss marks (blasts on every hero, each with its own warning)
        {1.0f,  0.f, .70f,5.0f,0.f,1300.f,1.0f,TellNone,  0.f,0.f,0u,CueBossRoar,CueBlastBurst},                   // 26 boss ring (a ring of shots)
        {1.0f, .70f, .90f,8.0f,300.f,1300.f,1.4f,TellLine,950.f,220.f,NeedsClearPath,CueChargeWind,CueChargeRun},   // 27 boss charge (Bellwether: along a red strip)
        {1.2f,  0.f, .90f,7.0f,0.f, 900.f,1.2f,TellDisc,800.f,0.f,0u,CueBossRoar,CueBossSlam},                    // 28 boss toll (Bellwether: two rings one after the other)
        {1.0f,  0.f, .70f,6.5f,250.f,1300.f,1.3f,TellNone, 0.f,0.f,0u,CueSummonWind,CueEggLay},                    // 29 boss seeds (Horticulturist: venom pods)
        {1.1f,  0.f, .70f,8.0f,0.f,1300.f,1.2f,TellNone,  0.f,0.f,0u,CueMeteorCall,CueNone},                       // 30 boss bloom (Horticulturist: void blooms)
        {1.0f,  0.f, .70f,6.0f,0.f,1300.f,1.4f,TellNone,  0.f,0.f,0u,CueBossRoar,CueNone},                         // 31 boss roots (Sovereign: roots erupt under every hero)
        {1.3f,  0.f,1.00f,9.0f,0.f,1000.f,1.6f,TellDisc,900.f,0.f,PhaseTwoOnlyFlag,CueBossRoar,CueBossSlam},        // 32 boss grasp (Sovereign, second phase: pulls every hero in, then a ring)
        {1.5f,  0.f, .60f,1.e9f,0.f,0.f,  0.f,TellNone,  0.f,0.f,StanceFlag|OneShotFlag|NoKitCooldownFlag,CueBossPhase,CueBossSummon}, // 33 boss phase (the roar at half health: set piece of its ward)
    };
    return Table[std::clamp(Id,0,AbilityCount-1)];
}

// ---- Kits (0.24): the numbers an ordinary enemy moves and attacks by ----
// One row per kind. Everything the old ASEnemy::Tick spelled out as nested conditionals (speed, attack range, wind-up, cooldown) lives here, together with how the kind
// likes to move: shooters and casters keep a distance band and side-step inside it, melee kinds circle just outside their reach while an attack recharges, and the
// frailest kinds fall back once when badly hurt. How heavy it feels (acceleration, braking, turn rate), which animation family poses it, and which abilities it owns
// are part of the row. TUNABLE.
constexpr int MaxKitAbilities=6;
constexpr int AnimHumanoid=0,AnimBlob=1,AnimFlier=2,AnimSkeletal=3; // 21-part armour rig / a single squashing body / wings / a skeletal mesh (the spike)
struct Kit {
    float Speed;        // base walk speed, before the elite / anger / approach / slow multipliers
    float AttackRange;  // may start an attack when the target is closer than this (flat distance)
    float Windup;       // seconds from the start of the telegraph to the hit
    float Cooldown;     // seconds after an attack before the next may start
    float BandMin,BandMax; // shooters/casters: preferred distance from the target (both 0 for melee kinds)
    float Circle;       // melee: radius it circles at while its attack recharges (0 = it closes in and waits, as before 0.24)
    float Strafe;       // fraction of Speed while circling or side-stepping
    float RetreatBelow; // falls back once when its health fraction is under this (0 = never)
    float Accel,Brake,TurnRate; // cm/s^2 up to speed, cm/s^2 when stopping, degrees per second of turning
    int AnimSet;        // which animation family poses it
    int Abilities[MaxKitAbilities]; // ability ids it may start (0-terminated)
};
inline const Kit& KitFor(int Kind) {
    static constexpr Kit Table[KindCount]={
        //  speed range windup  cd  bandMin bandMax circle strafe retreat  accel brake turn  anim         abilities
        {340.f, 180.f,.85f,1.8f,    0.f,    0.f, 250.f,.55f,  0.f, 2400.f,2400.f,540.f,AnimHumanoid,{AbilitySlash,AbilityShove,0,0,0,0}},      // 0 grunt
        {275.f,1300.f,.85f,1.8f,  650.f, 1050.f,   0.f,.60f, .25f, 2600.f,2600.f,600.f,AnimHumanoid,{AbilityShot,AbilityBackHop,0,0,0,0}},       // 1 archer
        {275.f, 180.f,.85f,1.8f,    0.f,    0.f, 260.f,.60f,  0.f, 3000.f,3000.f,720.f,AnimHumanoid,{AbilitySlash,AbilityParry,AbilityRiposte,0,0,0}},      // 2 duelist
        {275.f, 180.f,.85f,1.8f,    0.f,    0.f, 300.f,.45f,  0.f, 1200.f,1000.f,300.f,AnimHumanoid,{AbilitySlash,AbilityPound,AbilityEnrage,0,0,0}},      // 3 brute
        {235.f,1300.f,1.1f,2.0f,    0.f,    0.f,   0.f,.50f,  0.f, 1000.f, 800.f,220.f,AnimHumanoid,{AbilityBossAttack,0,0,0,0,0}}, // 4 boss (its own movement arrives with the boss kits)
        {390.f, 650.f,.85f,1.8f,    0.f,    0.f, 300.f,.70f,  0.f, 3200.f,2800.f,720.f,AnimBlob,    {AbilityPounce,AbilityBlink,AbilityBlinkStrike,0,0,0}},     // 5 leaper
        {230.f, 650.f,1.1f,1.8f,    0.f,    0.f, 240.f,.40f,  0.f,  900.f,1100.f,240.f,AnimHumanoid,{AbilityCharge,0,0,0,0,0}},     // 6 bulwark
        {275.f,1300.f,.85f,1.8f,  800.f, 1200.f,   0.f,.60f, .30f, 1800.f,1800.f,450.f,AnimHumanoid,{AbilityBlast,AbilityMeteors,0,0,0,0}},      // 7 shaman
        {275.f,1700.f,1.5f,1.8f, 1200.f, 1650.f,   0.f,.50f, .35f, 1800.f,1800.f,360.f,AnimHumanoid,{AbilitySnipe,0,0,0,0,0}},      // 8 sniper
        {275.f,1300.f,1.4f,1.8f,  700.f, 1100.f,   0.f,.50f, .30f, 1400.f,1400.f,300.f,AnimBlob,    {AbilitySummon,0,0,0,0,0}},     // 9 broodcaller
        {330.f,1100.f,.70f,5.0f,    0.f,    0.f,   0.f, 0.f,  0.f, 2000.f,2000.f,540.f,AnimFlier,   {AbilityDive,0,0,0,0,0}},       // 10 gloomwing (its flight loop uses the ASExpedition::Flier* numbers)
    };
    return Table[std::clamp(Kind,0,KindCount-1)];
}
// The radius a ground warning is drawn at: the melee slash's is the strike radius (the elite's is wider), every other warning's size is in its row.
inline float TellRadiusFor(int Ability,bool Elite) {
    if(Ability==AbilitySlash) return StrikeRadius(Elite);
    const float R=SpecFor(Ability).TellA;return R>0.f?R:StrikeRadius(Elite);
}
// Whether a kit owns an ability (the one-shot triggers are not chosen, they are asked for by id).
inline bool KitHas(const Kit& K,int Ability) {for(int I=0;I<MaxKitAbilities;++I) {if(K.Abilities[I]==AbilityNone) return false;if(K.Abilities[I]==Ability) return true;}return false;}
inline float WindupFor(const Kit& K,int Ability) {const float W=SpecFor(Ability).Windup;return W>0.f?W:K.Windup;}
// What the chooser needs to know about the moment.
struct Context {float Distance=0.f,Now=0.f;bool Sight=true,ClearPath=true,PhaseTwo=false;};
// Picks the ability a kit starts now: among its abilities whose distance window contains the target, whose own cooldown is over (ReadyAt[id], may be null) and whose
// path requirement holds; weighted by Roll in [0,1); never the same one twice running when something else is eligible. Returns AbilityNone when nothing is.
inline int ChooseAbility(const Kit& K,const Context& C,const float* ReadyAt,int Last,float Roll) {
    if(!std::isfinite(C.Distance)||!std::isfinite(C.Now)||!C.Sight) return AbilityNone;
    int Eligible[MaxKitAbilities];int N=0;
    for(int I=0;I<MaxKitAbilities;++I) {
        const int Id=K.Abilities[I];
        if(Id<=AbilityNone||Id>=AbilityCount) break;
        const AbilitySpec& A=SpecFor(Id);const float Max=A.MaxRange>0.f?A.MaxRange:K.AttackRange;
        if(!(C.Distance>=A.MinRange&&C.Distance<Max)||((A.Flags&NeedsClearPath)&&!C.ClearPath)||((A.Flags&PhaseTwoOnlyFlag)&&!C.PhaseTwo)||(ReadyAt&&!(C.Now>=ReadyAt[Id]))||!(A.Weight>0.f)) continue;
        Eligible[N++]=Id;
    }
    if(N==0) return AbilityNone;
    float Total=0.f,Weights[MaxKitAbilities];
    for(int I=0;I<N;++I) {Weights[I]=(N>1&&Eligible[I]==Last)?0.f:SpecFor(Eligible[I]).Weight;Total+=Weights[I];}
    if(!(Total>0.f)) return Eligible[0];
    float Target=std::clamp(std::isfinite(Roll)?Roll:0.f,0.f,.99999f)*Total;
    for(int I=0;I<N;++I) {Target-=Weights[I];if(Target<0.f&&Weights[I]>0.f) return Eligible[I];}
    return Eligible[N-1];
}

// ---- Fluid movement (0.24) ----
// Pure planning of where an enemy steps next. World-space flat vectors in, a flat unit direction and a speed fraction out. Nothing here touches the world:
// the caller supplies whether a probe step is clear, and the planner is exercised by Tests/enemy_ai_024_tests.cpp over hundreds of thousands of random states.
constexpr int ModeHold=0,ModeAdvance=1,ModeCircle=2,ModeBackpedal=3,ModeRetreat=4;
constexpr float CircleReachSlack=120.f;  // a melee enemy circles only while within Circle + this of its target; farther out it closes in
constexpr float HoldDistance=145.f;      // a melee enemy that has closed in this far (with sight) stands and waits, as before 0.24
constexpr float BackpedalSpeed=.9f,RetreatSpeed=1.15f;
namespace Detail {
inline void Rotate(float X,float Y,float Degrees,float& OutX,float& OutY) {
    const float Radians=Degrees*0.017453292f,C=std::cos(Radians),S=std::sin(Radians);
    OutX=X*C-Y*S;OutY=X*S+Y*C;
}
// Makes (X,Y) a unit vector; false (and a zero vector) when it has no usable direction.
inline bool Unit(float& X,float& Y) {
    const float L=std::sqrt(X*X+Y*Y);
    if(!(L>1e-4f)||!std::isfinite(L)) {X=Y=0.f;return false;}
    X/=L;Y/=L;return true;
}
}
struct MovePlan {int Mode=ModeHold;float X=0.f,Y=0.f,Speed=0.f;bool Face=false;}; // Face: turn to look at the target (strafing, backing away) instead of along the motion
// Distance: flat distance to the target. (ToX,ToY): from the enemy to the target. (RouteX,RouteY): the nav-grid direction toward the target (zero = use the straight line).
// Ready: its attack is off cooldown. Sight: clear line to the target. Side: +1 or -1, which way it circles. FlankDegrees: pack-spreading angle added to an advance.
inline MovePlan PlanMovement(const Kit& K,float Distance,float ToX,float ToY,float RouteX,float RouteY,bool Ready,bool Sight,bool Retreating,float Side,float FlankDegrees) {
    MovePlan P;
    if(!std::isfinite(Distance)||!std::isfinite(ToX)||!std::isfinite(ToY)) return P;
    float tx=ToX,ty=ToY;
    if(!Detail::Unit(tx,ty)) return P; // standing on the target: no sensible direction
    float rx=RouteX,ry=RouteY;
    if(!std::isfinite(rx)||!std::isfinite(ry)||!Detail::Unit(rx,ry)) {rx=tx;ry=ty;}
    const float Sign=Side<0.f?-1.f:1.f;
    auto Advance=[&]() {
        const float Flank=std::isfinite(FlankDegrees)?std::clamp(FlankDegrees,-45.f,45.f):0.f;
        Detail::Rotate(rx,ry,Flank,P.X,P.Y);P.Mode=ModeAdvance;P.Speed=1.f;P.Face=false;
    };
    // Walk around the target at Radius: mostly sideways, with a radial pull that eases it back onto the circle.
    auto Orbit=[&](float Radius,float Speed) {
        const float Pull=std::clamp((Distance-Radius)/180.f,-1.f,1.f);
        float dx=-ty*Sign*.85f+tx*Pull*.9f,dy=tx*Sign*.85f+ty*Pull*.9f;
        if(!Detail::Unit(dx,dy)) return;
        P.Mode=ModeCircle;P.X=dx;P.Y=dy;P.Speed=Speed;P.Face=true;
    };
    if(Retreating) { // away from the target and a little to one side, turning to run rather than backing up
        Detail::Rotate(-tx,-ty,20.f*Sign,P.X,P.Y);P.Mode=ModeRetreat;P.Speed=RetreatSpeed;P.Face=false;return P;
    }
    if(K.BandMax>0.f) { // shooter or caster
        if(!Sight||Distance>K.BandMax) {Advance();return P;}
        if(Distance<K.BandMin) {Detail::Rotate(-tx,-ty,25.f*Sign,P.X,P.Y);P.Mode=ModeBackpedal;P.Speed=BackpedalSpeed;P.Face=true;return P;}
        Orbit(.5f*(K.BandMin+K.BandMax),K.Strafe);
        if(P.Mode==ModeHold) Advance();
        return P;
    }
    if(K.Circle>0.f&&!Ready&&Sight&&Distance<K.Circle+CircleReachSlack) { // melee waiting for its attack: circle just outside reach instead of standing in the target's face
        Orbit(K.Circle,K.Strafe);
        if(P.Mode!=ModeHold) return P;
    }
    if(Distance<=HoldDistance&&Sight) {P.Mode=ModeHold;P.Face=true;return P;}
    Advance();return P;
}
// Which way to circle: +1 or -1, flipping every 2.2 to 4.0 seconds (a different rhythm per enemy), so a crowd does not all swirl the same way in lockstep.
inline float StrafeSide(unsigned Id,float Seconds) {
    if(!std::isfinite(Seconds)) return 1.f;
    const float Period=2.2f+float((Id*7u)%19u)*.1f;
    const long long N=static_cast<long long>(std::floor((Seconds+float(Id%13u)*.31f)/Period));
    return ((N+static_cast<long long>(Id&1u))&1LL)?1.f:-1.f;
}
// Pack spreading: each enemy approaches from its own angle (stable per enemy, up to FlankSpread degrees either way), fading out as it gets close.
constexpr float FlankSpread=34.f,FlankNear=450.f,FlankFar=1000.f;
inline float FlankDegrees(unsigned Id,float Distance) {
    if(!std::isfinite(Distance)) return 0.f;
    unsigned H=Id*2654435761u;H^=H>>15;H*=2246822519u;H^=H>>13;
    const float U=float(H%2001u)/1000.f-1.f; // -1..1
    const float T=std::clamp((Distance-FlankNear)/(FlankFar-FlankNear),0.f,1.f);
    return U*FlankSpread*T*T*(3.f-2.f*T);
}
// Falling back when hurt: once per enemy, for at most RetreatSeconds, and never while only a couple of enemies remain (the last stragglers must come to the
// player, not run from them). A retreat that was refused because too few enemies were left is not used up; it can still happen later.
constexpr float RetreatSeconds=2.f;
constexpr int RetreatMinLiving=3;
struct RetreatState {float Until=-1.f;bool Used=false;};
inline bool UpdateRetreat(const Kit& K,float HealthFraction,int Living,float Now,RetreatState& S) {
    if(!std::isfinite(Now)||!std::isfinite(HealthFraction)||HealthFraction<=0.f) return false;
    if(Now<S.Until) return true;
    if(S.Used||!(K.RetreatBelow>0.f)||HealthFraction>=K.RetreatBelow||Living<RetreatMinLiving) return false;
    S.Until=Now+RetreatSeconds;S.Used=true;return true;
}
// Stuck against something: step sideways to the wanted direction (75 or 90 degrees off), alternating side as the stuck time grows, instead of pushing into it.
inline void UnstickDirection(float DesiredX,float DesiredY,float StuckSeconds,unsigned Id,float& OutX,float& OutY) {
    OutX=OutY=0.f;
    float x=DesiredX,y=DesiredY;
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(StuckSeconds)||!Detail::Unit(x,y)) return;
    const int Whole=static_cast<int>(std::clamp(StuckSeconds,0.f,1000.f));
    const float Degrees=((Id+static_cast<unsigned>(Whole))&1u)?(Whole%2?90.f:75.f):-(Whole%2?90.f:75.f);
    Detail::Rotate(x,y,Degrees,OutX,OutY);
}
// Picks the first usable direction from a fan around the wanted one (straight, then 40, 80 and 125 degrees to either side, nearest first). Clear(dx,dy) says whether a
// short probe step that way is free. Returns false (and a zero direction) when every direction is blocked.
template<class ClearFn> inline bool ChooseClearDirection(float WantX,float WantY,ClearFn&& Clear,float& OutX,float& OutY) {
    OutX=OutY=0.f;
    float x=WantX,y=WantY;
    if(!std::isfinite(x)||!std::isfinite(y)||!Detail::Unit(x,y)) return false;
    static constexpr float Offsets[]={0.f,40.f,-40.f,80.f,-80.f,125.f,-125.f};
    for(float Offset:Offsets) {
        float cx,cy;Detail::Rotate(x,y,Offset,cx,cy);
        if(Clear(cx,cy)) {OutX=cx;OutY=cy;return true;}
    }
    return false;
}

// ---- Fliers: where they may go (Gloomwing and the flying boss) ----
// Fliers ignore walls and floor collision, so they are kept inside the arena by explicit limits. TUNABLE.
constexpr float FlightFloorClearance=25.f;   // the capsule's bottom never goes below this height above the floor
constexpr float FlightCeiling=1100.f;        // the capsule's centre never goes above this height above the floor
constexpr float FlightEdgeMargin=260.f;      // Endless: stay this far inside the arena walls
constexpr float FlightRoomSlack=300.f;       // story rooms: may overhang a room by this much (corridors between rooms)
// Height band for a flier's CENTRE.
inline float ClampFlightHeight(float Z,float FloorZ,float CapsuleHalfHeight) {
    const float Low=FloorZ+std::max(0.f,CapsuleHalfHeight)+FlightFloorClearance,High=std::max(Low+50.f,FloorZ+FlightCeiling);
    if(std::isnan(Z)) return Low; // NaN lands on the lowest legal height; +-infinity clamps to the nearest limit
    return std::clamp(Z,Low,High);
}
// Clamp a point into a rectangle: NaN lands on the rectangle's centre, +-infinity on the nearest edge. The rectangle is normalised if its limits are crossed.
inline void ClampToRect(float MinX,float MaxX,float MinY,float MaxY,float& X,float& Y) {
    if(MinX>MaxX) MinX=MaxX=(MinX+MaxX)*.5f;
    if(MinY>MaxY) MinY=MaxY=(MinY+MaxY)*.5f;
    X=std::isnan(X)?(MinX+MaxX)*.5f:std::clamp(X,MinX,MaxX);
    Y=std::isnan(Y)?(MinY+MaxY)*.5f:std::clamp(Y,MinY,MaxY);
}
// A flier circles its target. The radius "breathes" between Min and Max so melee players get regular windows when it sweeps within reach:
// Min at the start of each cycle, Max half a cycle later.
inline float OrbitRadius(float Seconds,float Min,float Max,float CycleSeconds) {
    if(!std::isfinite(Seconds)||!(CycleSeconds>0.f)) return Max;
    const float Phase=Seconds/CycleSeconds*6.2831853f;
    return Min+(Max-Min)*(.5f-.5f*std::cos(Phase));
}

// ---- Flying boss: the molt ----
// At half health the boss rears back (MoltWindupSeconds, telegraphed by the wings), then bursts out hatchlings (small Gloomwings) from its wings.
// MoltDue is true once per fight. TUNABLE.
constexpr float MoltHealthFraction=.5f,MoltWindupSeconds=1.5f,HatchlingHealthScale=.6f;
constexpr int MoltHatchlingsSolo=4,MoltHatchlingsCoop=6;
inline bool MoltDue(bool AlreadyMolted,float Health,float MaxHealth) {
    return !AlreadyMolted&&std::isfinite(Health)&&std::isfinite(MaxHealth)&&MaxHealth>0.f&&Health>0.f&&Health<=MaxHealth*MoltHealthFraction;
}
inline int MoltHatchlings(int Players) {return Players>1?MoltHatchlingsCoop:MoltHatchlingsSolo;}

// ---- Broodcaller ----
// It summons minions in pairs whenever it has room under the cap and its summon cooldown is over; otherwise it shoots. (Before 0.24 it summoned on its
// first two attacks only, because a lifetime attack counter was compared with 2 and never reset.) TUNABLE.
constexpr int BroodMinionPair=2,BroodMinionCap=4,BroodMinionCapCoop=6;
constexpr float BroodSummonCooldown=6.5f;
inline bool BroodShouldSummon(int Live,int Cap,float Now,float NextSummon) {
    if(!std::isfinite(Now)||!std::isfinite(NextSummon)) return false;
    return Now>=NextSummon&&std::max(0,Live)+BroodMinionPair<=std::max(BroodMinionPair,Cap);
}
}
