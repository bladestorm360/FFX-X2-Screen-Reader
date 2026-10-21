#include "owned_retirement.hpp"
#include <atomic>
namespace ffx::retirement {
namespace {
std::atomic<PoolBase*> head{nullptr};
std::atomic<unsigned> failed{0};
std::atomic<HANDLE> signal{nullptr};
std::atomic<bool> started{false};
void drain() noexcept {
    for(auto* pool=head.load(std::memory_order_acquire);pool;pool=pool->next)
        pool->drain(pool);
}
DWORD WINAPI worker(void*) {
    for(;;) {drain();WaitForSingleObject(signal.load(std::memory_order_acquire),INFINITE);}
}
}
bool registerPool(PoolBase* pool) noexcept {
    // Bootstrap-only; registration remains safe if the worker has started.
    pool->next=head.load(std::memory_order_relaxed);
    head.store(pool,std::memory_order_release);return true;
}
void wake() noexcept {if(const auto event=signal.load(std::memory_order_acquire)) SetEvent(event);}
void rejected() noexcept {failed.fetch_add(1,std::memory_order_relaxed);}
unsigned failures() noexcept {return failed.load(std::memory_order_acquire);}
bool start(HMODULE module) noexcept {
    if(started.load(std::memory_order_acquire)) return true;
    HMODULE pinned=nullptr;(void)module;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&start),&pinned)) return false;
    const auto event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event) return false;
    signal.store(event,std::memory_order_release);
    const auto thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);
    if(!thread) return false; // Published pools/event are process-lifetime.
    CloseHandle(thread);started.store(true,std::memory_order_release);return true;
}
#ifdef FFX_RETIREMENT_TEST
void drainForTest() noexcept {drain();}
#endif
}
