// Parse catalogued warp destinations on preparation; execute only after native
// owner/context checks. Ordinary warps preserve progress. Explicit Macalania and
// Gagazet test requests have separate validation and native setup semantics.
// See ../../Docs/features/distribution/index.md#developer-utilities.
#include "utility_warps.hpp"
#include "utility_memory.hpp"
#include "utility_scripts.hpp"
#include "utility_warps_data.hpp"
#include "events.hpp"
#include <sstream>
#include <cwctype>

namespace ffx {
namespace {
std::wstring lower(std::wstring value) { for (auto& c : value) c = towlower(c); return value; }
std::wstring wide(const char* value) { std::wstring out; while (*value) out += *value++; return out; }
bool number(const std::wstring& value, unsigned& result) {
    if (value.empty() || value.size() > 5) return false;
    result = 0;
    for (auto c : value) { if (c < L'0' || c > L'9') return false; result = result * 10 + c - L'0'; }
    return true;
}
unsigned alias(const std::wstring& value) {
    const wchar_t* names[] = {L"besaid", L"kilika", L"djose", L"macalania", L"bevelle", L"zanarkand"};
    const unsigned rooms[] = {122,108,214,239,306,320};
    for (unsigned i = 0; i < 6; ++i) if (value == names[i]) return rooms[i];
    return 0;
}
}
std::wstring listWarps(const std::wstring& filter) {
    if (filter.empty()) return L"Trial warps: besaid, kilika, djose, macalania, bevelle, zanarkand. Use warps followed by an area name to search other rooms.";
    std::vector<std::wstring> results;
    unsigned matches = 0;
    for (const auto& place : warpLocations) {
        if (lower(place.name).find(filter) == std::wstring::npos && wide(place.script).find(filter) == std::wstring::npos &&
            std::to_wstring(place.room) != filter) continue;
        ++matches;
        if (results.size() < 24) results.push_back(std::wstring(place.name) + L", room " +
            std::to_wstring(place.room) + L", " + wide(place.script) + L", entrances 0 through " + std::to_wstring(place.entrances - 1));
    }
    if (!matches) return L"No matching warp locations.";
    auto reply = join(results, L"\r\n");
    if (matches > results.size()) reply += L"\r\n" + std::to_wstring(matches) + L" matches. Narrow the search to hear the rest.";
    return reply;
}
UtilityWarpRequest prepareUtilityWarp(std::wstring_view arguments) {
    UtilityWarpRequest request;
    std::wistringstream stream{std::wstring(arguments)};
    std::wstring target, spawnText, extra; stream >> target >> spawnText >> extra;
    const bool force = target == L"force";
    if (force) {
        target = spawnText; spawnText = extra; extra.clear(); stream >> extra;
    }
    unsigned room = alias(target), spawn = 0;
    if(target==L"macalania-test") {
        if(force || !spawnText.empty() || !extra.empty()) {request.refusal=L"Use warp macalania-test without force or an entrance. This testing warp advances an early save to Macalania's playable trial stage.";return request;}
        room=239;spawn=1;request.macalaniaTest=true;
    } else if(target==L"gagazet-test") {
        if(force || !spawnText.empty() || !extra.empty()) {request.refusal=L"Use warp gagazet-test without force or an entrance. This testing warp prepares an early save for Gagazet's cave trials.";return request;}
        room=272;spawn=0;request.gagazetTest=true;
    }
    if (!extra.empty() || (!spawnText.empty() && !number(spawnText, spawn)))
        {request.refusal=L"Use warp followed by a trial name, script name, or room number, and an optional entrance number.";return request;}
    if (!room && !number(target, room)) {
        unsigned matches = 0;
        for (const auto& place : warpLocations) if (wide(place.script) == target) { room = place.room; ++matches; }
        if (matches > 1) {request.refusal=L"That script has several rooms. Use warps followed by its name, then warp to a room number.";return request;}
        if (!matches) {request.refusal=L"Unknown warp destination. Use warps followed by an area name.";return request;}
    }
    const WarpLocation* destination = nullptr;
    for (const auto& place : warpLocations) if (place.room == room) { destination = &place; break; }
    if (!destination || spawn >= destination->entrances) {request.refusal=L"That room or entrance is not in the extracted warp catalog.";return request;}
    request.room=room;request.spawn=spawn;request.index=static_cast<unsigned>(destination-warpLocations);request.force=force;return request;
}
UtilityText performUtilityWarp(const UtilityWarpRequest& request) {
    if(!nativeGameThread()) return L"This utility requires the native game owner.";
    if(!request.refusal.empty()) return request.refusal;
    const auto room=request.room,spawn=request.spawn;const bool force=request.force;
    const WarpLocation* destination=request.index<_countof(warpLocations)?&warpLocations[request.index]:nullptr;
    if(!destination || destination->room!=room || spawn>=destination->entrances) return L"That room or entrance is not in the extracted warp catalog.";
    if (inBattle()) return L"Warp commands are available in the field.";
    if (!force && utilityBusy()) return L"Finish the running utility script before warping, or use warp force followed by the destination to leave it.";
    using Mode = int(__cdecl*)();
    if (!force && reinterpret_cast<Mode>(address(0x46c840))() != 0) return L"Close the current game dialogue before warping, or use warp force followed by the destination.";
    if(request.macalaniaTest && (room!=239 || spawn!=1 || force)) return L"Invalid Macalania test destination.";
    if(request.gagazetTest && (room!=272 || spawn!=0 || force || request.macalaniaTest)) return L"Invalid Gagazet test destination.";
    utilityWarp(room, spawn,force,request.macalaniaTest,request.gagazetTest);
    if(request.macalaniaTest) return L"Macalania test warp queued. This advances the current early save to story stage 1550 and enters the unfinished trial through its native return entrance. Use a disposable test session; reload your original save after testing.";
    if(request.gagazetTest) return L"Gagazet test setup queued. This advances the current early save to story stage 2625, resets the cave trial flags, and enters Mountain Cave. Follow the Prepare swimming party event before entering the Submerged Passage; the native event adds Wakka and Rikku. Both trials remain unsolved. Reload your original save after testing.";
    return UtilityText(L"Warping to ")+destination->name+L", room "+utilityInteger(room)+L", entrance "+utilityInteger(spawn)+L".";
}
UtilityText warpCommand(std::wstring_view arguments) {return performUtilityWarp(prepareUtilityWarp(arguments));}

}
