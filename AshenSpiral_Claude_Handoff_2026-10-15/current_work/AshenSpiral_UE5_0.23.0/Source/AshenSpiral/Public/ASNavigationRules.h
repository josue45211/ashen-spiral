#pragma once
#include "ASRules.h"
#include <limits>
namespace ASNavigation {
struct Link {int A=-1,B=-1;float Cost=0;bool Jump=true;};
struct Endpoint {int Node=-1;float Cost=0;};
// Virtual source/goal edges let the search choose reachable approaches, rather
// than snapping either end through an obstacle to an unrelated nearest node.
inline std::vector<int> GuidePath(const ASRules::NavGrid& Grid,const std::vector<Endpoint>& Starts,const std::vector<Endpoint>& Goals,const std::vector<Link>& Links) {
    const float Infinity=std::numeric_limits<float>::infinity();
    std::vector<float> Cost(Grid.Open.size(),Infinity),EndCost(Grid.Open.size(),Infinity);
    std::vector<int> Parent(Grid.Open.size(),-1);
    using Entry=std::pair<float,int>;std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> Queue;
    for(auto G:Goals) if(Grid.Valid(G.Node)&&std::isfinite(G.Cost)&&G.Cost>=0) EndCost[G.Node]=std::min(EndCost[G.Node],G.Cost);
    for(auto S:Starts) if(Grid.Valid(S.Node)&&std::isfinite(S.Cost)&&S.Cost>=0&&S.Cost<Cost[S.Node]) {Cost[S.Node]=S.Cost;Parent[S.Node]=S.Node;Queue.push({S.Cost,S.Node});}
    int Best=-1;float BestCost=Infinity;
    while(!Queue.empty()) {
        const auto Item=Queue.top();Queue.pop();const int At=Item.second;
        if(Item.first>Cost[At]) continue;
        if(Item.first>=BestCost) break;
        if(Cost[At]+EndCost[At]<BestCost) {Best=At;BestCost=Cost[At]+EndCost[At];}
        const auto Visit=[&](int Next,float Distance) {
            if(!Grid.Valid(Next)||!std::isfinite(Distance)||Distance<=0||Cost[Next]<=Cost[At]+Distance) return;
            Cost[Next]=Cost[At]+Distance;Parent[Next]=At;Queue.push({Cost[Next],Next});
        };
        for(int Next:{At-1,At+1,At-Grid.Width,At+Grid.Width}) if(Grid.CanStep(At,Next)) Visit(Next,1.f);
        for(const auto& L:Links) {if(L.A==At) Visit(L.B,L.Cost);else if(L.B==At) Visit(L.A,L.Cost);}
    }
    if(Best<0) return {};
    std::vector<int> Result{Best};while(Parent[Result.back()]!=Result.back()) Result.push_back(Parent[Result.back()]);
    std::reverse(Result.begin(),Result.end());return Result;
}
inline std::vector<int> GuidePath(const ASRules::NavGrid& Grid,int Start,int Goal,const std::vector<Link>& Links) {
    return GuidePath(Grid,std::vector<Endpoint>{{Start,0}},std::vector<Endpoint>{{Goal,0}},Links);
}
inline bool IsJump(const std::vector<Link>& Links,int A,int B) {
    for(const auto& L:Links) if(L.Jump&&((L.A==A&&L.B==B)||(L.B==A&&L.A==B))) return true;
    return false;
}
}
