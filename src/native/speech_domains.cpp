#include "speech_domains.hpp"
#include <atomic>
namespace ffx::speechq {
namespace {
std::atomic<DomainHandler> handlers[DomainLimit];
std::atomic<DomainReset> resets[DomainLimit];
std::atomic<DomainMaintenance> maintenance[DomainLimit];
std::atomic<DomainProtection> protection[DomainLimit];
std::atomic<DomainMaintenance> protectionReset[DomainLimit];
unsigned retiredEpoch=0;
}
void registerDomain(unsigned domain,DomainHandler handler,DomainReset reset) {
    if(domain>=DomainLimit) return;
    resets[domain].store(reset,std::memory_order_release);
    handlers[domain].store(handler,std::memory_order_release);
}
void registerDomainProtection(unsigned domain,DomainProtection fn,DomainMaintenance reset) {
    if(domain>=DomainLimit) return;
    protectionReset[domain].store(reset,std::memory_order_release);protection[domain].store(fn,std::memory_order_release);
}
bool domainsProtectSpeech(const Header& header) {
    for(auto& entry:protection) if(const auto fn=entry.load(std::memory_order_acquire)) if(fn(header)) return true;
    return false;
}
void resetDomainProtection() {
    for(auto& entry:protectionReset) if(const auto fn=entry.load(std::memory_order_acquire)) fn();
}
void registerDomainMaintenance(unsigned domain,DomainMaintenance fn) {
    if(domain<DomainLimit) maintenance[domain].store(fn,std::memory_order_release);
}
void maintainDomains() {
    for(auto& entry:maintenance) if(const auto fn=entry.load(std::memory_order_acquire)) fn();
}
void dispatchDomain(const Header& header,const std::wstring& text) {
    if(header.control<DomainBase || header.control>=DomainBase+DomainLimit || header.generation!=generation()) return;
    // Current-generation copied state is prepared even when its old speech was
    // canceled by a review key. emitOwned retains the original cancellation
    // anchor; the fresh key can then read the owned buffer without a getter.
    advanceDomains(header.generation);
    if(const auto handler=handlers[header.control-DomainBase].load(std::memory_order_acquire)) handler(header,text);
}
void resetDomains(unsigned epoch) {
    for(auto& reset:resets) if(const auto fn=reset.load(std::memory_order_acquire)) fn(epoch);
}
void advanceDomains(unsigned epoch) {
    if(retiredEpoch && static_cast<int>(epoch-retiredEpoch)<=0) return;
    retiredEpoch=epoch;resetDomains(epoch);
}
}
