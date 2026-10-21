#include "utility_context.hpp"
#include "utilities.hpp"
#include "events.hpp"
#include "minigames.hpp"
#include "mod_menu.hpp"
#include "lightning.hpp"
#include "nonblocking_snapshots.hpp"
namespace ffx {
namespace {
struct PublishedContext { UtilityContext token; unsigned allowed=0; };
async::AtomicSnapshot<PublishedContext> published;
// Native owner alone mutates this POD state and publishes it. UI readers make
// one bounded copy; resource generation independently invalidates old tokens.
UtilityContext current;
unsigned serial=0;
bool allowed=false;
int previousMode=-1;
#ifndef FFX_READER_TEST
HWND rootForeground() { return GetAncestor(GetForegroundWindow(),GA_ROOT); }
#endif
void invalidateEvent(Event&) { invalidateUtilityContext(); }
}
bool utilityGameThread() { return nativeGameThread(); }
UtilityContext utilityContext() {
    PublishedContext value;
    return published.tryRead(value)?value.token:UtilityContext{};
}
bool utilityContextValid(const UtilityContext& token,bool requireGameFocus) {
    PublishedContext value;
    const bool valid=published.tryRead(value) && value.allowed && token.revision &&
        token.epoch==generation() && token.revision==value.token.revision &&
        token.epoch==value.token.epoch && token.window==value.token.window;
    if(!valid) return false;
#ifndef FFX_READER_TEST
    if(!IsWindow(token.window)) return false;
    if(requireGameFocus && rootForeground()!=token.window) return false;
#else
    (void)requireGameFocus;
#endif
    return true;
}
void invalidateUtilityContext() {
    if(!utilityGameThread()) return;
    cancelLightningWarning();
    allowed=false; current.revision=++serial;
    published.publish({current,0});
    invalidateUtilityConsole();
}
void updateUtilityContext() {
    if(!utilityGameThread()) return;
    const bool field=!scene && !inBattle() && !minigameActive() && !modMenuActive();
    using Mode=int(__cdecl*)();
    const int mode=field?reinterpret_cast<Mode>(address(0x46c840))():-1;
    HWND candidate=nullptr;
#ifndef FFX_READER_TEST
    if(!utilityConsoleActive()) {
        candidate=rootForeground(); DWORD process=0;
        GetWindowThreadProcessId(candidate,&process);
        if(process!=GetCurrentProcessId()) candidate=nullptr;
    }
#else
    candidate=reinterpret_cast<HWND>(1);
#endif
    bool invalidate=false;
    if(!current.window && candidate) current.window=candidate;
    const bool focused=candidate==current.window || utilityConsoleActive();
    const bool next=field && current.window && focused;
    if(next!=allowed || current.epoch!=generation() || mode!=previousMode) {
        current.revision=++serial; invalidate=!next || allowed;
    }
    allowed=next; current.epoch=generation(); previousMode=mode;
    published.publish({current,allowed?1u:0u});
    if(invalidate) invalidateUtilityConsole();
}
bool utilityActionAllowed(bool force) {
    if(!utilityGameThread() || !utilityContextValid(utilityContext(),true) ||
        scene || inBattle() || minigameActive() || modMenuActive() || utilityConsoleActive()) return false;
    using Mode=int(__cdecl*)();
    return force || reinterpret_cast<Mode>(address(0x46c840))()==0;
}
void installUtilityContext() {
    // Invalidate before the original starts changing combat/UI/resources, so a
    // delayed Win32 message cannot focus a console using a prior field token.
    for(const auto rva:{0x46fed0u,0x5093b0u,0x4aa0b0u,0x4a0d70u,
        0x4b8980u,0x4de350u,0x4de5b0u,0x4b5f60u,0x4b6260u,0x4b6490u})
        hook(rva,invalidateEvent,nullptr);
}
}
