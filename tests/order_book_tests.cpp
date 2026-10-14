#include "trading/order_book.hpp"
#include <cassert>
#include <iostream>
using namespace trading;
int main() {
    OrderBook book;
    assert(book.submit({1, Side::Sell, Type::Limit, 101, 10}).status == Status::Accepted);
    assert(book.submit({2, Side::Sell, Type::Limit, 101, 20}).status == Status::Accepted);
    auto result = book.submit({3, Side::Buy, Type::Limit, 105, 15});
    assert(result.status == Status::Filled && result.fills.size() == 2);
    assert(result.fills[0].resting == 1 && result.fills[0].price == 101 && result.fills[0].quantity == 10);
    assert(result.fills[1].resting == 2 && result.fills[1].quantity == 5);
    assert(book.resting_count() == 1 && book.best_ask() == 101);
    assert(book.cancel(2) == Status::Cancelled && book.cancel(2) == Status::NotFound);
    assert(book.submit({3, Side::Buy, Type::Limit, 99, 1}).status == Status::Rejected);
    assert(book.submit({4, Side::Buy, Type::Limit, -1, 1}).status == Status::Rejected);
    assert(book.submit({5, Side::Buy, Type::Limit, 100, 0}).status == Status::Rejected);
    assert(book.submit({6, Side::Buy, Type::Limit, 99, 7}).status == Status::Accepted);
    auto market = book.submit({7, Side::Sell, Type::Market, 0, 10});
    assert(market.status == Status::PartiallyFilled && market.remaining == 3);
    assert(market.fills.size() == 1 && market.fills[0].price == 99);
    assert(book.resting_count() == 0 && !book.best_bid() && !book.best_ask());
    assert(book.submit({8, Side::Sell, Type::Market, 0, 1}).status == Status::Accepted);
    assert(book.resting_count() == 0);
    std::cout << "all order book tests passed\n";
}
