# Low-Latency Trading Engine — C++20 / Linux

An independent, runnable **real-time exchange gateway and order matching simulator**. This repository is a systems-engineering portfolio project, not a live exchange or financial service.

## Implemented

- Single-writer limit-order book with price-time FIFO priority
- Limit/market orders, partial fills, cancellation, duplicate ID rejection
- Trades execute at the **resting** order's price
- Linux nonblocking TCP gateway with poll, partial-read framing and bounded per-client buffers
- Multiple simultaneous clients, explicit invalid-command errors and connection cleanup
- C++20 SPSC bounded ring queue using acquire/release atomics (tested independently; **not yet wired into the TCP matching path**)
- Unit tests, multithreaded queue test, TCP integration test, CTest and GCC/Clang CI
- Synthetic throughput benchmark (single-threaded; not end-to-end)

## Build and test (Linux)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Run a real TCP trading session

Terminal 1:

```bash
./build/trading_gateway 9000
```

Terminal 2:

```bash
nc 127.0.0.1 9000
ORDER 1 SELL LIMIT 10100 50
ORDER 2 BUY LIMIT 10200 20
TOP
STATS
CANCEL 1
```

Commands use newline-delimited ASCII. Prices are **integer ticks** and quantities are positive integers.

- `ORDER <id> BUY|SELL LIMIT|MARKET <price_ticks> <quantity>`
- `CANCEL <id>`
- `TOP`
- `STATS`
- `QUIT` (returns BYE; session may remain open)

The gateway binds to `127.0.0.1` by default. It is for local demonstration, **not** deployment on an untrusted network.

## Benchmark

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j
./build-release/matching_benchmark 1000000
```

This benchmark uses a synthetic single-threaded crossing workload. It does **not** establish p99 latency, production throughput, or superiority over any other implementation. For meaningful comparisons, record CPU, compiler, optimization flags, workload and test conditions.

## Architecture

```text
TCP clients
    |
nonblocking poll gateway (framed ASCII protocol)
    |
single-writer OrderBook
    |-- ordered bid/ask price levels
    |-- FIFO resting orders
    |-- order ID lookup / cancellation
    |
execution responses / top-of-book / statistics

SPSC ring queue: independently implemented and tested; future threaded pipeline
```

## Engineering limitations and next steps

This is an **end-to-end runnable prototype**, not a production-ready low-latency exchange. Current matching uses `std::map`, `std::list`, and dynamic allocations. The gateway and matcher run in the same event loop; a slow client is bounded but the design has no durable order journal. The gateway is not authenticated or encrypted, and no real-money orders should be sent to it.

Future engineering work: reference-model/property tests, fixed-capacity pools, sequenced queue pipeline, pre-trade risk limits, journaling and deterministic replay, latency histograms and controlled performance tuning.

The implementation and documentation should be evaluated on demonstrable behavior and reproducible measurements, not unverified speed claims.
