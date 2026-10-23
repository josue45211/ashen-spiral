// 0.24.2 Phase F: builds the hero's animation blueprint, ABP_AshenHero, from the Third Person template's ABP_Unarmed (strafing locomotion, jump/fall/land, foot IK), adding an
// UPPER-BODY layer so an attack plays on the arms and torso while the legs keep running:
//   Main States -> Save cached pose 'HeroBase'
//   Use 'HeroBase' -> Slot 'UpperBody' --\
//   Use 'HeroBase' ------------------- Layered blend per bone (from spine_01, local space) -> Slot 'DefaultSlot' (full body: dashes, smashes, the fall) -> foot IK -> Output
// Editor only (this module is never in a packaged game). Run:
//   UnrealEditor-Cmd.exe AshenSpiral.uproject -run=ASBuildHeroAnim -unattended -nosplash -NullRHI
// The editor's Python cannot create the cached-pose readers (their menu entries are made on the fly by the graph schema), so this is C++.
#include "ASBuildHeroAnimCommandlet.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/Skeleton.h"
#include "AnimGraphNode_SaveCachedPose.h"
#include "AnimGraphNode_UseCachedPose.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_TwoBoneIK.h"
#include "K2Node_VariableGet.h"
#include "EdGraphSchema_K2.h"
#include "AnimationGraphSchema.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"


namespace ASHeroAnimBuild {
const TCHAR* Template=TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed");
const TCHAR* Folder=TEXT("/Game/Characters/Hero");
const TCHAR* Name=TEXT("ABP_AshenHero");
const FName UpperSlot(TEXT("UpperBody"));

UEdGraphPin* FindPin(UEdGraphNode* Node,const TCHAR* PinName,EEdGraphPinDirection Direction) {
    if(!Node) return nullptr;
    for(UEdGraphPin* Pin:Node->Pins) if(Pin&&Pin->Direction==Direction&&Pin->PinName==FName(PinName)) return Pin;
    return nullptr;
}
bool Link(UEdGraphPin* From,UEdGraphPin* To) {
    if(!From||!To) return false;
    const UEdGraphSchema* Schema=From->GetSchema();
    return Schema?Schema->TryCreateConnection(From,To):(From->MakeLinkTo(To),true);
}
template<class T> T* Place(UEdGraph& Graph,int32 X,int32 Y) {
    FGraphNodeCreator<T> Creator(Graph);T* Node=Creator.CreateNode();Node->NodePosX=X;Node->NodePosY=Y;Creator.Finalize();return Node;
}
bool Save(UObject* Asset) {
    UPackage* Package=Asset?Asset->GetOutermost():nullptr;if(!Package) return false;
    Package->MarkPackageDirty();
    const FString File=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;
    return UPackage::SavePackage(Package,Asset,*File,Args);
}
// 0.26 G4: the variables the game sets on the blueprint each frame (ASCharacterSkeletal.cpp, DriveTwoHandGrip): how much the left hand is held on the weapon (0 for one-handed weapons), and where
// on it, as a point in the RIGHT hand bone's own space (so the target follows the animated right hand exactly, with no frame of lag).
const FName TwoHandAlpha(TEXT("TwoHandAlpha")),LeftGripOffset(TEXT("LeftGripOffset"));
UK2Node_VariableGet* PlaceGet(UEdGraph& Graph,FName Variable,int32 X,int32 Y) {
    FGraphNodeCreator<UK2Node_VariableGet> Creator(Graph);UK2Node_VariableGet* Node=Creator.CreateNode();
    Node->VariableReference.SetSelfMember(Variable);Node->NodePosX=X;Node->NodePosY=Y;Creator.Finalize();return Node;
}
// Inserts a two-bone IK on the left arm (upperarm_l, lowerarm_l, hand_l) after the full-body slot and before whatever the slot fed (the template's foot IK): the left hand is taken to a point on the
// weapon's handle. The elbow is pointed out to the left and down, behind the body, so the arm folds the way a swung two-handed weapon's does. Returns false (and logs the pins) when it cannot wire.
bool AddTwoHandIK(UAnimBlueprint* Blueprint,UEdGraph& Graph,UAnimGraphNode_Slot* FullBody) {
    FEdGraphPinType FloatType(UEdGraphSchema_K2::PC_Real,UEdGraphSchema_K2::PC_Float,nullptr,EPinContainerType::None,false,FEdGraphTerminalType());
    FEdGraphPinType VectorType(UEdGraphSchema_K2::PC_Struct,NAME_None,TBaseStructure<FVector>::Get(),EPinContainerType::None,false,FEdGraphTerminalType());
    FBlueprintEditorUtils::AddMemberVariable(Blueprint,TwoHandAlpha,FloatType);
    FBlueprintEditorUtils::AddMemberVariable(Blueprint,LeftGripOffset,VectorType);
    UEdGraphPin* SlotOut=FindPin(FullBody,TEXT("Pose"),EGPD_Output);if(!SlotOut) return false;
    TArray<UEdGraphPin*> Downstream=SlotOut->LinkedTo;SlotOut->BreakAllPinLinks();   // the nodes the slot fed
    auto* IK=Place<UAnimGraphNode_TwoBoneIK>(Graph,FullBody->NodePosX+300,FullBody->NodePosY);
    IK->Node.IKBone.BoneName=TEXT("hand_l");
    IK->Node.EffectorLocationSpace=BCS_BoneSpace;IK->Node.EffectorTarget.BoneReference.BoneName=TEXT("hand_r");
    IK->Node.JointTargetLocationSpace=BCS_ComponentSpace;IK->Node.JointTargetLocation=FVector(70.f,-30.f,70.f); // the mesh faces +Y, its left is +X: the elbow goes out to the left, down and back
    IK->Node.bTakeRotationFromEffectorSpace=false;IK->Node.bMaintainEffectorRelRot=true;IK->Node.bAllowStretching=false;
    IK->ReconstructNode();
    auto* GetAlpha=PlaceGet(Graph,TwoHandAlpha,IK->NodePosX-260,IK->NodePosY+250),*GetGrip=PlaceGet(Graph,LeftGripOffset,IK->NodePosX-260,IK->NodePosY+330);
    bool Linked=Link(FindPin(GetAlpha,TEXT("TwoHandAlpha"),EGPD_Output),FindPin(IK,TEXT("Alpha"),EGPD_Input))                       // a variable getter's output pin carries the variable's name
        &&Link(FindPin(GetGrip,TEXT("LeftGripOffset"),EGPD_Output),FindPin(IK,TEXT("EffectorLocation"),EGPD_Input))
        &&Link(SlotOut,FindPin(IK,TEXT("ComponentPose"),EGPD_Input));
    for(UEdGraphPin* Target:Downstream) Linked=Linked&&Link(FindPin(IK,TEXT("Pose"),EGPD_Output),Target);
    if(!Linked) for(UEdGraphNode* Node:{(UEdGraphNode*)IK,(UEdGraphNode*)GetAlpha,(UEdGraphNode*)GetGrip}) for(UEdGraphPin* Pin:Node->Pins) UE_LOG(LogTemp,Display,TEXT("AshenHeroAnim: IK %s pin %s dir %d"),*Node->GetName(),*Pin->PinName.ToString(),int32(Pin->Direction));
    return Linked;
}
}

int32 UASBuildHeroAnimCommandlet::Main(const FString& Params) {
    using namespace ASHeroAnimBuild;
    const FString Path=FString(Folder)/Name;
    // a rebuild replaces this command's own earlier output: its file goes before anything loads it (renaming a loaded blueprint away collides with its generated class)
    const FString OldFile=FPackageName::LongPackageNameToFilename(Path,FPackageName::GetAssetPackageExtension());
    if(IFileManager::Get().FileExists(*OldFile)&&!IFileManager::Get().Delete(*OldFile,false,true)) {UE_LOG(LogTemp,Error,TEXT("AshenHeroAnim: could not replace %s"),*OldFile);return 1;}
    UAnimBlueprint* Source=LoadObject<UAnimBlueprint>(nullptr,Template);
    if(!Source) {UE_LOG(LogTemp,Error,TEXT("AshenHeroAnim: the template %s is missing (Tools/import_mannequin.ps1 copies it)"),Template);return 1;}
    IAssetTools& Tools=FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    UAnimBlueprint* Blueprint=Cast<UAnimBlueprint>(Tools.DuplicateAsset(Name,Folder,Source));
    if(!Blueprint) {UE_LOG(LogTemp,Error,TEXT("AshenHeroAnim: could not duplicate the template"));return 1;}
    UEdGraph* Graph=nullptr;
    for(UEdGraph* G:Blueprint->FunctionGraphs) if(G&&G->GetFName()==FName(TEXT("AnimGraph"))) Graph=G;
    if(!Graph) {UE_LOG(LogTemp,Error,TEXT("AshenHeroAnim: no AnimGraph"));return 1;}
    UAnimGraphNode_StateMachine* Main=nullptr;UAnimGraphNode_Slot* FullBody=nullptr;
    for(UEdGraphNode* Node:Graph->Nodes) {
        if(auto* Machine=Cast<UAnimGraphNode_StateMachine>(Node)) if(Machine->GetStateMachineName()==TEXT("Main States")) Main=Machine;
        if(auto* Slot=Cast<UAnimGraphNode_Slot>(Node)) FullBody=Slot;
    }
    if(!Main||!FullBody) {UE_LOG(LogTemp,Error,TEXT("AshenHeroAnim: the template's graph changed (Main States %d, DefaultSlot %d)"),Main?1:0,FullBody?1:0);return 1;}
    UEdGraphPin* MainPose=FindPin(Main,TEXT("Pose"),EGPD_Output);UEdGraphPin* FullSource=FindPin(FullBody,TEXT("Source"),EGPD_Input);
    if(!MainPose||!FullSource) {UE_LOG(LogTemp,Error,TEXT("AshenHeroAnim: pins missing"));return 1;}
    MainPose->BreakAllPinLinks();
    // cache the locomotion so it feeds both sides of the layer
    auto* Cache=Place<UAnimGraphNode_SaveCachedPose>(*Graph,FullBody->NodePosX-900,FullBody->NodePosY-320);Cache->CacheName=TEXT("HeroBase");
    auto* UseUpper=Place<UAnimGraphNode_UseCachedPose>(*Graph,FullBody->NodePosX-700,FullBody->NodePosY+120);UseUpper->SaveCachedPoseNode=Cache;
    auto* UseBase=Place<UAnimGraphNode_UseCachedPose>(*Graph,FullBody->NodePosX-700,FullBody->NodePosY-120);UseBase->SaveCachedPoseNode=Cache;
    auto* Upper=Place<UAnimGraphNode_Slot>(*Graph,FullBody->NodePosX-450,FullBody->NodePosY+120);Upper->Node.SlotName=UpperSlot;
    auto* Layer=Place<UAnimGraphNode_LayeredBoneBlend>(*Graph,FullBody->NodePosX-220,FullBody->NodePosY);
    // from the first spine bone up, the attack, eased in over three spine bones. LOCAL space: the Paragon clips spin the whole body (their pelvis turns up to 180 degrees over a swing),
    // and a mesh-space blend would copy that spin onto the torso while the legs keep running forward; in local space only the torso's twist against the hips carries over.
    Layer->Node.LayerSetup.SetNum(1);Layer->Node.LayerSetup[0].BranchFilters={FBranchFilter{FName(TEXT("spine_01")),3}};Layer->Node.bMeshSpaceRotationBlend=false;
    Layer->ReconstructNode();Cache->ReconstructNode();UseUpper->ReconstructNode();UseBase->ReconstructNode();Upper->ReconstructNode();
    const bool Linked=Link(MainPose,FindPin(Cache,TEXT("Pose"),EGPD_Input))
        &&Link(FindPin(UseUpper,TEXT("Pose"),EGPD_Output),FindPin(Upper,TEXT("Source"),EGPD_Input))
        &&Link(FindPin(UseBase,TEXT("Pose"),EGPD_Output),FindPin(Layer,TEXT("BasePose"),EGPD_Input))
        &&Link(FindPin(Upper,TEXT("Pose"),EGPD_Output),FindPin(Layer,TEXT("BlendPoses_0"),EGPD_Input))
        &&Link(FindPin(Layer,TEXT("Pose"),EGPD_Output),FullSource);
    if(!Linked) {
        for(UEdGraphNode* Node:{(UEdGraphNode*)Cache,(UEdGraphNode*)UseUpper,(UEdGraphNode*)UseBase,(UEdGraphNode*)Upper,(UEdGraphNode*)Layer}) for(UEdGraphPin* Pin:Node->Pins) UE_LOG(LogTemp,Display,TEXT("AshenHeroAnim: %s pin %s dir %d"),*Node->GetName(),*Pin->PinName.ToString(),int32(Pin->Direction));
        UE_LOG(LogTemp,Error,TEXT("AshenHeroAnim: could not wire the layer"));return 1;
    }
    // 0.26 G4: both hands on a two-handed weapon (the maul): the left hand follows a point on its handle
    if(!AddTwoHandIK(Blueprint,*Graph,FullBody)) {UE_LOG(LogTemp,Error,TEXT("AshenHeroAnim: could not add the two-hand IK"));return 1;}
    // the new slot must be known to the skeleton (in the default group) for montages to find it
    if(USkeleton* Skeleton=Blueprint->TargetSkeleton) {
        if(Skeleton->GetSlotGroupName(UpperSlot)==NAME_None||!Skeleton->ContainsSlotName(UpperSlot)) {Skeleton->SetSlotGroupName(UpperSlot,FAnimSlotGroup::DefaultGroupName);Save(Skeleton);}
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    const bool Clean=Blueprint->Status!=BS_Error;
    const bool Saved=Save(Blueprint);
    UE_LOG(LogTemp,Display,TEXT("AshenHeroAnim: %s built (compile %s, saved %d)"),*Path,Clean?TEXT("clean"):TEXT("ERRORS"),Saved?1:0);
    return Clean&&Saved?0:1;
}
