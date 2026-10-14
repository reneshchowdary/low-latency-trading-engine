# Low-Latency Trading Engine (C++20)

An independently implemented educational matching engine for Linux systems and performance engineering.

## Milestone 1: matching core

Implemented: limit and market orders, FIFO price-time priority, execution at the resting order's price, partial fills, cancellation, integer-tick prices, duplicate order-ID rejection, unit tests, and a simple throughput benchmark.

**Not yet implemented:** TCP networking, lock-free queues, memory pools, risk management, journaling, replay, or p99 latency measurement. No claim of superior performance has been established.

## Build and test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/trading_demo
```

## Benchmark

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j
./build-release/matching_benchmark 1000000
```

The benchmark measures only a synthetic single-threaded crossing workload, **not** real-world network-to-execution latency. It must not be compared directly to third-party benchmarks without equivalent workloads and hardware.

## Architecture

A single thread owns the book. Bids use descending ordered price levels, asks use ascending ordered levels, and linked FIFO queues preserve priority within a level. An order-ID index supports cancellation. The initial implementation uses standard containers to make correctness straightforward; it is not allocation-free.

## Roadmap

1. More comprehensive deterministic and randomized correctness tests
2. Fixed-capacity pools and alternative cache-efficient order-book representations
3. Bounded queues, sequencer and thread lifecycle management
4. Nonblocking framed TCP gateway and backpressure
5. Risk checks, journal/replay, profiling and reproducible latency measurements

## Reference

Inspired by general limit-order-book concepts explored in [Anubhaw6557/trading](https://github.com/Anubhaw6557/trading). This is an independently written implementation, not a copy of that repository's source.
