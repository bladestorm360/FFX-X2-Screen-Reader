#include "utilities.hpp"
#include "via_purifico.hpp"
#include "utility_memory.hpp"
#include "utility_trials.hpp"
#include "utility_warps.hpp"
#include "events.hpp"
#include "puzzle_solver.hpp"
#include "mod_menu.hpp"
#include "minigames.hpp"
#include "nonblocking_snapshots.hpp"
#include <cstring>
#include <cwctype>
#include <stdexcept>

namespace ffx {
namespace {
// The console limits input to500 characters; this transport accepts up to2047.
// Copy its ready command,
// never a UI string owner. Exactly eight accepted pending commands remain.
struct Command {std::array<wchar_t,2048> text{};UtilityContext context;
#ifdef FFX_DEV_UTILITIES
    UtilityWarpRequest warp;bool isWarp=false;
#endif
};
async::TryCaptureQueue<Command,8> commands;
std::atomic<unsigned> commandCount{0};
std::atomic<unsigned> preparedRevision{1};
Command retainedCommand;bool haveRetainedCommand=false;
uintptr_t awaitingChunk=0;unsigned awaitingRevision=0;
void resourcePrepared() noexcept {preparedRevision.fetch_add(1,std::memory_order_release);}
void releaseCommand() {
    retainedCommand={};haveRetainedCommand=false;awaitingChunk=0;awaitingRevision=0;
    commandCount.fetch_sub(1,std::memory_order_acq_rel);
}
std::wstring normalized(std::wstring value) {
    const auto begin = value.find_first_not_of(L" \r\n\t"), end = value.find_last_not_of(L" \r\n\t");
    if (begin == std::wstring::npos) return {};
    value = value.substr(begin, end - begin + 1);
    for (auto& c : value) c = towlower(c);
    return value;
}
#ifdef FFX_DEV_UTILITIES
void consoleKey(bool) {
    if(scene || inBattle() || minigameActive()) output(L"Utilities are available in the field",true,"utility-context");
    else openUtilityConsole();
}
// Reserve the whole development chord even when its context is unavailable.
// Letting Shift+F1 fall through can run the game's native F1 action.
bool consoleAvailable() { return true; }
#endif
}
bool queueUtility(const std::wstring& text) {
    return queueUtilityContext(text,utilityContext());
}
bool queueUtilityAt(const std::wstring& text, unsigned epoch) {
    auto token=utilityContext(); token.epoch=epoch;
    return queueUtilityContext(text,token);
}
bool queueUtilityContext(const std::wstring& text,const UtilityContext& token) {
    const auto command = normalized(text);
    if (command.empty()) return false;
    if (command == L"help"
#ifdef FFX_DEV_UTILITIES
        || command == L"warps" || command.rfind(L"warps ", 0) == 0
#endif
    ) {
        utilityReply(executeUtility(command)); return false;
    }
    if (commandCount.load(std::memory_order_acquire)>=8) { utilityReply(L"The command queue is full."); return false; }
    if(!utilityContextValid(token)) { utilityReply(L"Command cancelled because the game context changed."); return false; }
    if(command.size()>=2048) {utilityReply(L"The command is too long.");return false;}
    Command packet;packet.context=token;
#ifdef FFX_DEV_UTILITIES
    packet.isWarp=command.rfind(L"warp ",0)==0;if(packet.isWarp) packet.warp=prepareUtilityWarp(std::wstring_view(command).substr(5));
#endif
    std::wmemcpy(packet.text.data(),command.c_str(),command.size()+1);
    if(!commands.tryPush(packet)) {utilityReply(L"The command queue is full.");return false;}
    commandCount.fetch_add(1,std::memory_order_release);return true;
}
UtilityText executeUtilityNative(std::wstring_view command) {
    const auto split = command.find(L' ');
    const auto verb = command.substr(0, split);
    const std::wstring_view arguments = split == std::wstring::npos ? std::wstring_view{} : command.substr(split + 1);
    if (verb == L"puzzle") return puzzleCommand(arguments);
    if(verb==L"via") return viaCommand(arguments);
#ifdef FFX_DEV_UTILITIES
    if (verb == L"help") return
        L"Shift+F1 in the field opens utilities. Type a command and press Enter. Escape returns to the game.\r\n"
        L"where: current script. warps: trial destinations. warps besaid: search rooms.\r\n"
        L"warp besaid: first trial. warp 122 0: room and entrance. Warps preserve story progress.\r\n"
        L"warp macalania-test: early-save testing only; advances story to the playable Macalania trial stage. Reload your original save after testing.\r\n"
        L"warp gagazet-test: early-save testing only; prepares the unfinished cave trials. Follow Prepare swimming party before entering the water. Reload your original save after testing.\r\n"
        L"warp force besaid: leave a running field script or dialogue and warp.\r\n"
        L"trial treasure: complete the Destruction Sphere objective while staying in the chamber.\r\n"
        L"trial collect: open its chest using the game script. trial finish: leave after collecting.\r\n"
        L"trial status: check progress. These utilities change the game only when you run a command.";
    if (verb == L"warps") return L"Use warps in the development console.";
    if (verb == L"where") {
        if (!arguments.empty()) return L"Use where with no arguments.";
        const auto script = utilityScript();
        UtilityText result(L"Current script: ");result.narrow(script.name.c_str()).literal(L".");return result;
    }
    if (verb == L"warp") return L"The warp destination must be prepared by the development console.";
    if (verb == L"trial") return trialCommand(arguments);
#else
    if (verb == L"help") return L"F8 opens the accessibility mod menu. F6 toggles beacon; F7 toggles autodetail. I reads descriptions. In trials F9 selects the previous step, F10 the next, and F11 repeats the current step. Outside trials F11 repeats speech.";
#endif
    return L"Unknown command. Type help for available commands.";
}
std::wstring executeUtility(const std::wstring& text) {
    const auto command=normalized(text);
#ifdef FFX_DEV_UTILITIES
    if(command==L"warps" || command.rfind(L"warps ",0)==0) return listWarps(command==L"warps"?L"":command.substr(6));
#endif
    if(command!=L"help") return L"Unknown command. Type help for available commands.";
    return renderUtilityText(executeUtilityNative(command));
}
void pumpUtilities() {
    if(!commandCount.load(std::memory_order_acquire)) return;
    if (utilityConsoleActive() || modMenuActive()) return;
#ifndef FFX_READER_TEST
    DWORD foreground = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
    if (foreground != GetCurrentProcessId()) return;
#endif
    if(!haveRetainedCommand) {if(!commands.tryPop(retainedCommand)) return;haveRetainedCommand=true;}
    const auto& command=retainedCommand;
    if (!utilityContextValid(command.context,true)) { releaseCommand();utilityReply(L"Command cancelled because the game context changed. Enter it again."); return; }
    const bool force=wcsncmp(command.text.data(),L"warp force ",11)==0;
    if(!utilityActionAllowed(force)) { releaseCommand();utilityReply(L"Command cancelled because field control is unavailable."); return; }
    const auto readyRevision=preparedRevision.load(std::memory_order_acquire);
    if(awaitingChunk && awaitingRevision==readyRevision) return;
    uintptr_t currentChunk=0;const auto preparation=utilityPreparation(&currentChunk);
    if(awaitingChunk && awaitingChunk!=currentChunk) {releaseCommand();utilityReply(L"Command cancelled because the field script changed.");return;}
    if(preparation==AtelPreparation::Pending) {awaitingChunk=currentChunk;awaitingRevision=readyRevision;return;}
    try {
#ifdef FFX_DEV_UTILITIES
        if(command.isWarp) emitUtilityText(performUtilityWarp(command.warp));else
#endif
        emitUtilityText(executeUtilityNative(command.text.data()));
    }
    catch (const std::exception& error) {
        emitUtilityText(utilityError(L"Utility stopped: ",error.what()));
        recordLog("utility failed: %s", error.what());
    } catch (...) { utilityReply(L"Utility stopped after a native error."); recordLog("utility native exception"); }
    releaseCommand();
}
void installUtilities() {
    installUtilityText();
    onAtelPrepared(resourcePrepared);
    startUtilityConsole();
    installUtilityContext();
    installUtilityScripts(); installPuzzleSolver(); installViaPurifico(); installModMenu();
#ifdef FFX_DEV_UTILITIES
    hotkeyChordWhen(VK_F1,true,consoleKey,consoleAvailable,true);
#endif
}
}
