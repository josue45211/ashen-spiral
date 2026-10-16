// Optional source preview: g++ -std=c++17 Tools/export_layouts.cpp -o export_layouts
// Run export_layouts > Docs/layout_samples.json. Does not use Unreal.
#include "../Source/AshenSpiral/Public/ASRules.h"
#include <iostream>
#include <set>
int main() {
    std::set<int> Seen;std::cout<<"[";
    for(unsigned Seed=1;Seen.size()<5;++Seed) {
        const auto L=ASRules::Generate(Seed);if(Seen.count(L.Shape)) continue;
        if(!Seen.empty()) std::cout<<",";Seen.insert(L.Shape);
        std::cout<<"{\"seed\":"<<Seed<<",\"shape\":"<<L.Shape<<",\"exit\":"<<L.Exit<<",\"rooms\":[";
        for(std::size_t I=0;I<L.Cells.size();++I) {
            const auto& C=L.Cells[I];if(I) std::cout<<",";
            std::cout<<"["<<C.WorldX<<","<<C.WorldY<<","<<C.HalfX<<","<<C.HalfY<<","<<C.SpanX<<","<<C.SpanY<<"]";
        }
        std::cout<<"],\"edges\":[";
        for(std::size_t I=0;I<L.Edges.size();++I) {const auto& E=L.Edges[I];if(I) std::cout<<",";std::cout<<"["<<E.A<<","<<E.B<<","<<(E.Gap?1:0)<<"]";}
        std::cout<<"]}";
    }
    std::cout<<"]\n";
}
