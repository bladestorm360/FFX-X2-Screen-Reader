#pragma once
#include "utility_text.hpp"
namespace ffx {
// Native-owner APIs. The default-Off saved preference is visible on all Sin
// maps and survives entry/battles. Actual assistance still needs
// the current exact Nucleus resource, worker, instruction and pickup predicate.
bool sinCollectionActive();
bool sinCollectionArmed();
UtilityText sinCollectionCommand(std::wstring_view action);
void installSinCollection();
void resetSinCollection();
}
