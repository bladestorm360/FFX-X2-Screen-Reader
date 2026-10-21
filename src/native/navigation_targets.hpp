#pragma once
#include "atel.hpp"
#include <string>
#include <functional>
#include "nav_entries.hpp"
namespace ffx {
// identity>=2 supplies an authored label;3 also keeps a known entry region
// listed as inactive when its real callback is disabled (currently Baaj seals).
struct NavigationTarget { unsigned worker, category; int room, properName; const wchar_t* label; EntryMask entries; unsigned identity=0; };
struct NavigationScript { const char* name; unsigned length, checksum, start, count; };
std::wstring navigationEntityName(const NavigationTarget*,unsigned model,bool actor);
std::wstring navigationEntityName(const NavigationTarget*,unsigned model,bool actor,const std::function<std::wstring(int)>& names);
unsigned navigationEntityCategory(const NavigationTarget*,unsigned model,bool actor);
bool navigationSaveSphereModel(unsigned model);
bool navigationActorPresence(const NavigationScript*,unsigned model,bool actor);
const NavigationScript* navigationCatalog(const AtelScript&);
const NavigationTarget* navigationTarget(const NavigationScript*, unsigned worker);
}
