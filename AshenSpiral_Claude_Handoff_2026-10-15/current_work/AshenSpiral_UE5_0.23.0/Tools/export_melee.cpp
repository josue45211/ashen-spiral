#include "../Source/AshenSpiral/Public/ASMeleeRules.h"
#include "../Source/AshenSpiral/Public/ASArsenalRules.h"
#include "../Source/AshenSpiral/Public/ASArchiveRules.h"
#include <iostream>
int main() {
    std::cout<<"{\"poses\":[";bool First=true;
    for(int W:{0,1,5}) for(int Step=0;Step<ASMelee::ComboLength(W);++Step) for(int Frame=0;Frame<=100;++Frame) {
        if(!First) std::cout<<',';
        First=false;const auto P=ASMelee::Swing(W,Step,Frame/100.f);
        std::cout<<"{\"weapon\":"<<W<<",\"step\":"<<Step<<",\"frame\":"<<Frame
            <<",\"cycle\":"<<ASArsenal::Rate(W)*ASMelee::TimeScale(W,Step)
            <<",\"support\":"<<ASMelee::SupportHeight(W)<<",\"pose\":["<<P.X<<','<<P.Y<<','<<P.Z<<','<<P.Pitch<<','<<P.Yaw<<','<<P.Roll<<','<<P.BodyYaw<<','<<P.BodyPitch<<"]}";
    }
    std::cout<<"],\"archives\":[";
    for(int Room=0;Room<4;++Room) {
        if(Room) std::cout<<',';
        const auto P=ASArchive::Dress(1103,Room,1200,1200);
        std::cout<<"{\"motif\":"<<P.Motif<<",\"props\":[";First=true;
        for(const auto& D:P.Props) {if(!First)std::cout<<',';First=false;
            std::cout<<'['<<int(D.Type)<<','<<D.X<<','<<D.Y<<','<<D.Yaw<<','<<D.Scale<<','<<D.ZScale<<','<<D.Radius<<']';}
        std::cout<<"]}";
    }
    std::cout<<"]}\n";
}
