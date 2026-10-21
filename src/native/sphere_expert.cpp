// Expert Grid guidance over an owned, fully matched site/edge layout. Depth from
// the character's verified start/selected anchor gives deterministic forward and
// backward branches; it does not infer native directions from coarse angles.
// See Docs/features/menus/index.md#sphere-grid.
#include "sphere_owned.hpp"
#include "navigation_cancel.hpp"
#include "events.hpp"
#include "sphere_grid.hpp"
#include "sphere_expert_data.hpp"
#include <algorithm>
#include <queue>
namespace ffx {
bool sphereExpert(const GridSnapshot& grid) {
    if(grid.nodes.size()!=sphereExpertSites.size() || grid.links.size()!=sphereExpertEdges.size()) return false;
    for(size_t i=0;i<grid.nodes.size();++i) {
        const auto site=sphereExpertSites[i]; const auto& node=grid.nodes[i];
        if(node.x!=site.x || node.y!=site.y || node.valid!=site.present) return false;
    }
    std::vector<std::pair<int,int>> actual,expected;
    for(const auto& e:grid.links) actual.push_back(std::minmax(e.a,e.b));
    for(const auto& e:sphereExpertEdges) expected.push_back(std::minmax(static_cast<int>(e.a),static_cast<int>(e.b)));
    std::sort(actual.begin(),actual.end()); std::sort(expected.begin(),expected.end());
    return actual==expected;
}
std::wstring sphereExpertStep(const GridSnapshot& grid,bool forward,int destination,int anchor,int first) {
    if(!sphereExpert(grid)) return L"This Sphere Grid layout is not supported by path guidance.";
    if(grid.character<0 || grid.character>=7) return {};
    const int at=grid.selected;
    if(at<0 || static_cast<size_t>(at)>=grid.nodes.size() || !grid.nodes[at].valid) return {};
    if(anchor<0 || static_cast<size_t>(anchor)>=grid.nodes.size() || !grid.nodes[anchor].valid) anchor=sphereExpertStarts[grid.character];
    std::vector<std::vector<int>> adj(grid.nodes.size());
    for(const auto& e:grid.links) if(e.a>=0 && e.b>=0 && grid.nodes[e.a].valid && grid.nodes[e.b].valid) {
        adj[e.a].push_back(e.b); adj[e.b].push_back(e.a);
    }
    std::vector<int> depth(grid.nodes.size(),-1); depth[anchor]=0;
    std::queue<int> pending; pending.push(anchor);
    while(!pending.empty()) {
        const int here=pending.front(); pending.pop();
        for(int next:adj[here]) if(depth[next]<0) { depth[next]=depth[here]+1; pending.push(next); }
    }
    std::wstring text=L"Expert Sphere Grid. "; int preferred=-1;
    if(forward && destination>=0 && static_cast<size_t>(destination)<grid.nodes.size()) {
        if(first>=0 && std::find(adj[at].begin(),adj[at].end(),first)!=adj[at].end()) {
            preferred=first; text+=L"Toward "+sphereNodeLabel(grid,destination)+L": "+sphereNextText(grid,GridRoute{{at,first}})+L". ";
        } else if(at==destination) text+=L"Destination reached. ";
    }
    std::vector<std::wstring> choices;
    for(int next:adj[at]) {
        if(next==preferred) continue;
        // Equal-depth links need one stable direction to avoid calling both
        // ends forward; the node-number tie break does not alter native inputs.
        const bool ahead=depth[next]>depth[at] || (depth[next]==depth[at] && next>at);
        if(ahead!=forward) continue;
        auto choice=sphereNextText(grid,GridRoute{{at,next}});
        if(grid.nodes[next].locked) choice+=L", locked, requires 1 "+sphereItemLabel(grid.nodes[next].sphere);
        choices.push_back(choice);
    }
    if(!forward && at==anchor) return text+L"Start of this path.";
    if(choices.empty()) {
        if(preferred>=0) return text;
        return text+(forward && adj[at].size()>1?L"This branch rejoins earlier nodes. F5 gives the backward connections.":
            forward?L"End of this path branch.":L"No backward connection on this path.");
    }
    return text+(preferred>=0?L"Other forward branches: ":forward?L"Forward branches: ":L"Backward branches: ")+join(choices,L". ")+L".";
}
}
