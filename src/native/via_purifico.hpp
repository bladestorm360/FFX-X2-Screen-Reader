#pragma once
#include "utility_text.hpp"
#include "utility_memory.hpp"
namespace ffx {
bool viaPurificoActive();
bool viaCollecting();
UtilityText viaCommand(std::wstring_view);
void resetViaPurifico();
void pumpViaPurifico();
void viaScriptCompleted(bool completed,bool owned);
void installViaPurifico();
}
