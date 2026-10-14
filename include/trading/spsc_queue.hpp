#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>
namespace trading {
// Single producer, single consumer; capacity must be a power of two.
// Producer and consumer must be distinct or the same thread, never multiple producers.
template<class T, std::size_t Capacity>
class SpscQueue {
    static_assert(Capacity >= 2 && (Capacity & (Capacity - 1)) == 0);
    static_assert(std::is_nothrow_move_assignable_v<T>);
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
    std::array<T, Capacity> slots_{};
public:
    [[nodiscard]] bool try_push(T value) noexcept {
        const auto tail = tail_.load(std::memory_order_relaxed);
        if (tail - head_.load(std::memory_order_acquire) == Capacity) return false;
        slots_[tail & (Capacity - 1)] = std::move(value);
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }
    [[nodiscard]] bool try_pop(T& value) noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire)) return false;
        value = std::move(slots_[head & (Capacity - 1)]);
        head_.store(head + 1, std::memory_order_release);
        return true;
    }
    [[nodiscard]] std::size_t approximate_size() const noexcept {
        return tail_.load(std::memory_order_acquire) - head_.load(std::memory_order_acquire);
    }
};
}
