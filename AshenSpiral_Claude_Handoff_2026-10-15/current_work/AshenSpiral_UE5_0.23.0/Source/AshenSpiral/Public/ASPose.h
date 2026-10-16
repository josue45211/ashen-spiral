#pragma once
#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "ASAnimationRules.h"
namespace ASPose {
inline void Segment(UStaticMeshComponent* Part,const FVector& From,const FVector& To,float MeshLength) {
    const FVector V=From-To;Part->SetRelativeLocation((From+To)*.5f);
    Part->SetRelativeRotation(FQuat::FindBetweenNormals(FVector::UpVector,V.GetSafeNormal()));
    Part->SetRelativeScale3D(FVector(1,1,V.Size()/MeshLength));
}
inline FVector Limb(UStaticMeshComponent* Upper,UStaticMeshComponent* Lower,const FVector& Root,const FVector& Target,const FVector& Pole,float A=42.f,float B=42.f,float MeshLengthA=42.f,float MeshLengthB=42.f) {
    FVector Axis=(Target-Root).GetSafeNormal();if(Axis.IsNearlyZero()) Axis=-FVector::UpVector;
    const auto Bend=ASAnimation::TwoBone(FVector::Distance(Root,Target),A,B);
    FVector Side=Pole-Root;Side=(Side-Axis*FVector::DotProduct(Side,Axis)).GetSafeNormal();if(Side.IsNearlyZero()) Side=FVector::RightVector;
    const FVector Joint=Root+Axis*Bend.Along+Side*Bend.Out,End=Root+Axis*Bend.Reach;
    Segment(Upper,Root,Joint,MeshLengthA);Segment(Lower,Joint,End,MeshLengthB);return End;
}
}
