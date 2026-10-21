#include "sphere_owned.hpp"
#include "navigation_cancel.hpp"
#include "events.hpp"
#include "sphere_grid.hpp"
#include "sphere_sections_data.hpp"
#include <algorithm>
#include <map>
#include <queue>
namespace ffx {
std::vector<int> sphereSections(const GridSnapshot& grid) {
    if(grid.nodes.size()!=sphereSectionSites.size() || grid.links.size()!=sphereSectionEdges.size()) return {};
    std::vector<int> sections;
    for(size_t i=0;i<grid.nodes.size();++i) {
        const auto& site=sphereSectionSites[i]; const auto& node=grid.nodes[i];
        if(!node.valid || node.x!=site.x || node.y!=site.y) return {};
        sections.push_back(site.section);
    }
    std::vector<std::pair<int,int>> actual,expected;
    for(const auto& e:grid.links) actual.push_back(std::minmax(e.a,e.b));
    for(const auto& e:sphereSectionEdges) expected.push_back(std::minmax(static_cast<int>(e.a),static_cast<int>(e.b)));
    std::sort(actual.begin(),actual.end()); std::sort(expected.begin(),expected.end());
    return actual==expected?sections:std::vector<int>{};
}
std::wstring sphereSectionStep(const GridSnapshot& grid,bool forward,int section) {
    const auto sections=sphereSections(grid);
    if(sections.empty()) return L"Character paths require the standard Sphere Grid layout.";
    if(grid.character<0 || grid.character>=7) return {};
    const int at=grid.selected,pawn=grid.positions[grid.character];
    if(at<0 || static_cast<size_t>(at)>=sections.size() || pawn<0 || static_cast<size_t>(pawn)>=sections.size()) return {};
    if(section<0) section=sections[pawn];
    if(section<0) return L"Shared connector between character paths. Use F2 to select a path.";
    const auto pathName=spherePlayerLabel(section)+L" path";
    std::vector<std::wstring> choices;
    unsigned degree=0;
    for(const auto& edge:grid.links) {
        const int next=edge.a==at?edge.b:edge.b==at?edge.a:-1;
        if(next<0 || sections[next]!=section) continue;
        ++degree;
        const auto here=sphereSectionSites[at].depth,there=sphereSectionSites[next].depth;
        const bool ahead=there>here || (there==here && next>at);
        if(ahead!=forward) continue;
        auto text=sphereNextText(grid,GridRoute{{at,next}});
        if(grid.nodes[next].locked) text+=L", locked, requires 1 "+sphereItemLabel(grid.nodes[next].sphere);
        choices.push_back(text);
    }
    const int terminals[]={377,444,634,-1,212,333,527};
    if(!forward && at==sphereSectionStarts[section]) return L"Start of "+pathName+L".";
    if(choices.empty()) {
        if(forward && at==terminals[section]) return L"End of "+pathName+L".";
        if(forward && degree>1) return pathName+L". This branch rejoins earlier nodes. Use F5 for its backward connections.";
        return (forward?L"End of this branch of ":L"Start of this branch of ")+pathName+L".";
    }
    auto prefix=pathName+L". "+(forward?L"Forward":L"Backward");
    if(forward && at==terminals[section]) prefix=L"End of "+pathName+L". Side branch";
    return prefix+(choices.size()>1?L" branches: ":L": ")+join(choices,L". ")+L".";
}
namespace {
struct Label { int at,parent; unsigned cost; std::array<unsigned,4> spent{}; bool live=true; };
bool dominates(const Label& a,const Label& b) {
    if(a.cost>b.cost) return false;
    for(size_t i=0;i<a.spent.size();++i) if(a.spent[i]>b.spent[i]) return false;
    return true;
}
SectionRoute search(const GridSnapshot& grid,const std::vector<int>& sections,int section,
    const std::map<unsigned,unsigned>& inventory,bool limited,bool activation=false) {
    SectionRoute result;
    if(grid.character<0 || grid.character>=7 || sections.size()!=grid.nodes.size()) return result;
    const int start=grid.positions[grid.character];
    if(start<0 || static_cast<size_t>(start)>=grid.nodes.size()) return result;
    std::map<unsigned,unsigned> keys;
    for(const auto& node:grid.nodes) if(node.locked && node.sphere && !keys.count(node.sphere)) {
        if(keys.size()==4) { result.complete=false; return result; }
        keys.emplace(node.sphere,static_cast<unsigned>(keys.size()));
    }
    std::array<unsigned,4> stock{};
    for(const auto& [id,index]:keys) {
        const auto item=inventory.find(id);
        stock[index]=limited?(item==inventory.end()?0:item->second):static_cast<unsigned>(grid.nodes.size());
    }
    std::vector<std::vector<std::pair<int,unsigned>>> adjacent(grid.nodes.size());
    for(const auto& e:grid.links) if(e.a>=0 && e.b>=0 && static_cast<size_t>(std::max(e.a,e.b))<grid.nodes.size()) {
        const unsigned cost=e.travelled&(1u<<grid.character)?1:4;
        adjacent[e.a].push_back({e.b,cost}); adjacent[e.b].push_back({e.a,cost});
    }
    std::vector<Label> labels{{start,-1,0}};
    std::vector<std::vector<int>> visited(grid.nodes.size()); visited[start].push_back(0);
    using Entry=std::pair<unsigned,int>;
    std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> pending; pending.push({0,0});
    while(!pending.empty()) {
        nav::checkComputation();
        const int index=pending.top().second; pending.pop();
        const auto current=labels[index]; if(!current.live) continue;
        if(sections[current.at]==section && !grid.nodes[current.at].locked) {
            for(int i=index;i>=0;i=labels[i].parent) result.route.nodes.push_back(labels[i].at);
            std::reverse(result.route.nodes.begin(),result.route.nodes.end());
            sphereRouteCosts(grid,result.route,grid.character,!activation); result.reachable=limited; return result;
        }
        for(const auto& [next,cost]:adjacent[current.at]) {
            if(!grid.nodes[next].valid) continue;
            const unsigned movement=activation && sections[next]==section?0:cost;
            // Unlocking splits movement into separate commits. Round the
            // previous segment exactly as the game does before moving on.
            const unsigned committed=grid.nodes[next].locked?(current.cost+3)/4*4:current.cost;
            Label candidate{next,index,committed+movement,current.spent};
            if(grid.nodes[next].locked) {
                const auto key=keys.find(grid.nodes[next].sphere);
                if(key==keys.end() || ++candidate.spent[key->second]>stock[key->second]) continue;
            }
            bool dominated=false;
            for(int old:visited[next]) if(labels[old].live && dominates(labels[old],candidate)) { dominated=true; break; }
            if(dominated) continue;
            for(int old:visited[next]) if(labels[old].live && dominates(candidate,labels[old])) labels[old].live=false;
            if(labels.size()>=100000) { result.complete=false; return result; }
            const int id=static_cast<int>(labels.size()); labels.push_back(candidate); visited[next].push_back(id);
            pending.push({candidate.cost,id});
        }
    }
    return result;
}
}
SectionRoute sphereNearestSection(const GridSnapshot& grid,const std::vector<int>& sections,int section,
    const std::map<unsigned,unsigned>& inventory) {
    if(section<0 || section>=7) return {};
    auto result=search(grid,sections,section,inventory,true);
    if(result.complete && result.route.nodes.empty()) result=search(grid,sections,section,inventory,false);
    return result;
}
SectionRoute sphereAbilityRoute(const GridSnapshot& grid,int goal,const std::map<unsigned,unsigned>& inventory) {
    if(goal<0 || static_cast<size_t>(goal)>=grid.nodes.size() || !grid.nodes[goal].valid || !grid.nodes[goal].ability) return {};
    std::vector<int> goals(grid.nodes.size(),-1); goals[goal]=0;
    auto result=search(grid,goals,0,inventory,true,true);
    if(result.complete && result.route.nodes.empty()) result=search(grid,goals,0,inventory,false,true);
    return result;
}
void sphereSectionKey(bool backward,bool fromCursor) {
    if(queueSphereWork(SphereWork::Section,backward,0,fromCursor)) return;
    GridSnapshot grid;
    if(!sphereSnapshot(grid,true)) { sphereEmit(L"Open the Sphere Grid for character path guidance.",true,"sphere-section"); return; }
    if(grid.character<0 || grid.character>=7) return;
    if(!fromCursor) grid.selected=grid.positions[grid.character];
    sphereEmit(sphereExpert(grid)?sphereExpertGuidance(grid,!backward):sphereStandardGuidance(grid,!backward),true,"sphere-section");
}
}
