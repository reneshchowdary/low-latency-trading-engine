#pragma once
#include "trading/order_book.hpp"
#include <cstdint>
#include <limits>
namespace trading {
// Stateless pre-trade validation. Caller retains single-writer ownership.
struct RiskLimits {
    Quantity max_order_quantity{1000000};
    Price max_price_ticks{1000000000};
    std::uint64_t max_notional_ticks{1000000000000ULL};
};
class RiskValidator {
    RiskLimits limits_;
public:
    explicit RiskValidator(RiskLimits limits = {}) : limits_(limits) {}
    [[nodiscard]] bool validate(const Request& order) const noexcept {
        if (!order.id || !order.quantity || order.quantity > limits_.max_order_quantity)
            return false;
        if (order.type == Type::Market) return true; // market notional requires book-aware validation
        if (order.price <= 0 || order.price > limits_.max_price_ticks)
            return false;
        const auto price = static_cast<std::uint64_t>(order.price);
        return price <= limits_.max_notional_ticks / order.quantity;
    }
};
}
