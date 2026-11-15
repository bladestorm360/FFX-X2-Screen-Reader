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
std::wstring puzzleTargetPathOwned(const nav::Scene&,const Mesh&,const std::vector<Entity>&,Entity,const std::wstring& finish,unsigned manual,unsigned intentRevision);
}
