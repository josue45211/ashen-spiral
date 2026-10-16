#pragma once
#include <vector>
#include <cmath>
#include <initializer_list>
namespace ASEffectArt {
struct Point {float X,Y;};
struct Stroke {std::vector<Point> Points;float Width=3,Alpha=1;};
enum Icon {Blade,Shield,Flame,Venom,Frost,Bolt,Fork,Dash,Vortex,Star,Magazine,Reload,Precision,Dice,Heart,Bow};
inline std::vector<Stroke> Draw(int Kind) {
    std::vector<Stroke> Out;
    const auto Line=[&](std::initializer_list<Point> P,float Width=3.f,float Alpha=1.f) {Out.push_back({std::vector<Point>(P),Width,Alpha});};
    const auto Ring=[&](float X,float Y,float Radius,float Begin,float End,float Alpha=1.f) {
        Stroke S;S.Alpha=Alpha;for(int I=0;I<=32;++I) {const float A=Begin+(End-Begin)*I/32;S.Points.push_back({X+std::cos(A)*Radius,Y+std::sin(A)*Radius});}Out.push_back(S);
    };
    // A sparse rune halo and offset etching provide texture without a filled slab.
    Ring(50,50,43,.25f,2.55f,.22f);Ring(50,50,43,3.4f,5.7f,.22f);
    Line({{9,20},{17,12},{28,12}},1,.35f);Line({{73,88},{84,88},{92,80}},1,.35f);
    switch(Kind) {
    case Blade:Line({{29,77},{65,22},{78,15},{78,32},{42,82},{29,77}},5);Line({{23,65},{53,84}},5);Line({{25,80},{18,90}},6);Line({{55,44},{71,23}},2,.5f);break;
    case Shield:Line({{22,22},{50,14},{78,22},{75,58},{65,74},{50,86},{35,74},{25,58},{22,22}},5);Line({{34,34},{50,29},{66,34},{63,55},{50,69},{37,55},{34,34}},2,.5f);Line({{40,46},{48,56},{64,39}},4);break;
    case Flame:Line({{50,12},{63,37},{68,29},{81,57},{77,72},{64,84},{45,88},{29,77},{21,62},{26,45},{37,56},{42,34},{50,12}},4);Line({{48,52},{62,69},{59,80},{44,81},{37,71},{48,52}},5);break;
    case Venom:Line({{27,75},{32,46},{56,26},{79,23},{72,47},{56,66},{27,75}},4);Line({{27,75},{66,36}},4);Line({{39,62},{39,46}},3);Line({{49,52},{64,51}},3);Ring(75,76,6,0,6.283f);break;
    case Frost:for(int I=0;I<6;++I) {const float A=I*1.0472f;const float X=std::cos(A),Y=std::sin(A);Line({{50,50},{50+X*34,50+Y*34}},4);Line({{50+X*20-Y*9,50+Y*20+X*9},{50+X*27,50+Y*27},{50+X*20+Y*9,50+Y*20-X*9}},3);}break;
    case Bolt:Line({{55,13},{27,55},{47,55},{38,88},{77,40},{55,40},{55,13}},5);Line({{21,34},{13,43}},2,.5f);Line({{79,70},{87,61}},2,.5f);break;
    case Fork:Line({{24,78},{47,51},{47,25}},6);Line({{47,51},{73,37}},5);Line({{37,35},{47,23},{57,35}},4);Line({{59,34},{77,35},{72,51}},4);Line({{18,70},{31,83}},2,.5f);break;
    case Dash:Line({{36,24},{54,24},{76,49},{52,77},{33,77},{54,49},{36,24}},5);Line({{14,36},{33,36}},3,.65f);Line({{8,49},{29,49}},4,.8f);Line({{14,64},{33,64}},3,.65f);break;
    case Vortex:{Stroke S;S.Width=4;for(int I=0;I<100;++I){const float A=I*.14f,R=4+I*.31f;S.Points.push_back({50+std::cos(A)*R,50+std::sin(A)*R});}Out.push_back(S);break;}
    case Star:Line({{50,12},{59,38},{85,50},{59,61},{50,87},{40,61},{14,50},{40,38},{50,12}},4);Ring(50,50,10,0,6.283f);break;
    case Magazine:Line({{28,19},{68,19},{76,68},{67,85},{37,85},{28,19}},5);for(int I=0;I<4;++I) Line({{38,float(32+I*11)},{62,float(32+I*11)}},4,.8f);Line({{80,17},{80,33}},3);Line({{72,25},{88,25}},3);break;
    case Reload:Ring(50,50,30,-2.6f,2.2f);Line({{28,13},{25,31},{43,28}},5);Line({{64,68},{72,77},{59,84}},4);Line({{50,31},{50,51},{63,58}},4);break;
    case Precision:Ring(50,50,25,0,6.283f);Ring(50,50,5,0,6.283f);Line({{50,12},{50,33}},4);Line({{50,67},{50,88}},4);Line({{12,50},{33,50}},4);Line({{67,50},{88,50}},4);break;
    case Dice:Line({{21,20},{67,12},{78,61},{30,70},{21,20}},4);Line({{78,31},{88,76},{42,88},{38,73}},4);for(const Point P:std::vector<Point>{{35,32},{61,27},{40,55},{65,50},{50,41},{63,73},{76,65}})Ring(P.X,P.Y,2.7f,0,6.283f);break;
    case Heart:Line({{50,84},{22,59},{16,40},{25,24},{39,24},{50,36},{61,24},{75,24},{85,39},{80,58},{50,84}},4);Line({{21,51},{37,51},{44,41},{53,64},{61,49},{81,49}},3);break;
    case Bow:Line({{39,15},{67,31},{76,50},{67,70},{39,86}},5);Line({{39,15},{28,50},{39,86}},2);Line({{19,50},{88,50}},4);Line({{77,42},{88,50},{77,58}},3);break;
    default:break;
    }
    return Out;
}
}
