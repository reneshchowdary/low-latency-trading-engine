#include "trading/order_book.hpp"
#include <algorithm>
#include <iterator>
#include <utility>
namespace trading {
Result OrderBook::submit(Request request) {
    if (request.id == 0 || request.quantity == 0 ||
        (request.type == Type::Limit && request.price <= 0) || seen_.contains(request.id)) {
        return {Status::Rejected, request.quantity, {}};
    }
    seen_.emplace(request.id, true);
    Quantity remaining = request.quantity;
    std::vector<Fill> fills;
    if (request.side == Side::Buy) {
        while (remaining && !asks_.empty()) {
            auto level = asks_.begin();
            if (request.type == Type::Limit && level->first > request.price) break;
            auto& fifo = level->second;
            auto resting = fifo.begin();
            const auto qty = std::min(remaining, resting->remaining);
            fills.push_back({request.id, resting->id, level->first, qty});
            remaining -= qty;
            resting->remaining -= qty;
            if (!resting->remaining) { locations_.erase(resting->id); fifo.erase(resting); }
            if (fifo.empty()) asks_.erase(level);
        }
        if (remaining && request.type == Type::Limit) {
            auto& fifo = bids_[request.price];
            fifo.push_back({request.id, remaining});
            locations_.emplace(request.id, Location{Side::Buy, request.price, std::prev(fifo.end())});
        }
    } else {
        while (remaining && !bids_.empty()) {
            auto level = bids_.begin();
            if (request.type == Type::Limit && level->first < request.price) break;
            auto& fifo = level->second;
            auto resting = fifo.begin();
            const auto qty = std::min(remaining, resting->remaining);
            fills.push_back({request.id, resting->id, level->first, qty});
            remaining -= qty;
            resting->remaining -= qty;
            if (!resting->remaining) { locations_.erase(resting->id); fifo.erase(resting); }
            if (fifo.empty()) bids_.erase(level);
        }
        if (remaining && request.type == Type::Limit) {
            auto& fifo = asks_[request.price];
            fifo.push_back({request.id, remaining});
            locations_.emplace(request.id, Location{Side::Sell, request.price, std::prev(fifo.end())});
        }
    }
    const auto status = remaining == 0 ? Status::Filled :
        (!fills.empty() ? Status::PartiallyFilled : Status::Accepted);
    return {status, remaining, std::move(fills)};
}
Status OrderBook::cancel(OrderId id) {
    auto found = locations_.find(id);
    if (found == locations_.end()) return Status::NotFound;
    const auto location = found->second;
    if (location.side == Side::Buy) {
        auto level = bids_.find(location.price);
        level->second.erase(location.it);
        if (level->second.empty()) bids_.erase(level);
    } else {
        auto level = asks_.find(location.price);
        level->second.erase(location.it);
        if (level->second.empty()) asks_.erase(level);
    }
    locations_.erase(found);
    return Status::Cancelled;
}
std::optional<Price> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}
std::optional<Price> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}
}
