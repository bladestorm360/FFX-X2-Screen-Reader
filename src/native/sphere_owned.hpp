#pragma once
#include "sphere_grid.hpp"
#include "speech_domains.hpp"
namespace ffx {
enum class SphereWork : unsigned {Abilities,Paths,Key,Section,Orientation,Reset,Clear};
void startSphereWork();
bool queueSphereWork(SphereWork,bool shift=false,unsigned key=0,bool cursor=false);
void cancelSphereWork(bool clear=false); // Atomic retirement, safe on a foreign reset owner.
bool spherePreparationOwner();
bool sphereOpeningPending();
bool sphereCopiedSnapshot(GridSnapshot&);
bool sphereCopiedOpen(bool&);
std::wstring spherePlayerLabel(int);
std::wstring sphereItemLabel(unsigned);
std::wstring sphereStatLabel(int);
unsigned sphereStock(unsigned);
bool sphereEmit(std::wstring,bool,const char*);
#ifdef FFX_READER_TEST
bool captureSphereFactsForTest(); // Isolated Native capture, excludes semantic domain transport.
bool sphereOwnedGridForTest(GridSnapshot&); // Preparation-owner inspection only.
#endif
}
