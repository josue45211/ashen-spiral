#include "../Source/AshenSpiral/Public/ASHammerRules.h"
#include <iostream>
int main() {
    using namespace ASHammer;std::cout<<"[";bool First=true;
    for(int Mode=0;Mode<6;++Mode) for(int I=0;I<=240;++I) {
        const float T=I/240.f;const auto P=Mode<3?Swing(Mode,T):Leap(Mode-2,T);const auto F=Mode<3?SwingFrame(Mode,T):LeapFrame(Mode-2,T);
        if(!First)std::cout<<',';First=false;
        std::cout<<"{\"mode\":"<<Mode<<",\"frame\":"<<I<<",\"pose\":["<<P.X<<','<<P.Y<<','<<P.Z<<','<<P.Pitch<<','<<P.Yaw<<','<<P.Roll<<','<<P.BodyYaw<<','<<P.BodyPitch<<"],\"rotation\":[";
        const V Rows[]={F.X,F.Y,F.Z};for(int R=0;R<3;++R) {if(R)std::cout<<',';std::cout<<'['<<Rows[R].X<<','<<Rows[R].Y<<','<<Rows[R].Z<<']';}std::cout<<"]}";
    }
    std::cout<<"]\n";
}
