// Optional Nucleus collection assistance. The original pickup scripts still
// own appearance/expiry, control, rewards, flags, counters and the final warp.
// Only their exact transient distance READ receives an assisted value. No game
// variable, code, item, completion counter or player position is written.
#include "sin_collection.hpp"
#include "utility_memory.hpp"
#include "events.hpp"
#include "settings.hpp"
#include "room_areas.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include <cstring>
#include <cmath>
namespace ffx {
namespace {
thread_local uintptr_t interpreter=0;
thread_local unsigned interpreterEpoch=0;
constexpr float assistedDistance=0;
int sinArea() {
    if(!nativeGameThread()) return -1;
    const auto area=areaIndex(read<uint16_t>(address(0xd2ca90)));
    return area>=307 && area<=316?area:-1;
}
bool nucleus(UtilityScript& s) {
    // Menu visibility uses the copied room/area mapping alone. Assistance has
    // the stronger resource/ownership contract, including both Nucleus rooms
    // (324 and341). Preparation readiness must never hide the setting.
    if(sinArea()!=312) return false;
    if(!nativeGameThread() || inBattle() || !captureUtilityScript(s) ||
        read<uintptr_t>(s.controller+0x48)!=s.chunk || s.name!="sins0500" || s.length!=60258 || s.workers!=44) return false;
    // The controller can also own other resources' workers. The existing
    // indexed getter validates this resource's actual global membership below;
    // equating the global count with its local44-worker header is incorrect.
    AtelIdentity id;
    return readyAtelIdentity(s.chunk,id) && id.span.serial==s.allocationSerial && id.length==s.length &&
        id.checksum==0x05e485ed && id.workers==s.workers && s.name==id.name;
}
bool counts(UtilityScript& s,int& total) {
    uintptr_t worker=0;if(!captureUtilityWorker(s,0,worker)) return false;s.worker=worker;s.header=read<uintptr_t>(worker);
    int a=-1,b=-1;
    if(!captureUtilityValue(s,0x0c0000b0,1,0,a) || !captureUtilityValue(s,0x0c0000b1,1,0,b) || a<0 || a>5 || b<0 || b>5) return false;
    total=a+b;return true;
}
void entered(Event& e) {
    e.before[24]=interpreter;e.before[25]=interpreterEpoch;
    interpreter=e.args[0];interpreterEpoch=generation();
}
void left(Event& e) {interpreter=e.before[24];interpreterEpoch=static_cast<unsigned>(e.before[25]);}
void distanceRead(Event& e) {
    if(!setting(Setting::SinCollectionAssist) || !nativeGameThread() || e.overrideResult || e.epoch!=generation() ||
        e.parentRva!=0x464180 || interpreterEpoch!=generation() || interpreter!=e.args[0] || e.args[2] ||
        scene || !gameFocused() || modMenuActive() || utilityConsoleActive()) return;
    UtilityScript s;if(!nucleus(s)) return;
    const auto worker=e.args[0];const unsigned local=read<uint16_t>(worker+0x2e,0xffff);
    if(local<24 || local>33 || read<uintptr_t>(worker+4)!=s.chunk) return;
    unsigned header=0;if(!readyAtelWorkerHeader(s.chunk,local,header) || read<uintptr_t>(worker)!=s.chunk+header || read<uint8_t>(s.chunk+header)!=1) return;
    // Reuse the verified allocation/controller worker index. It validates the
    // actual global getter; a local worker number is never assumed to be global.
    // Ordinary native discovery can warm this registry. A cold read collects
    // the current controller's bounded (at most500) loaded global identities.
    uintptr_t actual=0;if(!captureUtilityWorker(s,local,actual) || actual!=worker) return;
    const auto signal=read<uintptr_t>(worker+0x90);const unsigned level=read<uint8_t>(worker+0x32,255);
    if(!signal || level>8 || read<uint16_t>(signal+8,0xffff)!=6 || (read<uint8_t>(signal+14)&15u)!=level) return;
    // Ten original bodies have the same layout,596 bytes apart. Each PUSHV
    // reads its own single float declaration; POPV/write sites never qualify.
    const unsigned index=local-24,pc=0xbeda+596*index,variable=207+3*index;
    if(e.args[1]!=variable || read<uintptr_t>(worker+0x12c+level*0x4c+0x18)!=s.chunk+s.codeOffset+pc ||
        read<uint8_t>(s.chunk+s.codeOffset+pc)!=0x9f || read<uint16_t>(s.chunk+s.codeOffset+pc+1)!=variable) return;
    unsigned low=0,high=0;if(!readyAtelDeclarationAt(s.chunk,header,variable,low,high) || low!=0x66000000 || high!=1) return;
    const auto declarations=read<unsigned>(s.chunk+header+0x14);AtelSpan span;
    if(!nativeAtelSpan(s.chunk,span) || declarations>span.size || (variable+1ull)*8>span.size-declarations ||
        read<unsigned>(s.chunk+declarations+variable*8)!=low || read<unsigned>(s.chunk+declarations+variable*8+4)!=high) return;
    float original=0;if(!e.result || !copy(e.result,&original,sizeof(original)) || !std::isfinite(original) || original<0) return;
    // 86C5A0 consumes this pointer synchronously as a float. Static storage
    // outlives the call; the x87 ABI and underlying computed distance survive.
    e.result=reinterpret_cast<uintptr_t>(&assistedDistance);e.overrideResult=true;
}
}
bool sinCollectionActive() {return sinArea()>=0;}
bool sinCollectionArmed() {return setting(Setting::SinCollectionAssist)!=0 && sinCollectionActive();}
UtilityText sinCollectionCommand(std::wstring_view action) {
    if(action==L"off") {setSetting(Setting::SinCollectionAssist,0);return L"Automatic object collection off.";}
    const auto area=sinArea();
    if(area<0) return L"Sin object collection is available on Sin maps.";
    // Other Sin maps have no collection counters. Never enter their variable
    // getters using Nucleus declarations merely to display or change a setting.
    UtilityScript s;int total=0;const bool counted=area==312 && nucleus(s) && counts(s,total);
    if(action==L"on") {
        if(counted && total>=10) {setSetting(Setting::SinCollectionAssist,0);return L"All ten objects are collected. Wait for the game's next area.";}
        setSetting(Setting::SinCollectionAssist,1);
        return area!=312?
            L"Automatic object collection on. Close the mod menu and enter The Nucleus. This setting stays on through battles. Confirm any game reward dialogue.":
            L"Automatic object collection on. Close the mod menu. This setting stays on through battles. Confirm any game reward dialogue.";
    }
    if(action==L"status") {
        const auto state=setting(Setting::SinCollectionAssist)?L"Automatic collection on.":L"Automatic collection off.";
        if(area!=312) return UtilityText(state)+L" Ready for The Nucleus. The setting survives entry and battles.";
        if(!counted) return UtilityText(state)+L" Collection count is not ready.";
        return UtilityText(L"Sin objects: ")+utilityInteger(total)+L" of 10 collected. "+state;
    }
    return L"Enable automatic collection on any Sin map before entering The Nucleus. Collect ten objects to continue. Each appears briefly. Avoid the rising spikes and confirm native rewards. The setting is saved and stays on through battles; Off restores manual collection.";
}
// Reset borrowed interpreter scope, not the user's preference. Every later
// read must qualify its fresh allocation/worker/PC again before assistance.
void resetSinCollection() {interpreter=0;interpreterEpoch=0;}
void installSinCollection() {hook(0x464180,entered,left);hook(0x46c500,nullptr,distanceRead);}
}
