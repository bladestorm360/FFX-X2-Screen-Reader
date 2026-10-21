#include "sphere_owned.hpp"
#include "navigation_cancel.hpp"
#include "events.hpp"
#include "sphere_grid.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <queue>
#include <cmath>
namespace ffx {
namespace {
std::wstring locationDirection(const GridNode& a,const GridNode& b) {
    const int dx=b.x-a.x,dy=b.y-a.y;
    if(!dx && !dy) return L"same grid coordinates";
    if(!dx) return dy<0?L"up":L"down";
    if(!dy) return dx<0?L"left":L"right";
    return std::wstring(L"diagonally ")+(dy<0?L"up":L"down")+L" and "+(dx<0?L"left":L"right");
}
}
std::array<int,8> sphereArrowChoices(const GridSnapshot& grid,int from) {
    // A56BD0 accepts both arrow axes together. A56860 selects the largest
    // dot(input, offset)/distance^2, not the nearest rounded compass bearing.
    // Use the unrestricted browsing selector (A45010); future route nodes
    // cannot be filtered by the current Move/Use CAN_TARGET flags (A45000).
    struct Arrow { int x,y; const wchar_t* text; };
    const Arrow arrows[]={{0,-1,L"up"},{1,0,L"right"},{0,1,L"down"},{-1,0,L"left"},
        {-1,-1,L"diagonally up and left"},{1,-1,L"diagonally up and right"},
        {-1,1,L"diagonally down and left"},{1,1,L"diagonally down and right"}};
    std::array<int,8> choices; choices.fill(-1);
    std::array<float,8> best{};
    const auto& origin=grid.nodes[from];
    for(size_t i=0;i<grid.nodes.size();++i) {
        const auto& node=grid.nodes[i]; if(!node.valid) continue;
        const float dx=static_cast<float>(node.x-origin.x),dy=static_cast<float>(node.y-origin.y);
        const float distance=static_cast<float>(std::sqrt(static_cast<double>(dx)*dx+static_cast<double>(dy)*dy));
        if(!distance) continue;
        for(size_t key=0;key<best.size();++key) {
            const float dot=dx*arrows[key].x+dy*arrows[key].y;
            if(dot<=0.5f) continue;
            const float score=static_cast<float>(static_cast<double>(dot)/(static_cast<double>(distance)*distance));
            // The original strict comparison keeps the first node on ties.
            if(score>best[key]) { best[key]=score; choices[key]=static_cast<int>(i); }
        }
    }
    return choices;
}
namespace {
std::wstring inputDirection(const GridSnapshot& grid,int from,int to) {
    const auto choices=sphereArrowChoices(grid,from);
    const wchar_t* wordsForKey[]={L"up",L"right",L"down",L"left",L"diagonally up and left",L"diagonally up and right",L"diagonally down and left",L"diagonally down and right"};
    std::vector<std::wstring> words;
    for(size_t key=0;key<choices.size();++key) {
        if(key==4 && !words.empty()) break; // Prefer any valid single arrow.
        if(choices[key]==to) words.push_back(wordsForKey[key]);
    }
    return join(words,L" or ");
}
unsigned edgeCost(const GridSnapshot& grid,int a,int b,int character) {
    for(const auto& link:grid.links) if((link.a==a && link.b==b) || (link.a==b && link.b==a))
        return link.travelled&(1u<<character)?1:4;
    return 0;
}
}
std::wstring sphereNodeLabel(const GridSnapshot& grid,int index) {
    if(index<0 || static_cast<size_t>(index)>=grid.nodes.size()) return L"Unknown node";
    return (grid.nodes[index].name.empty()?L"Unnamed node":grid.nodes[index].name)+L", node "+std::to_wstring(index+1);
}
GridRoute sphereRoute(const GridSnapshot& grid,int start,int goal,int character) {
    GridRoute result;
    if(character<0 || character>=7 || start<0 || goal<0 || static_cast<size_t>(start)>=grid.nodes.size() ||
        static_cast<size_t>(goal)>=grid.nodes.size() || !grid.nodes[start].valid || !grid.nodes[goal].valid) return result;
    using Entry=std::pair<unsigned,int>;
    std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> pending;
    const auto infinity=std::numeric_limits<unsigned>::max();
    std::vector<unsigned> cost(grid.nodes.size(),infinity);
    std::vector<int> parent(grid.nodes.size(),-1);
    cost[start]=0; pending.push({0,start});
    while(!pending.empty()) {
        nav::checkComputation();
        const auto [value,at]=pending.top(); pending.pop();
        if(value!=cost[at]) continue;
        if(at==goal) break;
        for(const auto& edge:grid.links) {
            const int next=edge.a==at?edge.b:edge.b==at?edge.a:-1;
            if(next<0 || static_cast<size_t>(next)>=grid.nodes.size() || !grid.nodes[next].valid) continue;
            const unsigned movement=next==goal && grid.nodes[goal].ability?0:(edge.travelled&(1u<<character)?1:4);
            const unsigned candidate=value+movement+(grid.nodes[next].locked?4097:0);
            if(candidate<cost[next]) { cost[next]=candidate; parent[next]=at; pending.push({candidate,next}); }
        }
    }
    if(cost[goal]==infinity) return result;
    for(int at=goal;at!=-1;at=parent[at]) { result.nodes.push_back(at); if(at==start) break; }
    if(result.nodes.back()!=start) return {};
    std::reverse(result.nodes.begin(),result.nodes.end());
    sphereRouteCosts(grid,result,character);
    return result;
}
void sphereRouteCosts(const GridSnapshot& grid,GridRoute& result,int character,bool standOnGoal) {
    if(result.nodes.empty()) return;
    const int goal=result.nodes.back();
    result.quarters=result.levels=result.standingLevels=result.locks=0;
    unsigned segment=0,standingSegment=0;
    for(size_t i=0;i<result.nodes.size();++i) {
        if(grid.nodes[result.nodes[i]].locked) {
            ++result.locks; result.levels+=(segment+3)/4; result.standingLevels+=(standingSegment+3)/4;
            segment=standingSegment=0;
        }
        if(i) {
            const auto quarters=edgeCost(grid,result.nodes[i-1],result.nodes[i],character);
            standingSegment+=quarters;
            if(i+1<result.nodes.size() || standOnGoal || !grid.nodes[goal].ability) { result.quarters+=quarters; segment+=quarters; }
        }
    }
    result.levels+=(segment+3)/4;
    result.standingLevels+=(standingSegment+3)/4;
}
std::wstring sphereRouteText(const GridSnapshot& grid,const GridRoute& route,int character) {
    if(route.nodes.empty()) return L"No connected path.";
    std::wstring text=std::to_wstring(route.nodes.size()-1)+L" connections. ";
    std::map<unsigned,unsigned> materials;
    bool unknown=false;
    // Count every still-unactivated node on the route, grouped by its actual
    // native activation sphere. These are totals to activate the whole route;
    // movement itself only needs locks cleared, not every stat purchased.
    for(int index:route.nodes) {
        const auto& node=grid.nodes[index];
        if(!node.locked && (node.activated&(1u<<character))) continue;
        if(node.sphere) ++materials[node.sphere];
        else if(node.locked || node.ability || node.flags) unknown=true;
    }
    std::vector<std::wstring> counts;
    for(const auto& [id,count]:materials) counts.push_back(std::to_wstring(count)+L" "+sphereItemLabel(id));
    text+=counts.empty()?L"0 spheres":join(counts);
    if(unknown) text+=L", unresolved sphere types";
    return text+L". "+std::to_wstring(route.levels)+L" Sphere Levels.";
}
std::wstring sphereNextText(const GridSnapshot& grid,const GridRoute& route) {
    if(route.nodes.empty()) return L"No connected path.";
    if(route.nodes.size()==1) return sphereNodeLabel(grid,route.nodes[0]);
    const int from=route.nodes[0],to=route.nodes[1];
    const auto direction=inputDirection(grid,from,to);
    if(!direction.empty()) return direction+L" to "+sphereNodeLabel(grid,to);
    // Crowded layouts can need several cursor selections. Report position
    // without presenting an arrow combination that selects a different node.
    return sphereNodeLabel(grid,to)+L" lies "+locationDirection(grid.nodes[from],grid.nodes[to])+L" from this node. No direct arrow combination selects it. F6 selects a node by number.";
}
std::wstring sphereOrientation(const GridSnapshot& grid) {
    if(grid.selected<0 || static_cast<size_t>(grid.selected)>=grid.nodes.size()) return {};
    const auto& at=grid.nodes[grid.selected];
    auto text=spherePlayerLabel(grid.character)+L": highlighting "+sphereNodeLabel(grid,grid.selected)+L". Character position: "+
        sphereNodeLabel(grid,grid.positions[grid.character])+L". Grid coordinates "+std::to_wstring(at.x)+L", "+std::to_wstring(at.y)+L". ";
    std::vector<std::wstring> neighbours;
    for(const auto& edge:grid.links) {
        const int next=edge.a==grid.selected?edge.b:edge.b==grid.selected?edge.a:-1;
        if(next<0 || static_cast<size_t>(next)>=grid.nodes.size()) continue;
        auto direction=inputDirection(grid,grid.selected,next);
        if(direction.empty()) direction=L"position "+locationDirection(at,grid.nodes[next])+L"; use F6";
        auto label=direction+L": "+sphereNodeLabel(grid,next);
        if(grid.nodes[next].locked) label+=L", locked";
        label+=edge.travelled&(1u<<grid.character)?L", previously travelled":L", new link";
        neighbours.push_back(label);
    }
    text+=std::to_wstring(neighbours.size())+L" connections. "+join(neighbours,L". ");
    return text+(sphereExpert(grid)?L". F2 chooses a stat focus. F4 and F5 guide forward and backward through branches.":
        L". F2 chooses a character path. F4 and F5 guide forward and backward on the section you occupy.")+
        L" F3 chooses an ability. Backslash reads the chosen route.";
}
}
