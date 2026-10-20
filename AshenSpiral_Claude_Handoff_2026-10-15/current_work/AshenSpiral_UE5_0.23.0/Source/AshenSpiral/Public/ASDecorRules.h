#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// 0.25 phase C: the Endless arena's skin. A ring of monumental ruins, rocks, dead trees and bones stands round the fight square and shows the same three states as the sky: clear (waves 1-20),
// gloomy (21-49), red (50+). Pure rules, no Unreal types, tested as plain C++ (Tests/decor_025_tests.cpp). Everything here is TUNABLE. The game builds it from these placements on every machine.
// 0.24.2 (Josue: "make the walls further out so that we can explore the map"): the arena walls moved out beyond the ring (WallHalf), so the ring is ground to explore, and every piece is SOLID:
// a collision footprint authored here per kit (Body, Legs), built on every machine from the same placements whether or not the meshes are imported, so all machines collide alike.
// The placements use only add, multiply, divide and compare (no hypot), which IEEE 754 makes identical on every machine running this build.
// The meshes are the Dragon Cave stone kit, imported locally by Tools/import_vault_assets.py as SM_DC_<name>; the game skips any that are not there (ASAssets::HasMesh).
namespace ASDecor {

constexpr int StageCount=3;
constexpr float ArenaHalf=3100.f;                    // the fight square's half size (ASRules::EndlessCoverHalf), where the cover stands
constexpr float WallHalf=5600.f;                     // the arena walls (ASRules::EndlessHalf) since 0.24.2, beyond the ring
constexpr float RingInner=3450.f,RingOuter=4900.f;   // the decor stands between these distances from the centre, measured along the nearer axis, so the ring is a square band round the walls
constexpr float ScaleMin=.85f,ScaleMax=1.2f;

// One kind of scenery: the mesh, how tall it stands in the arena (cm, whatever the mesh's own size: the game scales it from its measured bounds), roughly how wide it is at that height, and
// the stage that first shows it. Weight is its share of that stage's picks.
// Body (0.24.2): the collision footprint's radius as a share of Radius (a dead tree collides at its trunk, a boulder nearly at its edge); Legs: an arch stands on two legs at either end.
struct Kit {const char* Mesh;float Height,Radius;int Stage,Weight;float Body=.75f;bool Legs=false;};
inline const std::array<Kit,32>& Kits() {
    static const std::array<Kit,32> Table={{
        // stage 0, always standing: the ruined stone of an old courtyard
        {"SM_DC_LargeRock01",900.f,300.f,0,2,0.80f,false},{"SM_DC_LargeRock02",600.f,230.f,0,2,0.80f,false},{"SM_DC_LargeRock03",550.f,200.f,0,1,0.80f,false},{"SM_DC_LargeRock04",800.f,280.f,0,2,0.80f,false},
        {"SM_DC_LargeRock05",600.f,220.f,0,1,0.80f,false},{"SM_DC_LargeRock06",600.f,220.f,0,1,0.80f,false},{"SM_DC_LargeRock07",800.f,260.f,0,2,0.80f,false},
        {"SM_DC_StoneworksPillar01",1300.f,200.f,0,2,0.70f,false},{"SM_DC_StoneworksPillar02",1300.f,210.f,0,2,0.70f,false},{"SM_DC_StoneworksPillar03",1000.f,200.f,0,1,0.70f,false},{"SM_DC_StoneworksPillar04",1500.f,220.f,0,1,0.70f,false},
        {"SM_DC_LargePillar01",1100.f,300.f,0,1,0.70f,false},{"SM_DC_LargePillar02",1500.f,300.f,0,1,0.70f,false},{"SM_DC_Column01",700.f,120.f,0,1,0.75f,false},{"SM_DC_Column02",700.f,120.f,0,1,0.75f,false},
        {"SM_DC_BridgeArc01",1300.f,380.f,0,1,0.22f,true},
        // stage 1, from wave 21: the land is dying
        {"SM_DC_DeadTree01",1100.f,300.f,1,3,0.18f,false},{"SM_DC_DeadTree02",800.f,260.f,1,2,0.20f,false},{"SM_DC_DeadTree03",700.f,260.f,1,2,0.20f,false},{"SM_DC_DeadTree04",600.f,200.f,1,1,0.22f,false},
        {"SM_DC_CaveRock01",600.f,330.f,1,2,0.75f,false},{"SM_DC_CaveRock02",550.f,260.f,1,1,0.75f,false},{"SM_DC_CaveRock03",550.f,280.f,1,1,0.75f,false},{"SM_DC_StoneWall01",500.f,230.f,1,1,0.60f,false},
        // stage 2, from wave 50: red, bloody gloom; the remains of the many who entered
        {"SM_DC_Skull01",520.f,260.f,2,3,0.60f,false},{"SM_DC_Skull02",480.f,240.f,2,2,0.60f,false},{"SM_DC_Skeleton04",520.f,200.f,2,2,0.45f,false},{"SM_DC_Skeleton05",420.f,220.f,2,2,0.45f,false},
        {"SM_DC_CoffinStone01",420.f,300.f,2,2,0.60f,false},{"SM_DC_InfernoRock01",700.f,260.f,2,2,0.75f,false},{"SM_DC_InfernoRock02",650.f,220.f,2,1,0.75f,false},{"SM_DC_InfernoRock04",600.f,200.f,2,1,0.75f,false},
    }};
    return Table;
}
constexpr std::array<int,StageCount> StageCounts={34,28,24}; // placements per stage (86 in all; the whole ring stays well under the 600-instance budget of the plan)

struct Placement {int Kit=0;float X=0.f,Y=0.f,Yaw=0.f,Scale=1.f;};

namespace Detail {
struct Rng {
    std::uint32_t S;
    explicit Rng(std::uint32_t Seed):S(Seed*2654435761u+977u) {}
    std::uint32_t Next() {S=S*1664525u+1013904223u;return S>>8;}
    float Unit() {return float(Next()&0xFFFFu)/65535.f;}
    float Range(float A,float B) {return A+(B-A)*Unit();}
};
}

// How many instances of each stage are visible: a stage shows from its own sky stage on (stage 2 shows everything).
inline bool StageVisible(int DecorStage,int SkyStage) {return DecorStage>=0&&DecorStage<StageCount&&SkyStage>=DecorStage;}

// The ring for one arena seed: every stage's placements, stage 0 first. Deterministic: the same seed gives the same list on every machine. Each placement keeps clear of the others by the
// sum of their radii (scaled), tries a bounded number of spots, and may come up short only if the ring is full, which the tests rule out for every seed they try.
inline std::vector<Placement> Ring(std::uint32_t Seed) {
    const auto& Table=Kits();
    std::vector<Placement> Out;Out.reserve(StageCounts[0]+StageCounts[1]+StageCounts[2]);
    Detail::Rng R(Seed);
    for(int Stage=0;Stage<StageCount;++Stage) {
        int Total=0;for(const auto& K:Table) if(K.Stage==Stage) Total+=K.Weight;
        for(int N=0;N<StageCounts[size_t(Stage)];++N) {
            for(int Try=0;Try<300;++Try) {
                int Pick=int(R.Next()%std::uint32_t(Total)),Index=0;
                for(int I=0;I<int(Table.size());++I) if(Table[size_t(I)].Stage==Stage) {Pick-=Table[size_t(I)].Weight;if(Pick<0) {Index=I;break;}}
                const float Scale=R.Range(ScaleMin,ScaleMax),Radius=Table[size_t(Index)].Radius*Scale;
                const float Depth=R.Range(RingInner+Radius*.4f,RingOuter),Along=R.Range(-RingOuter,RingOuter);
                float X,Y;
                switch(R.Next()&3u) {case 0: X=Depth;Y=Along;break;case 1: X=-Depth;Y=Along;break;case 2: X=Along;Y=Depth;break;default: X=Along;Y=-Depth;break;}
                bool Free=true;
                for(const auto& P:Out) {const float Dx=P.X-X,Dy=P.Y-Y,Need=(Table[size_t(P.Kit)].Radius*P.Scale+Radius)*.6f;if(Dx*Dx+Dy*Dy<Need*Need) {Free=false;break;}} // squared: no hypot (see above)
                if(!Free) continue;
                Placement P;P.Kit=Index;P.X=X;P.Y=Y;P.Yaw=R.Range(0.f,360.f);P.Scale=Scale;Out.push_back(P);break;
            }
        }
    }
    return Out;
}

// The solid footprint of one placed piece: one or two upright cylinders (offsets along the piece's own X axis, before its yaw) of this radius and height. Built on every machine.
struct Solid {int Count=1;float Radius=0.f,Height=0.f,Offset=0.f;}; // two legs sit at +-Offset along the piece's X axis
constexpr float SolidHeight=600.f; // tall enough that nobody jumps onto a piece
inline Solid SolidFor(const Kit& K,float Scale) {
    Solid S;const float Sc=std::isfinite(Scale)?Scale:1.f,R=K.Radius*Sc;
    S.Height=std::min(SolidHeight,K.Height*Sc);
    if(K.Legs) {S.Count=2;S.Radius=R*K.Body;S.Offset=R*.72f;} else {S.Count=1;S.Radius=R*K.Body;S.Offset=0.f;}
    return S;
}

} // namespace ASDecor
