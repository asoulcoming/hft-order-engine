# HFT 交易系统总体路线图(ROADMAP)

> 创建:2026-09-24 · 版本:v1
> 本文件是整个项目的**唯一权威路线图**,取代 README.md 原 5 迭代 Roadmap 与 `docs/design/2026-07-22-iteration1-design.md` 第 1 节的路线表(设计文档其余部分仍是 Iteration 1 的有效史料)。
>
> **终极目标:构建一个成熟的、可真正实操的 HFT(高频/低延迟)交易系统**——不是玩具,不是只能跑 demo 的课程作业,而是能在仿真环境长期无人值守稳定运行、最终可以用小额真实资金实盘、且每个技术决策都有数据支撑的系统。

---

## §0 使用协议(给未来的我,和未来的 AI 模型)

本项目会跨很多次对话、多个模型版本推进。为了让任何一次新对话都能无缝接上,约定如下:

1. **开工前必读**:本文件 + `JOURNAL.md`(顶部"当前状态块"、条目模板与索引)。需要细节时按索引打开 `docs/journal/` 里最新的 1–2 条记录。读这些就足够开工,不需要翻聊天历史。
2. **状态权威顺序**:`JOURNAL.md` 顶部状态块(最新)> 本文件 §4 总览表的状态列 > README.md。三者冲突时以更新时间新的为准,并顺手把旧的改对。
3. **推进纪律**:**只有当前阶段取得"完全的胜利"(过 §5 该阶段的 Gate + §6 验收方法论四关),才允许进入下一阶段**。不跳阶段,不做"顺手把下阶段的东西也写了"。推进中发现别的想法,记入 JOURNAL 的"下一步",不当场改道。
4. **修改规则**:
   - 阶段内容实质变化(增删任务、改 Gate、改方向)→ 改本文件对应小节 + 在 §8 变更记录追加一行;
   - 日常进展、决策、数据、踩坑 → 只写 `JOURNAL.md`,不动本文件;
   - 本文件保持自包含:不依赖对话上下文、不依赖"上次说过什么"。
5. **每阶段胜利后的固定动作**:
   - 更新 `JOURNAL.md` 状态块、本文件 §2/§4 的状态、README 的 Current 行;
   - 打 git tag:`stage-N-clear`(N 为阶段号);
   - 在 JOURNAL 写该阶段完整复盘(过 §6 四维清单 + 汇总数据)。

---

## §1 终态定义:什么叫"成熟、可实操的 HFT 交易系统"

### 1.1 系统组成

```
                 ┌─────────────────────────────────────────────┐
                 │        监控 / 日志 / 对账 / 故障恢复            │
                 └─────────────────────────────────────────────┘
                                   ▲
行情源 ──► ┌──────────────┐   ┌───┴───┐   ┌──────┐   ┌──────┐   ┌──────────┐
(实时)     │ 行情接入       │ ─►│ 策略   │ ─►│ 风控  │ ─►│ OMS  │ ─►│ 交易网关  │ ──► 交易所
           │ 解析/订单簿镜像 │   └───────┘   └──────┘   └──────┘   └──────────┘
                │               ▲ 回报(成交/撤/拒)                          │
                │(历史数据)      │                                          │
                ▼               │                                          │
           ┌──────────────┐     │        ┌──────────────┐                  │
           │ 回测运行器     │ ────┤        │ 撮合引擎       │ ◄── 模拟成交     │
           │(同策略接口)    │     └────────│(本仓库核心)    │                  │
           └──────────────┘              └──────────────┘                  │
```

| 模块 | 职责 | 首次建成 |
|---|---|---|
| 撮合引擎 | 模拟交易所核心;低延迟 C++ 技术训练场 | Stage 1–3 |
| 数据层 | 行情数据模型、录制、历史数据获取 | Stage 4 |
| 回测运行器 | 历史数据 → 策略 → 模拟成交 → 绩效报告 | Stage 4 |
| 策略层 | 信号与下单逻辑,回测/实时同一接口 | Stage 4(接口)、Stage 6(深化) |
| 行情接入 | WebSocket 解析、订单簿镜像、断线重连 | Stage 5 |
| 风控 | 下单前置硬限制,不可绕过 | Stage 5(v1)、持续 |
| OMS | 订单状态机、幂等回报处理 | Stage 5 |
| 交易网关 | 下单/撤单/查询,可插拔适配不同交易所 | Stage 5 |
| 监控运维 | 结构化日志、心跳、每日对账、崩溃恢复 | Stage 5(v1)、持续 |

### 1.2 一个必须先想清楚的认知:撮合引擎在真实系统中的角色

**实盘交易中,交易所替你撮合,你永远不需要自建撮合引擎。** 撮合引擎在本系统中有两个角色:

1. **回测/仿真的模拟交易所核心**(Stage 4 起的核心组件)——回测的可信度取决于它;
2. **低延迟 C++ 工程能力的训练场**——平坦数组、侵入式链表、内存池、零分配、无锁队列、缓存优化,这些技术会直接复用在实盘热路径(行情解析、订单簿镜像)上。

所以 Stage 1–3 的引擎优化不是绕远路:既保证回测正确性,又是实盘热路径的技术储备。

### 1.3 终极验收(整个项目"完成"的定义)

- [ ] 回测:同一份数据两次回测,输出 bit-identical;
- [ ] 仿真:testnet 连续 ≥2 周无人值守稳定运行,每日对账零差异;
- [ ] 实盘(可选):小额资金连续 ≥4 周稳定,对账零差异,回撤在预设范围;
- [ ] 每个阶段都有 JOURNAL 复盘条目和量化数据,任何技术选型都能回答"备选是什么、为什么没选、数据呢"。

---

## §2 现状快照(2026-09-25 Stage 0 完成后,每阶段结束时更新本节)

- **Iteration 1(Make it Work)已完成**:`std::map` + `std::deque` 正确性基线,价格-时间优先、GFD/IOC/FOK/市价单、撤单/改单、PRINT,43 个 GoogleTest 全过(price_level 8 + orderbook 21 + parser 14);
- **Stage 0(工程基座)已完成(2026-09-25,tag `stage-0-clear`)**:GitHub Actions CI 三任务(构建+测试 / ASan+UBSan / clang-format+clang-tidy)全绿;.clang-format/.clang-tidy 就位并全仓格式化;gtest 发现模式已改 PRE_TEST(规避并行构建串注册,详见 `docs/journal/2026-09-25-stage0-clear.md`);
- 目录:`include/hft/{core,book,io}` + `src/` + `tests/`,GoogleTest 经 FetchContent 引入(已配置 `FETCHCONTENT_UPDATES_DISCONNECTED`);
- **尚无**:多标的 Exchange 层、tick 定价、benchmark 套件(分别属 Stage 1/2)。

已知问题清单:

| # | 问题 | 位置 | 状态 |
|---|---|---|---|
| 1 | 协议表仍写已改名的 GTC | README.md 协议表 | 已修(2026-09-24) |
| 2 | parser 对未知订单类型 token 静默按 GFD 处理 | `src/io/parser.cpp` | 已修(2026-09-25,+2 测试) |
| 3 | 无 CI / lint / benchmark | 仓库级 | CI 与 lint 已建(2026-09-25);benchmark 属 Stage 2 |
| 4 | `InvalidPrice`/`InvalidQuantity` 拒绝原因已定义但未使用 | `include/hft/core/types.hpp` | Stage 1 随 tick 定价启用 |
| 5 | missing-field-initializers 编译警告(原有) | `src/book/order_book.cpp:130,136`、`tests/test_price_level.cpp` | Stage 1 顺手清 |

---

## §3 目标架构:两种运行模式,同一核心

**核心设计原则:回测与实时共用同一套策略代码与执行语义,只有"数据从哪来、单发到哪去"不同。**

```
              ┌───────────── 回测模式(Stage 4)─────────────┐
              │  历史数据文件 → 回放时钟 → 策略 → 撮合引擎模拟成交 │
              └───────────────────────────────────────────┘
共同部分:策略接口、风控、OMS、事件定义、撮合语义
              ┌───────────── 实时模式(Stage 5+)────────────┐
              │  WebSocket 行情 → 策略 → 风控 → OMS → 真实网关  │
              │  (testnet 仿真,通过 Gate 后可切实盘)           │
              └───────────────────────────────────────────┘
```

可插拔点(接口在 Stage 4/5 定义时就要抽象,实现在后):
- **MarketDataSource**:历史回放 / 实时 WebSocket / 录制回放;
- **ExecutionHandler**:撮合引擎模拟 / testnet 网关 / 实盘网关;
- **Clock**:回测虚拟时钟(交易所事件时间)/ 实时墙上时钟;
- **ExchangeGateway**(市场适配):币安 / 未来可选 CTP 等。

### 当前方向性决策(2026-09-24 定,可推翻;推翻时更新本节 + §8)

| 决策点 | 当前决策 | 理由 | 推翻成本 |
|---|---|---|---|
| 目标市场 | **先加密货币(币安)**,网关可插拔;CTP/SimNow 网关列为可选扩展 | testnet 免费、实盘只需 API key + 小额资金、mac 直接开发运行、行情数据免费好拿,是"真正实操"最快路径;CTP 保留对齐求职方向 | 前 4 个阶段与市场无关;Stage 5 起换市场 = 重写一个网关适配器 |
| 策略层语言 | **C++ 全栈**,回测框架预留 pybind11 绑定位置 | 延迟最优、无 IPC 边界、C++ 功底和面试故事最硬;研究层以后想要再补 Python | 低 |
| 实盘程度 | 仿真连续稳定后**小额实盘**(Stage 7,前置条件清单写死) | 风险可控的完整"实操"路径 | Stage 7 本身就是可选阶段 |

---

## §4 阶段总览表

量级是"业余时间投入下的健康节奏参考",**不是死线**——Gate 是唯一推进标准(Iteration 1 的实际节奏:主体 1 天、打磨 5 天、停滞 2 个月,证明日期没有意义)。

| Stage | 主题 | 一句话目标 | 状态 | 量级 |
|---|---|---|---|---|
| 0 | 工程基座与记录体系 | CI、lint、修已知 bug、记录体系就位 | ✅ 完成(2026-09-25,tag `stage-0-clear`) | 1–2 周 |
| 1 | 引擎:Make it Right | tick 定价、平坦数组、多标的、回放测试 | **下一个** | 2–3 周 |
| 2 | 引擎:Make it Fast | 侵入式链表、内存池、零分配、基准套件 | 未开始 | 3–4 周 |
| 3 | 引擎:Make it Reliable | 确定性回放、WAL+快照、崩溃恢复、火焰图 | 未开始 | 2–3 周 |
| 4 | 回测框架 | 数据→策略→模拟成交→报告,确定性回测 | 未开始 | 4–6 周 |
| 5 | 实时仿真全链路 | testnet 长跑:行情/策略/风控/OMS/网关/监控 | 未开始 | 4–8 周 |
| 6 | 策略与执行深化 | 认真策略、执行算法、端到端延迟打点 | 未开始 | 6–10 周 |
| 7 | 实盘小额(可选) | 前置条件全满足后,最小资金实盘 | 未开始 | 4+ 周 |

---

## §5 各阶段详情

> 每阶段的"参考"均指向三个参考仓库。它们**不进本仓库**(决策见 `docs/journal/2026-09-25-docs-reorg.md`),位于本地 HFT 工作目录下本仓库的**兄弟目录**;换电脑时按 README References 节的三条 clone 命令重新拉取:
> `../Simple-HFT-Engine`(saksham10arora-dotcom)、`../low-latency-matching-engine`(erictzhou)、`../order-matching-engine`(PIYUSH-KUMAR1809)。

### Stage 0 — 工程基座与记录体系

**为什么有这个阶段**:后面所有阶段的"胜利"都要靠客观证据(CI、测试、lint、记录)来判定;基座不牢,后面每个 Gate 都会掺水分。

**任务清单**:
- [x] 建立 ROADMAP.md(本文件)+ JOURNAL.md(过程记录,回填 Iteration 1 历史)
- [x] README:修 GTC 残留、Roadmap 节替换为指向本文件的简表
- [ ] GitHub Actions CI:job1 = ubuntu + cmake + ctest;job2 = clang 开 ASan+UBSan 跑测试(CI 里 FetchContent 首次要联网拉 GoogleTest,注意缓存)
- [ ] .clang-format(选 LLVM 或 Google 基准微调)+ .clang-tidy(先开 performance-*、bugprone-*、modernize-* 子集,别全开),全仓 `clang-format -i`(单独一个 commit,别和功能混)
- [ ] 修 parser bug:未知订单类型 token 静默按 GFD → 改为报错(抛 ParseError 或 Rejected),补测试覆盖

**交付物**:CI workflow、两个 lint 配置、parser 修复 + 测试。

**胜利标准(Gate)**:
- [ ] CI 两个 job 在 GitHub 上全绿(push 后亲眼确认)
- [ ] 本地 `cmake --build build && ctest --test-dir build` 全过,新增 parser 测试通过
- [ ] `clang-format --dry-run -Werror` 全仓无 diff;clang-tidy 无新增告警
- [ ] JOURNAL.md 有回填条目(已随本阶段完成)

**常见坑**:CI 首次构建慢(FetchContent 联网);format 全仓一次性大 diff 污染 blame(单独 commit);clang-tidy 全开告警风暴直接劝退(先子集)。

**完成后动作**:照 §0 第 5 条。

### Stage 1 — 引擎:Make it Right(原 Iteration 2)

**为什么**:回测要模拟真实交易所,价格必须落在合法 tick 上;真实系统天然多标的;golden replay + invariant 测试是后面一切"确定性"主张的地基。

**任务清单**:
- [ ] tick 定价:定义 TickSize 与价格合法域,非法价用已定义的 `InvalidPrice` 拒单;评估内部表示保留 int64 最小价位 vs 直接存 tick 索引(写决策记录进 JOURNAL)
- [ ] 平坦数组价格档位:`std::vector<PriceLevel>` 按 tick 直接索引。两条参考路线:
  - `../Simple-HFT-Engine/include/OrderBook.hpp`(flat array,配 Stage 2 的侵入式链表)
  - `../order-matching-engine/src/OrderBook.hpp` + `src/Bitset.hpp`(tombstone + headIndex + `__builtin_ctzll/clzll` 位图瞬移跳过空档)
  - 本阶段先做"直索引 + 位图找活跃档",稀疏大价差的内存问题在 JOURNAL 记录数据再决定是否加密
- [ ] 多标的 Exchange 层:symbol → OrderBook 路由 + 全局 order_to_book 索引(cancel 不扫 symbol)。参考 `../low-latency-matching-engine` 的 ARCHITECTURE.md exchange 层与 `order_to_book_` 设计
- [ ] Golden replay 测试:`tests/replay/` 每组 = 输入命令带 + `.expected` 期望输出带,byte-exact 比对(组织方式参考 `../low-latency-matching-engine/tests/replay/`,16 组夹具)
- [ ] Invariant 随机化测试:固定种子生成随机操作流,校验守恒不变量(买卖量守恒、book 与索引一致、事件流无矛盾);种子可重放
- [ ] 旧 map 版引擎**保留**为对照(挪 `legacy/` 或 CMake 双引擎选项),同 workload 出对比表(做法参考 `../Simple-HFT-Engine/research/` 的三架构同种子对比 + CSV)

**交付物**:新引擎结构、Exchange 层、replay/invariant 测试体系、新旧架构对照脚本。

**胜利标准(Gate)**:
- [ ] 测试 ≥60 个全过(现 41)
- [ ] 回放测试 byte-exact;invariant 在 ≥10 个种子下通过
- [ ] 新旧架构对照基准脚本能一键跑出对比表(不设性能目标,只要可复现)
- [ ] 四维清单过关(重点:失败态——非法价格/数量的拒单路径全覆盖)

**常见坑**:平坦数组遇到大价差/稀疏价位导致空间爆炸;位图扫描的"幽灵档位"(置位与清档不同步);modify 的 cancel-replace 语义在新结构下订单 ID 生命周期;换数据结构时 DebugSnapshot 要同步,否则测试静默失效。

### Stage 2 — 引擎:Make it Fast(原 Iteration 3)

**为什么**:这一步练的是"用数据结构换缓存"的硬功夫,产出物(侵入式链表、内存池、零分配)会原样搬进 Stage 5 的行情热路径。

**任务清单**:
- [ ] 侵入式双链表:Order 加 prev/next,PriceLevel 只存 head/tail + total_volume,cancel O(1) 解链;`static_assert(sizeof(Order) <= 64)` 保证单缓存行(参考 `../Simple-HFT-Engine/include/PriceLevel.hpp`、`include/Order.hpp`)
- [ ] 内存池:bump-pointer + free-list(参考 `../Simple-HFT-Engine/include/MemoryPool.hpp`)vs `std::pmr::monotonic_buffer_resource` + fallback(参考 `../order-matching-engine/src/OrderBook.cpp`)——两条路线都做微基准再选,决策记 JOURNAL
- [ ] 热路径零分配:写一个分配计数器(全局 new 钩子或 pmr 计数 resource),测试断言"N 单撮合零堆分配"
- [ ] Google Benchmark(FetchContent):三类基准——逐操作吞吐、混合流(submit/cancel/modify 按比例)、**单笔延迟**(≥1M 样本,nearest-rank p50/p95/p99/p999)。方法论照抄 `../low-latency-matching-engine/benchmarks/`:钉核(taskset)、预生成 workload 在计时环外、计时环内零 I/O、`DoNotOptimize/ClobberMemory`、明确"批量摊销延迟 ≠ 单笔尾延迟"
- [ ] BENCHMARKS.md 首版:环境元数据(编译器/CPU/构建选项)+ 方法论 + 数字表 + 每项优化的"瓶颈→依据→修复→收益"四要素(结构照抄 `../low-latency-matching-engine/BENCHMARKS.md`)

**交付物**:优化后的引擎、benchmark 套件、BENCHMARKS.md、零分配证明测试。

**胜利标准(Gate)**:
- [ ] ASan/UBSan 干净(CI 常开);TSan 对已引入的并发部分干净
- [ ] cancel O(1) 有测试证明(大 book 下撤单耗时与队列深度无关)
- [ ] 零分配断言通过
- [ ] 相对 Stage 1 基线,p999 单笔延迟与混合流吞吐有量化提升(不预设倍数,以实测为准并记入 BENCHMARKS.md)
- [ ] 基准口径写明:本地 mac 数字只做开发反馈;对外正式数字必须在 Linux 上、钉核、Release + `-march=native` 环境下出

**常见坑**:假基准(空循环被优化掉、订单生成在计时环内、计时环里有 I/O);mac 后台进程让 p999 抖动(多跑几轮取分布);pmr 大缓冲别学 order-matching 放 512MB(它放堆上,栈上放必炸);`alignas(128)` 在 M 系芯片与 x86 的差异。

### Stage 3 — 引擎:Make it Reliable(原 Iteration 4)

**为什么**:实盘系统的底线是"崩溃可恢复、行为可复现"。WAL+快照是**三个参考项目都没做的空白区**,自研这块最能形成差异化认知;确定性回放是回测可信的前提。

**任务清单**:
- [ ] 确定性回放:定义操作日志(journal)格式,重放器读日志逐条喂引擎;同日志 → 同事件流 byte-exact,且 Debug/Release、跨机器一致(时间戳、指针、迭代顺序等非确定性来源要清零)
- [ ] WAL + 快照崩溃恢复:写前日志 + 周期快照;恢复 = 载最近快照 + 重放 WAL 尾部;崩溃注入测试(kill -9 随机时刻 ≥100 次,恢复后状态与连续运行一致)
- [ ] perf/flamegraph(Linux;mac 用 Instruments Time Profiler 替代,只做诊断不做正式数字);至少完成一次"看火焰图→定位→优化"的闭环并记录
- [ ] 基准历史:每次正式基准结果入库(SQLite 或 md 表,含 commit hash、环境元数据;参考 `../low-latency-matching-engine/benchmarks/benchmark_history` 的 schema 思路)

**交付物**:journal 格式与重放器、WAL+快照+恢复、火焰图与优化闭环记录、基准历史库。

**胜利标准(Gate)**:
- [ ] kill -9 注入 ≥100 次全部恢复正确(订单簿状态、成交序列与不中断运行一致)
- [ ] 回放确定性:Debug/Release × 两台机器,输出 byte-exact
- [ ] BENCHMARKS.md 有 ≥2 次历史记录可对比
- [ ] 有火焰图 + 一次据此完成的优化闭环记录(四要素齐全)

**常见坑**:WAL 的 fsync 语义(每次 fsync 太慢、批量又丢尾部——权衡要有数据);快照原子性(写一半崩溃的快照必须能识别并丢弃);浮点/时间/地址混进日志破坏确定性。

### Stage 4 — 回测框架(交易系统线开始)

**为什么**:从本阶段起,项目从"撮合引擎"变成"交易系统"。回测是策略研究的地基,也是撮合引擎的第一次真实复用。

**任务清单**:
- [ ] 行情数据模型:tick 级事件流(snapshot + incremental L2 档位,或 trade+BBO 聚合,按目标市场数据可得性定,决策记 JOURNAL);时间戳统一用**交易所事件时间**
- [ ] 数据获取:币安官方历史数据下载(data.binance.vision)+ 自录脚本(参考 `../order-matching-engine/scripts/record_l3_data.py` 的做法)
- [ ] 策略接口(C++):`on_book_update / on_trade / on_order_update` + 下单上下文(submit/cancel);**回测与实时同一接口**——这是 §3 核心原则的落点,接口评审时逐条检查"有没有偷偷依赖回测特有信息"
- [ ] 模拟交易所:撮合引擎包装成 ExecutionHandler;滑点/手续费/排队位置模型第一版从简(固定费率 + 可配延迟),**模型假设必须写文档**
- [ ] 回测运行器:数据流 → 策略 → 模拟成交 → 回报 → 事件日志;回测虚拟时钟
- [ ] 绩效指标与报告:PNL 曲线、最大回撤、成交率、滑点分布;输出 md/CSV
- [ ] 示例策略:一个最简单但完整的策略(如固定网格),专门用来验证全链路
- [ ] 预留 pybind11 绑定位置(目录与 CMake 占位,不强求实现)

**交付物**:数据模型与录制/下载脚本、策略接口、回测运行器、报告器、示例策略回测报告。

**胜利标准(Gate)**:
- [ ] 同一数据两次回测输出 **bit-identical**
- [ ] 示例策略在 ≥1 个月真实历史数据上完整跑通并出报告
- [ ] 回测吞吐有数字(N 天数据耗时)
- [ ] 策略接口通过"同一份策略代码零改动接入实时模式"的设计评审(为 Stage 5 铺路,评审记录进 JOURNAL)

**常见坑**:前视偏差(look-ahead,策略用了当时不可能知道的数据——最常见的回测造假);撮合假设过于乐观(自己的挂单永远按最优价成交);时区与时间戳口径混乱;数据缺口(gap)未检测导致静默错误。

### Stage 5 — 实时仿真全链路(testnet)

**为什么**:第一次把所有模块接成活的系统。目标不是赚钱,是**稳定**——长时间无人值守不崩、不漏、不对不上账。

**任务清单**:
- [ ] 行情接入:WebSocket(库选型记录:Boost.Beast vs IXWebSocket vs 自写,决策进 JOURNAL);增量订单簿镜像:snapshot + diff + **序号连续性校验** + 心跳超时重连
- [ ] 运行时线程模型:**单线程事件循环起步**(策略+风控+OMS 同线程;行情线程只做解析,经 SPSC ring 投递)。刻意简单,分片多线程是可选扩展——理由:先把正确性做实,并发是乘法不是加法。SPSC 参考真无锁实现 `../Simple-HFT-Engine/include/SPSCRing.hpp`(acquire/release + 2^n 容量 + head/tail 分缓存行;**不要**参考 order-matching 的 RingBuffer——它 README 写 lock-free,实现是自旋锁)
- [ ] 下单网关抽象 + 币安 testnet 实现(REST 下单 + WebSocket 回报流)
- [ ] OMS:订单状态机(Submitted→Accepted→PartiallyFilled→Filled/Canceled/Rejected + 异常态);幂等(回报重复/乱序/未知单号)
- [ ] 风控 v1(硬限制,任何下单路径前置、不可绕过):最大仓位、单笔上限、每秒下单频率、日内亏损熔断、连续拒单熔断
- [ ] 监控 v1:结构化日志(带单调序列号)、心跳、每日对账脚本(本地 OMS vs 交易所回报)
- [ ] 故障处理:断线重连、行情 gap 处理、进程崩溃重启恢复(复用 Stage 3 的 WAL 思路重建仓位/挂单状态)

**交付物**:实时运行时全链路、testnet 部署与运行手册(哪怕只是台 mac)、对账脚本、风控模块。

**胜利标准(Gate)**:
- [ ] testnet 连续 ≥14 天无人值守(或累计 ≥20 个交易日)无崩溃、无未处理异常
- [ ] 内存:长跑 RSS 曲线平稳(无持续增长;启动记录基线,每日对比)
- [ ] 每日对账零差异(订单数/成交量/仓位)
- [ ] 风控注入测试:每条规则都有专门测试证明能拦住(构造越限场景)
- [ ] kill 重启演练 ≥5 次:恢复后本地状态与交易所一致
- [ ] 四维清单过关(重点:失败态——断网/断行情/回报超时/限频 429 各是什么行为,全部有明确处理与日志)

**常见坑**:增量流丢包不校验(序号不查 = 静默吃错数据,这是实时系统最阴的坑);WebSocket 重连风暴(退避策略);交易所限频(429/封 IP);本机时钟漂移影响对账;mac 合盖/休眠断连(挂机策略要设置)。

### Stage 6 — 策略与执行深化

**为什么**:系统稳了之后,研究的分量上来了。这一阶段弹性大,允许来回迭代。

**任务清单**:
- [ ] 认真策略 1–2 个:候选(做市、跨所/跨期价差、短期动量)用 Stage 4 回测研究;每个策略必须有研究记录:假设、证据、参数敏感性、**失效条件**
- [ ] 执行算法:父单拆子单、追价、撤补、报价偏移
- [ ] 端到端分段延迟打点:行情到达→解析完成→信号生成→下单发出,低开销打点,统计分位数(打点本身的开销要测出来并记录)
- [ ] 回测-仿真一致性:同一策略在回测 vs testnet 的行为差异分析(成交率、滑点、PNL 偏差及归因)

**交付物**:策略研究记录、执行算法、延迟打点数据、一致性分析报告。

**胜利标准(Gate)**:
- [ ] ≥1 个策略有完整研究记录(含失效条件与参数敏感性)
- [ ] 分段延迟 p50/p99/p999 数据表
- [ ] 一致性分析报告(偏差量化 + 归因到具体环节)

**常见坑**:过拟合(参数扫描挑最优 = 曲线拟合,留出集必须独立);回测成交假设与真实排队差异;策略对延迟敏感导致"回测赚、实盘亏"。

### Stage 7 — 实盘小额(可选阶段,前置条件不满足则不启动)

**为什么**:"真正实操"的最后一公里,但每一分风险都要用前置条件锁死。

**前置条件清单(全部满足才允许启动,写在配置与代码里,不靠自觉)**:
- [ ] Stage 5/6 Gate 全过
- [ ] kill switch 手工演练 ≥3 次(秒级全部撤单 + 停止新下单)
- [ ] 资金硬上限写入配置且代码强制(如 ≤100 USDT),超限拒单
- [ ] API key 权限最小化(只交易、不能出金)+ IP 白名单;密钥只走环境变量,**绝不进 git**
- [ ] 人工监控流程(每日检查清单)成文

**任务清单**:
- [ ] 仿真→实盘只改 endpoint/密钥/限额,其余代码零改动(验证 §3 可插拔设计)
- [ ] 每日对账升级为强制流程
- [ ] 异常自动停止:亏损超限 / 对账失败 / 心跳丢失 → 全撤 + 停机 + 告警

**胜利标准(Gate)**:
- [ ] 连续 ≥4 周实盘稳定运行
- [ ] 每日对账零差异
- [ ] 最大回撤在预设范围内
- [ ] 完整实盘复盘报告(所有异常事件清单与处理过程)

**常见坑**:testnet 与实盘行为差异(流动性、限频更严);手动干预冲动(纪律:只允许 kill switch,不手动救单);密钥泄漏;实盘特有的部分成交/自成交规则。

### 可选扩展(不排序,某阶段 Gate 通过后按需启动,启动时在本节标记所属阶段)

- CTP/SimNow 网关:国内期货,求职对口;需要 Linux 服务器跑 SDK;实现为又一个 ExchangeGateway 适配器,复用全部核心
- 多标的分片多线程:shard-per-core(order-matching 路线)+ Simple-HFT 的真无锁 SPSC
- Python 研究层:pybind11 暴露回测与数据
- 监控面板:Streamlit(参考 `../order-matching-engine/dashboard.py`)
- DPDK/AF_XDP 内核旁路:先 ROI 评估再立项

---

## §6 验收方法论(所有阶段通用)

### 6.1 "完全的胜利" = 四关全过

1. **功能关**:该阶段任务清单全部勾完,交付物齐;
2. **测试关**:全量测试通过,新增代码有对应测试;
3. **数据关**:有量化证据(测试数、基准数字、运行天数、对账结果),写进 JOURNAL/BENCHMARKS;
4. **四维清单关**(功能正确性之外固定过四维):

```text
□ 安全:资金与仓位上限?密钥与权限?资源归属校验?
□ 生命周期:内存会释放吗?缓存有过期清理吗?线程会泄漏吗?长跑 RSS 稳定吗?
□ 性能:查询条件有索引吗?量级推演过吗(峰值行情速率 × 持续时间)?
□ 失败态:依赖挂了用户看到什么?有超时吗?有重试上限吗?日志能定位吗?
```

### 6.2 工程纪律

- 提交顺序:**build → test(不接管道!)→ lint → push**;测试命令不接 `| grep`/`| tail`(吞退出码),要退出码用 PIPESTATUS 或分开跑;
- git 历史命令(revert/reset/amend)前先 `git branch --show-current` 确认分支;
- 每个功能 commit 前本地全量验证;基准/长跑脚本的结果文件不进 git(进 .gitignore),结论数字进 md。

### 6.3 基准纪律

- 环境:钉核(taskset)、Release + `-DNDEBUG`、固定 RNG 种子、预生成 workload 在计时环外、计时环内零 I/O、`DoNotOptimize` 防优化掉;
- 延迟:批量摊销延迟与单笔延迟**分列**,单笔延迟 ≥1M 样本出 nearest-rank 分位数;passive 与 aggressive 分开测;
- 口径:本地 mac = 开发反馈(标注清楚);正式数字 = Linux + 钉核 + 记录环境元数据(commit hash、编译器、lscpu);
- 诚实:测不出来的就说测不出来;被反压/插桩影响的指标要标注;基准太好看必有诈(参考 `../Simple-HFT-Engine/docs/dev_blog.md` 的 Lesson 1)。

### 6.4 JOURNAL 记录要求

- 每个技术决策记四要素:**瓶颈/问题 → 依据(数据或文档)→ 决策(含备选方案及为何没选)→ 收益(量化)**;
- 每次踩坑记:症状、根因、修复、如何防复发;
- 每阶段结束写复盘:Gate 逐项打勾证据 + 四维清单 + 数据汇总。

---

## §7 风险与未决问题

| # | 风险/问题 | 影响 | 对策 |
|---|---|---|---|
| 1 | 市场选择是默认决策(币安优先),用户未最终拍板 | Stage 5 起的方向 | 前 4 阶段市场无关;Stage 5 启动前确认一次 |
| 2 | 实盘资源(账户、资金、服务器)未落实 | Stage 7 | Stage 7 本身可选;启动前过前置清单 |
| 3 | mac 与 Linux 环境差异(CTP SDK、正式基准、taskset) | Stage 2/3/CTP 扩展 | 开发在 mac,正式数字与 CTP 在 Linux(云服务器或虚拟机) |
| 4 | 仓库名 hft-order-engine 与系统范围扩大不符 | 无功能影响 | 可不改;README 定位段说明系统全貌即可,改名属可选项 |
| 5 | 时间投入不稳定(历史上停滞 2 个月) | 节奏 | 量级只是参考,Gate 是唯一标准;停滞恢复时从 JOURNAL 状态块续 |
| 6 | 模型/对话更替导致上下文丢失 | 连续性 | §0 使用协议 + JOURNAL 状态块,新对话只读两个文件即可接上 |
| 7 | 单人项目无外部评审,决策盲区 | 质量 | JOURNAL 强制写"备选方案及为何没选";重要接口做书面设计评审(自评也算) |
| 8 | 参考项目不在本仓库(本地兄弟目录),换电脑需手动 clone | 文档引用路径失效 | README References 固化三条 clone 命令;ROADMAP 路径统一用 `../` 兄弟目录 |

## §8 变更记录

| 日期 | 版本 | 变更 |
|---|---|---|
| 2026-09-24 | v1 | 初版:将原 5 迭代撮合引擎路线升级为 8 阶段交易系统路线;建立 §0 跨对话使用协议;方向性决策(币安优先/C++全栈/小额实盘)按推荐默认设定,待用户确认 |
| 2026-09-25 | v1.1 | 仓库整理:移除参考项目 submodule(引用改回 `../` 本地兄弟目录,clone 命令见 README);过程记录拆分为 `docs/journal/` 单文件,`JOURNAL.md` 变为索引;删除 superpowers 脚手架文档 |
