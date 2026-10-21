#pragma once
#include "navigation.hpp"
#include "utility_text.hpp"
namespace ffx {
struct QueuedPuzzleTarget {Entity target;std::wstring finish,prefix;uintptr_t chunk=0;unsigned serial=0;};
void startPuzzleGuideTargets();
bool queuePuzzleGuidePoint(Point,int local,bool talk,const wchar_t* finish,const UtilityText& prefix);
bool takePuzzleGuideTarget(unsigned slot,unsigned ticket,QueuedPuzzleTarget&); // Worker only.
void retirePuzzleGuideTargets(); // Worker only, canceled/abandoned packet recovery.
}
