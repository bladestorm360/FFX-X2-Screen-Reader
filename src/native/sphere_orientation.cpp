// Describe the nearest branch in each direction without moving the pawn.
// Standard paths use recovered section depths; Expert/inter-path traversal uses
// breadth-first depth from an anchor. Native direction reporting remains shared
// with ordinary Grid guidance. See Docs/features/menus/index.md#sphere-grid.
#include "sphere_owned.hpp"
#include "navigation_cancel.hpp"
#include "events.hpp"
#include "sphere_grid.hpp"
#include "sphere_expert_data.hpp"
#include <queue>
namespace ffx {
std::wstring sphereBranchOrientation(const GridSnapshot& grid,int anchor) {
    if(grid.character<0 || grid.character>=7) return {};
    const auto owners=sphereSections(grid); const bool expert=sphereExpert(grid);
    if(owners.empty() && !expert) return L"Path orientation cannot resolve this grid layout.";
    const int at=grid.positions[grid.character];
    if(at<0 || static_cast<size_t>(at)>=grid.nodes.size()) return {};
    const int section=expert?0:owners[at];
    auto text=spherePlayerLabel(grid.character)+(expert?L" is on the Expert Sphere Grid. ":section<0?L" is between character paths. ":L" is on "+spherePlayerLabel(section)+L" path. ");
    std::vector<std::vector<int>> adj(grid.nodes.size());
    for(const auto& e:grid.links) if(e.a>=0 && e.b>=0 && grid.nodes[e.a].valid && grid.nodes[e.b].valid) {
        adj[e.a].push_back(e.b); adj[e.b].push_back(e.a);
    }
    std::vector<int> depth(grid.nodes.size(),-1);
    if(expert || section<0) {
        if(anchor<0 || static_cast<size_t>(anchor)>=grid.nodes.size() || !grid.nodes[anchor].valid)
            anchor=expert?sphereExpertStarts[grid.character]:sphereSectionStarts[grid.character];
        std::queue<int> pending; pending.push(anchor); depth[anchor]=0;
        while(!pending.empty()) { int here=pending.front(); pending.pop(); for(int next:adj[here]) if(depth[next]<0) { depth[next]=depth[here]+1; pending.push(next); } }
    } else for(size_t i=0;i<depth.size();++i) depth[i]=sphereSectionSites[i].depth;
    for(bool forward:{true,false}) {
        struct Step { int at,first; unsigned distance; };
        std::queue<Step> pending; pending.push({at,-1,0}); std::vector<bool> seen(grid.nodes.size(),false); seen[at]=true;
        bool found=false; Step end{at,-1,0};
        while(!pending.empty()) {
            const auto here=pending.front(); pending.pop(); end=here;
            std::vector<int> nextNodes;
            for(int next:adj[here.at]) {
                if(!expert && section>=0 && owners[next]!=section) continue;
                const bool ahead=depth[next]>depth[here.at] || (depth[next]==depth[here.at] && next>here.at);
                if(ahead==forward) nextNodes.push_back(next);
            }
            if(adj[here.at].size()>2 || nextNodes.size()>1) {
                text+=(forward?L"Nearest forward branch: ":L"Nearest backward branch: ")+sphereNodeLabel(grid,here.at)+L", "+std::to_wstring(here.distance)+L" connections away.";
                if(here.first>=0) text+=L" First: "+sphereNextText(grid,GridRoute{{at,here.first}})+L".";
                std::vector<std::wstring> branches;
                for(int next:adj[here.at]) {
                    auto option=sphereNextText(grid,GridRoute{{here.at,next}});
                    if(grid.nodes[next].locked) option+=L", locked, requires 1 "+sphereItemLabel(grid.nodes[next].sphere);
                    branches.push_back(option);
                }
                text+=L" Connections: "+join(branches,L". ")+L". "; found=true; break;
            }
            for(int next:nextNodes) if(!seen[next]) { seen[next]=true; pending.push({next,here.first<0?next:here.first,here.distance+1}); }
        }
        if(!found) text+=(forward?L"No forward branch before the path ends":L"No backward branch before the path starts")+std::wstring(L", ")+std::to_wstring(end.distance)+L" connections away. ";
    }
    return text;
}
}
