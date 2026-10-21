#pragma once
#include "utility_text.hpp"
namespace ffx {
bool gagazetActive();
UtilityText gagazetCommand(std::wstring_view);
void installGagazet();
void resetGagazet();
}
