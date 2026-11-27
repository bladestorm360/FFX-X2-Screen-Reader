#pragma once
#include "navigation.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <tuple>

namespace ffx::nav {
template<class Key> void orderEntities(std::vector<Entity>& found,const std::vector<Entity>& previous,Key key,bool held) {
    std::map<decltype(key(Entity{})),size_t> order;
    if(held) for(size_t i=0;i<previous.size();++i) order.insert_or_assign(key(previous[i]),i);
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
template<class Key> void keepVerdicts(std::vector<Entity>& found,const std::vector<Entity>& previous,Key key,bool sameRoomState=true) {
    // After a room state change the old verdicts are stale; rows stay listed until checked again.
    if(!sameRoomState) {for(auto& e:found) e.unreachable=false;return;}
    std::set<decltype(key(Entity{}))> hidden;
    for(const auto& e:previous) if(e.unreachable) hidden.insert(key(e));
    for(auto& e:found) e.unreachable=hidden.count(key(e))!=0;
}
// Hide unreachable trusts a background check over the modelled walk mesh. In Besaid that check
// hid trial recesses, a glyph, the chest and the pedestal that Auto walk then reached, so rows
// the player has to interact with (authored trial controls, pedestals, chests, save spheres)
// stay listed and are only described as possibly unreachable. Other rows are still hidden.
inline bool interactionRow(const Entity& e) {return e.pushControl || e.authoredTrialControl || e.category==L"Treasure Chest" || e.category==L"Save Sphere";}
// The puzzle guide's current target is never hidden either: the guide sends the player there.
inline bool hiddenAsUnreachable(const Entity& e,bool hideUnreachable,bool guideTarget=false) {return hideUnreachable && e.unreachable && !guideTarget && !interactionRow(e);}
// A row is the guide step's target when it comes from the step's worker in the guide's script
// part, and is a push row exactly when the step pushes (the guide's own target lookup rule).
inline bool guideStepRow(unsigned part,unsigned local,bool pushRow,unsigned stepPart,unsigned stepLocal,bool pushStep) {return part==stepPart && local==stepLocal && pushRow==pushStep;}
inline const wchar_t* reachabilityNote(const Entity& e,bool hideUnreachable) {return hideUnreachable && e.unreachable?L", may be unreachable":L"";}
// Room state for reachability: the trial script values captured for each part (sphere inserted,
// wall destroyed, door opened). Walls and doors that open mid-trial change what is reachable
// without changing the map, floor or collision gates, so cached verdicts must not outlive them.
template<class Parts> unsigned roomStateToken(const Parts& parts,unsigned count) {
    unsigned hash=2166136261u;const auto mix=[&](unsigned value) {for(unsigned i=0;i<4;++i) {hash^=(value>>(i*8))&0xffu;hash*=16777619u;}};
    for(unsigned part=0;part<count && part<parts.size();++part) {
        const auto& p=parts[part];if(!p.bindingsReady) continue;mix(part);
        for(unsigned i=0;i<p.bindings.count && i<p.bindings.rows.size();++i) {const auto& row=p.bindings.rows[i];mix(row.binding);mix(row.high);mix(row.element);mix(static_cast<unsigned>(row.value));}
    }
    return hash;
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
// Only repeated observations of the same dormant native object are redundant.
// Names or positions can coincide for distinct script workers.
inline void collapseInactiveDuplicates(std::vector<Entity>& entities) {
    std::set<std::tuple<int,int,unsigned,unsigned,std::wstring>> seen;
    entities.erase(std::remove_if(entities.begin(),entities.end(),[&](const Entity& e) {
        return e.inactive && !e.pushControl && (e.worker>=0 || e.actor>=0) &&
            !seen.emplace(e.worker,e.actor,e.id,e.pushLocal,e.category).second;
    }),entities.end());
}
// Whether losing the selected row (hidden as unreachable, or gone from a rescan) is handled and
// spoken now. While a scene, window or scripted animation holds the field (a pedestal dropping,
// a lift riding), verdicts describe a moving player, so the selection is kept and nothing is said;
// the first command after control returns rescans and reports any real loss.
inline bool selectionLossSpoken(bool fieldOpen,bool control) {return fieldOpen && control;}
// Whether a rescan may start a new reachability check. A choice prompt freezes the player, and
// a check taken then hid 11 of 19 Djose rows until the prompt closed. While a message, scene or
// scripted freeze holds the field, the rows keep their last verdicts and no check starts.
inline bool reachabilityRefreshAllowed(bool fieldOpen,bool control,bool message) {return fieldOpen && control && !message;}
struct ListStability {
    unsigned epoch=0;bool observed=false,confirmed=false,followupRequested=false;
    void observe(unsigned scanEpoch,bool sameRows) {
        if(!observed || scanEpoch!=epoch) {epoch=scanEpoch;observed=true;confirmed=followupRequested=false;return;}
        if(sameRows) confirmed=true;
    }
    bool requestConfirmation() {
        if(confirmed || followupRequested) return false;
        followupRequested=true;return true;
    }
    bool totals(bool hideUnreachable,bool checking) const {
        return confirmed && rowCountSettled(hideUnreachable,checking);
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
