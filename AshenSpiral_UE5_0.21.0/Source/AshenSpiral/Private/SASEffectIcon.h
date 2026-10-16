#pragma once
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElementTypes.h"
#include "ASEffectArt.h"
// Original code-native illustrations; shared with source previews.
class SASEffectIcon : public SLeafWidget {
public:
    SLATE_BEGIN_ARGS(SASEffectIcon):_Kind(0),_Color(FLinearColor::White),_Size(130.f) {}
        SLATE_ARGUMENT(int32,Kind) SLATE_ARGUMENT(FLinearColor,Color) SLATE_ARGUMENT(float,Size)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args) {Kind=Args._Kind;Color=Args._Color;Size=Args._Size;SetVisibility(EVisibility::HitTestInvisible);}
    virtual FVector2D ComputeDesiredSize(float) const override {return FVector2D(Size,Size);}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& Geometry,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override {
        const FVector2D Extent=Geometry.GetLocalSize();const float Scale=FMath::Min(Extent.X,Extent.Y)/100.f;
        const FVector2D Offset=(Extent-FVector2D(100.f*Scale,100.f*Scale))*.5f;
        for(const auto& Stroke:ASEffectArt::Draw(Kind)) {
            TArray<FVector2f> Points;for(const auto& P:Stroke.Points) Points.Add(FVector2f(float(Offset.X)+P.X*Scale,float(Offset.Y)+P.Y*Scale));
            auto Tint=Color;Tint.A*=Stroke.Alpha;
            FSlateDrawElement::MakeLines(Out,Layer,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,Tint,true,Stroke.Width*Scale);
        }
        return Layer+1;
    }
private:int32 Kind=0;FLinearColor Color;float Size=130;
};
