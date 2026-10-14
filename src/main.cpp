#include "trading/order_book.hpp"
#include <iostream>
int main() {
    trading::OrderBook book;
    book.submit({1, trading::Side::Sell, trading::Type::Limit, 10100, 50});
    const auto result = book.submit({2, trading::Side::Buy, trading::Type::Limit, 10200, 20});
    for (const auto& fill : result.fills)
        std::cout << "trade: " << fill.aggressor << " vs " << fill.resting
                  << " @ " << fill.price << " ticks x " << fill.quantity << '\n';
}
