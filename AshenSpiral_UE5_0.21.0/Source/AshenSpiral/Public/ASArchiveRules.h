#pragma once
#include "ASRules.h"
// Deterministic dressing only. Room topology, doors and encounter logic stay intact.
namespace ASArchive {
enum Kind {Shelf, BrokenShelf, Desk, BrokenDesk, Column, Debris};
struct Prop {Kind Type;float X,Y,Yaw,Scale,ZScale,Radius;int Tint;};
struct Plan {int Motif=0;std::vector<Prop> Props;};
inline Plan Dress(std::uint32_t Seed,int Room,float HalfX,float HalfY) {
    ASRules::Random R(Seed^((std::uint32_t(Room)+1)*0x9e3779b9u));
    Plan Out;Out.Motif=int((Seed%4+std::uint32_t(Room))%4);
    auto Place=[&](Kind Type,float X,float Y,float Scale,float ZScale,float Radius) {
        // Conservative bounding circles protect the 720 cm cross and wall margins.
        if(std::abs(X)-Radius<360||std::abs(Y)-Radius<360
            ||std::abs(X)+Radius>HalfX-35||std::abs(Y)+Radius>HalfY-35) return;
        for(const auto& P:Out.Props) if(P.Type!=Debris&&Type!=Debris
            &&std::hypot(P.X-X,P.Y-Y)<P.Radius+Radius+40) return;
        const float Yaw=std::atan2(-Y,-X)*180.f/3.14159265358979323846f;
        Out.Props.push_back({Type,X,Y,Yaw,Scale,ZScale,Radius,R.Range(4)});
    };
    const bool Large=HalfX>=1100&&HalfY>=1100;
    constexpr int Styles[4][4]={{0,0,1,0},{1,1,0,3},{2,3,2,2},{3,0,3,1}};
    constexpr float AnchorX[]={.70f,.60f,.73f,.68f},AnchorY[]={.62f,.72f,.58f,.68f};
    for(int SX:{-1,1}) for(int SY:{-1,1}) {
        const float Jitter=float(R.Range(91)-45);
        const int Corner=(SX>0?1:0)+(SY>0?2:0);
        const int Style=Styles[Out.Motif][Corner];
        if(!Large) {
            // Narrow rooms get tall, shallow perimeter stacks, never mid-lane props.
            const float X=SX*(HalfX-125),Y=SY*(HalfY-125);
            if(R.Range(5)!=0) {
                if(Style==1) Place(Desk,X,Y,.60f,1.f,70);
                else if(Style==2) Place(BrokenDesk,X,Y,.60f,1.f,70);
                else Place(R.Range(4)==0?BrokenShelf:Shelf,X,Y,.50f,1.15f,80);
            }
            Place(Debris,SX*(HalfX-165),SY*(HalfY-170),.55f,.55f,45);
            continue;
        }
        const float X=SX*(HalfX*AnchorX[Out.Motif]+Jitter),Y=SY*(HalfY*AnchorY[Out.Motif]-Jitter);
        if(Style==0) { // Paired stacks, with unequal heights and occasional missing bays.
            Place(Shelf,X,Y-SY*190,1.f,1.40f,160);
            Place(R.Range(3)==0?BrokenShelf:Shelf,X,Y+SY*190,.92f,1.12f,148);
        } else if(Style==1) { // Scriptorium: a reader's table opposite one wall of books.
            Place(Desk,X-SX*150,Y,.95f,.95f,115);
            Place(Shelf,X+SX*170,Y,1.f,1.55f,160);
        } else if(Style==2) { // Ruined bay: split table, spilled folios, one surviving column.
            Place(BrokenDesk,X,Y,.95f,.95f,115);
            Place(Column,X+SX*220,Y+SY*200,.65f,2.15f,62);
        } else { // A spare, vaulted reading bay creates deliberate breathing room.
            Place(BrokenShelf,X+SX*120,Y,1.1f,1.25f,176);
            Place(Column,X-SX*160,Y-SY*160,.65f,2.35f,62);
        }
        Place(Debris,X-SX*50,Y+SY*95,.9f,.9f,74);
    }
    return Out;
}
}
