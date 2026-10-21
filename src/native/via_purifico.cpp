#include "via_purifico.hpp"
#include "via_purifico_data.hpp"
#include "puzzle_guide.hpp"
#include "puzzle_guide_targets.hpp"
#include "utilities.hpp"
#include "mod_menu.hpp"
#include <stdexcept>
#include <cmath>
namespace ffx {
namespace {
unsigned epoch=0,step=0;
unsigned verifiedSerial=0;
uintptr_t verifiedChunk=0;
bool collecting=false,guiding=false;
int pending=-1;
constexpr unsigned chestOrder[]={0,1,5,6,2,3,4};
const wchar_t* directions[]={L"",L"north",L"west",L"south",L"east"};
UtilityScript context() {
    auto script=utilityScript();
    if(inBattle() || script.name!="bvyt0900" || script.length!=37649)
        throw std::runtime_error("The Via Purifico land dungeon is not active");
    if(scriptChecksum(script)!=0x97fd6083) throw std::runtime_error("Via Purifico differs from its verified script");
    if(epoch!=generation() || verifiedChunk!=script.chunk || verifiedSerial!=script.allocationSerial) {
        epoch=generation();verifiedChunk=script.chunk;step=0;guiding=collecting=false;pending=-1;
        verifiedSerial=script.allocationSerial;
    }
    script.worker=utilityWorker(script,0);script.header=read<uintptr_t>(script.worker);
    return script;
}
unsigned value(const UtilityScript& s,unsigned id) { return static_cast<unsigned>(utilityValue(s,id,1)); }
unsigned expected(const UtilityScript& s) {
    const auto flags=value(s,6);
    for(unsigned i=0;i<4;++i) if(!(flags&viaChests[chestOrder[i]].mask)) return i;
    if(!value(s,0xb)) {
        if(!value(s,0xa)) return 4;
        return 5;
    }
    if((flags&28)!=28) {
        if(value(s,9)!=5) return 6;
        for(unsigned i=4;i<7;++i) if(!(flags&viaChests[chestOrder[i]].mask)) return i+3;
    }
    return 10;
}
unsigned wanted(const UtilityScript& s) {
    if(step==6 && value(s,0xb)==1) {
        const auto node=value(s,9);
        return node<6?viaPanels[node].toChamberDirection:0;
    }
    return step==10 && value(s,9)==5?4:0;
}
void directionBefore(Event& e) {
    e.before[0]=0;
    if(!guiding || collecting || epoch!=generation() || inBattle() || modMenuActive() || !utilityActionAllowed()) return;
    const auto worker=e.args[0],header=read<uintptr_t>(worker),chunk=read<uintptr_t>(worker+4);AtelSpan span;
    const auto index=e.args[1];
    if(chunk!=verifiedChunk || !nativeAtelSpan(chunk,span) || span.serial!=verifiedSerial || header<chunk || header-chunk>span.size || span.size-(header-chunk)<0x1c || index>=read<uint16_t>(header+2)) return;
    const auto offset=read<unsigned>(header+0x14);
    if(offset>span.size || (index+1ull)*8>span.size-offset || read<unsigned>(chunk+offset+index*8)!=8) return;
    try {
        const auto s=context();const auto node=value(s,9),direction=wanted(s);
        if(node>=6 || !direction || e.args[3]!=direction || value(s,8)==direction || value(s,0xd)!=1) return;
        if(distanceXZ(playerPosition(),viaPanels[node].position)>45) return;
        e.before[0]=direction;
    } catch(...) {}
}
void directionAfter(Event& e) {
    if(e.before[0]) emitUtilityText(UtilityText(directions[e.before[0]])+L". Step onto the panel now.");
}
}
bool viaPurificoActive() {UtilityScript script;AtelIdentity identity;return nativeGameThread() && !inBattle() && captureUtilityScript(script) && script.name=="bvyt0900" && script.length==37649 && readyAtelIdentity(script.chunk,identity) && identity.span.serial==script.allocationSerial && identity.checksum==0x97fd6083;}
bool viaCollecting() { return collecting && epoch==generation(); }
void resetViaPurifico() {
    // Owner reset is serialized by resource_reset; no worker retains this state.
    epoch=step=verifiedSerial=0;verifiedChunk=0;collecting=guiding=false;pending=-1; }
UtilityText viaCommand(std::wstring_view action) {
    if(!nativeGameThread()) return L"This utility requires the native game owner.";
    const auto s=context();
    if(action==L"off") { collecting=false; return L"Treasure collection off. The current game chest dialogue can finish normally."; }
    if(action==L"on") {
        if(utilityBusy() || pending>=0) return L"Finish the current chest dialogue first.";
        for(const auto& c:viaChests) if(!(value(s,6)&c.mask)) checkUtilityScript(s,c.action);
        collecting=true;guiding=false;
        return L"Collecting all Via Purifico treasures through the game's chest scripts. Confirm each game dialogue. You remain in the dungeon.";
    }
    if(action==L"status") {
        const auto flags=value(s,6);unsigned n=0;for(const auto& c:viaChests) if(flags&c.mask) ++n;
        return UtilityText(L"Via Purifico. ")+utilityInteger(n)+L" of 7 treasures collected. "+(viaCollecting()?L"Automatic collection on.":L"Automatic collection off.");
    }
    if(action!=L"step" && action!=L"expected" && action!=L"next" && action!=L"previous" && action!=L"path") return L"Use the Via Purifico guide in the mod menu.";
    if(!guiding) { step=expected(s);guiding=true; }
    if(action==L"next" && step<10) ++step;
    if(action==L"previous" && step) --step;
    if(action==L"expected") step=expected(s);
    const auto prefix=UtilityText(L"Step ")+utilityInteger(step+1)+L" of 11. ";
    Point point;int local=-1;bool talk=false;UtilityText instruction;const wchar_t* finish=L"Interact with the guide target.";
    if(step<4 || (step>=7 && step<=9)) {
        const auto index=chestOrder[step<4?step:step-3];const auto& c=viaChests[index];
        point=c.position;local=c.action.worker;talk=true;
        instruction=UtilityText(L"Open treasure chest ")+utilityInteger(index+1)+L".";
        if(value(s,6)&c.mask) instruction=instruction+L" Already collected.";
    } else if(step==4) {
        point={-342.98187255859375,0,349.5180358886719};local=42;talk=true;
        instruction=L"Touch the glyph near treasure chest 1 to prepare the relocation glyph.";
        if(value(s,0xa) || value(s,0xb)) instruction=instruction+L" Already activated.";
    } else if(step==5) {
        point={225,-1.9684710502624512,675};local=43;talk=false;
        instruction=L"Step on the eastern relocation glyph. This moves the main warp panel to the northwest corridor and opens its connections.";
        if(value(s,0xb)) instruction=instruction+L" Already activated.";
    } else {
        const auto node=value(s,9);
        if(node>=6) return prefix+L"The warp panel location is unavailable.";
        if(step==6 && !value(s,0xb)) return prefix+L"Activate the two glyphs in steps 5 and 6 first.";
        if(step==6 && node==5) return prefix+L"You have reached the sealed western treasure chamber. Open chests 3, 4 and 5, then use the east arrow to return.";
        if(step==10 && node!=5) return prefix+L"All treasure guide steps are complete. Use Exit 1 near the northern Save Sphere to continue.";
        const auto& panel=viaPanels[node];
        // Stop beside the crossing circle; entering it before the desired
        // direction would invoke the wrong native destination.
        point=panel.position;point.z-=25;
        const auto direction=step==10?4:panel.toChamberDirection;
        instruction=L"Reach the active warp panel. Wait for "+UtilityText(directions[direction])+L", then step onto it when the guide says now. Read the current step after the warp to follow the next panel.";
        finish=L"Stand beside the panel and wait for its spoken cue.";
    }
    if(action==L"path" && !queuePuzzleGuidePoint(point,local,talk,finish,prefix+instruction)) return prefix+instruction+L" Navigation action is unavailable or busy. Request the route again after it recovers.";
    return action==L"path"?UtilityText{}:prefix+instruction;
}
void viaScriptCompleted(bool completed,bool owned) {
    if(!owned || pending<0 || epoch!=generation()) return;
    const auto index=static_cast<unsigned>(pending);pending=-1;
    try {
        if(!completed || !(value(context(),6)&viaChests[index].mask)) {
            collecting=false;utilityReply(L"Treasure collection stopped. The game did not finish the chest. Read dungeon status before trying again.");
        }
    } catch(...) { collecting=false; }
}
void pumpViaPurifico() {
    if(!collecting || pending>=0) return;
    if(epoch!=generation() || inBattle() || scene) { collecting=false;return; }
    if(!utilityActionAllowed()) return;
    try {
        const auto s=context();const auto flags=value(s,6);
        for(unsigned i=0;i<7;++i) if(!(flags&viaChests[i].mask)) {
            pending=static_cast<int>(i);runUtilityScript(s,viaChests[i].action);
            emitUtilityText(UtilityText(L"Opening treasure chest ")+utilityInteger(i+1)+L". Confirm the game dialogue.");return;
        }
        collecting=false;utilityReply(L"All 7 Via Purifico treasures collected. Use Exit 1 when ready to continue.");
    } catch(const std::exception& error) {
        pending=-1;collecting=false;emitUtilityText(utilityError(L"Treasure collection stopped: ",error.what()));
    }
}
void installViaPurifico() { hook(0x4713a0,directionBefore,directionAfter); }
}
