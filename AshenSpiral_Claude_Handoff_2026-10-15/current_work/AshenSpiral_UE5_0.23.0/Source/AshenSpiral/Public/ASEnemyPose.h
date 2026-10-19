#pragma once
#include <algorithm>
#include <cmath>
#include "ASAnimationRules.h"
#include "ASEnemyRules.h"
// Enemy poses (0.24, Tier 0 animation: no new assets). The 21-part armour rig is posed from DATA: every ability owns a track of per-channel curves (anticipation, strike,
// recovery) laid over the gait, and a pure solver turns gait + track into the transform of every part. Nothing here touches Unreal types, so Tests/enemy_pose_024_tests.cpp
// exercises it as plain C++, and Tools/export_enemy_poses.cpp + preview_enemy_poses.py draw the very same numbers from the real OBJ meshes (a contact sheet per ability).
// The math below is Unreal's own (FRotator::Quaternion, FQuat::RotateVector / FindBetweenNormals, left-handed: X forward, Y right, Z up); sources are cited where it matters.
namespace ASEnemyPose {
constexpr float DegToRad=0.017453292519943295f,RadToDeg=57.29577951308232f;

// ---- Small vector / quaternion kit (single precision, Unreal's conventions) ----
struct V3 {float X=0.f,Y=0.f,Z=0.f;};
inline V3 operator+(V3 A,V3 B) {return {A.X+B.X,A.Y+B.Y,A.Z+B.Z};}
inline V3 operator-(V3 A,V3 B) {return {A.X-B.X,A.Y-B.Y,A.Z-B.Z};}
inline V3 operator*(V3 A,float S) {return {A.X*S,A.Y*S,A.Z*S};}
inline float Dot(V3 A,V3 B) {return A.X*B.X+A.Y*B.Y+A.Z*B.Z;}
inline V3 Cross(V3 A,V3 B) {return {A.Y*B.Z-A.Z*B.Y,A.Z*B.X-A.X*B.Z,A.X*B.Y-A.Y*B.X};}
inline float Length(V3 A) {return std::sqrt(Dot(A,A));}
inline V3 Normalize(V3 A) {const float L=Length(A);return L>1e-6f&&std::isfinite(L)?A*(1.f/L):V3{};}  // FVector::GetSafeNormal: zero stays zero
inline V3 Lerp(V3 A,V3 B,float T) {return A+(B-A)*T;}
struct Q4 {float X=0.f,Y=0.f,Z=0.f,W=1.f;};
// FRotator(Pitch, Yaw, Roll).Quaternion(): UnrealMath.cpp, FRotator3d::Quaternion.
inline Q4 FromRotator(float PitchDeg,float YawDeg,float RollDeg) {
    const float P=std::fmod(PitchDeg,360.f)*DegToRad*.5f,Y=std::fmod(YawDeg,360.f)*DegToRad*.5f,R=std::fmod(RollDeg,360.f)*DegToRad*.5f;
    const float SP=std::sin(P),CP=std::cos(P),SY=std::sin(Y),CY=std::cos(Y),SR=std::sin(R),CR=std::cos(R);
    return {CR*SP*SY-SR*CP*CY,-CR*SP*CY-SR*CP*SY,CR*CP*SY-SR*SP*CY,CR*CP*CY+SR*SP*SY};
}
// FQuat::RotateVector: V' = V + w*T + Q x T with T = 2 (Q x V).
inline V3 Rotate(const Q4& Q,V3 V) {const V3 Qv{Q.X,Q.Y,Q.Z},T=Cross(Qv,V)*2.f;return V+T*Q.W+Cross(Qv,T);}
// A * B in Unreal's convention: applies B first, then A.
inline Q4 Mul(const Q4& A,const Q4& B) {
    return {A.W*B.X+A.X*B.W+A.Y*B.Z-A.Z*B.Y,A.W*B.Y-A.X*B.Z+A.Y*B.W+A.Z*B.X,A.W*B.Z+A.X*B.Y-A.Y*B.X+A.Z*B.W,A.W*B.W-A.X*B.X-A.Y*B.Y-A.Z*B.Z};
}
inline Q4 Normalized(Q4 Q) {const float L=std::sqrt(Q.X*Q.X+Q.Y*Q.Y+Q.Z*Q.Z+Q.W*Q.W);return L>1e-9f?Q4{Q.X/L,Q.Y/L,Q.Z/L,Q.W/L}:Q4{};}
// FQuat::FindBetweenNormals (UnrealMath.cpp FindBetween_Helper) for unit vectors.
inline Q4 Between(V3 A,V3 B) {
    float W=1.f+Dot(A,B);
    if(W>=1e-6f) {const V3 C=Cross(A,B);return Normalized({C.X,C.Y,C.Z,W});}
    const bool UseY=std::fabs(A.X)>std::fabs(A.Y)&&std::fabs(A.X)>std::fabs(A.Z);
    const V3 Basis=UseY?V3{0.f,1.f,0.f}:V3{-1.f,0.f,0.f},C=Cross(A,Basis);
    return Normalized({C.X,C.Y,C.Z,0.f});
}
// A transform: scale first, then rotation, then location (Unreal's FTransform for components without shear).
struct Xf {V3 Loc;Q4 Rot;V3 Scale{1.f,1.f,1.f};};
inline V3 Apply(const Xf& T,V3 P) {return T.Loc+Rotate(T.Rot,V3{P.X*T.Scale.X,P.Y*T.Scale.Y,P.Z*T.Scale.Z});}
// The transform of a child expressed relative to Parent, as a transform in Parent's own parent space.
inline Xf Compose(const Xf& Parent,const Xf& Child) {
    return {Apply(Parent,Child.Loc),Mul(Parent.Rot,Child.Rot),V3{Parent.Scale.X*Child.Scale.X,Parent.Scale.Y*Child.Scale.Y,Parent.Scale.Z*Child.Scale.Z}};
}

// ---- Rig slots: the 21 armour parts plus the weapon and the shield ----
constexpr int Torso=0,Head=1,Pelvis=2,UpperArmL=3,ForearmL=4,UpperArmR=5,ForearmR=6,ThighL=7,GreaveL=8,ThighR=9,GreaveR=10,BootL=11,BootR=12,PauldronL=13,PauldronR=14,
    Cape=15,Visor=16,HandL=17,HandR=18,CapeLower=19,Core=20,Weapon=21,Shield=22,SlotCount=23;
// Parent slot of each slot; -1 = the figure itself. Parents always have lower indices.
constexpr int ParentOf[SlotCount]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,Head,-1,-1,Cape,Torso,HandR,HandL};

// ---- Channels: every degree of freedom a track can drive. Values are DELTAS from the neutral pose (0 = neutral) ----
enum Channel : int {
    ChCrouch,ChLeanX,ChLeanY,                          // upper body drops / shifts forward / shifts right (cm); the feet stay, the legs bend
    ChBend,ChBendRoll,                                 // the whole upper body hinges about the hips (degrees): pitch (positive leans back) and roll (positive leans right)
    ChFigPitch,ChFigYaw,ChFigRoll,                     // whole-figure tilt about the feet (degrees)
    ChScaleX,ChScaleY,ChScaleZ,                        // whole-figure scale added to 1 (squash and stretch)
    ChTorsoPitch,ChTorsoYaw,ChTorsoRoll,               // degrees; positive pitch leans back, positive yaw turns right
    ChHeadPitch,ChHeadYaw,
    ChHandLX,ChHandLY,ChHandLZ,ChHandRX,ChHandRY,ChHandRZ, // hand targets moved from the walking pose (cm, figure space; the arm's IK reaches as far as it can)
    ChWeaponPitch,ChWeaponYaw,ChWeaponRoll,            // degrees; for an upright weapon positive pitch tips the tip backward
    ChCapePitch,                                       // degrees added to the cloth-driven drape
    ChPlant,ChFootLX,ChFootRX,ChStance,                // 0..1 blend from the gait to a planted stance; each planted foot's forward offset; extra half-width (cm)
    ChGlow,                                            // visor / core emission boost
    ChannelCount
};
// ---- Tracks: per-channel curves over track time U in [0,3]: [0,1) the wind-up, [1,2) the strike, [2,3) the recovery ----
struct Key {float T,V;};
constexpr int MaxKeys=12,MaxCurves=30;
struct Curve {int Channel;Key Keys[MaxKeys];};  // Channel < 0 ends the list; a key with T < 0 ends the curve
struct Track {Curve Curves[MaxCurves];};
constexpr float TrackEnd=3.f;

// Smoothstep between keys: every key is a brief rest, so a pose reads as a pose.
inline float EvalCurve(const Curve& C,float U) {
    int N=0;while(N<MaxKeys&&C.Keys[N].T>=0.f) ++N;
    if(N==0||!std::isfinite(U)) return 0.f;
    if(U<=C.Keys[0].T) return C.Keys[0].V;
    for(int I=1;I<N;++I) if(U<=C.Keys[I].T) {
        const float Span=C.Keys[I].T-C.Keys[I-1].T,A=Span>1e-6f?(U-C.Keys[I-1].T)/Span:1.f,S=A*A*(3.f-2.f*A);
        return C.Keys[I-1].V+(C.Keys[I].V-C.Keys[I-1].V)*S;
    }
    return C.Keys[N-1].V;
}
inline void ClearChannels(float* Out) {for(int I=0;I<ChannelCount;++I) Out[I]=0.f;}
// Out[] receives every channel's value at U (0 for channels the track does not drive).
inline void Evaluate(const Track& T,float U,float* Out) {
    ClearChannels(Out);
    for(int I=0;I<MaxCurves&&T.Curves[I].Channel>=0;++I) if(T.Curves[I].Channel<ChannelCount) Out[T.Curves[I].Channel]=EvalCurve(T.Curves[I],U);
}

// Track time from the replicated timing fields. While the telegraph is up U runs 0..1 over the wind-up; after the release it runs 1..2 over the strike (the ability's committed
// motion, or StrikeSeconds for an instant one) and 2..3 over the recovery. An instant ability's strike happens inside its Recover state, so its pose recovery is what is left of
// it (at least MinRecoverySeconds); a committed one recovers for its whole Recover. Either way the pose ends as the walker starts moving. -1 = no pose is playing.
constexpr float StrikeSeconds=.16f,MinRecoverySeconds=.2f;
inline float PoseRecoverySeconds(const ASEnemyRules::AbilitySpec& Spec) {
    return Spec.Attack>0.f?std::max(Spec.Recover,MinRecoverySeconds):std::max(Spec.Recover-StrikeSeconds,MinRecoverySeconds);
}
inline float AbilityU(const ASEnemyRules::AbilitySpec& Spec,bool Telegraphing,float Now,float WindupStart,float WindupEnd,float ReleaseTime) {
    if(!std::isfinite(Now)||!std::isfinite(WindupStart)||!std::isfinite(WindupEnd)||!std::isfinite(ReleaseTime)) return -1.f;
    if(Telegraphing) return std::clamp((Now-WindupStart)/std::max(.01f,WindupEnd-WindupStart),0.f,1.f);
    const float T=Now-ReleaseTime;
    if(!(T>=0.f)) return -1.f;
    const float Strike=Spec.Attack>0.f?Spec.Attack:StrikeSeconds,Recover=PoseRecoverySeconds(Spec);
    if(T<Strike) return 1.f+T/Strike;
    const float R=(T-Strike)/Recover;
    return R<1.f?2.f+R:-1.f;
}

// ---- The solver: gait + track deltas -> a transform for every slot ----
struct RigInput {
    float GaitPhase=0.f,Weight=0.f,Step=0.f;      // foot phase (radians), 0..1 how fast it is walking, sin(phase)*Weight (arm swing)
    float HeadingX=1.f,HeadingY=0.f;              // unit direction of travel in the enemy's own frame (forward = 1,0)
    bool Gun=false;                               // two-handed rifle hold (the sniper)
    float Release=0.f,Recoil=0.f,PushX=0.f,PushY=0.f; // ASAnimation::ReleaseGesture and Reaction, and the local direction of the hit
    float Seconds=0.f,Speed=0.f,Turn=0.f,Flare=0.f;   // the clock and the cloth drive (see ASAnimation::CapePoint)
};
struct RigOut {
    Xf Part[SlotCount];                           // each slot relative to its parent (ParentOf)
    V3 FigureOffset;                              // x, y of the whole figure (hit recoil); z stays with the capsule
    Q4 FigureRot;                                 // whole-figure tilt about the feet
    V3 FigureScale{1.f,1.f,1.f};                  // squash and stretch, multiplies the figure's own scale
    float Glow=0.f;
};
// FQuat::FindBetweenNormals(Up, From - To): the mesh's +Z (its upper end) points at From, its centre sits halfway, and it stretches to the segment's length.
inline void Segment(Xf& Part,V3 From,V3 To,float MeshLength) {
    const V3 V=From-To;
    Part.Loc=(From+To)*.5f;Part.Rot=Between(V3{0.f,0.f,1.f},Normalize(V));Part.Scale=V3{1.f,1.f,Length(V)/MeshLength};
}
// ASPose::Limb: a two-bone limb from Root toward Target, bending toward Pole. Returns where the end really is (the reach clamps).
inline V3 Limb(Xf& Upper,Xf& Lower,V3 Root,V3 Target,V3 Pole,float A=42.f,float B=42.f,float MeshA=42.f,float MeshB=42.f) {
    V3 Axis=Normalize(Target-Root);if(Length(Axis)<.5f) Axis=V3{0.f,0.f,-1.f};
    const auto Bend=ASAnimation::TwoBone(Length(Target-Root),A,B);
    V3 Side=Pole-Root;Side=Normalize(Side-Axis*Dot(Side,Axis));if(Length(Side)<.5f) Side=V3{0.f,1.f,0.f};
    const V3 Joint=Root+Axis*Bend.Along+Side*Bend.Out,End=Root+Axis*Bend.Reach;
    Segment(Upper,Root,Joint,MeshA);Segment(Lower,Joint,End,MeshB);return End;
}
constexpr V3 RestTorso{0.f,0.f,111.f},RestHead{0.f,0.f,165.f},RestPelvis{0.f,0.f,78.f},RestCape{-22.f,0.f,141.f};
inline void Solve(const RigInput& In,const float* D,RigOut& Out) {
    const float W=std::clamp(In.Weight,0.f,1.f),Step=In.Step,Recoil=In.Recoil;
    const V3 Body{D[ChLeanX],D[ChLeanY],-D[ChCrouch]},Hips{Body.X*.5f,Body.Y*.5f,-D[ChCrouch]};
    // The upper body hinges about the hips: a rest point is shifted by the lean and crouch, then carried round the (moved) hip line by the bend.
    const float Bend=D[ChBend];
    const V3 Pivot=V3{0.f,0.f,79.f}+Hips;
    const Q4 BendRot=FromRotator(Bend,0.f,D[ChBendRoll]);
    auto Carry=[&](V3 Rest) {return Pivot+Rotate(BendRot,(Rest+Body)-Pivot);};
    // Torso and head (the legacy windup lean now comes from tracks; the release gesture and the hit recoil are added as before).
    Out.Part[Torso]={Carry(RestTorso),FromRotator(-5.f-W*4.f+D[ChTorsoPitch]+Bend+In.Release*14.f-In.PushX*Recoil*18.f,Step*3.f+D[ChTorsoYaw],D[ChTorsoRoll]+D[ChBendRoll]+In.PushY*Recoil*12.f)};
    Out.Part[Head]={Carry(RestHead),FromRotator(5.f+D[ChHeadPitch]+Bend-Recoil*8.f,-Step*2.f+D[ChHeadYaw],D[ChBendRoll]),V3{.95f,1.f,.85f}};
    Out.Part[Pelvis]={RestPelvis+Hips,Q4{}};
    Out.Part[PauldronL]={Carry(V3{0.f,-34.f,135.f}),Q4{}};Out.Part[PauldronR]={Carry(V3{0.f,34.f,135.f}),Q4{}};
    Out.Part[Visor]={V3{},Q4{}};Out.Part[Core]={V3{},Q4{}}; // children of the head and torso, at their origins
    // Cape: the player's cloth centreline gives the drape (root to tip), so the enemy's cape streams, ripples and flares with speed the same way.
    const auto Root=ASAnimation::CapePoint(0,In.Seconds,W,In.Turn,In.Flare),Tip=ASAnimation::CapePoint(6,In.Seconds,W,In.Turn,In.Flare);
    const float Drape=std::atan2(Root.X-Tip.X,std::max(1.f,Root.Z-Tip.Z))*RadToDeg;
    Out.Part[Cape]={Carry(RestCape),FromRotator(-Drape+D[ChCapePitch]+Bend,Step*2.f,D[ChBendRoll])};
    Out.Part[CapeLower]={V3{-3.f,0.f,-55.f},FromRotator(-W*8.f+std::sin(In.Seconds*3.f)*3.f,0.f,Step*2.f)};
    // Legs: hips drop with the crouch while the feet stay; the gait's foot swing follows the direction of travel; a planted stance blends in over it.
    const float Plant=std::clamp(D[ChPlant],0.f,1.f);
    for(int Side=0;Side<2;++Side) {
        const float Sign=Side==0?-1.f:1.f;
        const auto F=ASAnimation::Foot(In.GaitPhase+float(Side)*ASAnimation::Pi,W);
        const V3 Gait{6.f+F.Forward*In.HeadingX,Sign*16.f+F.Forward*In.HeadingY,12.f+F.Lift};
        const V3 Planted{6.f+(Side==0?D[ChFootLX]:D[ChFootRX]),Sign*(16.f+D[ChStance]),12.f};
        const V3 Foot=Lerp(Gait,Planted,Plant);
        const V3 End=Limb(Out.Part[Side==0?ThighL:ThighR],Out.Part[Side==0?GreaveL:GreaveR],V3{Hips.X,Sign*16.f+Hips.Y,79.f+Hips.Z},Foot,V3{70.f,Sign*20.f,45.f},38.f,38.f,38.f,38.f);
        Out.Part[Side==0?BootL:BootR]={End+V3{3.f,0.f,-2.f},FromRotator(F.Pitch*(1.f-Plant),0.f,0.f)};
    }
    // Arms: from the (carried) shoulders to the hand targets: the walking swing or the sniper's rifle hold, as an offset from the shoulder, plus the track, bending toward an
    // elbow pole behind and outside. The offsets are the old absolute hand positions minus the shoulders, so nothing changes until the body bends.
    for(int Side=0;Side<2;++Side) {
        const float Sign=Side==0?-1.f:1.f;
        const V3 Shoulder=Carry(V3{0.f,Sign*34.f,133.f});
        const V3 Offset=In.Gun?(Side==0?V3{76.f,29.f,0.f}:V3{54.f,-9.f,-14.f}):V3{24.f+(Side==0?-Step:Step)*12.f,Sign*11.f,-59.f};
        const V3 Hand=Shoulder+Offset+(Side==0?V3{D[ChHandLX],D[ChHandLY],D[ChHandLZ]}:V3{D[ChHandRX],D[ChHandRY],D[ChHandRZ]});
        const V3 End=Limb(Out.Part[Side==0?UpperArmL:UpperArmR],Out.Part[Side==0?ForearmL:ForearmR],Shoulder,Hand,Carry(V3{-8.f,Sign*75.f,92.f}));
        Out.Part[Side==0?HandL:HandR]={End,Q4{}};
    }
    Out.Part[Weapon]={V3{},FromRotator(D[ChWeaponPitch],D[ChWeaponYaw],D[ChWeaponRoll])};
    Out.Part[Shield]={V3{14.f,0.f,15.f},Q4{}};
    Out.FigureOffset=V3{In.PushX*Recoil*12.f+In.Release*6.f,In.PushY*Recoil*12.f,0.f};
    Out.FigureRot=FromRotator(D[ChFigPitch],D[ChFigYaw],D[ChFigRoll]);
    Out.FigureScale=V3{std::max(.2f,1.f+D[ChScaleX]),std::max(.2f,1.f+D[ChScaleY]),std::max(.2f,1.f+D[ChScaleZ])};
    Out.Glow=D[ChGlow];
}
// Every slot's transform in figure space (parents composed in), for drawing and for tests.
inline void ToFigureSpace(const RigOut& R,Xf* World) {
    for(int I=0;I<SlotCount;++I) World[I]=ParentOf[I]<0?R.Part[I]:Compose(World[ParentOf[I]],R.Part[I]);
}
// The figure's own transform on the capsule: the humanoid rig is drawn narrower than the base meshes (.82 x .70) and scaled by the enemy's size.
inline Xf FigureXf(const RigOut& R,float Size,float HalfHeight) {
    return {V3{R.FigureOffset.X,R.FigureOffset.Y,-HalfHeight},R.FigureRot,V3{.82f*Size*R.FigureScale.X,.70f*Size*R.FigureScale.Y,Size*R.FigureScale.Z}};
}
// Blob bodies (the leaper and the broodcaller have no armour rig): the same channels squash, stretch and tip the single body.
struct BlobOut {Q4 Rot;V3 Scale{1.f,1.f,1.f};V3 Offset;float Glow=0.f;};
inline void SolveBlob(const float* D,BlobOut& Out) {
    Out.Rot=FromRotator(D[ChFigPitch],D[ChFigYaw],D[ChFigRoll]);
    Out.Scale=V3{std::max(.2f,1.f+D[ChScaleX]),std::max(.2f,1.f+D[ChScaleY]),std::max(.2f,1.f+D[ChScaleZ])};
    Out.Offset=V3{D[ChLeanX],D[ChLeanY],-D[ChCrouch]};Out.Glow=D[ChGlow];
}

// ---- Deaths (0.24): which baked Mannequin collapse plays (ASEnemyDeathTracks.h, generated by Tools/spike_manny_bake.py) ----
// By the direction the killing blow pushed the enemy, in its own frame (PushX forward, PushY right): from behind (pushed forward) it slumps forward ("back"), from the left or the
// right it goes down to the other side, from the front it doubles over (one of three, chosen by its id so a crowd of deaths varies).
constexpr int DeathFronts=3,DeathBack=3,DeathLeft=4,DeathRight=5,DeathVariantCount=6;
inline int DeathVariantFor(float PushX,float PushY,unsigned Id) {
    if(!std::isfinite(PushX)||!std::isfinite(PushY)) return static_cast<int>(Id%static_cast<unsigned>(DeathFronts));
    if(std::fabs(PushY)>std::fabs(PushX)*1.2f) return PushY>0.f?DeathLeft:DeathRight;
    if(PushX>0.f) return DeathBack;
    return static_cast<int>(Id%static_cast<unsigned>(DeathFronts));
}

// ---- The tracks (TUNABLE). One per ability id; see ASEnemyRules::Ability*. ----
// Hand deltas are from the walking hand position (24, +-45, 74); the shoulder sits at (0, +-34, 133) and an arm reaches about 84 cm, so the right hand overhead at
// (-12, 53, 196) is 66 cm from its shoulder. A positive WeaponPitch swings an upright weapon's tip backward, a negative one forward.
// The abilities that have a track of their own are 0-19; the elite mutations' signature abilities share a body language with one of them (TrackIndex).
constexpr int OwnTracks=20;
inline int TrackIndex(int Ability) {
    const int Id=std::clamp(Ability,0,ASEnemyRules::AbilityCount-1);
    switch(Id) {
    case ASEnemyRules::AbilityGlacial: return ASEnemyRules::AbilityPound;   // the frostbound elite's slam is a ground pound
    case ASEnemyRules::AbilityCinders: return ASEnemyRules::AbilitySlash;   // the burning elite sweeps its fire out with a swing
    case ASEnemyRules::AbilityStorm:   return ASEnemyRules::AbilityMeteors; // the shocking elite calls lightning down with its weapon raised to the sky
    case ASEnemyRules::AbilitySeeds:   return ASEnemyRules::AbilityBlast;   // the venomous elite lobs its pods with a heave
    case ASEnemyRules::AbilityBossSlam:   return ASEnemyRules::AbilityPound;      // a ground slam
    case ASEnemyRules::AbilityBossMarks:  return ASEnemyRules::AbilityBossAttack; // arms flung wide, the weapon overhead, a two-handed slam
    case ASEnemyRules::AbilityBossRing:   return ASEnemyRules::AbilityBossAttack;
    case ASEnemyRules::AbilityBossCharge: return ASEnemyRules::AbilityCharge;     // head down, weapon back, a hard lean into the run
    case ASEnemyRules::AbilityBossToll:   return ASEnemyRules::AbilityPound;
    case ASEnemyRules::AbilityBossSeeds:  return ASEnemyRules::AbilityBlast;      // a heave forward with the free hand pointing at the target
    case ASEnemyRules::AbilityBossBloom:  return ASEnemyRules::AbilityMeteors;    // the weapon raised to call it up
    case ASEnemyRules::AbilityBossRoots:  return ASEnemyRules::AbilityPound;      // a stamp
    case ASEnemyRules::AbilityBossGrasp:  return ASEnemyRules::AbilityEnrage;     // arms flung wide, then clenched
    case ASEnemyRules::AbilityBossPhase:  return ASEnemyRules::AbilityEnrage;     // the roar
    default: return Id<OwnTracks?Id:ASEnemyRules::AbilityNone;
    }
}
inline const Track& TrackFor(int Ability) {
    static constexpr Track Table[OwnTracks]={
        {{{-1,{}}}}, // 0 none
        {{{-1,{}}}}, // 1 dive: the fliers pose their own wings
        {{{-1,{}}}}, // 2 molt: ditto
        // 3 slash: coil back, rise onto the back foot, sword overhead and behind; swing down across the body, step in; settle
        {{
            {ChPlant,       {{0.f,0.f},{.3f,1.f},{2.3f,1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,0.f},{.4f,10.f},{1.4f,26.f},{2.4f,26.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,0.f},{.4f,-18.f},{1.4f,-12.f},{2.4f,-12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,0.f},{.4f,10.f},{2.4f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.85f,5.f},{1.f,5.f},{1.4f,10.f},{2.2f,7.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.85f,-8.f},{1.f,-8.f},{1.4f,20.f},{2.2f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,0.f},{.85f,12.f},{1.f,12.f},{1.4f,-16.f},{2.2f,-6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoYaw,    {{0.f,0.f},{.85f,28.f},{1.f,28.f},{1.4f,-34.f},{2.2f,-20.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadPitch,   {{0.f,0.f},{.85f,-8.f},{1.f,-8.f},{1.4f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRX,      {{0.f,0.f},{.85f,-36.f},{1.f,-36.f},{1.4f,38.f},{2.2f,30.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRY,      {{0.f,0.f},{.85f,8.f},{1.f,8.f},{1.4f,-52.f},{2.2f,-40.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRZ,      {{0.f,0.f},{.85f,122.f},{1.f,122.f},{1.4f,20.f},{2.2f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLX,      {{0.f,0.f},{.85f,12.f},{1.f,12.f},{1.4f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,0.f},{.85f,44.f},{1.f,44.f},{1.4f,18.f},{2.2f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,0.f},{.85f,62.f},{1.f,62.f},{1.4f,-52.f},{2.2f,-30.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{1.f,1.f},{1.4f,.4f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 4 shot (the archer's staff-lance): present the right side, raise the staff to point at the target with a steadying hand at the chest; the shot kicks the staff up
        {{
            {ChPlant,       {{0.f,0.f},{.3f,1.f},{2.4f,1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,0.f},{.4f,14.f},{2.4f,14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,0.f},{.4f,-16.f},{2.4f,-16.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,0.f},{.4f,12.f},{2.4f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.7f,3.f},{1.f,3.f},{1.3f,2.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoYaw,    {{0.f,0.f},{.7f,16.f},{1.f,18.f},{2.4f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,0.f},{.8f,-2.f},{1.f,-2.f},{1.08f,6.f},{1.8f,2.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadYaw,     {{0.f,0.f},{.7f,-14.f},{1.f,-16.f},{2.4f,-6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.8f,2.f},{1.f,2.f},{1.08f,-7.f},{1.8f,-2.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRX,      {{0.f,0.f},{.7f,32.f},{1.f,32.f},{1.08f,18.f},{1.8f,26.f},{2.6f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRY,      {{0.f,0.f},{.7f,-9.f},{1.f,-9.f},{2.f,-6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRZ,      {{0.f,0.f},{.7f,66.f},{1.f,66.f},{1.08f,72.f},{1.8f,60.f},{2.6f,18.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,0.f},{.7f,-76.f},{1.f,-78.f},{1.08f,-66.f},{1.5f,-72.f},{2.4f,-30.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLX,      {{0.f,0.f},{.7f,16.f},{1.f,16.f},{2.f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLY,      {{0.f,0.f},{.7f,9.f},{1.f,9.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,0.f},{.7f,38.f},{1.f,38.f},{1.08f,44.f},{2.f,28.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{1.f,1.2f},{1.08f,2.5f},{1.4f,.3f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 5 pounce (the leaper's blob body): squash down and coil toward the target; stretch out into the leap, nose up then down; squash on landing
        {{
            {ChScaleZ,      {{0.f,0.f},{.8f,-.30f},{1.f,-.32f},{1.15f,.16f},{1.6f,.12f},{2.f,0.f},{2.15f,-.24f},{2.6f,-.08f},{3.f,0.f},{-1.f,0.f}}},
            {ChScaleX,      {{0.f,0.f},{.8f,.16f},{1.f,.17f},{1.15f,-.08f},{1.6f,-.06f},{2.f,0.f},{2.15f,.14f},{2.6f,.05f},{3.f,0.f},{-1.f,0.f}}},
            {ChScaleY,      {{0.f,0.f},{.8f,.16f},{1.f,.17f},{1.15f,-.08f},{1.6f,-.06f},{2.f,0.f},{2.15f,.14f},{2.6f,.05f},{3.f,0.f},{-1.f,0.f}}},
            {ChFigPitch,    {{0.f,0.f},{.8f,-14.f},{1.f,-16.f},{1.2f,16.f},{1.6f,2.f},{1.9f,-20.f},{2.15f,-6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.8f,-10.f},{1.f,-10.f},{1.2f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{1.f,1.5f},{1.3f,.6f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 6 charge (the bulwark): shield out front, head down, maul cocked back; a hard lean into the run; a panting recovery
        {{
            {ChPlant,       {{0.f,0.f},{.35f,1.f},{1.f,1.f},{1.15f,0.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,0.f},{.4f,16.f},{1.f,16.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,0.f},{.4f,-20.f},{1.f,-20.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,0.f},{.4f,12.f},{1.f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.8f,12.f},{1.f,12.f},{1.4f,9.f},{2.f,9.f},{2.5f,5.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFigPitch,    {{0.f,0.f},{.8f,-8.f},{1.f,-9.f},{1.3f,-14.f},{2.f,-12.f},{2.5f,-5.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.8f,8.f},{1.f,8.f},{1.3f,14.f},{2.f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadPitch,   {{0.f,0.f},{.8f,-12.f},{1.3f,-16.f},{2.f,-14.f},{2.6f,-6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,0.f},{.8f,-6.f},{1.3f,-10.f},{2.f,-8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLX,      {{0.f,0.f},{.8f,38.f},{1.3f,46.f},{2.f,40.f},{2.6f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLY,      {{0.f,0.f},{.8f,17.f},{1.3f,22.f},{2.f,18.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,0.f},{.8f,26.f},{1.3f,34.f},{2.f,28.f},{2.6f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRX,      {{0.f,0.f},{.8f,-32.f},{1.3f,-36.f},{2.f,-30.f},{2.6f,-6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRZ,      {{0.f,0.f},{.8f,76.f},{1.3f,72.f},{2.f,60.f},{2.6f,14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,0.f},{.8f,45.f},{1.3f,55.f},{2.f,45.f},{2.6f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{1.f,.8f},{1.3f,1.4f},{2.f,.6f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 7 blast (the shaman): staff overhead, the free hand pointing at the target, head back; the staff thrust forward and down; the hand pulls in
        {{
            {ChPlant,       {{0.f,0.f},{.3f,1.f},{2.4f,1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,0.f},{.4f,12.f},{2.4f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,0.f},{.4f,-14.f},{2.4f,-14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,0.f},{.4f,8.f},{2.4f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.85f,-3.f},{1.f,-3.f},{1.3f,6.f},{2.2f,4.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.85f,-6.f},{1.f,-6.f},{1.3f,14.f},{2.2f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,0.f},{.85f,10.f},{1.f,10.f},{1.3f,-12.f},{2.2f,-4.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadPitch,   {{0.f,0.f},{.85f,10.f},{1.f,10.f},{1.3f,-8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRX,      {{0.f,0.f},{.85f,-16.f},{1.f,-16.f},{1.3f,28.f},{2.2f,18.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRY,      {{0.f,0.f},{.85f,3.f},{1.f,3.f},{1.3f,-7.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRZ,      {{0.f,0.f},{.85f,131.f},{1.f,131.f},{1.3f,44.f},{2.2f,20.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,0.f},{.85f,8.f},{1.f,8.f},{1.3f,-58.f},{2.2f,-20.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLX,      {{0.f,0.f},{.85f,44.f},{1.f,44.f},{1.3f,-4.f},{2.2f,-2.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLY,      {{0.f,0.f},{.85f,13.f},{1.f,13.f},{1.3f,5.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,0.f},{.85f,56.f},{1.f,56.f},{1.3f,36.f},{2.2f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{1.f,2.f},{1.3f,3.f},{2.2f,.6f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 8 snipe (the sniper, rifle already shouldered): settle into the sights, turn side-on, hold; the shot's recoil kicks the muzzle up and rocks the body back
        {{
            {ChPlant,       {{0.f,0.f},{.3f,1.f},{2.4f,1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,0.f},{.4f,16.f},{2.4f,16.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,0.f},{.4f,-14.f},{2.4f,-14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,0.f},{.4f,8.f},{2.4f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.7f,6.f},{1.f,6.f},{1.2f,4.f},{2.4f,5.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.8f,6.f},{1.f,6.f},{1.1f,-8.f},{1.6f,-3.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoYaw,    {{0.f,0.f},{.8f,12.f},{1.f,12.f},{2.4f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,0.f},{.8f,-3.f},{1.f,-3.f},{1.1f,8.f},{1.8f,3.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadPitch,   {{0.f,0.f},{.8f,-4.f},{1.f,-4.f},{1.1f,6.f},{1.8f,2.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLX,      {{0.f,0.f},{1.f,0.f},{1.1f,-12.f},{1.8f,-4.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRX,      {{0.f,0.f},{1.f,0.f},{1.1f,-14.f},{1.8f,-5.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,0.f},{1.f,0.f},{1.1f,10.f},{1.8f,3.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,0.f},{1.f,0.f},{1.1f,10.f},{1.8f,3.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{.9f,2.f},{1.f,2.5f},{1.1f,0.f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 9 summon (the broodcaller's blob body): rear up and swell, then heave forward and contract; relax
        {{
            {ChScaleZ,      {{0.f,0.f},{.8f,.12f},{1.f,.16f},{1.2f,-.10f},{2.f,-.04f},{3.f,0.f},{-1.f,0.f}}},
            {ChScaleX,      {{0.f,0.f},{.8f,.18f},{1.f,.22f},{1.2f,-.06f},{2.f,.04f},{3.f,0.f},{-1.f,0.f}}},
            {ChScaleY,      {{0.f,0.f},{.8f,.18f},{1.f,.22f},{1.2f,-.06f},{2.f,.04f},{3.f,0.f},{-1.f,0.f}}},
            {ChFigPitch,    {{0.f,0.f},{.8f,14.f},{1.f,16.f},{1.2f,-18.f},{2.f,-6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{1.f,1.5f},{1.2f,.5f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 10 boss attack: arms flung wide with the maul overhead and the body rearing back; a two-handed slam down and forward; heaving recovery
        {{
            {ChPlant,       {{0.f,0.f},{.3f,1.f},{2.4f,1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,0.f},{.4f,10.f},{1.4f,24.f},{2.4f,24.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,0.f},{.4f,-16.f},{1.4f,-10.f},{2.4f,-10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,0.f},{.4f,14.f},{2.4f,14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.85f,5.f},{1.f,5.f},{1.35f,16.f},{2.2f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.85f,-10.f},{1.f,-10.f},{1.35f,22.f},{2.2f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,0.f},{.85f,14.f},{1.f,14.f},{1.35f,-20.f},{2.2f,-8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadPitch,   {{0.f,0.f},{.85f,8.f},{1.f,8.f},{1.35f,-12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRX,      {{0.f,0.f},{.85f,-29.f},{1.f,-29.f},{1.35f,50.f},{2.2f,34.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRY,      {{0.f,0.f},{.85f,15.f},{1.f,15.f},{1.35f,-14.f},{2.2f,-10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRZ,      {{0.f,0.f},{.85f,136.f},{1.f,136.f},{1.35f,-4.f},{2.2f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLX,      {{0.f,0.f},{.85f,-29.f},{1.f,-29.f},{1.35f,50.f},{2.2f,34.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLY,      {{0.f,0.f},{.85f,-15.f},{1.f,-15.f},{1.35f,14.f},{2.2f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,0.f},{.85f,131.f},{1.f,131.f},{1.35f,-4.f},{2.2f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,0.f},{.85f,50.f},{1.f,50.f},{1.35f,-70.f},{2.2f,-30.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{1.f,2.f},{1.35f,3.f},{2.2f,.8f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 11 shove (the grunt): draw both hands back to the chest and wind the right shoulder back; drive both hands and the shoulder forward, step in; settle
        {{
            {ChPlant,       {{0.f,0.f},{.3f,1.f},{2.3f,1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,0.f},{.4f,8.f},{1.3f,24.f},{2.3f,24.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,0.f},{.4f,-14.f},{1.3f,-10.f},{2.3f,-10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,0.f},{.4f,8.f},{2.3f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.85f,7.f},{1.f,7.f},{1.3f,12.f},{2.2f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.85f,-10.f},{1.f,-10.f},{1.3f,26.f},{2.2f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,0.f},{.85f,8.f},{1.f,8.f},{1.3f,-18.f},{2.2f,-6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoYaw,    {{0.f,0.f},{.85f,24.f},{1.f,24.f},{1.3f,-22.f},{2.2f,-10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadPitch,   {{0.f,0.f},{.85f,-6.f},{1.f,-6.f},{1.3f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRX,      {{0.f,0.f},{.85f,-14.f},{1.f,-14.f},{1.3f,38.f},{2.2f,26.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRY,      {{0.f,0.f},{.85f,-14.f},{1.f,-14.f},{1.3f,-22.f},{2.2f,-12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRZ,      {{0.f,0.f},{.85f,54.f},{1.f,54.f},{1.3f,58.f},{2.2f,30.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLX,      {{0.f,0.f},{.85f,-12.f},{1.f,-12.f},{1.3f,36.f},{2.2f,24.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLY,      {{0.f,0.f},{.85f,14.f},{1.f,14.f},{1.3f,22.f},{2.2f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,0.f},{.85f,54.f},{1.f,54.f},{1.3f,58.f},{2.2f,30.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,0.f},{.85f,40.f},{1.f,40.f},{1.3f,-24.f},{2.2f,-10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{1.f,.8f},{1.3f,.3f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 12 back-hop (the archer): drop into a crouch with the weight forward; spring away backwards (the move itself is the committed motion), body tipped back, arms out for balance, staff on the target; land low and rise
        {{
            {ChPlant,       {{0.f,0.f},{.3f,1.f},{1.f,1.f},{1.08f,0.f},{1.9f,0.f},{2.05f,1.f},{2.6f,1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,0.f},{.4f,10.f},{1.f,10.f},{1.08f,0.f},{1.9f,0.f},{2.05f,8.f},{2.6f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,0.f},{.4f,-12.f},{1.f,-12.f},{1.08f,0.f},{1.9f,0.f},{2.05f,-8.f},{2.6f,-8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,0.f},{.4f,10.f},{1.f,10.f},{1.08f,0.f},{1.9f,0.f},{2.05f,12.f},{2.6f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.8f,14.f},{1.f,14.f},{1.12f,-2.f},{1.9f,-2.f},{2.05f,13.f},{2.5f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.8f,8.f},{1.f,8.f},{1.12f,-16.f},{1.9f,-14.f},{2.1f,4.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFigPitch,    {{0.f,0.f},{.8f,-8.f},{1.f,-8.f},{1.15f,12.f},{1.85f,10.f},{2.1f,-5.f},{2.6f,-2.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,0.f},{.8f,6.f},{1.f,6.f},{1.15f,-4.f},{1.9f,-2.f},{2.2f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadPitch,   {{0.f,0.f},{.8f,-8.f},{1.f,-8.f},{1.15f,8.f},{1.9f,4.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRY,      {{0.f,0.f},{.8f,4.f},{1.f,4.f},{1.15f,32.f},{1.9f,28.f},{2.2f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRZ,      {{0.f,0.f},{.8f,8.f},{1.f,8.f},{1.15f,36.f},{1.9f,30.f},{2.2f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLY,      {{0.f,0.f},{.8f,-4.f},{1.f,-4.f},{1.15f,-30.f},{1.9f,-26.f},{2.2f,-6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,0.f},{.8f,8.f},{1.f,8.f},{1.15f,30.f},{1.9f,26.f},{2.2f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,0.f},{.8f,-30.f},{1.f,-30.f},{1.15f,-50.f},{1.9f,-40.f},{2.4f,-10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{.9f,.6f},{1.1f,.2f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 13 parry (the duelist; the wind-up is the stance): plant side-on, blade up across the body and the free hand forward; hold the guard, glowing; lower it
        {{
            {ChPlant,       {{0.f,0.f},{.2f,1.f},{2.3f,1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,0.f},{.25f,14.f},{2.3f,14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,0.f},{.25f,-12.f},{2.3f,-12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,0.f},{.25f,12.f},{2.3f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.25f,8.f},{1.f,8.f},{2.f,5.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoYaw,    {{0.f,0.f},{.25f,-22.f},{1.f,-22.f},{2.f,-12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,0.f},{.25f,-2.f},{1.f,-2.f},{2.f,-1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadPitch,   {{0.f,0.f},{.25f,-4.f},{1.f,-4.f},{2.f,-2.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRX,      {{0.f,0.f},{.25f,26.f},{1.f,26.f},{2.f,14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRY,      {{0.f,0.f},{.25f,-18.f},{1.f,-18.f},{2.f,-8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRZ,      {{0.f,0.f},{.25f,62.f},{1.f,62.f},{2.f,28.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLX,      {{0.f,0.f},{.25f,20.f},{1.f,20.f},{2.f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLY,      {{0.f,0.f},{.25f,14.f},{1.f,14.f},{2.f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,0.f},{.25f,40.f},{1.f,40.f},{2.f,18.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,0.f},{.25f,-10.f},{1.f,-10.f},{2.f,-4.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{.25f,1.6f},{1.f,1.6f},{1.3f,.4f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 14 riposte (the duelist, straight out of the parry's guard, so it starts there instead of neutral): cock the blade back over the shoulder, cut across the body in a fast step-in; settle
        {{
            {ChPlant,       {{0.f,1.f},{2.3f,1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,14.f},{1.3f,26.f},{2.3f,26.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,-12.f},{2.3f,-12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,12.f},{2.3f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,8.f},{.9f,7.f},{1.f,7.f},{1.35f,12.f},{2.2f,7.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.9f,-8.f},{1.f,-8.f},{1.35f,22.f},{2.2f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoYaw,    {{0.f,-22.f},{.9f,24.f},{1.f,24.f},{1.35f,-34.f},{2.2f,-18.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,-2.f},{.9f,8.f},{1.f,8.f},{1.35f,-14.f},{2.2f,-6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadPitch,   {{0.f,-4.f},{.9f,-6.f},{1.f,-6.f},{1.35f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRX,      {{0.f,26.f},{.9f,-30.f},{1.f,-30.f},{1.35f,40.f},{2.2f,28.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRY,      {{0.f,-18.f},{.9f,8.f},{1.f,8.f},{1.35f,-50.f},{2.2f,-36.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRZ,      {{0.f,62.f},{.9f,112.f},{1.f,112.f},{1.35f,22.f},{2.2f,14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLX,      {{0.f,20.f},{.9f,10.f},{1.f,10.f},{1.35f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLY,      {{0.f,14.f},{.9f,10.f},{1.f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,40.f},{.9f,44.f},{1.f,44.f},{1.35f,18.f},{2.2f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,-10.f},{.9f,58.f},{1.f,58.f},{1.35f,-52.f},{2.2f,-28.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,1.6f},{1.f,2.4f},{1.35f,.6f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 15 ground pound (the brute): rise onto the toes with the maul overhead in both fists and the chest out; slam it into the floor with the whole body; stay bent over it, heaving; rise
        {{
            {ChPlant,       {{0.f,0.f},{.3f,1.f},{2.5f,1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,0.f},{.4f,8.f},{1.35f,22.f},{2.5f,22.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,0.f},{.4f,-12.f},{1.35f,-8.f},{2.5f,-8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,0.f},{.4f,14.f},{2.5f,14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.85f,-4.f},{1.f,-4.f},{1.3f,24.f},{2.1f,20.f},{2.6f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.85f,-12.f},{1.f,-12.f},{1.3f,30.f},{2.1f,26.f},{2.6f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,0.f},{.85f,18.f},{1.f,18.f},{1.3f,-34.f},{2.1f,-28.f},{2.6f,-10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadPitch,   {{0.f,0.f},{.85f,14.f},{1.f,14.f},{1.3f,-20.f},{2.1f,-14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRX,      {{0.f,0.f},{.85f,-30.f},{1.f,-30.f},{1.3f,46.f},{2.1f,40.f},{2.6f,14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRY,      {{0.f,0.f},{.85f,14.f},{1.f,14.f},{1.3f,-12.f},{2.1f,-10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRZ,      {{0.f,0.f},{.85f,138.f},{1.f,138.f},{1.3f,-34.f},{2.1f,-30.f},{2.6f,-8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLX,      {{0.f,0.f},{.85f,-30.f},{1.f,-30.f},{1.3f,46.f},{2.1f,40.f},{2.6f,14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLY,      {{0.f,0.f},{.85f,-14.f},{1.f,-14.f},{1.3f,12.f},{2.1f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,0.f},{.85f,132.f},{1.f,132.f},{1.3f,-34.f},{2.1f,-30.f},{2.6f,-8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,0.f},{.85f,55.f},{1.f,55.f},{1.3f,-84.f},{2.1f,-80.f},{2.6f,-30.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{1.f,1.8f},{1.3f,3.f},{2.1f,1.2f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 16 enrage (the brute's roar): sink, then throw the chest out and the head back with both fists wide, shaking with rage and glowing; clench and settle forward
        {{
            {ChPlant,       {{0.f,0.f},{.25f,1.f},{2.5f,1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,0.f},{.3f,8.f},{2.5f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,0.f},{.3f,-8.f},{2.5f,-8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,0.f},{.3f,16.f},{2.5f,16.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.3f,8.f},{.6f,-2.f},{1.f,-2.f},{1.4f,8.f},{2.4f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,0.f},{.3f,-8.f},{.6f,20.f},{1.f,20.f},{1.4f,-12.f},{2.4f,-8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadPitch,   {{0.f,0.f},{.3f,-10.f},{.6f,26.f},{1.f,26.f},{1.4f,-14.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFigRoll,     {{0.f,0.f},{.6f,0.f},{.67f,2.5f},{.75f,-2.5f},{.83f,2.5f},{.91f,-2.5f},{1.f,0.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRX,      {{0.f,0.f},{.6f,6.f},{1.f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRY,      {{0.f,0.f},{.3f,-6.f},{.6f,36.f},{1.f,36.f},{1.4f,-10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRZ,      {{0.f,0.f},{.3f,10.f},{.6f,64.f},{1.f,64.f},{1.4f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLX,      {{0.f,0.f},{.6f,6.f},{1.f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLY,      {{0.f,0.f},{.3f,6.f},{.6f,-36.f},{1.f,-36.f},{1.4f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,0.f},{.3f,10.f},{.6f,64.f},{1.f,64.f},{1.4f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,0.f},{.6f,30.f},{1.f,30.f},{1.4f,-20.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{.4f,1.f},{.6f,3.f},{1.f,3.f},{1.5f,1.6f},{2.4f,.8f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 17 rift blink (the leaper's blob body): flatten and swell with violet light, tipped toward the target; vanish; pop back in stretched
        {{
            {ChScaleZ,      {{0.f,0.f},{.85f,-.36f},{1.f,-.42f},{1.12f,.20f},{2.f,.04f},{3.f,0.f},{-1.f,0.f}}},
            {ChScaleX,      {{0.f,0.f},{.85f,.24f},{1.f,.30f},{1.12f,-.10f},{2.f,-.02f},{3.f,0.f},{-1.f,0.f}}},
            {ChScaleY,      {{0.f,0.f},{.85f,.24f},{1.f,.30f},{1.12f,-.10f},{2.f,-.02f},{3.f,0.f},{-1.f,0.f}}},
            {ChFigPitch,    {{0.f,0.f},{.85f,-10.f},{1.f,-12.f},{1.12f,8.f},{2.f,2.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.85f,6.f},{1.f,8.f},{1.12f,-4.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{.7f,1.8f},{1.f,3.f},{1.12f,2.f},{2.f,.6f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 18 blink strike (the leaper, just reappeared behind its prey, so it starts stretched and glowing from the pop): coil, lunge with a slash, settle
        {{
            {ChScaleZ,      {{0.f,.20f},{.5f,-.16f},{1.f,-.22f},{1.2f,.16f},{2.f,.04f},{3.f,0.f},{-1.f,0.f}}},
            {ChScaleX,      {{0.f,-.08f},{.5f,.10f},{1.f,.14f},{1.2f,-.08f},{2.f,-.01f},{3.f,0.f},{-1.f,0.f}}},
            {ChScaleY,      {{0.f,-.08f},{.5f,.10f},{1.f,.14f},{1.2f,-.08f},{2.f,-.01f},{3.f,0.f},{-1.f,0.f}}},
            {ChFigPitch,    {{0.f,6.f},{.5f,-10.f},{1.f,-14.f},{1.25f,14.f},{2.f,3.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.5f,-8.f},{1.f,-10.f},{1.25f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,2.4f},{.5f,1.6f},{1.f,2.4f},{1.25f,.6f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
        // 19 meteor rain (the shaman): staff raised high in both hands, head thrown back, calling fire down; sweep the staff forward and down to loose it; hold, breathing
        {{
            {ChPlant,       {{0.f,0.f},{.3f,1.f},{2.4f,1.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootLX,      {{0.f,0.f},{.4f,8.f},{2.4f,8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChFootRX,      {{0.f,0.f},{.4f,-10.f},{2.4f,-10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChStance,      {{0.f,0.f},{.4f,12.f},{2.4f,12.f},{3.f,0.f},{-1.f,0.f}}},
            {ChCrouch,      {{0.f,0.f},{.85f,-4.f},{1.f,-4.f},{1.3f,8.f},{2.2f,5.f},{3.f,0.f},{-1.f,0.f}}},
            {ChLeanX,       {{0.f,0.f},{.85f,-8.f},{1.f,-8.f},{1.3f,12.f},{2.2f,6.f},{3.f,0.f},{-1.f,0.f}}},
            {ChTorsoPitch,  {{0.f,0.f},{.85f,16.f},{1.f,16.f},{1.3f,-12.f},{2.2f,-4.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHeadPitch,   {{0.f,0.f},{.85f,26.f},{1.f,26.f},{1.3f,-10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRX,      {{0.f,0.f},{.85f,-8.f},{1.f,-8.f},{1.3f,34.f},{2.2f,18.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRY,      {{0.f,0.f},{.85f,10.f},{1.f,10.f},{1.3f,-8.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandRZ,      {{0.f,0.f},{.85f,138.f},{1.f,138.f},{1.3f,50.f},{2.2f,22.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLX,      {{0.f,0.f},{.85f,-6.f},{1.f,-6.f},{1.3f,26.f},{2.2f,10.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLY,      {{0.f,0.f},{.85f,-6.f},{1.f,-6.f},{1.3f,4.f},{3.f,0.f},{-1.f,0.f}}},
            {ChHandLZ,      {{0.f,0.f},{.85f,128.f},{1.f,128.f},{1.3f,46.f},{2.2f,18.f},{3.f,0.f},{-1.f,0.f}}},
            {ChWeaponPitch, {{0.f,0.f},{.85f,6.f},{1.f,6.f},{1.3f,-62.f},{2.2f,-22.f},{3.f,0.f},{-1.f,0.f}}},
            {ChGlow,        {{0.f,0.f},{.5f,2.f},{1.f,3.f},{1.3f,3.f},{2.2f,.8f},{3.f,0.f},{-1.f,0.f}}},
            {-1,{}}
        }},
    };
    return Table[TrackIndex(Ability)];
}
}
