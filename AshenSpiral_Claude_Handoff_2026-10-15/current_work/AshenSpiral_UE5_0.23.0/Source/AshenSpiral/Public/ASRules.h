#pragma once
// Engine-independent rules used by the game AND the native regression tests.
#include <array>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <queue>
#include <utility>
#include <functional>
namespace ASRules
{
constexpr int Grid = 4, MinCells = 5, MaxCells = 10, RoomsPerWard = 8, WardCount = 4;
inline int SideTrapKind(int SpawnIndex) {return SpawnIndex%2==0?4:5;}
inline int MaterialForWard(int Ward) {return Ward==1?1:0;}
inline void RetireUnusedProgress(int& TalentMask,int& Points,int& Scrap,int& Crystal,int& Vine) {
    if(TalentMask&(1<<9)) {Points=int(std::min<std::int64_t>(1000000,std::int64_t(std::max(0,Points))+1));TalentMask&=~(1<<9);}
    const std::int64_t Total=std::int64_t(std::max(0,Scrap))+std::max(0,Crystal)+std::max(0,Vine);
    Scrap=int(std::min<std::int64_t>(100000000,Total));Crystal=Vine=0;
}
inline float GrantShield(float Current,float Gain,float Cap) {return std::max(Current,std::min(Cap,Current+std::max(0.f,Gain)));}
constexpr int WeaponCount=9, WeaponMask=(1<<WeaponCount)-1;
constexpr int MaxEnemies = 42, MaxProjectiles = 180, MaxWells = 8;
constexpr int MaxCurrency = 100000000;
constexpr int HearthCount=20, RunUpgradeCount=6, MaxRunUpgradeRank=5, RunUpgradeOfferBase=100;
constexpr float BaseSpiralCooldown=3.3f;
constexpr float ReviveSeconds=2.f, ReviveRadius=300.f, ReviveVerticalReach=150.f;
struct Random {
    std::uint32_t State;
    explicit Random(std::uint32_t Seed) : State(Seed ? Seed : 0xA53B29u) {}
    std::uint32_t Next() { State ^= State << 13; State ^= State >> 17; State ^= State << 5; return State; }
    int Range(int Count) { return Count > 0 ? int(Next() % std::uint32_t(Count)) : 0; }
};
struct Cell { int X=0,Y=0,HalfX=600,HalfY=600,SpanX=1,SpanY=1,WorldX=0,WorldY=0; };
struct Edge { int A=0, B=0; bool Gap=false; };
struct Layout { std::vector<Cell> Cells; std::vector<Edge> Edges; int Exit=0,Shape=0,Width=5,Height=5,SpacingX=1920,SpacingY=1920; };
inline int UpgradeRank(int Rank) {return std::clamp(Rank,0,MaxRunUpgradeRank);}
inline float SpiralCooldown(int Permanent,int Run) {
    // Percentage points stack additively, with a 65% total reduction cap.
    return BaseSpiralCooldown*std::max(.35f,1.f-.10f*UpgradeRank(Permanent)-.08f*UpgradeRank(Run));
}
inline float AttackSpeed(int Permanent,int Run) {return 1.f+.08f*UpgradeRank(Permanent)+.12f*UpgradeRank(Run);}
inline float AttackInterval(float Base,int Permanent,int Run,bool Attuned=false) {
    return Base/(AttackSpeed(Permanent,Run)*(Attuned?1.25f:1.f));
}
inline float DamageMultiplier(int Permanent,int Run) {return 1.f+.10f*UpgradeRank(Permanent)+.15f*UpgradeRank(Run);}
inline float MoveSpeed(int Permanent,int Run) {return 640.f*(1.f+.05f*UpgradeRank(Permanent)+.05f*UpgradeRank(Run));}
inline float CriticalChance(int Permanent,int Run) {return std::min(.50f,.05f+.04f*UpgradeRank(Permanent)+.05f*UpgradeRank(Run));}
inline float StartingHealth(int Permanent,int Run) {return 110.f+8.f*std::clamp(Permanent,0,3)+15.f*UpgradeRank(Run);}
inline bool IsRunUpgradeOffer(int Choice) {return Choice>=RunUpgradeOfferBase&&Choice<RunUpgradeOfferBase+RunUpgradeCount;}
inline bool CanUpgradeRunStat(int Stat,int Rank,int Permanent=0) {
    if(Stat<0||Stat>=RunUpgradeCount||UpgradeRank(Rank)>=MaxRunUpgradeRank) return false;
    if(Stat==0) return SpiralCooldown(Permanent,Rank+1)<SpiralCooldown(Permanent,Rank)-.00001f;
    if(Stat==5) return CriticalChance(Permanent,Rank+1)>CriticalChance(Permanent,Rank)+.00001f;
    return true;
}
inline std::vector<int> RunUpgradeOffers(const std::array<int,RunUpgradeCount>& Ranks,std::uint32_t Seed,int PermanentCooldown=0,int PermanentCritical=0) {
    Random R(Seed);std::vector<int> Offers,Remaining;
    // Always make the two requested build paths discoverable until they are maxed.
    for(int I=0;I<RunUpgradeCount;++I) if(CanUpgradeRunStat(I,Ranks[I],I==0?PermanentCooldown:(I==5?PermanentCritical:0))) {
        if(I<2) Offers.push_back(RunUpgradeOfferBase+I);else Remaining.push_back(RunUpgradeOfferBase+I);
    }
    while(Offers.size()<3&&!Remaining.empty()) {
        const int Pick=R.Range(int(Remaining.size()));Offers.push_back(Remaining[Pick]);Remaining.erase(Remaining.begin()+Pick);
    }
    return Offers;
}
inline bool ReviveInReach(double DistanceSquared2D,double HeightDifference) {
    return std::isfinite(DistanceSquared2D)&&DistanceSquared2D>=0&&DistanceSquared2D<=ReviveRadius*ReviveRadius
        &&std::isfinite(HeightDifference)&&std::abs(HeightDifference)<=ReviveVerticalReach;
}
inline float AdvanceRevive(float Progress,float Delta,bool Eligible) {
    if(!Eligible) return 0.f;
    if(!std::isfinite(Progress)) Progress=0.f;
    Progress=std::clamp(Progress,0.f,ReviveSeconds);
    if(!std::isfinite(Delta)||Delta<=0) return Progress;
    return std::min(ReviveSeconds,Progress+Delta);
}
struct WeaponPose {float X=34.f,Y=62.f,Z=82.f,ForwardAngle=0.f,Swing=0.f;};
inline WeaponPose RightHandPose(float Age,float Duration,float WalkStep,bool Ranged) {
    constexpr float Pi=3.14159265358979323846f;
    const float Swing=Age>=0&&Age<Duration&&Duration>0?std::sin(Age/Duration*Pi):0.f;
    WeaponPose P;P.Swing=Swing;
    P.X+=std::clamp(WalkStep,-1.f,1.f)*4.f+Swing*(Ranged?12.f:50.f);
    P.Z+=std::abs(std::clamp(WalkStep,-1.f,1.f))*2.f+Swing*18.f;
    P.ForwardAngle=Swing*(Ranged?18.f:68.f)*Pi/180.f;
    return P;
}
inline WeaponPose WeaponGripPose(float Age,float Duration,float WalkStep,int Weapon) {
    auto P=RightHandPose(Age,Duration,WalkStep,Weapon>=2&&Weapon<=4);
    if(Weapon==1) P.Y=70.f;
    if(Weapon==2) {P.X=82.f-P.Swing*3.f;P.Y=-5.f;P.Z=123.f+std::abs(WalkStep);P.ForwardAngle=0;}
    if(Weapon>=6) {P.X=84.f-P.Swing*5.f;P.Y=25.f;P.Z=120.f+std::abs(WalkStep);P.ForwardAngle=-P.Swing*.04f;}
    return P;
}
inline Layout Generate(std::uint32_t Seed) {
    Random R(Seed);Layout L;L.Shape=R.Range(5);
    L.Width=4;L.Height=3;
    int Target=MinCells+R.Range(MaxCells-MinCells+1);
    if(L.Shape==1) {L.Width=2;L.Height=4;Target=5+R.Range(4);}
    if(L.Shape==2) Target=5+R.Range(3);
    if(L.Shape==3) {L.Width=3;L.Height=3;Target=8;}
    if(R.Range(2)) std::swap(L.Width,L.Height);
    // Spacing fields now describe the clear corridor length between column/row extents.
    L.SpacingX=600+R.Range(3)*120;L.SpacingY=600+R.Range(3)*120;
    auto Allowed=[&](int X,int Y) {
        if(X<0||Y<0||X>=L.Width||Y>=L.Height) return false;
        if(L.Shape==2) return X==L.Width/2||Y==L.Height/2;
        if(L.Shape==3) return X==0||Y==0||X==L.Width-1||Y==L.Height-1;
        return true;
    };
    auto AddCell=[&](int X,int Y) {
        // A footprint is one actual room, never four/nine rooms with hidden walls.
        const int Pick=L.Cells.empty()?1:R.Range(Target>=9?4:5);
        const int SX=Pick==0?1:(Pick==4?3:2),SY=Pick==0?1:(Pick>=3?3:2);
        Cell C;C.X=X;C.Y=Y;C.SpanX=SX;C.SpanY=SY;
        if(Pick==2) {C.SpanX=3;C.SpanY=2;}
        C.HalfX=600*C.SpanX;C.HalfY=600*C.SpanY;L.Cells.push_back(C);
    };
    AddCell(L.Shape==3?0:L.Width/2,L.Shape==3?0:L.Height/2);
    const int DX[4]={1,-1,0,0},DY[4]={0,0,1,-1};
    auto Index=[&](int X,int Y) {for(int I=0;I<int(L.Cells.size());++I) if(L.Cells[I].X==X&&L.Cells[I].Y==Y) return I;return -1;};
    while(int(L.Cells.size())<Target) {
        struct Candidate {int Parent,X,Y;};std::vector<Candidate> Choices;
        for(int I=0;I<int(L.Cells.size());++I) for(int D=0;D<4;++D) {
            const int X=L.Cells[I].X+DX[D],Y=L.Cells[I].Y+DY[D];
            if(Allowed(X,Y)&&Index(X,Y)<0) Choices.push_back({I,X,Y});
        }
        if(Choices.empty()) break;
        // The gallery topology grows mainly from the newest room for a winding spine.
        const auto C=L.Shape==4&&R.Range(3)!=0?Choices.back():Choices[R.Range(int(Choices.size()))];const int N=int(L.Cells.size());
        AddCell(C.X,C.Y);L.Edges.push_back({C.Parent,N,false});
    }
    int Gaps=0;
    for(int A=0;A<int(L.Cells.size());++A) for(int B=A+1;B<int(L.Cells.size());++B) {
        if(std::abs(L.Cells[A].X-L.Cells[B].X)+std::abs(L.Cells[A].Y-L.Cells[B].Y)!=1) continue;
        bool Found=false;for(auto E:L.Edges) if((E.A==A&&E.B==B)||(E.A==B&&E.B==A)) Found=true;
        if(!Found&&(L.Shape==3||R.Range(3)!=0)) {
            const bool Gap=L.Shape!=3&&Gaps<1&&R.Range(3)==0;Gaps+=Gap?1:0;L.Edges.push_back({A,B,Gap});
        }
    }
    std::array<int,Grid> Column{},Row{},CentersX{},CentersY{};
    for(const auto& C:L.Cells) {Column[C.X]=std::max(Column[C.X],C.HalfX);Row[C.Y]=std::max(Row[C.Y],C.HalfY);}
    for(int I=1;I<Grid;++I) {CentersX[I]=CentersX[I-1]+Column[I-1]+Column[I]+L.SpacingX;CentersY[I]=CentersY[I-1]+Row[I-1]+Row[I]+L.SpacingY;}
    for(auto& C:L.Cells) {C.WorldX=CentersX[C.X];C.WorldY=CentersY[C.Y];}
    // The reward is the farthest room by shortest WALKING distance, including long halls.
    std::vector<int> Distance(L.Cells.size(),0x3fffffff);Distance[0]=0;
    using Entry=std::pair<int,int>;std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> Queue;Queue.push({0,0});
    while(!Queue.empty()) {
        const auto At=Queue.top();Queue.pop();if(At.first!=Distance[At.second]) continue;
        for(auto E:L.Edges) if(!E.Gap) {
            const int Next=E.A==At.second?E.B:(E.B==At.second?E.A:-1);if(Next<0) continue;
            const auto& A=L.Cells[At.second];const auto& B=L.Cells[Next];
            const int Cost=At.first+std::abs(A.WorldX-B.WorldX)+std::abs(A.WorldY-B.WorldY);
            if(Cost<Distance[Next]) {Distance[Next]=Cost;Queue.push({Cost,Next});}
        }
    }
    for(int I=1;I<int(L.Cells.size());++I) if(Distance[I]>Distance[L.Exit]) L.Exit=I;
    return L;
}
// ---- Boss arenas (0.23 B7): one hand-authored arena per ward boss, designed around its attacks.
// Shape >= BossShapeBase marks a bespoke boss layout. Cell 0 is the arena (boss at its centre,
// players enter at the west edge), cell 1 is the reward alcove behind the east door.
constexpr int BossShapeBase=10,BossArenaHalf=1800;
inline constexpr float Pi180() {return 3.14159265358979323846f/180.f;}
inline Layout GenerateBoss(int Ward) {
    Layout L;L.Shape=BossShapeBase+std::clamp(Ward,0,WardCount-1);L.Width=2;L.Height=1;L.SpacingX=600;L.SpacingY=600;
    Cell Arena;Arena.X=0;Arena.Y=0;Arena.SpanX=3;Arena.SpanY=3;Arena.HalfX=BossArenaHalf;Arena.HalfY=BossArenaHalf;
    Cell Alcove;Alcove.X=1;Alcove.Y=0;Alcove.HalfX=600;Alcove.HalfY=600;
    L.Cells={Arena,Alcove};L.Edges.push_back({0,1,false});L.Exit=1;
    L.Cells[0].WorldX=0;L.Cells[0].WorldY=0;
    L.Cells[1].WorldX=L.Cells[0].HalfX+L.Cells[1].HalfX+L.SpacingX;L.Cells[1].WorldY=0;
    return L;
}
// Kind 0: collidable block (Length x Thickness, rotated by Yaw). Kind 1: non-colliding floor glow strip.
// Kind 2: non-colliding floor ring (Length = diameter). Props sharing a Group may touch.
struct BossProp {int Kind,Group;float X,Y,Length,Thickness,Yaw;};
inline std::vector<BossProp> BossProps(int Ward) {
    std::vector<BossProp> P;
    Ward=std::clamp(Ward,0,WardCount-1);int G=0;
    if(Ward==0) {
        // Radial-shot boss: the 12/18-way ring fires along multiples of 30/20 degrees, so six pillars on the
        // 60-degree rays are cover that always matters in both phases.
        for(int K=0;K<6;++K) {const float A=K*60.f*Pi180();P.push_back({0,G++,800.f*std::cos(A),800.f*std::sin(A),150.f,150.f,K*60.f});}
        P.push_back({2,G++,0,0,700,0,0});P.push_back({2,G++,0,0,1500,0,0});
    } else if(Ward==1) {
        // Charge boss: a serpentine of staggered walls makes charges end against stone and breaks straight lines.
        const float Xs[4]={-800.f,-270.f,270.f,800.f},Ys[4]={750.f,-750.f,750.f,-750.f};
        for(int K=0;K<4;++K) {P.push_back({0,G++,Xs[K],Ys[K],1000.f,120.f,90.f});P.push_back({1,G++,Xs[K]+((K%2)?-300.f:300.f),0,1600.f,60.f,90.f});}
    } else if(Ward==2) {
        // Inversion boss (wells are reversed in phase two): open ground for wells and dashes, low plinths only at the corners.
        for(int SX:{-1,1}) for(int SY:{-1,1}) P.push_back({0,G++,SX*1300.f,SY*1300.f,220.f,220.f,45.f});
        P.push_back({2,G++,0,0,1000,0,0});P.push_back({2,G++,0,0,1700,0,0});
    } else {
        // Blast-and-pull boss: a broken ring with four gates interrupts the phase-two pull; corner alcoves shelter blasts.
        for(int K=0;K<4;++K) {
            const float A=(45.f+90.f*K)*Pi180(),D=750.f*std::cos(30.f*Pi180());
            P.push_back({0,G++,D*std::cos(A),D*std::sin(A),750.f,130.f,45.f+90.f*K+90.f});
        }
        for(int SX:{-1,1}) for(int SY:{-1,1}) {
            const int Group=G++;P.push_back({0,Group,SX*1200.f,SY*950.f,500.f,120.f,0});P.push_back({0,Group,SX*950.f,SY*1200.f,500.f,120.f,90.f});
        }
        P.push_back({2,G++,0,0,1500,0,0});
    }
    return P;
}
// ---- Endless arena (0.23 B8): one very large enclosed cell, about three times a boss arena by area.
// The centre (chest and spawn) stays clear. Obstacles are generated deterministically from a seed so server and
// clients rebuild the same arena; a shuffle re-rolls the seed while keeping cover off players.
constexpr int EndlessRoom=100,EndlessShape=20,EndlessHalf=3100;
inline Layout GenerateEndless() {
    Layout L;L.Shape=EndlessShape;L.Width=1;L.Height=1;
    Cell C;C.X=0;C.Y=0;C.SpanX=5;C.SpanY=5;C.HalfX=EndlessHalf;C.HalfY=EndlessHalf;C.WorldX=0;C.WorldY=0;
    L.Cells={C};L.Exit=0;return L;
}
struct EndlessObstacle {float X,Y,Length,Thickness,Yaw,Height;};
struct KeepOut {float X=0,Y=0;bool Active=false;};
constexpr float EndlessCentreClear=800.f,EndlessKeepRadius=520.f,EndlessGap=330.f;
constexpr float EndlessWallHeight=380.f,EndlessLowHeight=160.f; // pillars and walls share one height; low blocks stay jumpable cover
inline std::vector<EndlessObstacle> EndlessProps(std::uint32_t Seed,const std::array<KeepOut,2>& Keep) {
    Random R(Seed*2654435761u+977u);std::vector<EndlessObstacle> Out;
    const float Limit=float(EndlessHalf)-380.f;
    for(int Try=0;Try<6000&&Out.size()<72;++Try) {
        EndlessObstacle O;const int Kind=R.Range(10);
        O.X=float(R.Range(int(Limit*2)+1))-Limit;O.Y=float(R.Range(int(Limit*2)+1))-Limit;
        O.Yaw=float(R.Range(12))*15.f;
        if(Kind<4) {O.Length=O.Thickness=170.f+float(R.Range(4))*30.f;O.Height=EndlessWallHeight;}          // pillars
        else if(Kind<8) {O.Length=420.f+float(R.Range(5))*60.f;O.Thickness=130.f;O.Height=EndlessWallHeight;}   // walls
        else {O.Length=300.f;O.Thickness=300.f;O.Height=EndlessLowHeight;}                                    // low blocks (jumpable cover)
        const float Radius=.5f*std::hypot(O.Length,O.Thickness);bool Ok=std::hypot(O.X,O.Y)>=EndlessCentreClear+Radius;
        for(const auto& K:Keep) if(K.Active&&std::hypot(O.X-K.X,O.Y-K.Y)<EndlessKeepRadius+Radius) Ok=false;
        for(const auto& P:Out) if(Ok&&std::hypot(O.X-P.X,O.Y-P.Y)<Radius+.5f*std::hypot(P.Length,P.Thickness)+EndlessGap) Ok=false;
        if(Ok) Out.push_back(O);
    }
    return Out;
}
inline int NextCell(const Layout& L,int From,int To)
{
    if(From<0||To<0||From>=int(L.Cells.size())||To>=int(L.Cells.size())) return -1;
    if(From==To) return To;
    std::vector<int> Prev(L.Cells.size(),-1);Prev[From]=From;std::vector<int> Q{From};
    for(std::size_t k=0;k<Q.size();++k) for(auto E:L.Edges) if(!E.Gap) {
        int N=E.A==Q[k]?E.B:(E.B==Q[k]?E.A:-1);
        if(N>=0&&Prev[N]<0) {Prev[N]=Q[k];Q.push_back(N);}
    }
    if(Prev[To]<0) return -1;
    while(Prev[To]!=From && To!=From) To=Prev[To];
    return To;
}
// Shared grid search: four-way edges prevent cutting a capsule through prop corners.
struct NavGrid {
    int Width=0,Height=0;std::vector<std::uint8_t> Open,Blocked;
    bool Valid(int I) const {return Width>0&&Height>0&&Open.size()==std::size_t(Width)*std::size_t(Height)&&I>=0&&I<int(Open.size())&&Open[I]!=0;}
    int Direction(int A,int B) const {
        if(!Valid(A)||!Valid(B)) return -1;
        const int DX=B%Width-A%Width,DY=B/Width-A/Width;
        if(std::abs(DX)+std::abs(DY)!=1) return -1;
        return DX==1?0:(DX==-1?1:(DY==1?2:3));
    }
    bool CanStep(int A,int B) const {
        const int D=Direction(A,B);
        return D>=0&&(Blocked.empty()||(Blocked.size()==Open.size()&&!(Blocked[A]&(1<<D))));
    }
    void BlockEdge(int A,int B) {
        const int D=Direction(A,B);if(D<0) return;
        if(Blocked.size()!=Open.size()) Blocked.assign(Open.size(),0);
        Blocked[A]|=std::uint8_t(1<<D);Blocked[B]|=std::uint8_t(1<<Direction(B,A));
    }
    std::vector<int> Path(int Start,int Goal) const {
        if(!Valid(Start)||!Valid(Goal)||Width<=0||Height<=0) return {};
        std::vector<int> Parent(Open.size(),-1),Cost(Open.size(),0x3fffffff);
        using Entry=std::pair<int,int>;std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> Queue;
        auto H=[&](int I){return std::abs(I%Width-Goal%Width)+std::abs(I/Width-Goal/Width);};
        Cost[Start]=0;Parent[Start]=Start;Queue.push({H(Start),Start});
        while(!Queue.empty()) {
            const auto Item=Queue.top();Queue.pop();const int At=Item.second;
            if(Item.first!=Cost[At]+H(At)) continue;
            if(At==Goal) {std::vector<int> Result{At};while(Result.back()!=Start) Result.push_back(Parent[Result.back()]);std::reverse(Result.begin(),Result.end());return Result;}
            for(int N:{At-1,At+1,At-Width,At+Width}) {
                if(!CanStep(At,N)) continue;
                if(Cost[N]<=Cost[At]+1) continue;
                Cost[N]=Cost[At]+1;Parent[N]=At;Queue.push({Cost[N]+H(N),N});
            }
        }
        return {};
    }
    void KeepConnected(int Start) {
        if(!Valid(Start)) {std::fill(Open.begin(),Open.end(),0);return;}
        std::vector<std::uint8_t> Seen(Open.size(),0);std::vector<int> Q{Start};Seen[Start]=1;
        for(std::size_t K=0;K<Q.size();++K) for(int N:{Q[K]-1,Q[K]+1,Q[K]-Width,Q[K]+Width}) {
            if(CanStep(Q[K],N)&&!Seen[N]) {Seen[N]=1;Q.push_back(N);}
        }
        Open=std::move(Seen);
    }
};
inline float HealthBarAlpha(float SinceHit) {return !std::isfinite(SinceHit)?0.f:std::clamp((5.f-SinceHit)/2.f,0.f,1.f);}
inline bool CanAirDash(bool Grounded,bool Falling,int AirDashesUsed,int Capacity=1,int Available=1) {
    return Available>0&&(Grounded||(Falling&&AirDashesUsed<std::clamp(Capacity,1,3)));
}
// The integer phase values match EASPhase. Only Combat can grant proc benefits.
inline bool CanUseWeapons(int Phase) {return (Phase>=0&&Phase<=3)||Phase==5;}
inline bool CanGainCombatBenefits(int Phase) {return Phase==2;}
inline bool CanDamageTarget(int Phase,bool PracticeTarget,bool Cosmetic) {return !Cosmetic&&((Phase==0&&PracticeTarget)||(Phase==2&&!PracticeTarget));}
struct ElementState {
    float Burn=0,Poison=0,Frost=0,Frozen=0,Immune=0,BurnDPS=0,PoisonDPS=0,Bleed=0,BleedDPS=0,BleedDamage=0;
    int PoisonStacks=0,BleedStacks=0;
    void Apply(int Kind,float Power,bool BossTarget,float Duration=1.f) {
        Duration=std::clamp(Duration,1.f,1.8f);
        Power=std::max(0.f,Power);
        if(Kind==2) {Burn=3*Duration;BurnDPS=std::max(BurnDPS,6.f*Power);}
        if(Kind==3) {Poison=4*Duration;PoisonStacks=std::min(3,PoisonStacks+1);PoisonDPS=std::max(PoisonDPS,3.f*Power);}
        if(Kind==7) {Bleed=3*Duration;BleedStacks=std::min(2,BleedStacks+1);BleedDPS=std::max(BleedDPS,4.f*Power);}
        if(Kind==4) {Frost=std::min(100.f,Frost+34.f);if(Frost>=100&&Immune<=0) {Frozen=BossTarget?0.f:.8f;Immune=3.8f;Frost=70;}}
    }
    std::pair<float,float> Tick(float Delta) {
        BleedDamage=0;if(!std::isfinite(Delta)||Delta<=0) return {0,0};
        const float FireDamage=std::min(Delta,Burn)*BurnDPS,PoisonDamage=std::min(Delta,Poison)*PoisonDPS*PoisonStacks;
        BleedDamage=std::min(Delta,Bleed)*BleedDPS*BleedStacks;Bleed=std::max(0.f,Bleed-Delta);
        Burn=std::max(0.f,Burn-Delta);Poison=std::max(0.f,Poison-Delta);Frost=std::max(0.f,Frost-Delta*12.f);
        Frozen=std::max(0.f,Frozen-Delta);Immune=std::max(0.f,Immune-Delta);
        if(Bleed<=0) {BleedDPS=0;BleedStacks=0;}
        if(Burn<=0) BurnDPS=0;
        if(Poison<=0) {PoisonStacks=0;PoisonDPS=0;}
        return {FireDamage,PoisonDamage};
    }
};
inline int Ward(int Room) { return std::clamp(Room/RoomsPerWard,0,WardCount-1); }
inline bool Boss(int Room) { return Room>=0 && Room%RoomsPerWard==7; }
inline bool Elite(int Room) { return Room>=0 && Room%RoomsPerWard==3; }
inline bool Final(int Room) { return Room==RoomsPerWard*WardCount-1; }
inline int WaveCount(int Room) { return Boss(Room)?1:3; }
inline int WaveEnemies(int Room,int Wave,int Players) {
    int N=6+Ward(Room)*3+std::clamp(Wave,0,2)*3+(Players==2?4:0);
    if(Wave==2 && Ward(Room)==3) N*=3;
    return std::clamp(N,1,MaxEnemies);
}
inline bool Perfect(double Held,int Rank,bool Focus) {
    double Bonus=std::clamp(Rank,0,3)*0.025+(Focus?0.08:0.0);
    return std::isfinite(Held)&&Held>=0.70-Bonus&&Held<=0.92+Bonus;
}
inline int Cost(int Base,int Rank,int MaxRank) { return Rank>=0&&Rank<MaxRank?Base*(Rank+1):-1; }
inline bool Spend(int& Balance,int Price) { if(Price<0||Balance<Price) return false; Balance-=Price;return true; }
inline int Credit(int Balance,int Amount) { return int(std::clamp<std::int64_t>(std::int64_t(Balance)+std::max(Amount,0),0,MaxCurrency)); }
inline bool Refinement(int& Scrap,int& Bone,int& Ash,bool& Bought) {
    if(Bought||Scrap<15||Bone<8) return false;
    Scrap-=15;Bone-=8;Ash=Credit(Ash,40);Bought=true;return true;
}
inline bool Memory(int& Core,int& Bone,int& Points) {
    if(Core<3||Bone<12||Points>=MaxCurrency) return false;
    Core-=3;Bone-=12;++Points;return true;
}
}
