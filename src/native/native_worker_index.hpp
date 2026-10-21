#pragma once
#include "events.hpp"
#include <array>
#include <atomic>
namespace ffx {
struct NativeWorkerIdentity {uintptr_t worker=0;unsigned global=0;};
namespace worker_index {
constexpr unsigned MaxLoaded=500,MaxLocal=1024;
struct Registry {
    uintptr_t controller=0,chunk=0,allocation=0;
    unsigned epoch=0,count=0,revision=0;
    std::array<NativeWorkerIdentity,MaxLocal> local{};
};
inline std::array<Registry,4> registries{};
inline unsigned nextRegistry=0;
inline std::atomic<unsigned> revision{1};
// Native-owner-only ready identity collection. No decoding, hash, feature
// search, allocating container, worker engine pointer or wait. A cold context
// is captured once; every subsequent lookup is one indexed getter validation.
inline bool valid(const Registry& r,unsigned local,NativeWorkerIdentity& result) {
    if(local>=MaxLocal) return false;
    const auto found=r.local[local];if(!found.worker) return false;
    using Getter=uintptr_t(__cdecl*)(int);
    if(reinterpret_cast<Getter>(address(0x46a7c0))(static_cast<int>(found.global))!=found.worker ||
        read<uintptr_t>(found.worker+4)!=r.chunk || read<uint16_t>(found.worker+0x2e,0xffff)!=local) return false;
    result=found;return true;
}
inline void invalidateNativeWorkerIndex() noexcept {worker_index::revision.fetch_add(1,std::memory_order_release);}
}
inline bool nativeWorkerIdentity(uintptr_t controller,uintptr_t chunk,unsigned local,NativeWorkerIdentity& result) {
    using namespace worker_index;
    using Getter=uintptr_t(__cdecl*)(int);
    const auto count=read<uint16_t>(controller+0xc);
    const auto allocation=read<uintptr_t>(controller+0x1c);const auto epoch=generation();
    const auto currentRevision=revision.load(std::memory_order_acquire);
    if(!controller || !chunk || !count || count>MaxLoaded || local>=MaxLocal) return false;
    Registry* cached=nullptr;
    for(auto& r:registries) if(r.epoch==epoch && r.revision==currentRevision && r.controller==controller && r.chunk==chunk && r.count==count && r.allocation==allocation) {
        if(valid(r,local,result)) return true;
        // A missing row is stable until a native identity changes. A populated
        // stale row must recollect the current owned controller's ready facts.
        if(!r.local[local].worker) return false;cached=&r;break;
    }
    if(!cached) cached=&registries[nextRegistry++%registries.size()];
    auto& r=*cached;r.epoch=0;r.controller=controller;r.chunk=chunk;r.count=count;r.allocation=allocation;
    r.local={};const auto get=reinterpret_cast<Getter>(address(0x46a7c0));
    for(unsigned global=0;global<count;++global) {
        const auto worker=get(static_cast<int>(global));if(!worker || read<uintptr_t>(worker+4)!=chunk) continue;
        const auto id=read<uint16_t>(worker+0x2e,0xffff);
        if(id<MaxLocal && !r.local[id].worker) r.local[id]={worker,global};
    }
    if(epoch!=generation() || revision.load(std::memory_order_acquire)!=currentRevision || read<uintptr_t>(address(0xf26ae8))!=controller ||
       read<uint16_t>(controller+0xc)!=count || read<uintptr_t>(controller+0x1c)!=allocation) return false;
    r.revision=currentRevision;r.epoch=epoch;return valid(r,local,result);
}
inline void invalidateNativeWorkerIndex() noexcept {worker_index::revision.fetch_add(1,std::memory_order_release);}
}
