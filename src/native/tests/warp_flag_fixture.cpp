// Synthetic fixture for utilityWarp's forced-jump bit (0x80 of the field
// controller's first byte). No game data is needed. The drain below follows
// hooks.cpp drainActions: prepare, invoke under catch(...), then complete only
// in the same generation and outside a resource scope. The jump stub starts a
// new generation on entry, like reader.cpp's map-request hook on 0x46fed0.
#include "../utility_memory.hpp"
#include "../utility_context.hpp"
#include "../atel_resources.hpp"
#include "../events.hpp"
#include "../native_action.hpp"
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool ok,const char* why) { if(!ok) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); } }
unsigned checks=0;
void expect(bool ok,const char* why) { check(ok,why); ++checks; }

enum class Jump { Consume, FaultBeforeConsume, FaultAfterSwap, SwapAroundConsume };
std::vector<uint8_t> controllerA(0x300),controllerB(0x300),worker(0x10),chunk(0x1000);
uint8_t* globals=nullptr;
uintptr_t current=0;
unsigned epoch=1,warps=0,setterCalls=0;
bool loadingAfterInvoke=false,setterFaults=false,protectCave=false;
Jump jump=Jump::Consume;
std::vector<std::wstring> replies;
std::vector<ffx::NativeAction> queue;
uintptr_t ptr(std::vector<uint8_t>& v) { return reinterpret_cast<uintptr_t>(v.data()); }
template<class T> void put(uint8_t* at,T value) { std::memcpy(at,&value,sizeof(value)); }

uintptr_t __cdecl workerGetter(int index) { return index==0?ptr(worker):0; }
uintptr_t __cdecl storyGetter(uintptr_t,int,int) { return reinterpret_cast<uintptr_t>(globals)+0xa00; }
void __cdecl storySetter(uintptr_t,int,int,int,int) {
    ++setterCalls;
    if(setterFaults) RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr);
}
void __cdecl warp(int,int,int) {
    ++warps;++epoch;  // reader.cpp teardown: invalidate("map-request") on entry
    if(jump==Jump::FaultAfterSwap) current=ptr(controllerB);
    if(jump==Jump::FaultBeforeConsume || jump==Jump::FaultAfterSwap) RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr);
    const auto before=current;
    if(jump==Jump::SwapAroundConsume) current=ptr(controllerB);
    reinterpret_cast<uint8_t*>(current)[0]&=0x7f;  // 0x46ff84: and byte [controller],0x7f
    current=before;
}
void reset(uint8_t controllerFlags) {
    for(auto* v:{&controllerA,&controllerB,&worker,&chunk}) std::fill(v->begin(),v->end(),uint8_t{0});
    DWORD old=0;VirtualProtect(globals,0x2000,PAGE_READWRITE,&old);std::memset(globals,0,0x2000);
    current=ptr(controllerA);epoch=1;warps=setterCalls=0;
    loadingAfterInvoke=setterFaults=protectCave=false;jump=Jump::Consume;replies.clear();queue.clear();
    for(auto* c:{&controllerA,&controllerB}) {
        put<uint16_t>(c->data()+0xa,0);put<uint16_t>(c->data()+0xc,1);
        put(c->data()+0x2c,reinterpret_cast<uintptr_t>(globals));
    }
    controllerA[0]=controllerFlags;controllerB[0]=controllerFlags;
    put(worker.data(),ptr(chunk)+0x400);put(worker.data()+4,ptr(chunk));
    put<unsigned>(chunk.data(),0x10);put<unsigned>(chunk.data()+0xc,0x1d0);
    put<unsigned>(chunk.data()+0x30,0x100);put<uint16_t>(chunk.data()+0x34,1);
    std::memcpy(chunk.data()+0x1d0,"zzzz0100",8);
    put<uint16_t>(chunk.data()+0x402,1);put<unsigned>(chunk.data()+0x414,0x200);
    put<unsigned>(chunk.data()+0x200,0x20000a00);put<unsigned>(chunk.data()+0x204,1);
}
// One iteration of hooks.cpp drainActions for the single queued command.
void drain() {
    check(queue.size()==1,"one queued warp command");
    auto action=queue.back();queue.clear();
    if(action.epoch!=ffx::generation()) return;
    bool ready=false;
    try { ready=action.prepare && action.prepare(action); } catch(...) {}
    if(protectCave) { DWORD old=0;VirtualProtect(globals,0x1000,PAGE_READONLY,&old); }
    bool success=false;
    if(ready) { try { action.invoke(action);success=true; } catch(...) {} }
    if(action.epoch!=ffx::generation() || loadingAfterInvoke) return;
    if(action.complete) action.complete(action,success);
}
void run(unsigned room,unsigned spawn,bool gagazet) { ffx::utilityWarp(room,spawn,false,false,gagazet);drain(); }
}

namespace ffx {
uintptr_t address(unsigned rva) {
    switch(rva) {
    case 0xf26ae8: return reinterpret_cast<uintptr_t>(&current);
    case 0x46a7c0: return reinterpret_cast<uintptr_t>(&workerGetter);
    case 0x46c500: return reinterpret_cast<uintptr_t>(&storyGetter);
    case 0x4713a0: return reinterpret_cast<uintptr_t>(&storySetter);
    case 0x46fed0: return reinterpret_cast<uintptr_t>(&warp);
    }
    check(false,"unexpected native address");return 0;
}
bool copy(uintptr_t p,void* out,size_t length) {
    __try { std::memcpy(out,reinterpret_cast<void*>(p),length);return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
unsigned generation() { return epoch; }
bool nativeGameThread() { return true; }
UtilityContext utilityContext() { return {}; }
bool utilityContextValid(const UtilityContext&,bool) { return true; }
bool utilityActionAllowed(bool) { return true; }
bool utilityBusy() { return false; }
void utilityReply(const wchar_t* text) { replies.push_back(text); }
bool deferNativeAction(NativeAction action) { queue.push_back(action);return true; }
bool nativeAtelSpan(uintptr_t chunkAt,AtelSpan& span) { span.chunk=chunkAt;span.serial=7;span.size=0x1000;return chunkAt==ptr(chunk); }
bool readyAtelDeclaration(uintptr_t,unsigned,unsigned low,unsigned high,unsigned& index) { index=0;return low==0x20000a00 && high==1; }
AtelPreparation atelPreparation(uintptr_t) { return AtelPreparation::Missing; }
bool readyAtelIdentity(uintptr_t,AtelIdentity&) { return false; }
bool readyAtelWorkerHeader(uintptr_t,unsigned,unsigned&) { return false; }
}

int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    globals=static_cast<uint8_t*>(VirtualAlloc(nullptr,0x2000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(globals!=nullptr,"globals page");

    reset(0x04);run(10,1,false);
    expect(warps==1 && controllerA[0]==0x04,"consumed jump leaves the bit clear and keeps other controller bits");

    reset(0x04);jump=Jump::FaultBeforeConsume;run(10,1,false);
    expect(warps==1 && controllerA[0]==0x04,"jump fault after the map-request generation change clears the added bit");

    reset(0x04);jump=Jump::FaultAfterSwap;run(10,1,false);
    expect(warps==1 && controllerA[0]==0x84 && controllerB[0]==0x04,"jump fault never touches a controller that is no longer current");

    reset(0x04);jump=Jump::SwapAroundConsume;run(10,1,false);
    expect(warps==1 && controllerA[0]==0x04 && controllerB[0]==0x04,"controller switch around the native clear still leaves the added bit clear");

    reset(0x84);jump=Jump::FaultBeforeConsume;run(10,1,false);
    expect(warps==1 && controllerA[0]==0x84,"a bit that was already set is not cleared by the mod");

    reset(0x04);protectCave=true;run(272,0,true);
    expect(warps==0 && setterCalls==0 && controllerA[0]==0x04,"Gagazet setup refusal clears the added bit");
    expect(replies.size()==1 && replies[0].find(L"could not reset the cave flags")!=std::wstring::npos,"Gagazet setup refusal is reported");

    reset(0x04);protectCave=true;loadingAfterInvoke=true;run(272,0,true);
    expect(warps==0 && controllerA[0]==0x04,"Gagazet setup refusal clears the bit even when complete is skipped");

    reset(0x04);setterFaults=true;loadingAfterInvoke=true;run(272,0,true);
    expect(setterCalls==1 && warps==0 && controllerA[0]==0x04,"story setter fault clears the bit even when complete is skipped");

    reset(0x04);setterFaults=true;run(272,0,true);
    expect(controllerA[0]==0x04 && replies.size()==1,"story setter fault clears the bit and reports failure");

    reset(0x04);put<uint16_t>(globals+0xa00,1531);run(272,0,true);
    expect(warps==0 && controllerA[0]==0x04 && replies.size()==1,"prepare refusal never sets the bit");

    reset(0x04);epoch=1;ffx::utilityWarp(10,1);++epoch;drain();
    expect(warps==0 && controllerA[0]==0x04,"stale command is discarded before prepare");

    std::printf("PASS: warp forced-jump bit, %u checks.\n",checks);
    return 0;
}
