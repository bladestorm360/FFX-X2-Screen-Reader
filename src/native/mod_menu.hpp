#pragma once
#include "runtime.hpp"
#include "utility_text.hpp"
namespace ffx {
struct ModMenuState { std::wstring trial; UtilityText status; bool supported = false, armed = false, via = false; unsigned epoch = 0; bool zanarkand=false; bool gagazet=false; bool sin=false; };
void openModMenu(const ModMenuState&);
bool modMenuActive();
void installModMenu();
void modMenuReply(const std::wstring&);
void modMenuKey(unsigned vk,bool shift=false);
void resetModMenu();
}
