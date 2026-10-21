#pragma once
#include "puzzle_guide.hpp"
namespace ffx {
struct PuzzleTile { unsigned stage, worker; Point point; std::vector<UtilityBinding> blocked; };
void puzzleTileCompleted(uintptr_t worker, unsigned entry);
}
