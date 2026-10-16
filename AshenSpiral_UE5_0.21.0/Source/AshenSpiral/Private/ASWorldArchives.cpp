#include "ASWorld.h"
#include "ASArchiveRules.h"

void AASWorld::Archives(const FVector& P,int32 I) {
    const auto Half=CellHalfSize(I);
    const auto Plan=ASArchive::Dress(uint32(Config.Seed),I,Half.X,Half.Y);
    const FLinearColor Stone(.29f,.31f,.34f),Wood(.34f,.24f,.15f),Paper(.62f,.53f,.36f),Rust(.30f,.18f,.105f);
    FRandomStream R(Config.Seed^((I+1)*7919));
    for(const auto& D:Plan.Props) {
        const FVector At=P+FVector(D.X,D.Y,10),Scale(D.Scale,D.Scale,D.ZScale);
        const FRotator Facing(0,D.Yaw,0);const FString Variant=FString::FromInt(D.Tint);
        const float Age=.85f+D.Tint*.06f;
        switch(D.Type) {
        case ASArchive::Shelf:case ASArchive::BrokenShelf: {
            const bool Broken=D.Type==ASArchive::BrokenShelf;
            Add(Broken?TEXT("SM_ArchiveShelfBroken"):TEXT("SM_ArchiveShelf"),TEXT("ArchiveWood")+Variant,At,Scale,Wood*Age,true,D.Yaw);
            // Local +X is the open front. Rotate books and their offsets with it.
            for(int32 Row=0;Row<(Broken?2:4);++Row) for(int32 B=0;B<9;++B) {
                if(R.RandRange(0,4)==0) continue;
                const FVector Local(10,(-108+B*26.f),43+Row*68.f);
                const FLinearColor Cover=B%3==0?FLinearColor(.26f,.14f,.11f):(B%3==1?Paper:FLinearColor(.17f,.24f,.24f));
                Add(TEXT("SM_Book"),FString::Printf(TEXT("ArchiveBooks%d"),B%3),At+Facing.RotateVector(Local*Scale),
                    FVector(D.Scale*.95f,D.Scale*(.7f+R.FRand()*.3f),D.ZScale*(.65f+R.FRand()*.25f)),Cover,false,D.Yaw);
            }
            break;
        }
        case ASArchive::Desk:case ASArchive::BrokenDesk:
            Add(D.Type==ASArchive::Desk?TEXT("SM_ArchiveDesk"):TEXT("SM_ArchiveDeskBroken"),TEXT("ArchiveWood")+Variant,At,Scale,Wood*Age,true,D.Yaw);
            if(D.Type==ASArchive::Desk) for(int32 B=0;B<3;++B)
                Add(TEXT("SM_Book"),TEXT("ArchiveFolio"),At+Facing.RotateVector(FVector(-20+B*23,0,104)*Scale),FVector(.8f,.9f,.10f)*Scale,Paper,false,D.Yaw+B*13);
            break;
        case ASArchive::Column:
            Add(TEXT("SM_Pillar"),TEXT("ArchivePier")+Variant,At,Scale,Stone*Age,true,D.Yaw);
            Add(TEXT("SM_ArchiveIronwork"),TEXT("ArchiveIron")+Variant,At+FVector(0,0,180*D.ZScale),FVector(D.Scale),Rust,false,D.Yaw,0,.55f);
            break;
        case ASArchive::Debris:
            Add(TEXT("SM_ArchiveDebris"),TEXT("ArchiveDebris")+Variant,At,Scale,Paper*Age,false,D.Yaw+R.FRandRange(-35,35));
            break;
        }
    }
    // Pointed ribs sit above head height. They never create invisible blockers.
    const int32 RibCount=Half.X>=1100?3:2;
    for(int32 J=0;J<RibCount;++J) {
        const float X=FMath::Lerp(-Half.X*.62f,Half.X*.62f,float(J)/float(RibCount-1));
        Add(TEXT("SM_ArchiveRib"),TEXT("ArchiveGothicRibs"),P+FVector(X,0,0),FVector(1,(Half.Y-65)/450.f,1),Stone*1.12f,false);
    }
    // Two low pools of light rake the masonry upward, within the existing 48-light budget.
    for(int32 Side:{-1,1}) {
        const FVector Lamp=P+FVector(Side*Half.X*.62f,Side*(Half.Y-115),30);
        Add(TEXT("SM_ArchiveIronwork"),TEXT("ArchiveUplightIron"),Lamp,FVector(.48f),Rust,false,Side*90.f,0,.6f);
        const FLinearColor Color=Side>0?FLinearColor(.34f,.48f,.64f):FLinearColor(.74f,.47f,.23f);
        Light(Lamp+FVector(0,0,25),Color,Side>0?1800.f:1400.f,1200.f);
        // Sparse dust flecks in each pool, static and non-colliding.
        for(int32 M=0;M<10;++M) Add(TEXT("Sphere"),TEXT("ArchiveDustMotes"),Lamp+FVector(R.FRandRange(-80,80),R.FRandRange(-80,80),R.FRandRange(70,440)),
            FVector(.006f+R.FRand()*.008f),Paper,false,0,.18f);
    }
}
