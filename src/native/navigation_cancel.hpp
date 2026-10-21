#pragma once
#include <atomic>
namespace ffx::nav {
// Only copied-data computations install this scope. Native callers have no
// predicate and never wait or invoke worker code. TLS is trivial POD storage.
struct ComputationCancelled {};
inline thread_local const std::atomic<unsigned>* computationRevision=nullptr;
inline thread_local unsigned computationRequest=0;
class ComputationScope final {
    const std::atomic<unsigned>* previousRevision;
    unsigned previousRequest;
public:
    ComputationScope(const std::atomic<unsigned>& revision,unsigned request) noexcept
        : previousRevision(computationRevision),previousRequest(computationRequest) {
        computationRevision=&revision;computationRequest=request;
    }
    ~ComputationScope() noexcept {
        computationRevision=previousRevision;computationRequest=previousRequest;
    }
    ComputationScope(const ComputationScope&)=delete;
    ComputationScope& operator=(const ComputationScope&)=delete;
};
inline void checkComputation() {
    if(computationRevision && computationRevision->load(std::memory_order_acquire)!=computationRequest)
        throw ComputationCancelled{};
}
}
