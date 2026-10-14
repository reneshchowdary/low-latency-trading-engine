#include "trading/order_book.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
int main(int argc, char** argv) {
    const auto n = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 100000ULL;
    trading::OrderBook book;
    std::uint64_t fills = 0;
    const auto start = std::chrono::steady_clock::now();
    for (std::uint64_t i = 1; i <= n; ++i) {
        const auto side = i % 2 ? trading::Side::Buy : trading::Side::Sell;
        const auto result = book.submit({i, side, trading::Type::Limit, 10000, 1});
        fills += result.fills.size();
    }
    const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "orders=" << n << " fills=" << fills << " seconds=" << seconds
              << " orders_per_second=" << (seconds ? n / seconds : 0) << '\n';
}
