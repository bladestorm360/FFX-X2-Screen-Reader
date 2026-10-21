#include "events.hpp"
#include "map_transition.hpp"
#include "native_image.hpp"
#include "resource_reset.hpp"
#include "hook_calls.hpp"
#include <MinHook.h>
#include <memory>
#include <map>
#include <mutex>
#include <atomic>

namespace ffx {
namespace {
struct Listener { Handler enter, leave; };
struct Hook { uintptr_t target = 0, original = 0; void* stub = nullptr;
    unsigned rva = 0;std::array<Listener,NativeListenerLimit> listeners{};
    std::atomic<unsigned> count{0};std::atomic<bool> snapshotOnly{false}; };

std::vector<std::unique_ptr<Hook>> hooks;
// Ordinary game/input/UI readers are affine to the native main update owner.
// 62F000 -> 62E4F0 -> 625930 and the retained FFXApplication vtable prove
// input, game update and UI rendering share that owner. 624F00 is the
// synchronous DirectInput EnumDevices callback, not a native worker.
// Qualified audio/loader snapshots copy on their own originating owner.
// Registry construction is bootstrap-only and never entered by callbacks.
std::mutex registryMutex;
std::atomic<unsigned> resourceDepth{0};
thread_local NativeCallStack calls;
std::atomic<bool> callOverflow{false};
thread_local bool inReader = false;
thread_local bool draining = false;
thread_local NativeActionQueue actions;
std::atomic<DWORD> gameOwner{0};
struct NativeErrorState {
    DWORD value=GetLastError();
    ~NativeErrorState() { SetLastError(value); }
};
bool failed = false;
bool resourceCall(unsigned rva) { return rva==0x46fed0 || rva==0x5093b0 || rva==map_transition::Initialize; }

void dispatch(Handler fn, Event& event) {
    if (!fn || (nativeResourceLoading() && !resourceCall(event.rva) && !event.captureOnly)) return;
    if(!resourceCall(event.rva) && !event.captureOnly && !nativeGameThread()) return;
    if(!resourceCall(event.rva) && !event.captureOnly && !ownerResetReady() && !serviceOwnerReset()) return;
    const DWORD started=GetTickCount();
    try { fn(event); }
    catch (...) { recordLog("reader exception rva=%x gen=%u", event.rva, generation()); }
    const DWORD elapsed=GetTickCount()-started;
    if(elapsed>=20) recordLog("slow reader rva=%x owner=%lu elapsed-ms=%lu",event.rva,GetCurrentThreadId(),elapsed);
}
extern "C" void __cdecl enterBridge(uintptr_t* registers);
extern "C" void __cdecl leaveBridge(uintptr_t* registers);

// A signature-independent x86 adapter keeps ECX/EDX, all integer registers,
// flags, x87 and SSE state intact. The original trampoline executes with its
// original argument stack, including thiscall/stdcall callee cleanup. The
// callback never calls the original with a guessed decompiler prototype.
__declspec(naked) void entry() {
    __asm {
        pushfd
        pushad
        mov esi, esp
        sub esp, 528
        and esp, -16
        fxsave [esp]
        sub esp, 12
        push esi
        call enterBridge
        add esp, 16
        fxrstor [esp]
        mov esp, esi
        popad
        popfd
        ret
    }
}
__declspec(naked) void leaving() {
    __asm {
        push 0
        pushfd
        pushad
        mov esi, esp
        sub esp, 528
        and esp, -16
        fxsave [esp]
        sub esp, 12
        push esi
        call leaveBridge
        add esp, 16
        fxrstor [esp]
        mov esp, esi
        popad
        popfd
        ret
    }
}
extern "C" void __cdecl enterBridge(uintptr_t* r) {
    NativeErrorState errorState;
    Hook* h = reinterpret_cast<Hook*>(r[9]);
    r[9] = h->original;
    if (inReader) return;
    if(h->rva==0x225930) observeGameThread();
    // User32/DirectInput methods are process-wide. SDL and other native
    // workers can enter them while the input owner waits inside a callback.
    // Reject foreign producers before acquiring the shared reader lock.
    if(h->rva>=0xffff0000 && h->rva<=0xffff0003) {
        const auto owner=gameOwner.load();
        if(owner && owner!=GetCurrentThreadId()) return;
    }
    if(h->rva>=0xfffe0000 && h->rva<=0xfffe0003 &&
        (!r[10] || loadedImageBase(r[10])!=address(0))) return;
    // Read the published count before policy: an appended owner listener first
    // clears snapshotOnly, then publishes count. Mixed sites require the game
    // owner; reviewed all-snapshot sites retain their originating owner.
    const auto listenerCount=h->count.load(std::memory_order_acquire);
    const bool snapshot=h->snapshotOnly.load(std::memory_order_acquire);
    if(!resourceCall(h->rva) && !snapshot && !nativeGameThread()) return;
    bool openingResource=false;
    NativeCall* pending=nullptr;
    try {
        const auto stack = reinterpret_cast<uintptr_t>(&r[10]);
        // If a game exception bypassed a return adapter, discard its copied
        // context when the stack has unwound. Normal nested calls grow down;
        // a native tail jump keeps the same stack and our pending return.
        while (!calls.empty() && calls.back().stack <= stack) {
            if(calls.back().stack==stack && r[10]==reinterpret_cast<uintptr_t>(leaving)) break;
            if(calls.back().loading) {
                --resourceDepth;calls.back().loading=false;
                recordLog("native reader resource scope unwound rva=%x depth=%u owner=%lu",static_cast<Hook*>(calls.back().hook)->rva,resourceDepth.load(),GetCurrentThreadId());
            }
            calls.pop();
        }
        if(calls.full() || !(pending=claimNativeCall())) {
            if(resourceCall(h->rva)) {
                callOverflow.store(true,std::memory_order_release);
                Event event;event.rva=h->rva;event.epoch=generation();
                copy(reinterpret_cast<uintptr_t>(&r[11]),event.args.data(),sizeof(event.args));
                inReader=true;for(unsigned i=0;i<listenerCount;++i) dispatch(h->listeners[i].enter,event);inReader=false;
            }
            recordLog("native call capture pressure rva=%x",h->rva);return;
        }
        auto& call=*pending;call.hook=h;call.returnAddress=r[10];call.stack=stack;
        call.loading=resourceCall(h->rva);
        if(call.loading) {
            ++resourceDepth; openingResource=true;
            recordLog("native reader resource scope begin rva=%x depth=%u owner=%lu",h->rva,resourceDepth.load(),GetCurrentThreadId());
        }
        const bool readerReady=call.loading || snapshot || (!nativeResourceLoading() && (ownerResetReady() || serviceOwnerReset()));
        for (unsigned i=0;i<listenerCount;++i) {
            const auto listener=h->listeners[i];
            auto& event=call.events[call.count++];
            event.rva = h->rva; event.ecx = r[6]; event.epoch = generation(); event.caller=r[10];
            event.edi=r[0];event.nativeStack=stack;
            event.parentRva=calls.empty()?0:static_cast<Hook*>(calls.back().hook)->rva;
            event.captureOnly=snapshot;
            // A function that entered during loading has no valid before
            // snapshot. Do not run its return reader after loading finishes.
            if(!readerReady) event.epoch=0;
            copy(reinterpret_cast<uintptr_t>(&r[11]), event.args.data(), sizeof(event.args));
            inReader = true; dispatch(listener.enter, event); inReader = false;

        }
        calls.push(pending);pending=nullptr;
        openingResource=false;
        calls.back().originalStarted=GetTickCount();
        r[10] = reinterpret_cast<uintptr_t>(leaving);
    } catch (...) {
        if(openingResource) --resourceDepth;
        retireNativeCall(pending);
        inReader = false; recordLog("hook context allocation failed rva=%x", h->rva);
    }
}
void drainActions() {
    if(draining || !nativeGameThread() || !calls.empty()) return;
    draining=true;
    // Bound work at one return boundary; a follow-up input event can drain more.
    for(unsigned count=0;count<8 && !actions.empty();++count) {
        NativeAction action;
        struct Retire {NativeAction& action;~Retire() {try {if(action.retire) action.retire(action);}catch(...) {recordLog("native action ownership retirement failed");}}} retire{action};
        bool ready=false;
        {
            if(nativeResourceLoading() || !ownerResetReady() || !calls.empty()) break;
            action=actions.take();
            if(action.epoch!=generation()) continue;
            inReader=true;
            try { ready=action.prepare && action.prepare(action); } catch(...) { recordLog("native action preparation failed"); }
            inReader=false;
        }
        bool success=false;
        if(ready) {
            try { action.invoke(action); success=true; }
            catch(...) { recordLog("deferred native action failed gen=%u",action.epoch); }
        }
        {
            if(action.epoch!=generation() || nativeResourceLoading() || !ownerResetReady()) continue;
            inReader=true;
            try { if(action.complete) action.complete(action,success); } catch(...) { recordLog("native action completion failed"); }
            inReader=false;
        }
    }
    draining=false;
}
extern "C" void __cdecl leaveBridge(uintptr_t* r) {
    NativeErrorState errorState;
    NativeCall* finished=nullptr;
    {
    // A return adapter exists only after its context has been stored.
    finished=calls.take();auto& call=*finished;auto* h=static_cast<Hook*>(call.hook);
    if(call.loading) {
        --resourceDepth;call.loading=false;
        recordLog("native reader resource scope end rva=%x depth=%u owner=%lu",h->rva,resourceDepth.load(),GetCurrentThreadId());
    }
    r[9] = call.returnAddress;
    inReader = true;
    const DWORD originalElapsed=GetTickCount()-call.originalStarted;
    if(originalElapsed>=100) recordLog("slow original span rva=%x owner=%lu elapsed-ms=%lu",h->rva,GetCurrentThreadId(),originalElapsed);
    for (size_t i = 0; i < call.count; ++i) {
        auto& e = call.events[i]; e.result = r[7];
        // Scene initialization intentionally spans nested resource generations.
        // Only its outermost completed return reads fresh destination state;
        // no old resource-dependent snapshot is revived for ordinary readers.
        if(h->rva==map_transition::Initialize && !resourceDepth) e.epoch=generation();
        // Teardown invalidates resource-dependent snapshots from older calls.
        if (e.epoch == generation()) {
            dispatch(h->listeners[i].leave, e);
            if(e.overrideResult) r[7]=e.result;
        }
    }
    inReader = false;
    }
    // Preserve the adapter's saved registers/FPU state while native work runs;
    // no callback lock or inReader suppression survives into a native call.
    retireNativeCall(finished);
    drainActions();
}
}
void abandonNativeCall(NativeCall* call) noexcept {
    if(call && call->loading) {--resourceDepth;call->loading=false;}
    retireNativeCall(call);
}
bool observeGameThread() {
    DWORD expected=0;
    gameOwner.compare_exchange_strong(expected,GetCurrentThreadId());
    return nativeGameThread();
}
bool nativeGameThread() { return gameOwner.load()==GetCurrentThreadId(); }
bool nativeResourceLoading() noexcept {return callOverflow.load(std::memory_order_acquire) || resourceDepth.load(std::memory_order_acquire)!=0;}
bool nativeReaderResourcesReady() noexcept {return !nativeResourceLoading() && ownerResetReady();}
bool tryNativeReaderAccess() noexcept {
    NativeErrorState errorState;
    if(!nativeGameThread()) return false;
    if(nativeResourceLoading() || !serviceOwnerReset()) return false;
    return true;
}
void releaseNativeReaderAccess() noexcept {} // Owner access has no cross-thread lock to release.
bool deferNativeAction(NativeAction action) {
    if(!nativeGameThread() || actions.size()>=8 || !action.prepare || !action.invoke) return false;
    return actions.push(action);
}
bool hookAddressKind(uintptr_t target,unsigned id,Handler before,Handler after,bool snapshot) {
    // Production registrations are all bootstrap calls. Native callbacks may
    // never enter registry allocation, VirtualAlloc or MinHook operations.
    if(inReader) {recordLog("native callback hook registration refused id=%x",id);return false;}
    std::lock_guard<std::mutex> lock(registryMutex);
    for (auto& h : hooks) if (h->target == target) {
        const auto count=h->count.load(std::memory_order_acquire);
        for(unsigned i=0;i<count;++i) if(h->listeners[i].enter==before && h->listeners[i].leave==after) return true;
        if(count==NativeListenerLimit) {failed=true;return false;}
        if(!snapshot) h->snapshotOnly.store(false,std::memory_order_release);
        h->listeners[count]={before,after};h->count.store(count+1,std::memory_order_release);return true;
    }
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(reinterpret_cast<void*>(target), &info, sizeof(info)) ||
        info.State != MEM_COMMIT || !(info.Protect & 0xf0)) {
        failed = true; recordLog("hook target unavailable id=%x", id); return false;
    }
    auto h = std::make_unique<Hook>(); h->target = target; h->rva = id;
    h->listeners[0]={before,after};h->snapshotOnly.store(snapshot);h->count.store(1,std::memory_order_release);
    h->stub = VirtualAlloc(nullptr, 16, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!h->stub) { failed = true; return false; }
    auto bytes = static_cast<uint8_t*>(h->stub);
    bytes[0] = 0x68; *reinterpret_cast<uintptr_t*>(bytes + 1) = reinterpret_cast<uintptr_t>(h.get());
    bytes[5] = 0xe9; *reinterpret_cast<int32_t*>(bytes + 6) =
        static_cast<int32_t>(reinterpret_cast<uintptr_t>(entry) - reinterpret_cast<uintptr_t>(bytes + 10));
    DWORD protection; VirtualProtect(bytes, 16, PAGE_EXECUTE_READ, &protection);
    FlushInstructionCache(GetCurrentProcess(), bytes, 16);
    auto result = MH_CreateHook(reinterpret_cast<void*>(target), h->stub, reinterpret_cast<void**>(&h->original));
    if (result != MH_OK) { failed = true; recordLog("hook create id=%x: %s", id, MH_StatusToString(result)); }
    else if (MH_QueueEnableHook(reinterpret_cast<void*>(target)) != MH_OK) failed = true;
    hooks.push_back(std::move(h)); return !failed;
}
bool hookAddress(uintptr_t target,unsigned id,Handler before,Handler after) {return hookAddressKind(target,id,before,after,false);}
bool hook(unsigned rva, Handler before, Handler after) { return hookAddress(address(rva), rva, before, after); }
bool hookSnapshot(unsigned rva,Handler before,Handler after) {return hookAddressKind(address(rva),rva,before,after,true);}
bool enableHooks() {
    if (failed || MH_ApplyQueued() != MH_OK) { discardHooks(); return false; }
    recordLog("native hook batch enabled targets=%u", static_cast<unsigned>(hooks.size())); return true;
}
bool applyAddedHooks() {
    // Isolated quiescent fixture registration; production COM uses its bounded
    // owner-side vtable adapter and never calls MinHook at runtime.
    const auto status = MH_ApplyQueued();
    if (status != MH_OK) recordLog("native added-hook batch failed: %s", MH_StatusToString(status));
    return status == MH_OK;
}
void discardHooks() {
    MH_DisableHook(MH_ALL_HOOKS); MH_Uninitialize();
    for (auto& h : hooks) if (h->stub) VirtualFree(h->stub, 0, MEM_RELEASE);
    hooks.clear();
}
}
