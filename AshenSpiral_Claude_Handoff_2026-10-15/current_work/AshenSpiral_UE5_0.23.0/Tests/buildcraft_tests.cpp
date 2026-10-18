#include "../Source/AshenSpiral/Public/ASBuildRules.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>

static int Checks=0;
static void Check(bool OK,const char* What) {++Checks;if(!OK) {std::cerr<<"FAILED: "<<What<<"\n";std::exit(1);}}
static bool Near(float A,float B,float E=.0005f) {return std::abs(A-B)<E;}
int main() {
    using namespace ASBuild;
    Check(Count==69,"stable trait count");
    // 0.23.2: Attunement (elemental) and Teamwork (synergy). The synergy buff per rank must beat the elemental one, and Teamwork is co-op only.
    Check(Near(ElementalDamageMultiplier(0),1.f)&&Near(ElementalDamageMultiplier(5),1.6f)&&Near(ElementalDamageMultiplier(-3),1.f),"Attunement: +12% per rank, no penalty below zero");
    Check(Near(SynergyDamageMultiplier(0,0),1.f)&&Near(SynergyDamageMultiplier(5,0),2.f)&&Near(SynergyDamageMultiplier(2,3),2.f),"Teamwork: +20% per rank, summed over both players");
    Check(TeamworkPerRank>AttunementPerRank&&SynergyDamageMultiplier(Catalog[Index(Trait::Teamwork)].Max,0)>ElementalDamageMultiplier(Catalog[Index(Trait::Attunement)].Max),"synergy buff is larger than the elemental buff");
    {Context Solo,Duo;Duo.Coop=true;Check(!CanChoose(Solo,OfferBase+Index(Trait::Teamwork))&&CanChoose(Duo,OfferBase+Index(Trait::Teamwork))&&CanChoose(Solo,OfferBase+Index(Trait::Attunement)),"Teamwork is only offered in co-op; Attunement always");}
    // 0.23.3: Wide Arc is a melee-only reach trait, +10% per rank.
    Check(Near(MeleeReachMultiplier(0),1.f)&&Near(MeleeReachMultiplier(5),2.0f)&&Near(MeleeReachMultiplier(-2),1.f),"Wide Arc: +20% reach per rank");
    {const int Arc=OfferBase+Index(Trait::WideArc);Context Sword,Bow;Sword.Weapon=0;Bow.Weapon=3;Check(CanChoose(Sword,Arc)&&!CanChoose(Bow,Arc),"Wide Arc is offered for melee weapons only");}
    Context C;
    for(int I=0;I<Count;++I) {
        const auto& D=Catalog[I];Check(D.Max>0&&D.Max<=5,"finite pick caps");
        Check(D.Category>=0&&D.Category<=2&&D.Rarity>=0&&D.Rarity<=2,"valid category and rarity");
        Check(D.WeaponMask>0&&D.WeaponMask<=ASRules::WeaponMask&&D.Tags>0,"valid eligibility and affinity masks");
        Check(IsTrait(OfferBase+I),"stable trait offer ID");
    }
    Check(!IsTrait(199)&&!IsTrait(OfferBase+Count)&&IsTrait(OfferBase+Count-1),"offer range rejects neighbors");
    Check(!CanChoose(C,200+Index(Trait::Mend)),"full-health heroes do not get wasted healing offers");
    // FullHeal is now a compatibility alias of Vengeance (a lethal-hit save), not an instant heal; it must not be gated on missing health.
    Check(Trait::FullHeal==Trait::Vengeance,"FullHeal alias resolves to Vengeance");
    Check(!CanChoose(C,200+Index(Trait::Cleanse)),"cleanse requires a curse");
    Check(!CanChoose(C,200+Index(Trait::Refine))&&!CanChoose(C,200+Index(Trait::Duplicate)),"rare modifications require an owned eligible choice");
    C.Hurt=true;C.Curse=1;
    Check(CanChoose(C,200+Index(Trait::Mend))&&CanChoose(C,200+Index(Trait::Cleanse)),"context enables useful survival choices");
    C.MissingRelic=false;Check(!CanChoose(C,200+Index(Trait::Mystery)),"mystery excludes exhausted relic pool");
    C.Last=200+Index(Trait::Armor);C.Traits[Index(Trait::Armor)]=1;
    Check(CanChoose(C,200+Index(Trait::Refine))&&CanChoose(C,200+Index(Trait::Duplicate)),"owned repeatable trait permits evolution and duplicate");
    C.Traits[Index(Trait::Armor)]=5;Check(!CanChoose(C,200+Index(Trait::Duplicate)),"duplicate cannot bypass normal cap");
    Check(CanEvolve(C,Index(Trait::Armor)),"evolution works at normal rank cap");
    C.Evolutions=std::uint64_t(1)<<Index(Trait::Armor);Check(!CanEvolve(C,Index(Trait::Armor)),"cannot evolve the same trait twice");
    C.Traits[Index(Trait::DashCharge)]=1;Check(!CanEvolve(C,Index(Trait::DashCharge)),"charge evolution cannot exceed three-charge design limit");
    C.Traits[Index(Trait::Phoenix)]=1;Check(!CanEvolve(C,Index(Trait::Phoenix)),"one-use oath cannot be rearmed by evolution");
    C.Last=200+Index(Trait::Duplicate);Check(!CanChoose(C,C.Last),"duplicate recursion rejected");
    C.Last=100;C.Stats[0]=0;Check(CanChoose(C,200+Index(Trait::Duplicate)),"legacy stats can be duplicated");
    C.PermanentCooldown=5;C.Stats[0]=5;Check(!CanChoose(C,100),"cooldown cap respected");
    Check(!Evolved(~std::uint64_t(0),-1)&&!Evolved(~std::uint64_t(0),Count),"flag shifts range checked");

    ASRules::Random R(140013);
    for(int Sample=0;Sample<18000;++Sample) {
        Context B;B.Weapon=Sample%ASRules::WeaponCount;B.Room=Sample%32;B.Hurt=Sample%2;B.Curse=Sample%3==0;B.MissingRelic=Sample%7!=0;
        for(int I=0;I<Count;++I) B.Traits[I]=R.Range(Catalog[I].Max+1);
        for(int I=0;I<6;++I) B.Stats[I]=R.Range(6);
        B.Last=Sample%2?100+R.Range(6):200+R.Range(Count);
        std::array<bool,3> CategoryAvailable{};int Eligible=0;
        for(int V=100;V<OfferBase+Count;++V) if(CanChoose(B,V)) {CategoryAvailable[Category(V)]=true;++Eligible;}
        const auto A=Offers(B,Sample+1,5),Again=Offers(B,Sample+1,5);
        Check(A==Again,"identical seed and build gives identical choices");
        Check(A.size()==std::size_t(std::min(5,Eligible)),"offer count bounded by available pool");
        std::set<int> Seen;
        for(std::size_t I=0;I<A.size();++I) {
            Check(CanChoose(B,A[I]),"all chosen offers eligible for build and weapon");
            Check(Seen.insert(A[I]).second,"no duplicate offers in a chest");
            // Earlier fallback slots may use another category only if theirs is exhausted.
            if(I<3&&CategoryAvailable[0]&&CategoryAvailable[1]&&CategoryAvailable[2]) Check(Category(A[I])==int(I),"damage/survival/modifier mix");
        }
        Check(Offers(B,Sample,-1).empty(),"negative requested count is empty");
    }
    Context Fresh,Stacked;Fresh.Room=Stacked.Room=10;
    for(int& V:Stacked.Traits) V=5;
    for(int Offer=100;Offer<OfferBase+Count;++Offer) if(ASRules::IsRunUpgradeOffer(Offer)||IsTrait(Offer)) {
        Check(Weight(Stacked,Offer)>=Weight(Fresh,Offer),"synergy only increases weight");
        Check(Weight(Stacked,Offer)<=Weight(Fresh,Offer)*2,"affinity never exceeds 2x");
    }
    for(int Weapon=0;Weapon<ASRules::WeaponCount;++Weapon) {Context B;B.Weapon=Weapon;const bool Projectile=Weapon==2||Weapon==3||Weapon==4||Weapon==6||Weapon==7;Check(CanChoose(B,200+Index(Trait::Pierce))==Projectile,"melee excludes projectile-only traits");}
    for(int RankValue=-2;RankValue<=20;++RankValue) {
        Check(CriticalMultiplier(RankValue)>=1.8f&&CriticalMultiplier(RankValue)<=2.6f,"critical hard cap");
        Check(DamageReduction(RankValue,RankValue,true)>=0&&DamageReduction(RankValue,RankValue,true)<=.65f,"damage reduction hard cap");
        Check(MaxDashCharges(RankValue)>=1&&MaxDashCharges(RankValue)<=3,"dash capacity hard cap");
        Check(StatusDuration(RankValue)>=1&&StatusDuration(RankValue)<=1.8f,"status duration cap");
        Check(ProcChance(RankValue,.18f)>=0&&ProcChance(RankValue,.18f)<=.75f,"proc chance hard cap");
    }
    for(int RankValue=0;RankValue<=4;++RankValue) for(int Base=1;Base<=30;++Base) {
        float Fraction=0;int Sum=0;for(int I=0;I<1000;++I) {Sum+=CreditKills(Base,RankValue,Fraction);Check(Fraction>=0&&Fraction<1,"currency remainder bounded");}
        Check(std::abs(Sum-1000*Base*(1+.2*RankValue))<1.01,"fractional carry rewards low-value kills fairly");
    }
    for(int Capacity=1;Capacity<=3;++Capacity) for(int Initial=0;Initial<=Capacity;++Initial) {
        for(int Milliseconds=0;Milliseconds<5000;Milliseconds+=37) {
            int A=Initial,B=Initial;float TA=Initial<Capacity?1.1f:0,TB=TA;const float Delta=Milliseconds*.001f;
            RefillDash(A,TA,Delta,Capacity,1.1f);
            for(int I=0;I<100;++I) RefillDash(B,TB,Delta/100,Capacity,1.1f);
            Check(A==B&&Near(TA,TB),"dash refill independent of frame partition");
            Check(A>=0&&A<=Capacity&&TA>=0,"dash refill state bounded");
        }
    }
    {int A=0;float T=.6f;RefillDash(A,T,-1,3,1.1f);Check(A==0&&T==.6f,"negative delta cannot recharge");
     RefillDash(A,T,std::numeric_limits<float>::infinity(),3,1.1f);Check(A==0&&T==.6f,"invalid delta ignored");
     T=std::numeric_limits<float>::quiet_NaN();RefillDash(A,T,.1f,3,1.1f);Check(Near(T,1.f),"invalid timer repaired");
     RefillDash(A,T,1000,3,1.1f);Check(A==3&&T==0,"long frame never overfills charges");}
    for(int Reserve=0;Reserve<=4;++Reserve) for(int Recovery=0;Recovery<=4;++Recovery) for(int I=0;I<=100;++I) {
        const float Delta=I*.05f;float A=RefillFocus(0,Delta,Reserve,Recovery),B=0;
        for(int J=0;J<50;++J) B=RefillFocus(B,Delta/50,Reserve,Recovery);
        Check(Near(A,B),"Focus regeneration independent of frame partition");Check(A>=0&&A<=60+10*Reserve,"Focus pool capped");
    }
    Check(RefillFocus(20,-10,0,0)==20,"negative time never drains Focus");
    Check(RefillFocus(std::numeric_limits<float>::quiet_NaN(),1,0,0)==5,"invalid Focus repaired");
    for(int Ranks=0;Ranks<=4;++Ranks) for(int Partitions:{1,3,30,300}) {
        ASRules::ElementState E;E.Apply(7,1,false,StatusDuration(Ranks));E.Apply(7,1,false,StatusDuration(Ranks));E.Apply(7,1,false,StatusDuration(Ranks));
        float Sum=0;for(int I=0;I<Partitions;++I) {E.Tick(10.f/Partitions);Sum+=E.BleedDamage;}
        Check(Near(Sum,24*StatusDuration(Ranks),.002f),"bleed integrates exact capped duration and two stacks");
        Check(E.Bleed==0&&E.BleedStacks==0&&E.BleedDPS==0,"bleed fully expires");
    }
    for(int Previous=-1;Previous<=8;++Previous) {
        Check(ChallengeTier(Previous,2,1)==0,"co-op wager requires unanimity");
        Check(ChallengeTier(Previous,2,2)>=1&&ChallengeTier(Previous,2,2)<=3,"wagers bounded at three tiers");
        Check(ChallengeTier(Previous,0,0)==0,"empty party cannot opt in");
    }
    Check(Near(ChallengeHealth(3),1.6f)&&Near(ChallengeDamage(3),1.36f),"top-tier difficulty multipliers");
    Check(!FlameActive(1.999f)&&FlameActive(2)&&FlameActive(3.999f)&&!FlameActive(4),"flame warning and active boundaries");
    Check(!BladeActive(1.499f)&&BladeActive(1.5f)&&BladeActive(4.499f)&&!BladeActive(4.5f),"blade warning and active boundaries");
    Check(SweptCircle(0,0,1000,0,500,150,160),"long-frame dash still hits mid-segment foe");
    Check(!SweptCircle(0,0,1000,0,500,170,160),"dash excludes distant foe");
    Check(SweptCircle(0,0,0,0,0,0,160)&&!SweptCircle(0,0,0,0,200,0,160),"stationary dash trace handled");
    Check(BladeContact(0,180,0,200)&&!BladeContact(0,100,100,200),"blades hit strips, not entire disc");
    Check(BladeContact(.25f,100,100,200),"rotating blades follow visual angle");
    for(int I=0;I<10000;++I) {const float P=TrapPhase(I*.13f,30);Check(P>=0&&P<6,"trap cycle bounded");}
    std::cout<<"PASS: "<<Checks<<" buildcraft checks; 18000 weighted reward drafts; 69 stable trait IDs (68 selectable).\n";
        // 0.23 Endless offers: exclusives only in Endless, sidegrade shift after wave 15, deterministic.
    {
        Context N;N.Weapon=0;Context E=N;E.Endless=true;E.EndlessWave=20;
        int NormalExclusive=0,EndlessExclusive=0,Offence=0,Offers5=0;
        for(std::uint32_t Seed=1;Seed<3000;++Seed) {
            for(int V:Offers(N,Seed,5)) if(ASExpedition::Exclusive(V)) ++NormalExclusive;
            const auto A=Offers(E,Seed,5),B=Offers(E,Seed,5);
            Check(A==B,"endless offers are deterministic for a seed");
            Check(A.size()==5,"endless chest with 5 choices offers 5");
            for(int V:A) {if(ASExpedition::Exclusive(V)) ++EndlessExclusive;else if(Category(V)==0) ++Offence;}
            ++Offers5;
        }
        Check(NormalExclusive==0,"normal runs never offer Endless-exclusive traits");
        Check(EndlessExclusive>Offers5*ExclusiveChance(20)/100/2&&EndlessExclusive<Offers5*ExclusiveChance(20)/100*2,"exclusive traits appear at about the tuned rate");
        Check(Offence<=Offers5*2,"after wave 15 at most about two of five Endless choices are offence");
        Context Early=E;Early.EndlessWave=10;int Exclusive10=0;for(std::uint32_t Seed=1;Seed<500;++Seed) for(int V:Offers(Early,Seed,3)) if(ASExpedition::Exclusive(V)) ++Exclusive10;
        Check(Exclusive10>0,"exclusives can appear before wave 15");
        Context W1=E;W1.EndlessWave=1;for(std::uint32_t Seed=1;Seed<500;++Seed) for(int V:Offers(W1,Seed,3)) Check(!ASExpedition::Exclusive(V),"no exclusives in the first two waves");
    }
    std::cout<<"Shared C++ rules only: does not compile or run Unreal actors, replication, UI, physics or saves.\n";
}
