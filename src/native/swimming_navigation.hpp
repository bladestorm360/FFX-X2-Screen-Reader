#pragma once
#include "navigation.hpp"
namespace ffx {
namespace swimming_rva { inline constexpr unsigned DiveEnabled=0xf0077d; }
bool configureSwimmingGoal(Entity&,const char* verifiedScript,unsigned localWorker,uintptr_t worker);
void refreshSwimmingGoal(Entity&);
bool swimmingGoalActive(const Entity&);
bool swimmingGoalActive(const NativeRouteTarget&);
std::wstring swimmingGoalInstruction();
}
