// Guide commands submit owned navigation queries. Bevelle timing speaks only
// after the exact script/variable transition is qualified on the native owner.
// See ../../Docs/features/puzzles/index.md#cloisters.
#include "puzzle_guide.hpp"
#include "puzzle_guide_owned.hpp"
#include "puzzle_guide_targets.hpp"
#include "navigation_observations.hpp"
#include "navigation_intents.hpp"
#include "utilities.hpp"
#include "nav_audio.hpp"
#include "nav_format.hpp"
#include "settings.hpp"
#include "pedestal.hpp"
#include "auto_walk.hpp"
#include <cmath>
namespace ffx {
namespace {
void timingEnter(Event& e) {
    e.before[0]=0;if(!ownedPuzzleGuideActive() || e.args[3]!=1) return;
    const auto worker=e.args[0],header=read<uintptr_t>(worker),chunk=read<uintptr_t>(worker+4);AtelSpan span;
    if(!nativeAtelSpan(chunk,span) || header<chunk || header-chunk>span.size || span.size-(header-chunk)<0x1c || e.args[1]>=read<uint16_t>(header+2)) return;
    const auto offset=read<unsigned>(header+0x14);if(offset>span.size || (e.args[1]+1ull)*8>span.size-offset || read<unsigned>(chunk+offset+e.args[1]*8)!=0x0c00002c) return;
    UtilityScript script;int value=0;if(!captureUtilityScript(script) || script.chunk!=chunk || script.name!="bvyt1100" || !captureCurrentTrial(script) || !captureUtilityWorker(script,0,script.worker)) return;
    script.header=read<uintptr_t>(script.worker);if(captureUtilityValue(script,0x0c00002c,1,0,value)) e.before[0]=value!=1;
}
void timingLeave(Event& e) {if(e.before[0]) utilityReply(L"Bevelle rail junction active. Confirm a turn now; if you miss it, use the next circuit.");}
}
void resetPuzzleGuide() {resetOwnedPuzzleGuide();}
std::wstring puzzleGuide(std::wstring_view action) {
    nav::Query query=nav::Query::GuideStep;
    if(action==L"next") query=nav::Query::GuideNext;else if(action==L"previous") query=nav::Query::GuidePrevious;
    else if(action==L"expected") query=nav::Query::GuideExpected;else if(action==L"path") {query=nav::Query::GuidePath;nav::invalidateNavigationIntents();cancelPedestal();clearNavRoute();stopAutoWalk();}
    nav::submitNavigationQuery(query,false,ownedPuzzleGuideRevision());return {};
}
void puzzleGuideCompleted() {if(ownedPuzzleGuideActive() && claimOwnedGuideRefresh()) if(!nav::submitNavigationQuery(nav::Query::GuideCompleted,false,ownedPuzzleGuideRevision())) releaseOwnedGuideRefresh();}
void installPuzzleGuide() {hook(0x4713a0,timingEnter,timingLeave);}
}
