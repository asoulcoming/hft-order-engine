# hft-order-engine — Iteration 1 Design: Make it Work

> 设计日期：2026-07-22
> 目标：单标的撮合引擎，用最朴素的数据结构跑通完整撮合链路，建立正确性基线。

---

## 1. 项目定位

hft-order-engine 是一个从零开始、渐进优化的低延迟量化交易系统。面向国内传统股票/期货市场（CTP 接口），服务于求职量化开发岗位。

完整路线图（5 个迭代）：

| 迭代 | 主题 | 时间 | 核心目标 |
|---|---|---|---|
| 1 | Make it work | ~2 周 | 正确性：撮合逻辑跑通，CLI 交互 |
| 2 | Make it right | ~2 周 | 数据建模：tick 定价、平坦数组、回调输出 |
| 3 | Make it fast | ~2 周 | 性能：侵入式链表、内存池、零分配 |
| 4 | Make it reliable | ~1 周 | 工程化：benchmark、flamegraph、确定性回放 |
| 5 | Make it real | ~2 周 | 真实市场：CTP SimNow 网关接入 |

Iteration 1 是起点：**先保证逻辑正确，再谈性能优化。**

---

## 2. 参考项目

三个开源撮合引擎，各取所长：

| 项目 | 语言 | 核心数据结构 | 学习价值 |
|---|---|---|---|
| [Simple-HFT-Engine](https://github.com/saksham10arora-dotcom/Simple-HFT-Engine) | C++17 | 平坦数组 + 侵入式链表 | 完整的演进过程（v1→v3），有 learning mindset |
| [low-latency-matching-engine](https://github.com/erictzhou/low-latency-matching-engine) | C++20 | `std::map` + 侵入式链表 | 工程方法论最佳（CI/Docker/flamegraph/回放）|
| [order-matching-engine](https://github.com/PIYUSH-KUMAR1809/order-matching-engine) | C++20 | 平坦数组 + bitset + pmr | 极致优化思路（sharding/lock-free/bitset 扫描）|

Iteration 1 的朴素实现（`std::map` + `std::deque`）是**刻意保留的优化空间**——后续每个迭代的改进都能讲出"为什么、提升了多少"的故事。

---

## 3. 架构概览

### 3.1 系统边界

```
stdin（CLI命令/文件回放）
       │
       ▼
┌─────────────────────┐
│    CommandParser     │  文本 → Action（std::variant）
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│     OrderBook        │  撮合引擎（系统的"心脏"）
│  ┌───────────────┐  │
│  │  bids (map)   │  │  买方订单簿：std::map<Price, PriceLevel, greater>
│  │  asks (map)   │  │  卖方订单簿：std::map<Price, PriceLevel>
│  │  id→price map │  │  订单定位：std::unordered_map<OrderId, Price>
│  └───────────────┘  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   EventFormatter     │  Event（std::variant）→ 可读文本
└──────────┬──────────┘
           │
           ▼
         stdout（成交回显 / 状态变更）
```

Iteration 1 **不做**的东西：
- 无 Exchange（多标的路由层）— 单标的避免这一层
- 无持久化/数据库 — stdout 重定向就是"日志"
- 无网络层/CTP — Iteration 5 通过可插拔网关接入
- 无多线程 — 单线程确定性执行

### 3.2 数据流

```
用户输入: SUBMIT 1 BUY 15000 100
         │
    CommandParser.parse()
         │
    Action = SubmitAction{id=1, BUY, price=15000, qty=100}
         │
    OrderBook.submit(order, events)
         │
         ├─ match_buy():
         │    while best_ask <= 15000:
         │        fill = min(taker.qty, maker.qty)
         │        events.push(TradeEvent{...})
         │
         ├─ 未成交余量挂单:
         │    bids[15000].push_back(order)
         │    events.push(AcceptedEvent{1})
         │
    format_event() × N
         │
    输出: TRADE resting=2 incoming=1 price=15000 qty=50
         ACCEPTED 1
```

---

## 4. 核心数据结构

### 4.1 基础类型 (`include/hft/core/types.hpp`)

```cpp
namespace hft {
using OrderId  = uint64_t;
using Price    = int64_t;     // Iter 1 以分为单位，避免浮点
using Quantity = uint64_t;

enum class Side : uint8_t { Buy, Sell };
enum class OrderType : uint8_t { GTC, IOC, FOK };
enum class RejectReason : uint8_t {
    DuplicateOrderId,
    UnknownOrderId,
    InsufficientLiquidity,
    InvalidPrice,
    InvalidQuantity,
};
}
```

设计参考：low-latency-matching-engine 的 `core/order.hpp`（类型别名集中定义）

### 4.2 Order (`include/hft/core/order.hpp`)

```cpp
struct Order {
    OrderId   id;
    Side      side;
    OrderType type = OrderType::GTC;
    Price     price;           // 整数（分），CLI 层做转换
    Quantity  quantity;
    Quantity  filled_qty = 0;

    Quantity remaining() const { return quantity - filled_qty; }
    bool     is_filled()  const { return filled_qty == quantity; }
};
```

设计要点：
- 纯 POD，无指针、无虚函数 — 为后续塞入内存池做准备
- `filled_qty` 在 struct 内 — Iter 1 最直观的做法，不用改 quantity 本身
- 与 Simple-HFT-Engine v3 的区别：无 `prev/next` 侵入式指针（Iter 3 才加），无 `static_assert(<=64)` 约束
- 与 low-latency 的区别：不含 `TimeInForce` 的独立枚举（我们合入 `OrderType`）

### 4.3 Action (`include/hft/core/action.hpp`)

完全参考 low-latency-matching-engine 的 `core/action.hpp`：

```cpp
struct SubmitAction {
    OrderId    id;
    Side       side;
    OrderType  type = OrderType::GTC;
    Price      price;
    Quantity   quantity;
};

struct CancelAction  { OrderId order_id; };
struct ModifyAction  { OrderId order_id; Price new_price; Quantity new_qty; };
struct MarketAction  { OrderId id; Side side; Quantity qty; };
struct PrintAction   {};     // Iter 1 始终打印全部 book

using Action = std::variant<SubmitAction, CancelAction, ModifyAction,
                            MarketAction, PrintAction>;
```

为何用 `std::variant`：类型安全的 union，`std::visit` 遍历，清晰表达"命令是这几种之一"。

### 4.4 Event (`include/hft/core/event.hpp`)

```cpp
struct TradeEvent     { OrderId resting_id; OrderId aggressive_id;
                        Price price; Quantity qty; };
struct AcceptedEvent  { OrderId order_id; };
struct CanceledEvent  { OrderId order_id; };
struct RejectedEvent  { OrderId order_id; RejectReason reason; };
struct SnapshotEvent  { std::string message; };

using Event = std::variant<TradeEvent, AcceptedEvent, CanceledEvent,
                           RejectedEvent, SnapshotEvent>;
```

设计参考：low-latency-matching-engine 的 `core/event.hpp`

### 4.5 PriceLevel (`include/hft/book/price_level.hpp`)

```cpp
struct PriceLevel {
    Price              price;
    std::deque<Order*> orders;        // FIFO 队列（时间优先）
    Quantity           total_volume = 0;

    void push_back(Order* o);
    bool remove(OrderId order_id);    // Iter 1: O(n) 线性查找
    Order* front() const;
    bool empty() const;
    void pop_front();
};
```

设计要点：
- 存指针不存对象 — 订单对象生命周期由外部 `OrderManager` 管理
- `total_volume` — 参考 low-latency 的 `OrderQueue::total_volume`，避免每次查询遍历队列
- `remove()` 是 O(n) — **刻意保留的优化点**，Iter 3 换侵入式链表解决

### 4.6 OrderBook (`include/hft/book/order_book.hpp`)

```cpp
class OrderBook {
public:
    void submit(Order order, std::vector<Event>& out);
    void cancel(OrderId order_id, std::vector<Event>& out);
    void modify(OrderId order_id, Price new_price, Quantity new_qty,
                std::vector<Event>& out);
    void market(Order order, std::vector<Event>& out);
    void print(std::vector<Event>& out) const;

    // 供测试用 —— 参考 low-latency 的 DebugSnapshot
    struct DebugLevel   { Price price; Quantity volume; std::vector<OrderId> ids; };
    struct DebugSnapshot { std::vector<DebugLevel> bids, asks; };
    DebugSnapshot debug_snapshot() const;

    bool contains(OrderId id) const;

private:
    // bids: std::greater → begin() 即最高买价
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    // asks: 默认 less → begin() 即最低卖价
    std::map<Price, PriceLevel> asks_;

    // ID 查找：OrderId → 所在价位（用于 cancel 时找到哪个 PriceLevel）
    std::unordered_map<OrderId, Price> order_to_price_;

    void match_aggressive(Order& taker, std::vector<Event>& out);
    bool can_fill_fok(const Order& order) const;
};
```

关键设计决定：
- `std::map` 而非平坦数组 — Iter 2 才优化
- 每个方法接受 `std::vector<Event>& out` — **外部管理的 event buffer**，避免热路径上构造 vector
- `DebugSnapshot` — 来自 low-latency，对测试极其有用

### 4.7 CommandParser (`include/hft/io/parser.hpp`)

```cpp
class Parser {
public:
    // 返回 nullopt = 空行/注释行，抛异常 = 格式错误
    std::optional<Action> parse(const std::string& line) const;
};
```

### 4.8 EventFormatter (`include/hft/io/formatter.hpp`)

```cpp
// 返回稳定的文本表示，一行一个 event
std::string format_event(const Event& event);
```

---

## 5. CLI 命令协议

完全参考 low-latency-matching-engine 的命令格式：

| 命令 | 格式 | 示例 | 说明 |
|---|---|---|---|
| SUBMIT | `SUBMIT <id> <BUY\|SELL> <price> <qty> [GTC\|IOC\|FOK]` | `SUBMIT 1 BUY 15000 100 GTC` | 默认 GTC |
| MARKET | `MARKET <id> <BUY\|SELL> <qty>` | `MARKET 3 BUY 50` | 市价单，不挂单 |
| CANCEL | `CANCEL <id>` | `CANCEL 1` | 撤单 |
| MODIFY | `MODIFY <id> <new_price> <new_qty>` | `MODIFY 2 15050 100` | 改单 |
| PRINT | `PRINT` | `PRINT` | 打印完整订单簿 |

价格以"分"为单位的整数（避免浮点），如 `15000` 表示 150.00。

---

## 6. 撮合算法

### 6.1 submit（限价单）

```
submit(order):
    1. 若 order.type == FOK → can_fill_fok()，不满足则 return（不报不存）
    2. match_aggressive(order):
       while order.remaining() > 0 AND 对侧 book 非空:
           level = 对侧最佳价位
           if 限价单 且 价格不交叉 → break
           maker = level.front()
           fill_qty = min(taker.remaining(), maker.remaining())
           成交价 = maker.price（maker 的价格优先）
           生成 TradeEvent
           maker.filled_qty += fill_qty; 若满 → level.pop_front()
           taker.filled_qty += fill_qty
    3. 若 order.type != GTC（IOC/Market）→ 未成交余量丢弃
    4. 若 GTC 且有余量 → bids_/asks_[price].push_back(order)
```

### 6.2 cancel（撤单）

```
cancel(order_id):
    1. 查 order_to_price_ 获取 OrderId 所在价位
    2. 在该价位的 PriceLevel 中线性移除
    3. 若价位变空 → 从 bids_/asks_ 删除整个条目
```

### 6.3 modify（改单）

```
modify(order_id, new_price, new_qty):
    1. cancel(order_id)
    2. submit(新 Order{同 id, new_price, new_qty})
```

### 6.4 market（市价单）

```
market(order):
    1. order.price = (order.side == BUY) ? INT64_MAX : 0   // 无限吃
    2. match_aggressive(order)  // 同上逻辑，但不挂单
```

---

## 7. 目录结构

```
hft-order-engine/
├── CMakeLists.txt
├── README.md
├── docs/
│   └── design/
│       └── 2026-07-22-iteration1-design.md    ← 本文档
├── include/
│   └── hft/
│       ├── core/
│       │   ├── types.hpp          ← 类型别名 + 枚举
│       │   ├── order.hpp          ← Order struct
│       │   ├── action.hpp         ← Action variant
│       │   └── event.hpp          ← Event variant
│       ├── book/
│       │   ├── price_level.hpp    ← PriceLevel（std::deque 版）
│       │   └── order_book.hpp     ← OrderBook（std::map 版）
│       └── io/
│           ├── parser.hpp         ← 文本→Action
│           └── formatter.hpp      ← Event→文本
├── src/
│   ├── main.cpp                   ← 主循环：stdin → parse → process → format → stdout
│   ├── book/
│   │   └── order_book.cpp         ← 非模板逻辑
│   └── io/
│       ├── parser.cpp
│       └── formatter.cpp
└── tests/
    ├── CMakeLists.txt
    ├── test_orderbook.cpp          ← 撮合逻辑测试
    ├── test_parser.cpp             ← 解析器测试
    └── fixtures/
        └── basic_orders.txt        ← 批量回放用例
```

---

## 8. 构建与运行

### 8.1 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

依赖：C++20 编译器、CMake 3.20+。零外部依赖（GoogleTest 通过 CMake FetchContent 自动下载）。

### 8.2 运行

```bash
# 交互模式
./build/hft_order_engine

# 文件回放
./build/hft_order_engine < tests/fixtures/basic_orders.txt

# 保存输出供复盘
./build/hft_order_engine < tests/fixtures/basic_orders.txt > output.log
```

### 8.3 测试

```bash
ctest --test-dir build --output-on-failure
```

---

## 9. 测试计划

参考 Simple-HFT-Engine 的测试用例结构（21 个测试），以及 low-latency 的 DebugSnapshot 模式。

### 9.1 撮合逻辑测试

| 测试场景 | 验证点 |
|---|---|
| 挂单不交叉 | 2 单挂入，0 成交 |
| 简单撮合（同价） | buy 1000@50 × sell 1000@50 → 1 笔成交 50 股 |
| 部分成交 | buy 1000@30 吃 sell 1000@50 → buy 余 20 挂 bid 侧 |
| 买方跨价吃 | buy 10100 吃 ask@10000 先、ask@10100 后 |
| FIFO 时间优先 | 同价位 3 单 buy，sell 撮合时 buy1 先成交 |
| 市价单 | sweep 多个价位 |
| IOC 部分成交 | 可成交部分成交，余量丢弃 |
| FOK 足量 | 流动性够 → 全部成交 |
| FOK 不足 | 流动性不够 → 0 成交，book 不变 |
| 撤单 | 撤 middle order，FIFO 队列跳过被撤的单 |
| 撤单不存在 | 返回 false |
| 改单 | 价格+量一起改，等价于 cancel+submit |
| BBO 追踪 | 最高 bid/最低 ask 在各操作后正确更新 |

### 9.2 解析器测试

| 测试场景 |
|---|
| 合法 SUBMIT 各种组合 |
| 合法 CANCEL/MODIFY/MARKET/PRINT |
| 空行/注释行 → nullopt |
| 非法价格/数量 → 报错 |

---

## 10. 不在 Iteration 1 范围内的内容

| 内容 | 何时做 | 原因 |
|---|---|---|
| 平坦数组价格档位 | Iter 2 | 先保证逻辑对 |
| 侵入式链表 | Iter 3 | 需要内存池配合 |
| 内存池 | Iter 3 | 需要先理解 Order 生命周期 |
| Google Benchmark | Iter 3 | 需要先有性能基线 |
| 确定性回放 & flamegraph | Iter 4 | 需要 Linux 环境 |
| 多标的 (Exchange) | Iter 5 | 单标的够讲清楚核心逻辑 |
| CTP/XTP 网关 | Iter 5 | 需要 SimNow 账号 + 回调模型理解 |
| 多线程架构 | 第二期 | 单线程先稳定 |
| DPDK/XDP 内核旁路 | 待定 | 学习曲线极陡，ROI 待评估 |

---

## 11. 与其他项目的差异

本 Iteration 1 与三个参考项目有以下核心差异：

|  | Simple-HFT | low-latency | order-matching | **hft-order-engine I1** |
|---|---|---|---|---|
| 数据结构 | 平坦数组+侵入式 | map+侵入式 | 平坦数组+bitset | **map+deque（刻意朴素）** |
| Order 结构 | 带 prev/next | 带 prev/next | pmr::vector | **纯 POD 无指针** |
| 事件类型 | 直接 Trade | std::variant 6 种 | 散落各处 | **std::variant 5 种** |
| 输出方式 | 回调模板 | external buffer | callback+buffer | **external buffer** |
| 测试风格 | 手写 assert | GTest+DebugSnapshot | GTest | **GTest+DebugSnapshot** |
| 日志 | 无 | 无 | 无 | **无（stdout 重定向）** |

---

## 12. 设计评审记录

| 日期 | 评审内容 | 结论 |
|---|---|---|
| 2026-07-22 | 初版设计 | 批准，进入实现阶段 |
