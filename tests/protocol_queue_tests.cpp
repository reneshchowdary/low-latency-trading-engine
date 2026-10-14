#include "trading/protocol.hpp"
#include "trading/spsc_queue.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
void require(bool v) { if (!v) std::abort(); }
int main() {
    using namespace trading;
    auto command = parse_command("ORDER 42 BUY LIMIT 10500 7");
    require(command && command->order.id == 42 && command->order.price == 10500);
    require(!parse_command("ORDER 42 BUY LIMIT abc 7"));
    require(!parse_command("ORDER 42 BUY LIMIT 10500 0"));
    require(!parse_command("ORDER -1 BUY LIMIT 10500 7"));
    require(!parse_command("ORDER 42 BUY LIMIT 10500 7 garbage"));
    require(parse_command("CANCEL 42")->cancel_id == 42);
    require(parse_command("TOP")->kind == Command::Kind::Top);
    SpscQueue<int, 8> queue;
    for (int i = 0; i < 8; ++i) require(queue.try_push(i));
    require(!queue.try_push(9));
    int value = -1;
    for (int i = 0; i < 8; ++i) { require(queue.try_pop(value)); require(value == i); }
    require(!queue.try_pop(value));
    constexpr int N = 100000;
    std::atomic<bool> okay{true};
    std::thread producer([&] {
        for (int i = 0; i < N; ++i)
            while (!queue.try_push(i)) std::this_thread::yield();
    });
    std::thread consumer([&] {
        for (int i = 0; i < N; ++i) {
            while (!queue.try_pop(value)) std::this_thread::yield();
            if (value != i) okay.store(false, std::memory_order_relaxed);
        }
    });
    producer.join(); consumer.join();
    require(okay.load());
    std::cout << "protocol and SPSC tests passed\n";
}
