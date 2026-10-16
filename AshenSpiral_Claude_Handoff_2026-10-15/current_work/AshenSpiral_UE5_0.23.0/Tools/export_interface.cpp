// Optional source-preview exporter. Build with g++ -std=c++17 Tools/export_interface.cpp -o export_interface
#include "../Source/AshenSpiral/Public/ASEffectArt.h"
#include "../Source/AshenSpiral/Public/ASArsenalRules.h"
#include <iostream>
int main() {
    std::cout<<"{\"icons\":[";
    for(int K=0;K<16;++K) {if(K)std::cout<<',';std::cout<<'[';int J=0;
        for(const auto& S:ASEffectArt::Draw(K)) {if(J++)std::cout<<',';std::cout<<"{\"width\":"<<S.Width<<",\"alpha\":"<<S.Alpha<<",\"points\":[";int I=0;
            for(const auto& P:S.Points) {if(I++)std::cout<<',';std::cout<<'['<<P.X<<','<<P.Y<<']';}std::cout<<"]}";
        }std::cout<<']';
    }
    std::cout<<"],\"weapons\":[";
    for(int W=0;W<9;++W) {if(W)std::cout<<',';
        const float Damage=ASArsenal::Damage(W)*ASArsenal::Pellets(W);const int Mag=ASArsenal::BaseCapacity(W);
        std::cout<<"{\"weapon\":"<<W<<",\"capacity\":"<<Mag<<",\"capacity_rank_5\":"<<ASArsenal::Capacity(W,5)<<",\"reload\":"<<ASArsenal::ReloadSeconds(W)<<",\"damage\":"<<Damage<<",\"rate\":"<<ASArsenal::Rate(W)<<",\"sustained_dps\":"<<Damage*Mag/(Mag*ASArsenal::Rate(W)+ASArsenal::ReloadSeconds(W))<<'}';
    }
    std::cout<<"]}\n";
}
