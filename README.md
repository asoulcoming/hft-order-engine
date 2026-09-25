# hft-order-engine

A low-latency limit order book matching engine built in C++20, developed iteratively
from correctness-first to cache-optimized. This project is designed as a learning
journey toward quantitative trading system development.

**Current: Engine Iteration 1 complete — starting Stage 0 of the [trading-system roadmap](ROADMAP.md)**

Iteration 1 uses `std::map` + `std::deque` for a correctness-first implementation.
The project now targets a full, production-grade HFT trading system (market data,
strategy, risk, OMS, gateway, backtesting) — see [ROADMAP.md](ROADMAP.md) for the
staged plan with acceptance gates, and [JOURNAL.md](JOURNAL.md) for the progress log.

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
| SUBMIT | `SUBMIT <id> <BUY\|SELL> <price> <qty> [GFD\|IOC\|FOK]` | `SUBMIT 1 BUY 15000 100 GFD` |
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

The project follows a staged trading-system roadmap. Full details and acceptance
gates live in [ROADMAP.md](ROADMAP.md); progress is logged in [JOURNAL.md](JOURNAL.md).

| Stage | Theme | Status |
|---|---|---|
| 0 | Engineering foundation (CI, lint, journal) | in progress |
| 1 | Engine: make it right (flat arrays, multi-symbol, replay tests) | |
| 2 | Engine: make it fast (intrusive lists, memory pool, benchmarks) | |
| 3 | Engine: make it reliable (deterministic replay, WAL, crash recovery) | |
| 4 | Backtesting framework (data → strategy → simulated fills → report) | |
| 5 | Live simulation loop (testnet: market data / strategy / risk / OMS / gateway) | |
| 6 | Strategy & execution depth (real strategies, execution algos, latency tracing) | |
| 7 | Small-size live trading (optional, gated by strict preconditions) | |

## References

Three open-source matching engines, vendored as git submodules under
`references/` (clone this repo with `--recursive`, or run
`git submodule update --init --recursive` in an existing checkout):

- [Simple-HFT-Engine](https://github.com/saksham10arora-dotcom/Simple-HFT-Engine)
- [low-latency-matching-engine](https://github.com/erictzhou/low-latency-matching-engine)
- [order-matching-engine](https://github.com/PIYUSH-KUMAR1809/order-matching-engine)

## License

MIT
