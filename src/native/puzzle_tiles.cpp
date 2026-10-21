// Only qualified Zanarkand dome0700 locals 57..83 at entry 2 request an owned
// tile refresh. This observer does not change tile state.
// See ../../Docs/features/puzzles/index.md#cloisters.
#include "puzzle_tiles.hpp"
#include "puzzle_guide_owned.hpp"
#include "navigation_observations.hpp"
namespace ffx {
void puzzleTileCompleted(uintptr_t worker,unsigned entry) {
    if(!nativeGameThread() || !ownedPuzzleGuideActive() || entry!=2) return;
    UtilityScript script;if(!captureUtilityScript(script) || script.name!="dome0700" || !captureCurrentTrial(script) || read<uintptr_t>(worker+4)!=script.chunk) return;
    const auto local=read<uint16_t>(worker+0x2e,0xffff);if(local<57 || local>83) return;
    nav::submitNavigationQuery(nav::Query::GuideTileCompleted,false,ownedPuzzleGuideRevision(),local,script.chunk,script.allocationSerial);
}
}
