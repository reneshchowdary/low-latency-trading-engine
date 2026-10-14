#include "trading/order_book.hpp"
#include <cstdlib>
#include <iostream>
#include <string_view>
using namespace trading;
void check(bool value, std::string_view what) {
    if (!value) { std::cerr << "FAILED: " << what << '\n'; std::exit(1); }
}
int main() {
    OrderBook book;
    check(book.submit({1, Side::Sell, Type::Limit, 101, 10}).status == Status::Accepted, "first ask");
    check(book.submit({2, Side::Sell, Type::Limit, 101, 20}).status == Status::Accepted, "second ask");
    auto result = book.submit({3, Side::Buy, Type::Limit, 105, 15});
    check(result.status == Status::Filled && result.fills.size() == 2, "two fills");
    check(result.fills[0].resting == 1 && result.fills[0].price == 101 && result.fills[0].quantity == 10, "FIFO first");
    check(result.fills[1].resting == 2 && result.fills[1].quantity == 5, "FIFO second");
    check(book.resting_count() == 1 && book.best_ask() == 101, "remaining ask");
    check(book.cancel(2) == Status::Cancelled && book.cancel(2) == Status::NotFound, "cancel");
    check(book.submit({3, Side::Buy, Type::Limit, 99, 1}).status == Status::Rejected, "duplicate id");
    check(book.submit({4, Side::Buy, Type::Limit, -1, 1}).status == Status::Rejected, "negative price");
    check(book.submit({5, Side::Buy, Type::Limit, 100, 0}).status == Status::Rejected, "zero qty");
    check(book.submit({6, Side::Buy, Type::Limit, 99, 7}).status == Status::Accepted, "resting bid");
    auto market = book.submit({7, Side::Sell, Type::Market, 0, 10});
    check(market.status == Status::PartiallyFilled && market.remaining == 3, "market remainder");
    check(market.fills.size() == 1 && market.fills[0].price == 99, "resting execution price");
    check(book.resting_count() == 0 && !book.best_bid() && !book.best_ask(), "book empty");
    check(book.submit({8, Side::Sell, Type::Market, 0, 1}).status == Status::Accepted, "unfilled market");
    check(book.resting_count() == 0, "market must not rest");
    OrderBook spread;
    check(spread.submit({10, Side::Buy, Type::Limit, 100, 5}).status == Status::Accepted, "bid");
    check(spread.submit({11, Side::Sell, Type::Limit, 102, 5}).status == Status::Accepted, "ask");
    check(spread.best_bid() == 100 && spread.best_ask() == 102, "spread");
    auto crossing = spread.submit({12, Side::Buy, Type::Limit, 103, 8});
    check(crossing.fills.size() == 1 && crossing.fills[0].price == 102, "aggressive buy resting price");
    check(crossing.remaining == 3 && crossing.status == Status::PartiallyFilled, "partial fill");
    check(spread.best_bid() == 103 && !spread.best_ask(), "residual rests");
    check(spread.cancel(12) == Status::Cancelled, "cancel residual");
    check(spread.best_bid() == 100, "previous bid restored");
    std::cout << "order book tests passed\n";
}
