# hft-order-engine

一个用 C++20 编写的低延迟限价单簿撮合引擎,从"正确性优先"起步、按迭代逐步做到缓存优化。本项目的定位是**通往量化交易系统开发的学习之旅**,最终目标是构建完整、可实操的 HFT 交易系统。

**当前状态:引擎 Iteration 1 与路线图 Stage 0(工程基座)已完成 —— 下一步 [交易系统总体路线图](ROADMAP.md) 的 Stage 1(引擎 Make it Right)**

Iteration 1 使用 `std::map` + `std::deque` 实现正确性优先的基线。项目现在瞄准完整的
生产级 HFT 交易系统(行情接入、策略、风控、OMS、交易网关、回测)——分阶段计划与各阶段
验收标准见 [ROADMAP.md](ROADMAP.md),开发过程记录见 [JOURNAL.md](JOURNAL.md)。

## 快速开始

### 环境要求

- C++20 编译器(GCC 10+、Clang 12+、Apple Clang 14+)
- CMake 3.20+

### 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### 交互模式运行

```bash
./build/hft_order_engine
```

输入命令:
```
SUBMIT 1 BUY 15000 100
SUBMIT 2 SELL 15100 50
PRINT
```

### 批量回放运行

```bash
./build/hft_order_engine < tests/fixtures/basic_orders.txt
```

### 运行测试

```bash
ctest --test-dir build --output-on-failure
```

## 命令协议

| 命令 | 格式 | 示例 |
|---|---|---|
| SUBMIT | `SUBMIT <id> <BUY\|SELL> <price> <qty> [GFD\|IOC\|FOK]` | `SUBMIT 1 BUY 15000 100 GFD` |
| CANCEL | `CANCEL <id>` | `CANCEL 1` |
| MODIFY | `MODIFY <id> <new_price> <new_qty>` | `MODIFY 1 15100 200` |
| MARKET | `MARKET <id> <BUY\|SELL> <qty>` | `MARKET 3 BUY 50` |
| PRINT  | `PRINT` | `PRINT` |

价格以整数"分"为单位(如 `15000` = ¥150.00 / $150.00),避免浮点误差。

## 架构

```
stdin → Parser → OrderBook → Formatter → stdout
```

- **Parser**:文本命令 → `std::variant<SubmitAction, CancelAction, ...>`
- **OrderBook**:买卖两侧 `std::map<Price, PriceLevel>`,价格-时间优先撮合
- **Formatter**:Event variant → 可读文本

## 目录结构

```
include/hft/core/     → types.hpp, order.hpp, action.hpp, event.hpp
include/hft/book/     → price_level.hpp, order_book.hpp
include/hft/io/       → parser.hpp, formatter.hpp
src/                  → 实现(.cpp)
tests/                → GoogleTest 测试套件(41 个)+ fixtures
docs/                 → 设计文档(design/)+ 过程记录(journal/)
```

## 路线图

项目按分阶段路线推进,完整细节与各阶段验收 Gate 见 [ROADMAP.md](ROADMAP.md),
过程记录索引见 [JOURNAL.md](JOURNAL.md)。

| Stage | 主题 | 状态 |
|---|---|---|
| 0 | 工程基座(CI、lint、过程记录体系) | ✅ 已完成(2026-09-25) |
| 1 | 引擎:Make it Right(平坦数组、多标的、回放测试) | 下一个 |
| 2 | 引擎:Make it Fast(侵入式链表、内存池、基准套件) | |
| 3 | 引擎:Make it Reliable(确定性回放、WAL、崩溃恢复) | |
| 4 | 回测框架(数据 → 策略 → 模拟成交 → 报告) | |
| 5 | 实时仿真全链路(testnet:行情/策略/风控/OMS/网关) | |
| 6 | 策略与执行深化(正经策略、执行算法、延迟打点) | |
| 7 | 实盘小额(可选,严格前置条件) | |

## 参考项目

三个用作学习参考的开源撮合引擎。它们**不进本仓库**——请保持为本地兄弟目录
(ROADMAP.md 中以 `../<name>` 引用):

```bash
git clone https://github.com/saksham10arora-dotcom/Simple-HFT-Engine.git ../Simple-HFT-Engine
git clone https://github.com/erictzhou/low-latency-matching-engine.git ../low-latency-matching-engine
git clone https://github.com/PIYUSH-KUMAR1809/order-matching-engine.git ../order-matching-engine
```

- [Simple-HFT-Engine](https://github.com/saksham10arora-dotcom/Simple-HFT-Engine)
- [low-latency-matching-engine](https://github.com/erictzhou/low-latency-matching-engine)
- [order-matching-engine](https://github.com/PIYUSH-KUMAR1809/order-matching-engine)

## License

MIT
