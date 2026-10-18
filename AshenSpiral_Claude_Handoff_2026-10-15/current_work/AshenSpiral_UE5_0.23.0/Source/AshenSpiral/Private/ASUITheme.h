#pragma once
// Central UI theme (0.23.3 UI pass). Palette and brushes shared by the menu pages; HUD colours live in ASHUD.cpp.
// Keep contrast high: text on the panel must stay readable (minimum body font is 14, set in SASMenu.cpp).
#include "CoreMinimal.h"
#include "Brushes/SlateRoundedBoxBrush.h"
namespace ASUI {
inline const FLinearColor Gold(.98f,.64f,.25f),Muted(.67f,.73f,.79f),Teal(.15f,.87f,.74f),Ink(.018f,.024f,.035f,.97f);
inline const FLinearColor CardFill(.030f,.043f,.056f,1.f),CardHoverFill(.050f,.070f,.088f,1.f),CardEdge(.18f,.24f,.28f,.9f),CardHoverEdge(.98f,.64f,.25f,.75f);
inline const FLinearColor ButtonIdle(.055f,.16f,.18f),ButtonHover(.09f,.27f,.29f);
// Rounded, outlined surfaces. Static so the brushes outlive every widget that points at them.
inline const FSlateBrush* PanelBrush() {static const FSlateRoundedBoxBrush Brush(Ink,14.f,FLinearColor(.98f,.64f,.25f,.55f),1.5f);return &Brush;}
inline const FSlateBrush* CardBrush() {static const FSlateRoundedBoxBrush Brush(CardFill,8.f,CardEdge,1.f);return &Brush;}
inline const FSlateBrush* CardHoverBrush() {static const FSlateRoundedBoxBrush Brush(CardHoverFill,8.f,CardHoverEdge,1.5f);return &Brush;}
}
