#pragma once
#include "navigation.hpp"
#include <algorithm>
#include <map>
#include <set>

namespace ffx::nav {
template<class Key> void orderEntities(std::vector<Entity>& found,const std::vector<Entity>& previous,Key key,bool held) {
    std::map<decltype(key(Entity{})),size_t> order;
    if(held) for(size_t i=0;i<previous.size();++i) order[key(previous[i])]=i;
    std::stable_sort(found.begin(),found.end(),[&](const Entity& a,const Entity& b) {
        const auto x=order.find(key(a)),y=order.find(key(b));
        if(x!=order.end() && y!=order.end()) return x->second<y->second;
        if(x!=order.end() || y!=order.end()) return x!=order.end();
        return a.distance<b.distance;
    });
}
// Every command rescans, which builds fresh rows. Carry each row's last verdict across by key
// until the next reachability result replaces it. A request reuses verdicts only when the context
// and every target's facts are identical; any difference (a push target's contact bounds follow
// the player's live body) started over with nothing hidden, so ] or [ landed on hidden rows.
template<class Key> void keepVerdicts(std::vector<Entity>& found,const std::vector<Entity>& previous,Key key) {
    std::set<decltype(key(Entity{}))> hidden;
    for(const auto& e:previous) if(e.unreachable) hidden.insert(key(e));
    for(auto& e:found) e.unreachable=hidden.count(key(e))!=0;
}
// Row to select when a reachability verdict hides the selected entity. The list keeps its order:
// verdicts are matched to targets by position, so reordering would drop them and the hidden row
// would come back. A negative step picks the visible row before the lost one, otherwise the row after.
template<class Key,class Lost,class Visible> int rowAfterLoss(const std::vector<Entity>& entities,const Lost& lost,Key key,Visible visible,int step) {
    unsigned before=0,count=0;bool passed=false;
    for(const auto& e:entities) if(key(e)==lost) passed=true;else if(visible(e)) {++count;if(!passed) ++before;}
    if(!count) return -1;
    return static_cast<int>(step<0?(before+count-1)%count:before%count);
}
}
