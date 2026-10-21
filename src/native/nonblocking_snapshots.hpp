#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace ffx::async {

static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
              "ffx::async requires lock-free 32-bit atomics");

// Single-writer, bounded-read snapshot. A false return means the one attempt
// observed a write in progress or overlapping the read; callers may try later.
template <typename T>
class AtomicSnapshot final {
    static_assert(std::is_trivially_copyable<T>::value,
                  "snapshot payload must be trivially copyable");
    static_assert(sizeof(T) % sizeof(std::uint32_t) == 0,
                  "snapshot payload size must be a multiple of four");

    static constexpr std::size_t kWordCount = sizeof(T) / sizeof(std::uint32_t);

public:
    AtomicSnapshot() noexcept : version_(0), words_{} {}
    AtomicSnapshot(const AtomicSnapshot&) = delete;
    AtomicSnapshot& operator=(const AtomicSnapshot&) = delete;

    // Exactly one publisher may call this method at a time.
    void publish(const T& value) noexcept {
        std::array<std::uint32_t, kWordCount> local{};
        std::memcpy(local.data(), &value, sizeof(T));

        const std::uint32_t priorVersion = version_.load(std::memory_order_relaxed);
        const std::uint32_t writing = priorVersion + 1u;
        version_.store(writing, std::memory_order_seq_cst);
        for (std::size_t i = 0; i < kWordCount; ++i) {
            words_[i].store(local[i], std::memory_order_relaxed);
        }
        version_.store(writing + 1u, std::memory_order_release);
    }

    // Performs one bounded attempt and never waits or retries.
    bool tryRead(T& result) const noexcept {
        const std::uint32_t before = version_.load(std::memory_order_acquire);
        if ((before & 1u) != 0u) {
            return false;
        }

        std::array<std::uint32_t, kWordCount> local{};
        for (std::size_t i = 0; i < kWordCount; ++i) {
            local[i] = words_[i].load(std::memory_order_relaxed);
        }

        std::atomic_thread_fence(std::memory_order_acquire);
        const std::uint32_t after = version_.load(std::memory_order_acquire);
        if (before != after || (after & 1u) != 0u) {
            return false;
        }
        std::memcpy(&result, local.data(), sizeof(T));
        return true;
    }

private:
    std::atomic<std::uint32_t> version_;
    std::array<std::atomic<std::uint32_t>, kWordCount> words_;
};

// Bounded MPSC FIFO. Producers make one claim attempt; the single consumer
// owns dequeue_position_ and never exposes a slot's storage to callers.
template <typename T, std::size_t N>
class TryCaptureQueue final {
    static_assert(std::is_trivially_copyable<T>::value,
                  "queue payload must be trivially copyable");
    static_assert(N >= 2 && (N & (N - 1)) == 0,
                  "queue capacity must be a power of two and at least two");
    static_assert(N <= UINT32_MAX,
                  "queue capacity must fit the 32-bit sequence counter");

    struct Slot final {
        std::atomic<std::uint32_t> sequence;
        T value;
    };

public:
    TryCaptureQueue() noexcept : enqueue_position_(0), dequeue_position_(0), slots_{} {
        for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(N); ++i) {
            slots_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }
    TryCaptureQueue(const TryCaptureQueue&) = delete;
    TryCaptureQueue& operator=(const TryCaptureQueue&) = delete;

    // Multi-producer: exactly one strong CAS; contention is an explicit failure.
    bool tryPush(const T& value) noexcept {
        const std::uint32_t position = enqueue_position_.load(std::memory_order_relaxed);
        Slot& slot = slots_[position & static_cast<std::uint32_t>(N - 1)];
        if (slot.sequence.load(std::memory_order_acquire) != position) {
            return false;
        }

        std::uint32_t expected = position;
        if (!enqueue_position_.compare_exchange_strong(
                expected, position + 1u, std::memory_order_relaxed,
                std::memory_order_relaxed)) {
            return false;
        }

        std::memcpy(&slot.value, &value, sizeof(T));
        slot.sequence.store(position + 1u, std::memory_order_release);
        return true;
    }

    // Single-consumer only. A true result copies the item into caller storage.
    bool tryPop(T& result) noexcept {
        const std::uint32_t position = dequeue_position_;
        Slot& slot = slots_[position & static_cast<std::uint32_t>(N - 1)];
        if (slot.sequence.load(std::memory_order_acquire) != position + 1u) {
            return false;
        }

        std::memcpy(&result, &slot.value, sizeof(T));
        slot.sequence.store(position + static_cast<std::uint32_t>(N),
                            std::memory_order_release);
        dequeue_position_ = position + 1u;
        return true;
    }

private:
    std::atomic<std::uint32_t> enqueue_position_;
    std::uint32_t dequeue_position_;
    std::array<Slot, N> slots_;
};

} // namespace ffx::async
