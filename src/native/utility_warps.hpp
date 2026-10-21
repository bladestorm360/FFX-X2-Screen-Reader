#pragma once
#include "utility_text.hpp"
#include <string>
namespace ffx {
struct WarpLocation { unsigned room, entrances; const char* script; const wchar_t* name; };
struct UtilityWarpRequest {unsigned room=0,spawn=0,index=~0u;bool force=false,macalaniaTest=false,gagazetTest=false;UtilityText refusal;};
UtilityWarpRequest prepareUtilityWarp(std::wstring_view); // UI/copied-data owner only.
UtilityText performUtilityWarp(const UtilityWarpRequest&); // Native effects and current guards only.
UtilityText warpCommand(std::wstring_view);
std::wstring listWarps(const std::wstring&);
}
