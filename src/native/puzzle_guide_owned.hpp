#pragma once
#include "navigation_scene.hpp"
#include "speech_domains.hpp"
namespace ffx {
enum class OwnedGuideAction : unsigned {Step,Next,Previous,Expected,Path,Completed,TileCompleted};
void resetOwnedPuzzleGuide();
bool ownedPuzzleGuideActive();
unsigned ownedPuzzleGuideRevision();
bool claimOwnedGuideRefresh();
void releaseOwnedGuideRefresh();
std::wstring puzzleGuideOwned(OwnedGuideAction,const nav::Scene&,const Mesh&,const std::vector<Entity>&,const speechq::Header&,unsigned manual,unsigned intentRevision,unsigned local=0,uintptr_t chunk=0,unsigned serial=0);
// Guide text for a pushable pedestal target when the selected guide step uses
// that pedestal without pushing it (a sphere socket). Empty otherwise.
std::wstring ownedGuidePedestalStep(const nav::Scene&,const Entity& target);
// The pedestal face (unit X vector) a selected Djose sphere step needs.
// True when the selected guide step puts a sphere in or takes one out of this
// pedestal. face is the socket side to approach, or zero for any side.
bool ownedGuidePedestalFace(const nav::Scene&,const Entity& pedestal,Point& face);
// The selected guide step is this pedestal reset tile, which works whenever
// the player steps on it, so navigation may route there.
bool ownedGuideTargetsTile(const nav::Scene&,const Entity& tile);
std::wstring puzzleTargetPathOwned(const nav::Scene&,const Mesh&,const std::vector<Entity>&,Entity,const std::wstring& finish,unsigned manual,unsigned intentRevision);
}
