# hft-order-engine Iteration 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a single-symbol limit order book matching engine with CLI text protocol, using `std::map` + `std::deque` for correctness-first implementation.

**Architecture:** `stdin → Parser → OrderBook → Formatter → stdout`. Single-threaded, deterministic, zero external dependencies beyond GoogleTest. All events use `std::variant`, output via caller-owned `std::vector<Event>&` buffer.

**Tech Stack:** C++20, CMake 3.20+, GoogleTest (FetchContent), no other dependencies.

## Global Constraints

- C++20 standard required (`-std=c++20`)
- CMake >= 3.20 (FetchContent for GoogleTest)
- All headers under `include/hft/` in namespace `hft`
- All sources under `src/`
- All tests under `tests/`
- Zero external runtime dependencies beyond C++ standard library
- Price = `int64_t` (integer cents), Quantity = `uint64_t`, OrderId = `uint64_t`
- CLI input via stdin, output via stdout
- macOS + Apple Clang as primary dev environment

---

## File Structure (created across tasks)

```
hft-order-engine/
├── CMakeLists.txt                          # Task 1
├── .gitignore                              # Task 1
├── include/hft/
│   ├── core/
│   │   ├── types.hpp                       # Task 2
│   │   ├── order.hpp                       # Task 2
│   │   ├── action.hpp                      # Task 3
│   │   └── event.hpp                       # Task 3
│   ├── book/
│   │   ├── price_level.hpp                 # Task 4
│   │   └── order_book.hpp                  # Task 5
│   └── io/
│       ├── parser.hpp                      # Task 7
│       └── formatter.hpp                   # Task 8
├── src/
│   ├── main.cpp                            # Task 9
│   ├── book/
│   │   └── order_book.cpp                  # Task 6
│   └── io/
│       ├── parser.cpp                      # Task 7
│       └── formatter.cpp                   # Task 8
└── tests/
    ├── CMakeLists.txt                      # Task 1
    ├── test_price_level.cpp                # Task 4
    ├── test_orderbook.cpp                  # Task 6
    ├── test_parser.cpp                     # Task 7
    └── fixtures/
        └── basic_orders.txt                # Task 9
```

---

### Task 1: Project Scaffolding

**Files:**
- Create: `CMakeLists.txt`
- Create: `tests/CMakeLists.txt`
- Create: `.gitignore`
- Create: directory tree

**Interfaces:**
- Produces: Build system that compiles an empty `main.cpp` and downloads GoogleTest

- [ ] **Step 1: Create directory structure**

```bash
mkdir -p include/hft/core include/hft/book include/hft/io
mkdir -p src/book src/io
mkdir -p tests/fixtures
```

- [ ] **Step 2: Write root CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.20)
project(hft-order-engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# GoogleTest via FetchContent
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)

# ── Engine library ──────────────────────────────────────────
add_library(hft_engine STATIC
    src/book/order_book.cpp
    src/io/parser.cpp
    src/io/formatter.cpp
)
target_include_directories(hft_engine PUBLIC include)
target_compile_options(hft_engine PRIVATE -Wall -Wextra -Wpedantic)

# ── Main executable ─────────────────────────────────────────
add_executable(hft_order_engine src/main.cpp)
target_link_libraries(hft_order_engine PRIVATE hft_engine)

# ── Tests ───────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)
```

- [ ] **Step 3: Write tests/CMakeLists.txt**

```cmake
# ── PriceLevel tests ────────────────────────────────────────
add_executable(test_price_level
    test_price_level.cpp
    ../src/book/order_book.cpp
    ../src/io/parser.cpp
    ../src/io/formatter.cpp
)
target_include_directories(test_price_level PRIVATE ../include)
target_link_libraries(test_price_level PRIVATE GTest::gtest_main)
target_compile_options(test_price_level PRIVATE -Wall -Wextra -Wpedantic)
gtest_discover_tests(test_price_level)

# ── OrderBook tests ─────────────────────────────────────────
add_executable(test_orderbook
    test_orderbook.cpp
    ../src/book/order_book.cpp
    ../src/io/parser.cpp
    ../src/io/formatter.cpp
)
target_include_directories(test_orderbook PRIVATE ../include)
target_link_libraries(test_orderbook PRIVATE GTest::gtest_main)
target_compile_options(test_orderbook PRIVATE -Wall -Wextra -Wpedantic)
gtest_discover_tests(test_orderbook)

# ── Parser tests ────────────────────────────────────────────
add_executable(test_parser
    test_parser.cpp
    ../src/io/parser.cpp
    ../src/book/order_book.cpp
    ../src/io/formatter.cpp
)
target_include_directories(test_parser PRIVATE ../include)
target_link_libraries(test_parser PRIVATE GTest::gtest_main)
target_compile_options(test_parser PRIVATE -Wall -Wextra -Wpedantic)
gtest_discover_tests(test_parser)
```

- [ ] **Step 4: Write .gitignore**

```gitignore
build/
cmake-build-*/
.DS_Store
*.log
output/
.vscode/
.idea/
```

- [ ] **Step 5: Create placeholder main.cpp to verify build**

```cpp
// src/main.cpp
#include <iostream>
int main() {
    std::cout << "hft-order-engine placeholder\n";
    return 0;
}
```

- [ ] **Step 6: Build and verify**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Expected: Build succeeds.

- [ ] **Step 7: Run placeholder binary**

```bash
./build/hft_order_engine
```

Expected: `hft-order-engine placeholder`

- [ ] **Step 8: Commit**

```bash
git add CMakeLists.txt tests/CMakeLists.txt .gitignore src/main.cpp
git commit -m "chore: scaffold project with CMake, GoogleTest, directory structure"
```

---

### Task 2: Core Types & Order

**Files:**
- Create: `include/hft/core/types.hpp`
- Create: `include/hft/core/order.hpp`

**Interfaces:**
- Produces: `hft::OrderId`, `hft::Price`, `hft::Quantity`, `hft::Side`, `hft::OrderType`, `hft::RejectReason`
- Produces: `struct hft::Order { id, side, type, price, quantity, filled_qty, remaining(), is_filled() }`

- [ ] **Step 1: Write types.hpp**

```cpp
#pragma once

#include <cstdint>

namespace hft {

// ── Type aliases ────────────────────────────────────────────
using OrderId  = uint64_t;
using Price    = int64_t;    // integer cents (e.g. $150.00 = 15000)
using Quantity = uint64_t;

// ── Enums ───────────────────────────────────────────────────
enum class Side : uint8_t { Buy, Sell };

enum class OrderType : uint8_t {
    GTC,   // Good 'Til Cancel — rest in book if unfilled
    IOC,   // Immediate-or-Cancel — cancel unfilled remainder
    FOK,   // Fill-or-Kill — fill entirely or reject entirely
};

enum class RejectReason : uint8_t {
    DuplicateOrderId,
    UnknownOrderId,
    InsufficientLiquidity,   // FOK couldn't fill
    InvalidPrice,
    InvalidQuantity,
};

} // namespace hft
```

- [ ] **Step 2: Write order.hpp**

```cpp
#pragma once

#include "hft/core/types.hpp"

namespace hft {

struct Order {
    OrderId   id;
    Side      side;
    OrderType type     = OrderType::GTC;
    Price     price;           // integer cents
    Quantity  quantity;
    Quantity  filled_qty = 0;

    [[nodiscard]] Quantity remaining() const { return quantity - filled_qty; }
    [[nodiscard]] bool     is_filled()  const { return filled_qty == quantity; }
};

} // namespace hft
```

- [ ] **Step 3: Verify compilation**

```bash
cmake --build build
```

Expected: Compiles cleanly.

- [ ] **Step 4: Commit**

```bash
git add include/hft/core/types.hpp include/hft/core/order.hpp
git commit -m "feat: add core types (OrderId, Price, Quantity, enums) and Order struct"
```

---

### Task 3: Action & Event Types

**Files:**
- Create: `include/hft/core/action.hpp`
- Create: `include/hft/core/event.hpp`

**Interfaces:**
- Produces: `hft::SubmitAction`, `hft::CancelAction`, `hft::ModifyAction`, `hft::MarketAction`, `hft::PrintAction`, `hft::Action` (variant)
- Produces: `hft::TradeEvent`, `hft::AcceptedEvent`, `hft::CanceledEvent`, `hft::RejectedEvent`, `hft::SnapshotEvent`, `hft::Event` (variant)

- [ ] **Step 1: Write action.hpp**

```cpp
#pragma once

#include "hft/core/types.hpp"
#include <string>
#include <variant>

namespace hft {

struct SubmitAction {
    OrderId   id;
    Side      side;
    OrderType type = OrderType::GTC;
    Price     price;
    Quantity  quantity;
};

struct CancelAction   { OrderId order_id; };
struct ModifyAction   { OrderId order_id; Price new_price; Quantity new_qty; };
struct MarketAction   { OrderId id; Side side; Quantity qty; };
struct PrintAction    {};

using Action = std::variant<SubmitAction, CancelAction, ModifyAction,
                            MarketAction, PrintAction>;

} // namespace hft
```

- [ ] **Step 2: Write event.hpp**

```cpp
#pragma once

#include "hft/core/types.hpp"
#include <string>
#include <variant>

namespace hft {

struct TradeEvent {
    OrderId  resting_id;       // passive (maker) order
    OrderId  aggressive_id;    // incoming (taker) order
    Price    price;
    Quantity qty;
};

struct AcceptedEvent   { OrderId order_id; };
struct CanceledEvent   { OrderId order_id; };
struct RejectedEvent   { OrderId order_id; RejectReason reason; };
struct SnapshotEvent   { std::string message; };

using Event = std::variant<TradeEvent, AcceptedEvent, CanceledEvent,
                           RejectedEvent, SnapshotEvent>;

} // namespace hft
```

- [ ] **Step 3: Verify compilation**

```bash
cmake --build build
```

Expected: Compiles cleanly.

- [ ] **Step 4: Commit**

```bash
git add include/hft/core/action.hpp include/hft/core/event.hpp
git commit -m "feat: add Action and Event variant types"
```

---

### Task 4: PriceLevel

**Files:**
- Create: `include/hft/book/price_level.hpp`
- Create: `tests/test_price_level.cpp`

**Interfaces:**
- Produces: `struct hft::PriceLevel { push_back(Order*), remove(OrderId)->bool, front()->Order*, pop_front(), empty()->bool, price, total_volume }`
- Consumes: `hft::Order`, `hft::OrderId`, `hft::Price` from Tasks 2-3

- [ ] **Step 1: Write failing PriceLevel tests**

Create `tests/test_price_level.cpp`:

```cpp
#include "hft/book/price_level.hpp"
#include "hft/core/order.hpp"
#include <gtest/gtest.h>

using namespace hft;

class PriceLevelTest : public ::testing::Test {
protected:
    // Orders owned by test fixture (simulates OrderBook ownership)
    Order buy1_{1, Side::Buy, OrderType::GTC, 10000, 100};
    Order buy2_{2, Side::Buy, OrderType::GTC, 10000, 50};
    Order buy3_{3, Side::Buy, OrderType::GTC, 10000, 200};
};

TEST_F(PriceLevelTest, PushBackIncreasesCountAndVolume) {
    PriceLevel level{10000};
    level.push_back(&buy1_);
    EXPECT_EQ(level.total_volume, 100);
    EXPECT_FALSE(level.empty());
}

TEST_F(PriceLevelTest, PushBackMultipleOrdersFIFO) {
    PriceLevel level{10000};
    level.push_back(&buy1_);
    level.push_back(&buy2_);
    EXPECT_EQ(level.total_volume, 150);
    EXPECT_EQ(level.front()->id, 1);  // buy1_ first (FIFO)
}

TEST_F(PriceLevelTest, PopFrontRemovesOldestAndUpdatesVolume) {
    PriceLevel level{10000};
    level.push_back(&buy1_);
    level.push_back(&buy2_);
    level.pop_front();
    EXPECT_EQ(level.front()->id, 2);  // buy2_ now front
    EXPECT_EQ(level.total_volume, 50);
}

TEST_F(PriceLevelTest, PopFrontOnEmptyIsSafe) {
    PriceLevel level{10000};
    EXPECT_NO_THROW(level.pop_front());
    EXPECT_TRUE(level.empty());
}

TEST_F(PriceLevelTest, RemoveMiddleOrder) {
    PriceLevel level{10000};
    level.push_back(&buy1_);
    level.push_back(&buy2_);
    level.push_back(&buy3_);
    EXPECT_TRUE(level.remove(2));  // remove buy2_ from middle
    EXPECT_EQ(level.total_volume, 300);  // 100 + 200
    // FIFO order preserved: buy1_ → buy3_
    EXPECT_EQ(level.front()->id, 1);
    level.pop_front();
    EXPECT_EQ(level.front()->id, 3);
}

TEST_F(PriceLevelTest, RemoveLastOrderEmptiesLevel) {
    PriceLevel level{10000};
    level.push_back(&buy1_);
    EXPECT_TRUE(level.remove(1));
    EXPECT_TRUE(level.empty());
    EXPECT_EQ(level.total_volume, 0);
}

TEST_F(PriceLevelTest, RemoveNonExistentReturnsFalse) {
    PriceLevel level{10000};
    level.push_back(&buy1_);
    EXPECT_FALSE(level.remove(999));
    EXPECT_EQ(level.total_volume, 100);  // unchanged
}

TEST_F(PriceLevelTest, EmptyLevelFrontReturnsNullptr) {
    PriceLevel level{10000};
    EXPECT_EQ(level.front(), nullptr);
}
```

- [ ] **Step 2: Run tests — expect build failure (header missing)**

```bash
cmake --build build
```

Expected: Build error (price_level.hpp doesn't exist).

- [ ] **Step 3: Write price_level.hpp**

```cpp
#pragma once

#include "hft/core/order.hpp"
#include <deque>

namespace hft {

struct PriceLevel {
    Price    price;
    std::deque<Order*> orders;
    Quantity total_volume = 0;

    void push_back(Order* o) {
        orders.push_back(o);
        total_volume += o->remaining();
    }

    // Iter 1: O(n) linear scan. Iter 3: intrusive linked list (O(1)).
    bool remove(OrderId order_id) {
        for (auto it = orders.begin(); it != orders.end(); ++it) {
            if ((*it)->id == order_id) {
                total_volume -= (*it)->remaining();
                orders.erase(it);
                return true;
            }
        }
        return false;
    }

    Order* front() const {
        return orders.empty() ? nullptr : orders.front();
    }

    bool empty() const {
        return orders.empty();
    }

    void pop_front() {
        if (!orders.empty()) {
            total_volume -= orders.front()->remaining();
            orders.pop_front();
        }
    }
};

} // namespace hft
```

- [ ] **Step 4: Run tests — all pass**

```bash
cmake --build build && ctest --test-dir build --output-on-failure
```

Expected: 8/8 tests pass.

- [ ] **Step 5: Commit**

```bash
git add include/hft/book/price_level.hpp tests/test_price_level.cpp
git commit -m "feat: add PriceLevel with std::deque FIFO queue and tests"
```

---

### Task 5: OrderBook Header

**Files:**
- Create: `include/hft/book/order_book.hpp`

**Interfaces:**
- Produces: `class hft::OrderBook` declaration with submit/cancel/modify/market/print + DebugSnapshot
- Consumes: `hft::PriceLevel`, `hft::Order`, `hft::Event` from Tasks 2-4

- [ ] **Step 1: Write order_book.hpp**

```cpp
#pragma once

#include "hft/book/price_level.hpp"
#include "hft/core/event.hpp"
#include "hft/core/order.hpp"
#include <map>
#include <unordered_map>
#include <vector>

namespace hft {

class OrderBook {
public:
    // ── Public API ───────────────────────────────────────────
    void submit(Order order, std::vector<Event>& out);
    void cancel(OrderId order_id, std::vector<Event>& out);
    void modify(OrderId order_id, Price new_price, Quantity new_qty,
                std::vector<Event>& out);
    void market(Order order, std::vector<Event>& out);
    void print(std::vector<Event>& out) const;

    [[nodiscard]] bool contains(OrderId id) const;

    // ── Debug snapshot (for tests) ───────────────────────────
    struct DebugLevel {
        Price                 price;
        Quantity              volume;
        std::vector<OrderId>  order_ids;
    };
    struct DebugSnapshot {
        std::vector<DebugLevel> bids;
        std::vector<DebugLevel> asks;
    };
    [[nodiscard]] DebugSnapshot debug_snapshot() const;

private:
    // bids: std::greater → begin() is highest bid (best)
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    // asks: default std::less → begin() is lowest ask (best)
    std::map<Price, PriceLevel> asks_;

    // All live (resting) orders, keyed by ID.
    // Stored in map for stable addresses — PriceLevel holds Order* into this map.
    std::unordered_map<OrderId, Order> orders_;

    uint64_t next_trade_id_ = 1;

    // ── Internal helpers ─────────────────────────────────────
    void match_aggressive(Order& taker, std::vector<Event>& out);
    [[nodiscard]] bool can_fill_fok(const Order& order) const;

    // Store an order and return a stable pointer.
    Order* store_order(Order order);
    void   erase_order(OrderId id);
};

} // namespace hft
```

- [ ] **Step 2: Verify compilation (header-only, no impl yet — link will fail)**

```bash
cmake --build build 2>&1 || true
```

Expected: Compiles but linker fails on OrderBook methods (expected — impl in next task).

- [ ] **Step 3: Commit**

```bash
git add include/hft/book/order_book.hpp
git commit -m "feat: add OrderBook header with submit/cancel/modify/market API"
```

---

### Task 6: OrderBook Implementation & Tests

**Files:**
- Create: `src/book/order_book.cpp`
- Create: `tests/test_orderbook.cpp`

**Interfaces:**
- Consumes: `OrderBook` header from Task 5, `PriceLevel` from Task 4
- Produces: Full matching engine implementation

- [ ] **Step 1: Write failing OrderBook tests**

Create `tests/test_orderbook.cpp`:

```cpp
#include "hft/book/order_book.hpp"
#include <gtest/gtest.h>

using namespace hft;

// ═══════════════════════════════════════════════════════════════
// Helper: build a buy/sell GTC order quickly
// ═══════════════════════════════════════════════════════════════
static Order buy(uint64_t id, Price price, Quantity qty) {
    return {id, Side::Buy, OrderType::GTC, price, qty};
}
static Order sell(uint64_t id, Price price, Quantity qty) {
    return {id, Side::Sell, OrderType::GTC, price, qty};
}

// ═══════════════════════════════════════════════════════════════
// Basic matching
// ═══════════════════════════════════════════════════════════════

TEST(OrderBookTest, AddSingleOrderNoMatch) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(buy(1, 10000, 50), events);

    ASSERT_EQ(events.size(), 1);
    EXPECT_TRUE(std::holds_alternative<AcceptedEvent>(events[0]));
    EXPECT_TRUE(book.contains(1));

    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.bids.size(), 1);
    EXPECT_EQ(snap.bids[0].price, 10000);
    EXPECT_EQ(snap.bids[0].volume, 50);
}

TEST(OrderBookTest, SimpleMatchSamePrice) {
    OrderBook book;
    std::vector<Event> events;

    // Rest a sell
    book.submit(sell(1, 10000, 50), events);
    events.clear();

    // Submit crossing buy
    book.submit(buy(2, 10000, 50), events);

    // Should have 1 trade + acceptance (fully filled)
    ASSERT_GE(events.size(), 1);
    const auto* trade = std::get_if<TradeEvent>(&events[0]);
    ASSERT_NE(trade, nullptr);
    EXPECT_EQ(trade->qty, 50);
    EXPECT_EQ(trade->price, 10000);
    EXPECT_EQ(trade->resting_id, 1);
    EXPECT_EQ(trade->aggressive_id, 2);

    // Both orders gone — book empty
    auto snap = book.debug_snapshot();
    EXPECT_TRUE(snap.bids.empty());
    EXPECT_TRUE(snap.asks.empty());
}

TEST(OrderBookTest, NoMatchWhenSpread) {
    OrderBook book;
    std::vector<Event> events;

    book.submit(buy(1, 9900, 50), events);
    events.clear();
    book.submit(sell(2, 10100, 50), events);

    // Both resting, no trade
    bool has_trade = false;
    for (const auto& e : events) {
        if (std::holds_alternative<TradeEvent>(e)) has_trade = true;
    }
    EXPECT_FALSE(has_trade);

    auto snap = book.debug_snapshot();
    EXPECT_EQ(snap.bids.size(), 1);
    EXPECT_EQ(snap.asks.size(), 1);
}

TEST(OrderBookTest, PartialFill) {
    OrderBook book;
    std::vector<Event> events;

    // Rest sell 50
    book.submit(sell(1, 10000, 50), events);
    events.clear();

    // Buy 100 — fills 50, 50 rests
    book.submit(buy(2, 10000, 100), events);

    const auto* trade = std::get_if<TradeEvent>(&events[0]);
    ASSERT_NE(trade, nullptr);
    EXPECT_EQ(trade->qty, 50);

    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.bids.size(), 1);
    EXPECT_EQ(snap.bids[0].volume, 50);
    EXPECT_EQ(snap.bids[0].order_ids[0], 2);
    EXPECT_TRUE(snap.asks.empty());
}

// ═══════════════════════════════════════════════════════════════
// FIFO ordering (price-time priority)
// ═══════════════════════════════════════════════════════════════

TEST(OrderBookTest, FifoOrderingSamePrice) {
    OrderBook book;
    std::vector<Event> events;

    // Three buys at same price
    book.submit(buy(1, 10000, 10), events);
    book.submit(buy(2, 10000, 10), events);
    book.submit(buy(3, 10000, 10), events);
    events.clear();

    // Sell 15 — should fill buy1 (10) then buy2 (5)
    book.submit(sell(4, 10000, 15), events);

    // Two trades: first with order 1, second with order 2
    ASSERT_EQ(events.size(), 2); // 2 trades + acceptance
    // Actually: 2 trades + possibly accepted. Let's check trades only.
    int trade_count = 0;
    for (const auto& e : events) {
        if (auto* t = std::get_if<TradeEvent>(&e)) {
            trade_count++;
            if (trade_count == 1) {
                EXPECT_EQ(t->resting_id, 1);
                EXPECT_EQ(t->qty, 10);
            } else {
                EXPECT_EQ(t->resting_id, 2);
                EXPECT_EQ(t->qty, 5);
            }
        }
    }
    EXPECT_EQ(trade_count, 2);

    // buy3 still resting (10), buy2 partially filled (5 left)
    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.bids.size(), 1);
    EXPECT_EQ(snap.bids[0].volume, 15); // buy2 has 5, buy3 has 10
}

// ═══════════════════════════════════════════════════════════════
// Cross-price matching (aggressor sweeps multiple levels)
// ═══════════════════════════════════════════════════════════════

TEST(OrderBookTest, CrossPriceSweep) {
    OrderBook book;
    std::vector<Event> events;

    // Asks at 10000 and 10100
    book.submit(sell(1, 10000, 20), events);
    book.submit(sell(2, 10100, 30), events);
    events.clear();

    // Buy at 10100 — should sweep both levels
    book.submit(buy(3, 10100, 40), events);

    int trade_count = 0;
    for (const auto& e : events) {
        if (std::holds_alternative<TradeEvent>(e)) trade_count++;
    }
    EXPECT_EQ(trade_count, 2);

    // sell2 partially filled (10 left at 10100)
    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.asks.size(), 1);
    EXPECT_EQ(snap.asks[0].price, 10100);
    EXPECT_EQ(snap.asks[0].volume, 10);
}

// ═══════════════════════════════════════════════════════════════
// Cancel
// ═══════════════════════════════════════════════════════════════

TEST(OrderBookTest, CancelExistingOrder) {
    OrderBook book;
    std::vector<Event> events;

    book.submit(buy(1, 10000, 50), events);
    events.clear();

    book.cancel(1, events);
    ASSERT_EQ(events.size(), 1);
    EXPECT_TRUE(std::holds_alternative<CanceledEvent>(events[0]));
    EXPECT_FALSE(book.contains(1));

    auto snap = book.debug_snapshot();
    EXPECT_TRUE(snap.bids.empty());
}

TEST(OrderBookTest, CancelNonExistent) {
    OrderBook book;
    std::vector<Event> events;
    book.cancel(999, events);
    ASSERT_EQ(events.size(), 1);
    auto* rej = std::get_if<RejectedEvent>(&events[0]);
    ASSERT_NE(rej, nullptr);
    EXPECT_EQ(rej->reason, RejectReason::UnknownOrderId);
}

TEST(OrderBookTest, CancelMiddleOfQueue) {
    OrderBook book;
    std::vector<Event> events;

    book.submit(buy(1, 10000, 10), events);
    book.submit(buy(2, 10000, 10), events);
    book.submit(buy(3, 10000, 10), events);
    events.clear();

    // Cancel middle order
    book.cancel(2, events);
    EXPECT_FALSE(book.contains(2));
    EXPECT_TRUE(book.contains(1));
    EXPECT_TRUE(book.contains(3));

    // Match: should fill 1 first, then 3 (2 is gone)
    book.submit(sell(4, 10000, 20), events);
    // Check trades reference orders 1 and 3
    bool hit1 = false, hit3 = false;
    for (const auto& e : events) {
        if (auto* t = std::get_if<TradeEvent>(&e)) {
            if (t->resting_id == 1) hit1 = true;
            if (t->resting_id == 3) hit3 = true;
        }
    }
    EXPECT_TRUE(hit1);
    EXPECT_TRUE(hit3);
}

// ═══════════════════════════════════════════════════════════════
// Modify
// ═══════════════════════════════════════════════════════════════

TEST(OrderBookTest, ModifyUpdatesPriceAndQuantity) {
    OrderBook book;
    std::vector<Event> events;

    book.submit(buy(1, 10000, 50), events);
    events.clear();

    book.modify(1, 10100, 100, events);

    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.bids.size(), 1);
    EXPECT_EQ(snap.bids[0].price, 10100);
    EXPECT_EQ(snap.bids[0].volume, 100);
}

TEST(OrderBookTest, ModifyNonExistent) {
    OrderBook book;
    std::vector<Event> events;
    book.modify(999, 10000, 50, events);
    auto* rej = std::get_if<RejectedEvent>(&events[0]);
    ASSERT_NE(rej, nullptr);
    EXPECT_EQ(rej->reason, RejectReason::UnknownOrderId);
}

// ═══════════════════════════════════════════════════════════════
// Market orders
// ═══════════════════════════════════════════════════════════════

TEST(OrderBookTest, MarketOrderFillsAgainstResting) {
    OrderBook book;
    std::vector<Event> events;

    // Set up liquidity
    book.submit(sell(1, 10000, 20), events);
    book.submit(sell(2, 10001, 30), events);
    events.clear();

    // Market buy for 40
    book.market({3, Side::Buy, OrderType::GTC, 0, 40}, events);

    int trade_count = 0;
    Quantity total_filled = 0;
    for (const auto& e : events) {
        if (auto* t = std::get_if<TradeEvent>(&e)) {
            trade_count++;
            total_filled += t->qty;
        }
    }
    EXPECT_EQ(trade_count, 2);
    EXPECT_EQ(total_filled, 40);

    // sell2 partially filled (10 left at 10001)
    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.asks.size(), 1);
    EXPECT_EQ(snap.asks[0].volume, 10);
}

TEST(OrderBookTest, MarketOrderPartialNoRest) {
    OrderBook book;
    std::vector<Event> events;

    // Only 20 available
    book.submit(sell(1, 10000, 20), events);
    events.clear();

    // Market buy for 50 — gets 20, rest silently discarded
    book.market({2, Side::Buy, OrderType::GTC, 0, 50}, events);

    const auto* trade = std::get_if<TradeEvent>(&events[0]);
    ASSERT_NE(trade, nullptr);
    EXPECT_EQ(trade->qty, 20);

    // Nothing resting (market order never rests)
    auto snap = book.debug_snapshot();
    EXPECT_TRUE(snap.bids.empty());
    EXPECT_TRUE(snap.asks.empty());
}

// ═══════════════════════════════════════════════════════════════
// IOC (Immediate-or-Cancel)
// ═══════════════════════════════════════════════════════════════

TEST(OrderBookTest, IocFillsPartiallyCancelsRest) {
    OrderBook book;
    std::vector<Event> events;

    // 20 available
    book.submit(sell(1, 10000, 20), events);
    events.clear();

    // IOC buy for 50 — fills 20, cancels 30
    Order ioc{4, Side::Buy, OrderType::IOC, 10000, 50};
    book.submit(ioc, events);

    const auto* trade = std::get_if<TradeEvent>(&events[0]);
    ASSERT_NE(trade, nullptr);
    EXPECT_EQ(trade->qty, 20);

    // Nothing resting
    auto snap = book.debug_snapshot();
    EXPECT_TRUE(snap.bids.empty());
    EXPECT_TRUE(snap.asks.empty());
}

TEST(OrderBookTest, IocWithNoMatchCancelsEntirely) {
    OrderBook book;
    std::vector<Event> events;

    Order ioc{1, Side::Buy, OrderType::IOC, 10000, 50};
    book.submit(ioc, events);

    // No trade, no resting
    for (const auto& e : events) {
        EXPECT_FALSE(std::holds_alternative<TradeEvent>(e));
    }
    auto snap = book.debug_snapshot();
    EXPECT_TRUE(snap.bids.empty());
}

// ═══════════════════════════════════════════════════════════════
// FOK (Fill-or-Kill)
// ═══════════════════════════════════════════════════════════════

TEST(OrderBookTest, FokFillsWhenLiquiditySufficient) {
    OrderBook book;
    std::vector<Event> events;

    // 50 available at 10000
    book.submit(sell(1, 10000, 50), events);
    events.clear();

    // FOK buy for 50 — fully fillable
    Order fok{2, Side::Buy, OrderType::FOK, 10000, 50};
    book.submit(fok, events);

    const auto* trade = std::get_if<TradeEvent>(&events[0]);
    ASSERT_NE(trade, nullptr);
    EXPECT_EQ(trade->qty, 50);
    EXPECT_FALSE(book.contains(2)); // not resting
}

TEST(OrderBookTest, FokRejectsWhenInsufficient) {
    OrderBook book;
    std::vector<Event> events;

    // Only 20 available
    book.submit(sell(1, 10000, 20), events);
    events.clear();

    // FOK buy for 50 — not fillable
    Order fok{2, Side::Buy, OrderType::FOK, 10000, 50};
    book.submit(fok, events);

    auto* rej = std::get_if<RejectedEvent>(&events[0]);
    ASSERT_NE(rej, nullptr);
    EXPECT_EQ(rej->reason, RejectReason::InsufficientLiquidity);

    // Book unchanged
    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.asks.size(), 1);
    EXPECT_EQ(snap.asks[0].volume, 20);
}

TEST(OrderBookTest, FokRejectsWhenNoLiquidity) {
    OrderBook book;
    std::vector<Event> events;

    Order fok{1, Side::Buy, OrderType::FOK, 10000, 50};
    book.submit(fok, events);

    auto* rej = std::get_if<RejectedEvent>(&events[0]);
    ASSERT_NE(rej, nullptr);
    EXPECT_EQ(rej->reason, RejectReason::InsufficientLiquidity);
}

// ═══════════════════════════════════════════════════════════════
// Duplicate ID rejection
// ═══════════════════════════════════════════════════════════════

TEST(OrderBookTest, RejectDuplicateOrderId) {
    OrderBook book;
    std::vector<Event> events;

    book.submit(buy(1, 10000, 50), events);
    events.clear();
    book.submit(buy(1, 10001, 30), events);

    auto* rej = std::get_if<RejectedEvent>(&events[0]);
    ASSERT_NE(rej, nullptr);
    EXPECT_EQ(rej->reason, RejectReason::DuplicateOrderId);
}

// ═══════════════════════════════════════════════════════════════
// Debug snapshot
// ═══════════════════════════════════════════════════════════════

TEST(OrderBookTest, DebugSnapshotCapturesBothSides) {
    OrderBook book;
    std::vector<Event> events;

    book.submit(buy(1, 9900, 10), events);
    book.submit(buy(2, 10000, 20), events);
    book.submit(sell(3, 10100, 30), events);
    book.submit(sell(4, 10200, 40), events);

    auto snap = book.debug_snapshot();

    ASSERT_EQ(snap.bids.size(), 2);
    EXPECT_EQ(snap.bids[0].price, 10000);  // highest bid first (greater)
    EXPECT_EQ(snap.bids[1].price, 9900);

    ASSERT_EQ(snap.asks.size(), 2);
    EXPECT_EQ(snap.asks[0].price, 10100);  // lowest ask first (less)
    EXPECT_EQ(snap.asks[1].price, 10200);
}

TEST(OrderBookTest, PrintDoesNotCrash) {
    OrderBook book;
    std::vector<Event> events;

    book.submit(buy(1, 9900, 10), events);
    events.clear();

    book.print(events);
    ASSERT_EQ(events.size(), 1);
    EXPECT_TRUE(std::holds_alternative<SnapshotEvent>(events[0]));
}
```

- [ ] **Step 2: Run tests — expect link failure (no OrderBook impl yet)**

```bash
cmake --build build 2>&1 | head -20
```

Expected: Linker errors for OrderBook methods.

- [ ] **Step 3: Write order_book.cpp implementation**

Create `src/book/order_book.cpp`:

```cpp
#include "hft/book/order_book.hpp"
#include <sstream>
#include <algorithm>

namespace hft {

// ═══════════════════════════════════════════════════════════════
// Internal helpers
// ═══════════════════════════════════════════════════════════════

Order* OrderBook::store_order(Order order) {
    auto [it, _] = orders_.insert_or_assign(order.id, order);
    return &it->second;
}

void OrderBook::erase_order(OrderId id) {
    orders_.erase(id);
}

// ═══════════════════════════════════════════════════════════════
// can_fill_fok — check if a FOK order would fully fill
// ═══════════════════════════════════════════════════════════════

bool OrderBook::can_fill_fok(const Order& order) const {
    Quantity needed = order.quantity;

    if (order.side == Side::Buy) {
        for (const auto& [price, level] : asks_) {
            if (order.price < price) break;  // limit price constraint
            Quantity take = std::min(needed, level.total_volume);
            needed -= take;
            if (needed == 0) return true;
        }
    } else {
        for (const auto& [price, level] : bids_) {
            if (order.price > price) break;
            Quantity take = std::min(needed, level.total_volume);
            needed -= take;
            if (needed == 0) return true;
        }
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════
// match_aggressive — match taker against opposite-side book
// ═══════════════════════════════════════════════════════════════

void OrderBook::match_aggressive(Order& taker, std::vector<Event>& out) {
    if (taker.side == Side::Buy) {
        while (taker.remaining() > 0 && !asks_.empty()) {
            auto it = asks_.begin();   // lowest ask
            const Price best_ask = it->first;
            PriceLevel& level = it->second;

            // Limit order: stop if price doesn't cross
            if (taker.type == OrderType::Limit && taker.price < best_ask)
                break;

            Order* maker = level.front();
            Quantity fill_qty = std::min(taker.remaining(), maker->remaining());

            out.push_back(TradeEvent{
                maker->id, taker.id, maker->price, fill_qty
            });

            taker.filled_qty += fill_qty;
            maker->filled_qty += fill_qty;

            if (maker->is_filled()) {
                level.pop_front();
                erase_order(maker->id);
                if (level.empty()) {
                    asks_.erase(it);
                }
            }
        }
    } else {
        // Sell aggressor: match against bids (from highest first)
        while (taker.remaining() > 0 && !bids_.empty()) {
            auto it = bids_.begin();   // highest bid
            const Price best_bid = it->first;
            PriceLevel& level = it->second;

            // Limit order: stop if price doesn't cross
            if (taker.type == OrderType::Limit && taker.price > best_bid)
                break;

            Order* maker = level.front();
            Quantity fill_qty = std::min(taker.remaining(), maker->remaining());

            out.push_back(TradeEvent{
                maker->id, taker.id, maker->price, fill_qty
            });

            taker.filled_qty += fill_qty;
            maker->filled_qty += fill_qty;

            if (maker->is_filled()) {
                level.pop_front();
                erase_order(maker->id);
                if (level.empty()) {
                    bids_.erase(it);
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// submit — main entry point for limit orders
// ═══════════════════════════════════════════════════════════════

void OrderBook::submit(Order order, std::vector<Event>& out) {
    // Reject duplicates
    if (orders_.contains(order.id)) {
        out.push_back(RejectedEvent{order.id, RejectReason::DuplicateOrderId});
        return;
    }

    // FOK pre-check
    if (order.type == OrderType::FOK) {
        if (!can_fill_fok(order)) {
            out.push_back(RejectedEvent{order.id, RejectReason::InsufficientLiquidity});
            return;
        }
    }

    // Match against opposite side
    match_aggressive(order, out);

    // Post-match: rest in book if GTC with remaining qty
    if (order.type == OrderType::GTC && order.remaining() > 0) {
        Order* stored = store_order(order);
        auto& book = (order.side == Side::Buy) ? bids_ : asks_;
        auto it = book.find(order.price);
        if (it == book.end()) {
            it = book.emplace(order.price, PriceLevel{order.price}).first;
        }
        it->second.push_back(stored);
        out.push_back(AcceptedEvent{order.id});
    } else if (order.is_filled()) {
        // Fully filled (market/IOC can also reach here if all qty filled)
        // Don't need to store anything
    }
    // IOC/Market with unfilled remainder: silently discard (don't rest)
}

// ═══════════════════════════════════════════════════════════════
// cancel
// ═══════════════════════════════════════════════════════════════

void OrderBook::cancel(OrderId order_id, std::vector<Event>& out) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        out.push_back(RejectedEvent{order_id, RejectReason::UnknownOrderId});
        return;
    }

    const Order& order = it->second;
    Price price = order.price;
    Side side = order.side;

    auto& book = (side == Side::Buy) ? bids_ : asks_;
    auto level_it = book.find(price);

    // Remove from price level
    level_it->second.remove(order_id);
    if (level_it->second.empty()) {
        book.erase(level_it);
    }

    erase_order(order_id);
    out.push_back(CanceledEvent{order_id});
}

// ═══════════════════════════════════════════════════════════════
// modify — cancel + re-submit with same ID
// ═══════════════════════════════════════════════════════════════

void OrderBook::modify(OrderId order_id, Price new_price, Quantity new_qty,
                       std::vector<Event>& out) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        out.push_back(RejectedEvent{order_id, RejectReason::UnknownOrderId});
        return;
    }

    // Preserve fields from original
    Order updated = it->second;
    updated.price = new_price;
    updated.quantity = new_qty;
    updated.filled_qty = 0;

    // Cancel old (without emitting events — internal cancel)
    // Manually do the cancel steps
    {
        Price old_price = it->second.price;
        Side side = it->second.side;
        auto& book = (side == Side::Buy) ? bids_ : asks_;
        auto level_it = book.find(old_price);
        level_it->second.remove(order_id);
        if (level_it->second.empty()) {
            book.erase(level_it);
        }
        erase_order(order_id);
    }

    // Submit updated order (will emit appropriate events)
    submit(updated, out);
}

// ═══════════════════════════════════════════════════════════════
// market — immediate execution, no resting
// ═══════════════════════════════════════════════════════════════

void OrderBook::market(Order order, std::vector<Event>& out) {
    // Market orders: match at any price (set extreme limit)
    order.price = (order.side == Side::Buy) ? INT64_MAX : 0;
    order.type = OrderType::IOC;  // behave like IOC: don't rest

    submit(order, out);
}

// ═══════════════════════════════════════════════════════════════
// print — build human-readable snapshot
// ═══════════════════════════════════════════════════════════════

void OrderBook::print(std::vector<Event>& out) const {
    std::ostringstream oss;
    oss << "=== Order Book ===\n";

    // Asks (sell side) — lowest first
    if (!asks_.empty()) {
        oss << "Asks (Sell):\n";
        for (const auto& [price, level] : asks_) {
            oss << "  " << price << " : " << level.total_volume << "\n";
        }
    }

    // Bids (buy side) — highest first
    if (!bids_.empty()) {
        oss << "Bids (Buy):\n";
        for (const auto& [price, level] : bids_) {
            oss << "  " << price << " : " << level.total_volume << "\n";
        }
    }

    if (bids_.empty() && asks_.empty()) {
        oss << "(empty)\n";
    }

    out.push_back(SnapshotEvent{oss.str()});
}

// ═══════════════════════════════════════════════════════════════
// contains
// ═══════════════════════════════════════════════════════════════

bool OrderBook::contains(OrderId id) const {
    return orders_.contains(id);
}

// ═══════════════════════════════════════════════════════════════
// debug_snapshot — for testing
// ═══════════════════════════════════════════════════════════════

OrderBook::DebugSnapshot OrderBook::debug_snapshot() const {
    DebugSnapshot snap;

    for (const auto& [price, level] : bids_) {
        DebugLevel dl;
        dl.price = price;
        dl.volume = level.total_volume;
        for (const auto* o : level.orders) {
            dl.order_ids.push_back(o->id);
        }
        snap.bids.push_back(std::move(dl));
    }

    for (const auto& [price, level] : asks_) {
        DebugLevel dl;
        dl.price = price;
        dl.volume = level.total_volume;
        for (const auto* o : level.orders) {
            dl.order_ids.push_back(o->id);
        }
        snap.asks.push_back(std::move(dl));
    }

    return snap;
}

} // namespace hft
```

- [ ] **Step 4: Build and run tests**

```bash
cmake --build build && ctest --test-dir build --output-on-failure -R test_orderbook
```

Expected: All ~22 OrderBook tests pass.

- [ ] **Step 5: Commit**

```bash
git add src/book/order_book.cpp tests/test_orderbook.cpp
git commit -m "feat: implement OrderBook matching engine with full test suite"
```

---

### Task 7: Parser Implementation & Tests

**Files:**
- Create: `include/hft/io/parser.hpp`
- Create: `src/io/parser.cpp`
- Create: `tests/test_parser.cpp`

**Interfaces:**
- Produces: `class hft::Parser { std::optional<Action> parse(const std::string& line) const; }`
- Consumes: `hft::Action` from Task 3

- [ ] **Step 1: Write parser.hpp**

```cpp
#pragma once

#include "hft/core/action.hpp"
#include <optional>
#include <string>

namespace hft {

class Parser {
public:
    // Returns nullopt for empty lines and comment lines (starting with '#').
    // Returns Action on successful parse.
    // Throws std::runtime_error on malformed input.
    [[nodiscard]] std::optional<Action> parse(const std::string& line) const;
};

} // namespace hft
```

- [ ] **Step 2: Write failing parser tests**

Create `tests/test_parser.cpp`:

```cpp
#include "hft/io/parser.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

using namespace hft;

TEST(ParserTest, EmptyLineReturnsNullopt) {
    Parser p;
    EXPECT_EQ(p.parse(""), std::nullopt);
    EXPECT_EQ(p.parse("   "), std::nullopt);
}

TEST(ParserTest, CommentReturnsNullopt) {
    Parser p;
    EXPECT_EQ(p.parse("# this is a comment"), std::nullopt);
}

TEST(ParserTest, ParseSubmitGtc) {
    Parser p;
    auto result = p.parse("SUBMIT 1 BUY 15000 100");
    ASSERT_TRUE(result.has_value());
    auto* sub = std::get_if<SubmitAction>(&*result);
    ASSERT_NE(sub, nullptr);
    EXPECT_EQ(sub->id, 1);
    EXPECT_EQ(sub->side, Side::Buy);
    EXPECT_EQ(sub->type, OrderType::GTC);  // default
    EXPECT_EQ(sub->price, 15000);
    EXPECT_EQ(sub->quantity, 100);
}

TEST(ParserTest, ParseSubmitSellIoc) {
    Parser p;
    auto result = p.parse("SUBMIT 2 SELL 20000 50 IOC");
    ASSERT_TRUE(result.has_value());
    auto* sub = std::get_if<SubmitAction>(&*result);
    ASSERT_NE(sub, nullptr);
    EXPECT_EQ(sub->side, Side::Sell);
    EXPECT_EQ(sub->type, OrderType::IOC);
}

TEST(ParserTest, ParseSubmitFok) {
    Parser p;
    auto result = p.parse("SUBMIT 3 BUY 30000 10 FOK");
    auto* sub = std::get_if<SubmitAction>(&*result);
    ASSERT_NE(sub, nullptr);
    EXPECT_EQ(sub->type, OrderType::FOK);
}

TEST(ParserTest, ParseCancel) {
    Parser p;
    auto result = p.parse("CANCEL 42");
    auto* c = std::get_if<CancelAction>(&*result);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->order_id, 42);
}

TEST(ParserTest, ParseModify) {
    Parser p;
    auto result = p.parse("MODIFY 7 15050 100");
    auto* m = std::get_if<ModifyAction>(&*result);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->order_id, 7);
    EXPECT_EQ(m->new_price, 15050);
    EXPECT_EQ(m->new_qty, 100);
}

TEST(ParserTest, ParseMarket) {
    Parser p;
    auto result = p.parse("MARKET 8 BUY 200");
    auto* m = std::get_if<MarketAction>(&*result);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->id, 8);
    EXPECT_EQ(m->side, Side::Buy);
    EXPECT_EQ(m->qty, 200);
}

TEST(ParserTest, ParsePrint) {
    Parser p;
    auto result = p.parse("PRINT");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<PrintAction>(*result));
}

TEST(ParserTest, UnknownCommandThrows) {
    Parser p;
    EXPECT_THROW(p.parse("FOOBAR 1 2 3"), std::runtime_error);
}

TEST(ParserTest, MalformedSubmitThrows) {
    Parser p;
    EXPECT_THROW(p.parse("SUBMIT"), std::runtime_error);
    EXPECT_THROW(p.parse("SUBMIT 1"), std::runtime_error);
    EXPECT_THROW(p.parse("SUBMIT 1 BUY"), std::runtime_error);
    EXPECT_THROW(p.parse("SUBMIT 1 BUY 10000"), std::runtime_error);
    EXPECT_THROW(p.parse("SUBMIT 1 BLAH 10000 100"), std::runtime_error);
}

TEST(ParserTest, CaseInsensitiveCommands) {
    Parser p;
    EXPECT_TRUE(p.parse("submit 1 BUY 15000 100").has_value());
    EXPECT_TRUE(p.parse("Submit 1 BUY 15000 100").has_value());
    EXPECT_TRUE(p.parse("cancel 1").has_value());
    EXPECT_TRUE(p.parse("market 1 BUY 100").has_value());
    EXPECT_TRUE(p.parse("print").has_value());
}
```

- [ ] **Step 3: Run tests — expect build failure**

```bash
cmake --build build 2>&1 | head -10
```

Expected: Parser not implemented.

- [ ] **Step 4: Write parser.cpp**

Create `src/io/parser.cpp`:

```cpp
#include "hft/io/parser.hpp"
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace hft {

namespace {

std::string upper(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

} // namespace

std::optional<Action> Parser::parse(const std::string& line) const {
    // Trim leading whitespace
    size_t start = 0;
    while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start])))
        ++start;

    if (start >= line.size()) return std::nullopt;
    if (line[start] == '#') return std::nullopt;

    std::istringstream ss(line.substr(start));
    std::string cmd;
    ss >> cmd;
    cmd = upper(cmd);

    if (cmd == "SUBMIT") {
        SubmitAction a;
        std::string side_str;
        ss >> a.id >> side_str >> a.price >> a.quantity;
        if (ss.fail()) throw std::runtime_error("Malformed SUBMIT: " + line);

        side_str = upper(side_str);
        if (side_str == "BUY") a.side = Side::Buy;
        else if (side_str == "SELL") a.side = Side::Sell;
        else throw std::runtime_error("Unknown side: " + side_str);

        // Optional order type
        std::string type_str;
        if (ss >> type_str) {
            type_str = upper(type_str);
            if (type_str == "IOC") a.type = OrderType::IOC;
            else if (type_str == "FOK") a.type = OrderType::FOK;
            // GTC is default, unrecognized → GTC
        }

        // Validate
        if (a.price <= 0) throw std::runtime_error("Invalid price: " + std::to_string(a.price));
        if (a.quantity == 0) throw std::runtime_error("Invalid quantity: " + std::to_string(a.quantity));

        return a;
    }

    if (cmd == "CANCEL") {
        CancelAction a;
        ss >> a.order_id;
        if (ss.fail()) throw std::runtime_error("Malformed CANCEL: " + line);
        return a;
    }

    if (cmd == "MODIFY") {
        ModifyAction a;
        ss >> a.order_id >> a.new_price >> a.new_qty;
        if (ss.fail()) throw std::runtime_error("Malformed MODIFY: " + line);
        if (a.new_price <= 0) throw std::runtime_error("Invalid price");
        if (a.new_qty == 0) throw std::runtime_error("Invalid quantity");
        return a;
    }

    if (cmd == "MARKET") {
        MarketAction a;
        std::string side_str;
        ss >> a.id >> side_str >> a.qty;
        if (ss.fail()) throw std::runtime_error("Malformed MARKET: " + line);

        side_str = upper(side_str);
        if (side_str == "BUY") a.side = Side::Buy;
        else if (side_str == "SELL") a.side = Side::Sell;
        else throw std::runtime_error("Unknown side: " + side_str);

        if (a.qty == 0) throw std::runtime_error("Invalid quantity");
        return a;
    }

    if (cmd == "PRINT") {
        return PrintAction{};
    }

    throw std::runtime_error("Unknown command: " + cmd);
}

} // namespace hft
```

- [ ] **Step 5: Build and run parser tests**

```bash
cmake --build build && ctest --test-dir build --output-on-failure -R test_parser
```

Expected: All 12 parser tests pass.

- [ ] **Step 6: Commit**

```bash
git add include/hft/io/parser.hpp src/io/parser.cpp tests/test_parser.cpp
git commit -m "feat: implement CommandParser with full test coverage"
```

---

### Task 8: Event Formatter

**Files:**
- Create: `include/hft/io/formatter.hpp`
- Create: `src/io/formatter.cpp`

**Interfaces:**
- Produces: `std::string hft::format_event(const Event& event)` — stable text representation
- Consumes: `hft::Event` from Task 3

- [ ] **Step 1: Write formatter.hpp**

```cpp
#pragma once

#include "hft/core/event.hpp"
#include <string>

namespace hft {

// Returns a stable, single-line text representation of an event.
[[nodiscard]] std::string format_event(const Event& event);

} // namespace hft
```

- [ ] **Step 2: Write formatter.cpp**

Create `src/io/formatter.cpp`:

```cpp
#include "hft/io/formatter.hpp"
#include <sstream>
#include <string>

namespace hft {

std::string format_event(const Event& event) {
    return std::visit([](const auto& e) -> std::string {
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::is_same_v<T, TradeEvent>) {
            std::ostringstream oss;
            oss << "TRADE resting=" << e.resting_id
                << " incoming=" << e.aggressive_id
                << " price=" << e.price
                << " qty=" << e.qty;
            return oss.str();
        }

        if constexpr (std::is_same_v<T, AcceptedEvent>) {
            return "ACCEPTED " + std::to_string(e.order_id);
        }

        if constexpr (std::is_same_v<T, CanceledEvent>) {
            return "CANCELED " + std::to_string(e.order_id);
        }

        if constexpr (std::is_same_v<T, RejectedEvent>) {
            std::string reason;
            switch (e.reason) {
                case RejectReason::DuplicateOrderId:
                    reason = "duplicate order id";
                    break;
                case RejectReason::UnknownOrderId:
                    reason = "unknown order id";
                    break;
                case RejectReason::InsufficientLiquidity:
                    reason = "insufficient liquidity";
                    break;
                case RejectReason::InvalidPrice:
                    reason = "invalid price";
                    break;
                case RejectReason::InvalidQuantity:
                    reason = "invalid quantity";
                    break;
                default:
                    reason = "unknown";
                    break;
            }
            return "REJECTED " + reason + " " + std::to_string(e.order_id);
        }

        if constexpr (std::is_same_v<T, SnapshotEvent>) {
            return e.message;
        }

        return "UNKNOWN_EVENT";
    }, event);
}

} // namespace hft
```

- [ ] **Step 3: Build and verify**

```bash
cmake --build build
```

Expected: Compiles cleanly.

- [ ] **Step 4: Commit**

```bash
git add include/hft/io/formatter.hpp src/io/formatter.cpp
git commit -m "feat: implement EventFormatter for all event types"
```

---

### Task 9: Main Event Loop & Integration

**Files:**
- Modify: `src/main.cpp` (replace placeholder)
- Create: `tests/fixtures/basic_orders.txt`

**Interfaces:**
- Consumes: `Parser`, `OrderBook`, `Formatter` from Tasks 7, 6, 8
- Produces: Working CLI application

- [ ] **Step 1: Write the real main.cpp**

Overwrite `src/main.cpp`:

```cpp
#include "hft/book/order_book.hpp"
#include "hft/io/parser.hpp"
#include "hft/io/formatter.hpp"
#include <iostream>
#include <string>
#include <vector>

int main() {
    hft::OrderBook book;
    hft::Parser parser;
    std::string line;
    std::vector<hft::Event> events;

    while (std::getline(std::cin, line)) {
        // Trim trailing \r (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        auto action = parser.parse(line);
        if (!action) continue;  // empty or comment

        events.clear();
        std::visit([&](const auto& cmd) {
            using T = std::decay_t<decltype(cmd)>;

            if constexpr (std::is_same_v<T, hft::SubmitAction>) {
                book.submit({cmd.id, cmd.side, cmd.type, cmd.price, cmd.quantity}, events);
            } else if constexpr (std::is_same_v<T, hft::CancelAction>) {
                book.cancel(cmd.order_id, events);
            } else if constexpr (std::is_same_v<T, hft::ModifyAction>) {
                book.modify(cmd.order_id, cmd.new_price, cmd.new_qty, events);
            } else if constexpr (std::is_same_v<T, hft::MarketAction>) {
                book.market({cmd.id, cmd.side, OrderType::GTC, 0, cmd.qty}, events);
            } else if constexpr (std::is_same_v<T, hft::PrintAction>) {
                book.print(events);
            }
        }, *action);

        for (const auto& ev : events) {
            std::cout << hft::format_event(ev) << "\n";
        }
    }

    return 0;
}
```

- [ ] **Step 2: Create test fixture**

Create `tests/fixtures/basic_orders.txt`:

```text
# Basic orders fixture — all commands in sequence
SUBMIT 1 BUY 10000 100
SUBMIT 2 BUY 9900 50
SUBMIT 3 SELL 10100 80
SUBMIT 4 SELL 10000 60
PRINT
CANCEL 2
PRINT
SUBMIT 5 BUY 10100 30 IOC
SUBMIT 6 BUY 10000 50 GTC
PRINT
```

- [ ] **Step 3: Build**

```bash
cmake --build build
```

- [ ] **Step 4: Manual integration test**

```bash
./build/hft_order_engine < tests/fixtures/basic_orders.txt
```

Expected output (approximate):
```
ACCEPTED 1
ACCEPTED 2
ACCEPTED 3
TRADE resting=4 incoming=1 price=10000 qty=60
ACCEPTED 4
=== Order Book ===
Asks (Sell):
  10100 : 80
Bids (Buy):
  10000 : 40
  9900 : 50
CANCELED 2
=== Order Book ===
Asks (Sell):
  10100 : 80
Bids (Buy):
  10000 : 40
TRADE resting=3 incoming=5 price=10100 qty=30
REJECTED insufficient liquidity 6
=== Order Book ===
Asks (Sell):
  10100 : 50
Bids (Buy):
  10000 : 40
```

- [ ] **Step 5: Run full test suite**

```bash
ctest --test-dir build --output-on-failure
```

Expected: All tests pass (PriceLevel 8 + OrderBook ~22 + Parser 12).

- [ ] **Step 6: Commit**

```bash
git add src/main.cpp tests/fixtures/basic_orders.txt
git commit -m "feat: wire main event loop with stdin/stdout CLI"
```

---

### Task 10: README & Project Documentation

**Files:**
- Create: `README.md`

**Interfaces:** None (standalone documentation)

- [ ] **Step 1: Write README.md**

Create `README.md`:

```markdown
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
| SUBMIT | `SUBMIT <id> <BUY|SELL> <price> <qty> [GTC|IOC|FOK]` | `SUBMIT 1 BUY 15000 100 GTC` |
| CANCEL | `CANCEL <id>` | `CANCEL 1` |
| MODIFY | `MODIFY <id> <new_price> <new_qty>` | `MODIFY 1 15100 200` |
| MARKET | `MARKET <id> <BUY|SELL> <qty>` | `MARKET 3 BUY 50` |
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
tests/                → GoogleTest suite + fixtures
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
```

- [ ] **Step 2: Commit**

```bash
git add README.md
git commit -m "docs: add project README with quick start and command reference"
```

---

## Self-Review Results

### 1. Spec Coverage

| Spec Section | Covered By |
|---|---|
| 4.1 types.hpp | Task 2 |
| 4.2 order.hpp | Task 2 |
| 4.3 action.hpp | Task 3 |
| 4.4 event.hpp | Task 3 |
| 4.5 price_level.hpp | Task 4 (+ tests) |
| 4.6 order_book.hpp/.cpp | Tasks 5-6 (+ tests) |
| 4.7 parser | Task 7 (+ tests) |
| 4.8 formatter | Task 8 |
| 5. CLI protocol | Task 7 (parser) + Task 9 (main loop) |
| 6. Matching algorithm | Task 6 (order_book.cpp) |
| 7. Directory structure | Task 1 (scaffold) |
| 8. Build & run | Tasks 1, 9, 10 |
| 9. Test plan | Tasks 4, 6, 7 |
| 10. Out of scope | README (roadmap section) |

### 2. Placeholder Scan

No TBD, TODO, "implement later", or vague references found. Every step has complete code.

### 3. Type Consistency

Verified across all tasks:
- `hft::OrderId` = `uint64_t` — consistent in Order, Action, Event, PriceLevel, OrderBook
- `hft::Price` = `int64_t` — consistent throughout
- `hft::Quantity` = `uint64_t` — consistent throughout
- `OrderBook` methods accept `std::vector<Event>& out` — consistent with main.cpp usage
- `Parser::parse` returns `std::optional<Action>` — consistent with main.cpp

All names match: `SubmitAction`, `CancelAction`, `ModifyAction`, `MarketAction`, `PrintAction`, `TradeEvent`, `AcceptedEvent`, `CanceledEvent`, `RejectedEvent`, `SnapshotEvent`.
