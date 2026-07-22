# hft-order-engine

A low-latency limit order book matching engine built in C++20, developed iteratively
from correctness-first to cache-optimized. This project is designed as a learning
journey toward quantitative trading system development.

**Current: Iteration 1 — Make it Work**

Uses `std::map` + `std::deque` for correctness-first implementation. Later iterations
will progressively introduce flat arrays, intrusive linked lists, memory pools, and
deterministic replay.

## Quick Start

### Prerequisites

- C++20 compiler (GCC 10+, Clang 12+, Apple Clang 14+)
- CMake 3.20+

### Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Run (Interactive)

```bash
./build/hft_order_engine
```

Type commands:
```
SUBMIT 1 BUY 15000 100
SUBMIT 2 SELL 15100 50
PRINT
```

### Run (Batch Replay)

```bash
./build/hft_order_engine < tests/fixtures/basic_orders.txt
```

### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

## Command Protocol

| Command | Format | Example |
|---|---|---|
| SUBMIT | `SUBMIT <id> <BUY\|SELL> <price> <qty> [GTC\|IOC\|FOK]` | `SUBMIT 1 BUY 15000 100 GTC` |
| CANCEL | `CANCEL <id>` | `CANCEL 1` |
| MODIFY | `MODIFY <id> <new_price> <new_qty>` | `MODIFY 1 15100 200` |
| MARKET | `MARKET <id> <BUY\|SELL> <qty>` | `MARKET 3 BUY 50` |
| PRINT  | `PRINT` | `PRINT` |

Prices are in integer cents (e.g., `15000` = $150.00).

## Architecture

```
stdin → Parser → OrderBook → Formatter → stdout
```

- **Parser**: Text commands → `std::variant<SubmitAction, CancelAction, ...>`
- **OrderBook**: `std::map<Price, PriceLevel>` bid/ask sides, price-time priority
- **Formatter**: Event variant → human-readable text

## Project Structure

```
include/hft/core/     → types.hpp, order.hpp, action.hpp, event.hpp
include/hft/book/     → price_level.hpp, order_book.hpp
include/hft/io/       → parser.hpp, formatter.hpp
src/                  → implementation (.cpp)
tests/                → GoogleTest suite (41 tests) + fixtures
```

## Roadmap

| Iteration | Theme | Goal |
|---|---|---|
| 1 (current) | Make it work | Correctness with std::map + std::deque |
| 2 | Make it right | Flat array price levels, tick pricing |
| 3 | Make it fast | Intrusive lists, memory pool, zero-alloc |
| 4 | Make it reliable | Benchmark suite, flamegraph, replay |
| 5 | Make it real | CTP/XTP exchange gateway |

## References

Built with reference to three open-source matching engines:
- [Simple-HFT-Engine](https://github.com/saksham10arora-dotcom/Simple-HFT-Engine)
- [low-latency-matching-engine](https://github.com/erictzhou/low-latency-matching-engine)
- [order-matching-engine](https://github.com/PIYUSH-KUMAR1809/order-matching-engine)

## License

MIT
