// Prints the enemy pose solver's output as JSON for Tools/preview_enemy_poses.py: for each preview kind and ability, the final figure-space transform (location, quaternion,
// scale) of every rig slot at a series of track times. It uses the very header the game compiles (ASEnemyPose.h), so the preview shows the numbers the game applies.
// Build and run through preview_enemy_poses.py, or by hand:  cl /std:c++17 /EHsc Tools\export_enemy_poses.cpp && export_enemy_poses.exe > poses.json
#include "../Source/AshenSpiral/Public/ASEnemyPose.h"
#include "../Source/AshenSpiral/Public/ASEnemyDeathTracks.h"
#include "../Source/AshenSpiral/Public/ASEnemyClipTracks.h"
#include <cstdio>
#include <string>
#include <vector>
using namespace ASEnemyPose;
namespace ER=ASEnemyRules;
struct Preview {int Kind;int Ability;bool Gun;float Size;};
// One representative kind per ability: the one that owns it (sizes: 0.9 ordinary, 1.45 elite, 2.15 boss, as ASEnemy::RefreshLook).
static const std::vector<Preview>& Previews() {
    static const std::vector<Preview> P={
        {0,ER::AbilitySlash,false,.9f},{3,ER::AbilitySlash,false,1.45f},{1,ER::AbilityShot,false,.9f},{6,ER::AbilityCharge,false,.9f},{7,ER::AbilityBlast,false,.9f},
        {8,ER::AbilitySnipe,true,.9f},{4,ER::AbilityBossAttack,false,2.15f},{5,ER::AbilityPounce,false,1.f},{9,ER::AbilitySummon,false,1.f},
        {0,ER::AbilityShove,false,.9f},{1,ER::AbilityBackHop,false,.9f},{2,ER::AbilityParry,false,.9f},{2,ER::AbilityRiposte,false,.9f},{3,ER::AbilityPound,false,1.45f},{3,ER::AbilityEnrage,false,1.45f},
        {5,ER::AbilityBlink,false,1.f},{5,ER::AbilityBlinkStrike,false,1.f},{7,ER::AbilityMeteors,false,.9f},
        // 100 + variant: the baked Mannequin death tracks (ASEnemyDeathTracks.h), played over U 0..1
        {0,100,false,.9f},{0,101,false,.9f},{0,102,false,.9f},{0,103,false,.9f},{0,104,false,.9f},{0,105,false,.9f},
    };
    return P;
}
static int WeaponFor(int Kind) {const bool Elite=Kind==3||Kind==4;return Kind==8?7:(Kind==1||Kind==7?4:(Elite||Kind==6?1:0));}
static void Print(const Xf& X) {std::printf("[%.4f,%.4f,%.4f,%.5f,%.5f,%.5f,%.5f,%.4f,%.4f,%.4f]",X.Loc.X,X.Loc.Y,X.Loc.Z,X.Rot.X,X.Rot.Y,X.Rot.Z,X.Rot.W,X.Scale.X,X.Scale.Y,X.Scale.Z);}
int main(int argc,char** argv) {
    const bool Baked=argc>1&&std::string(argv[1])=="baked"; // "baked" previews the Paragon clips (BakedTrack) whatever BakedClipsOn says; otherwise what the game plays (PlayedTrack)
    const float Times[]={0.f,.4f,.85f,1.f,1.12f,1.25f,1.45f,1.8f,2.2f,2.6f,3.f};
    std::printf("{\"samples\":[");bool First=true;
    // The neutral reference first (walking pose, no ability).
    std::vector<Preview> All={{0,ER::AbilityNone,false,.9f}};for(const auto& P:Previews()) All.push_back(P);
    for(const Preview& P:All) {
        const bool Death=P.Ability>=100;
        const Track& T=Death?DeathTrack(P.Ability-100):(Baked?BakedTrack(P.Ability):PlayedTrack(P.Ability));
        static const float DeathTimes[]={0.f,.12f,.25f,.4f,.55f,.7f,.85f,1.f};
        for(float U:(P.Ability==ER::AbilityNone?std::vector<float>{0.f}:(Death?std::vector<float>(DeathTimes,DeathTimes+8):std::vector<float>(Times,Times+sizeof(Times)/sizeof(Times[0]))))) {
            float D[ChannelCount];Evaluate(T,U,D);
            if(!First) std::printf(",");
            First=false;
            if(P.Kind==5||P.Kind==9) { // blob bodies: one mesh squashed, stretched and tipped by the same channels
                BlobOut B;SolveBlob(D,B);
                std::printf("{\"kind\":%d,\"ability\":%d,\"u\":%.3f,\"size\":%.2f,\"glow\":%.3f,\"parts\":[],\"blob\":",P.Kind,P.Ability,U,P.Size,B.Glow);
                Print(Xf{B.Offset,B.Rot,B.Scale});std::printf("}");
                continue;
            }
            RigInput In;In.Gun=P.Gun;In.Seconds=1.f;
            RigOut O;Solve(In,D,O);Xf World[SlotCount];ToFigureSpace(O,World);
            const Xf Fig=FigureXf(O,P.Size,0.f);
            std::printf("{\"kind\":%d,\"ability\":%d,\"u\":%.3f,\"size\":%.2f,\"weapon\":%d,\"shield\":%s,\"glow\":%.3f,\"parts\":[",P.Kind,P.Ability,U,P.Size,WeaponFor(P.Kind),P.Kind==6?"true":"false",O.Glow);
            for(int I=0;I<SlotCount;++I) {if(I) std::printf(",");Print(Compose(Fig,World[I]));}
            std::printf("],\"rig\":[");  // the same slots in figure space BEFORE the figure's own transform (for retargeting tools), then that transform
            for(int I=0;I<SlotCount;++I) {if(I) std::printf(",");Print(World[I]);}
            std::printf("],\"figure\":");Print(Fig);
            std::printf("}");
        }
    }
    std::printf("]}\n");
    return 0;
}
