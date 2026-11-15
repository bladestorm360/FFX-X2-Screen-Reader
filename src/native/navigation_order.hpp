#pragma once
#include "navigation.hpp"
#include <algorithm>
#include <cmath>
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
// Index of the selected entity among the visible rows, or -1 when it is hidden or gone.
// The selected row index counts visible rows only, so it must be found again whenever
// visibility changes (Hide unreachable toggled) or "N of M" names the wrong row.
template<class Key,class Selection,class Visible> int visibleRowOf(const std::vector<Entity>& entities,const Selection& selection,Key key,Visible visible) {
    unsigned row=0;
    for(const auto& e:entities) if(visible(e)) {if(key(e)==selection) return static_cast<int>(row);++row;}
    return -1;
}
// While Hide unreachable waits for its first verdicts every row is still visible, so a spoken
// "N of M" would give the unfiltered count and change on the next press. Omit it until settled.
inline bool rowCountSettled(bool hideUnreachable,bool checking) {return !hideUnreachable || !checking;}
// After a room change the destination set fills in over several scans, so early totals
// ("2 of 11", "5 of 23") are wrong. A total is spoken once the scanned destination count has
// held for ListSettleMs in the same map generation. Every ] or [ rescans, and a rescan can count
// one destination more or less or restart the reachability check; once the room has settled, that
// must not take the totals away again, so settling is latched per map generation and the current
// count is spoken. Turning Hide unreachable on, by U or the mod menu, waits for its first check again.
constexpr unsigned long long ListSettleMs=1500;
// The Destruction Sphere area in Djose lists several "Sphere ... currently inactive" rows at one
// spot: separate script workers for the same dormant object. An inactive row with the same name
// and category as an earlier inactive row within half a unit (and two units of height) adds
// nothing to walk to, so only the first is kept. Active rows and pedestals are never merged.
inline void collapseInactiveDuplicates(std::vector<Entity>& entities) {
    std::vector<Entity> kept;kept.reserve(entities.size());
    for(auto& e:entities) {
        const bool duplicate=e.inactive && !e.pushControl && std::any_of(kept.begin(),kept.end(),[&](const Entity& k) {
            return k.inactive && !k.pushControl && k.name==e.name && k.category==e.category &&
                std::hypot(k.position.x-e.position.x,k.position.z-e.position.z)<=.5 && std::abs(k.position.y-e.position.y)<=2;
        });
        if(!duplicate) kept.push_back(std::move(e));
    }
    entities=std::move(kept);
}
// Whether losing the selected row (hidden as unreachable, or gone from a rescan) is handled and
// spoken now. While a scene, window or scripted animation holds the field (a pedestal dropping,
// a lift riding), verdicts describe a moving player, so the selection is kept and nothing is said;
// the first command after control returns rescans and reports any real loss.
inline bool selectionLossSpoken(bool fieldOpen,bool control) {return fieldOpen && control;}
struct ListStability {
    unsigned epoch=0;size_t count=0;unsigned long long since=0;bool counted=false,checked=false,hidden=false;
    void observe(unsigned scanEpoch,size_t found,unsigned long long at) {
        if(since && scanEpoch==epoch && settled(at)) counted=true;
        if(!since || scanEpoch!=epoch) {epoch=scanEpoch;count=found;since=at;counted=checked=false;}
        else if(found!=count) {count=found;since=at;}
    }
    bool settled(unsigned long long at) const {return since && at>=since && at-since>=ListSettleMs;}
    bool totals(unsigned long long at,bool hideUnreachable,bool checking) {
        if(settled(at)) counted=true;
        if(hideUnreachable!=hidden) {hidden=hideUnreachable;checked=false;}
        if(rowCountSettled(hideUnreachable,checking)) checked=true;
        return counted && (!hideUnreachable || checked);
    }
};
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
