#pragma once
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>
namespace trading {
using OrderId = std::uint64_t;
using Price = std::int64_t;
using Quantity = std::uint64_t;
enum class Side { Buy, Sell };
enum class Type { Limit, Market };
enum class Status { Accepted, Rejected, Filled, PartiallyFilled, Cancelled, NotFound };
struct Request { OrderId id; Side side; Type type; Price price; Quantity quantity; };
struct Fill { OrderId aggressor; OrderId resting; Price price; Quantity quantity; };
struct Result { Status status; Quantity remaining; std::vector<Fill> fills; };
class OrderBook {
    struct Resting { OrderId id; Quantity remaining; };
    using Level = std::list<Resting>;
    using Bids = std::map<Price, Level, std::greater<Price>>;
    using Asks = std::map<Price, Level>;
    struct Location { Side side; Price price; Level::iterator it; };
    Bids bids_;
    Asks asks_;
    std::unordered_map<OrderId, Location> locations_;
    std::unordered_map<OrderId, bool> seen_;
public:
    Result submit(Request request);
    Status cancel(OrderId id);
    [[nodiscard]] std::optional<Price> best_bid() const;
    [[nodiscard]] std::optional<Price> best_ask() const;
    [[nodiscard]] std::size_t resting_count() const { return locations_.size(); }
};
}
