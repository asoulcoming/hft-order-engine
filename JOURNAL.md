# 开发过程记录(JOURNAL)

> 本文件是项目的**过程日志**:决策、数据、踩坑、复盘,**只增不改**(写错了在新条目里更正并注明,不删旧条目)。
> 与 ROADMAP.md 的分工:ROADMAP 回答"要去哪、怎么算到了",本文件回答"已经发生了什么、为什么"。
> 记录风格融合两个参考:order-matching-engine 的 `FeedbackAndImprovements.md`(决策日志:问题→方案→量化闭环)与 Simple-HFT-Engine 的 `docs/dev_blog.md`(犯错史:瓶颈→依据→修复→收益)。

## 当前状态块(每次工作会话结束时更新)

- **最后更新**:2026-09-24
- **当前阶段**:Stage 0 — 工程基座与记录体系(进行中)
- **已完成(本阶段)**:ROADMAP.md 建立、JOURNAL.md 建立(本文件,含 Iteration 1 回填)、README 修复(GTC 残留、指向 ROADMAP)
- **下一步**:Stage 0 剩余任务 —— ① GitHub Actions CI(build+ctest,ASan/UBSan job)② .clang-format/.clang-tidy 并全仓 format ③ 修 parser 静默 GFD bug + 补测试
- **状态权威说明**:本块 > ROADMAP.md §4 > README.md;新对话开工先读本块与最后 3 条记录

## 条目格式(新条目按此模板,追加在文末"历史条目"下)

```markdown
### YYYY-MM-DD | Stage N | 一句话主题

**做了什么**:
- ...

**决策与理由**(备选方案、为何没选):
- ...

**数据与证据**(测试数 / 基准数字 / 运行天数 / 对账结果):
- ...

**踩坑**(症状 → 根因 → 修复 → 如何防复发):
- ...

**下一步**:
- ...
```

---

## 历史条目(新条目追加在此行下方,按时间正序)

### 2026-07-22 ~ 2026-07-27 | Iteration 1:Make it Work(回填条目)

> 2026-09-24 补写。内容从 git log(13 条提交)、`docs/design/2026-07-22-iteration1-design.md` 与 `docs/superpowers/plans/2026-07-22-iteration1-plan.md` 还原,非当时实时记录。

**做了什么**:
- 2026-07-22 一天完成主体:设计文档(472 行)→ CMake + GoogleTest(FetchContent)脚手架 → 撮合引擎实现(`std::map` + `std::deque`,价格-时间优先、GFD/IOC/FOK/市价单、撤单/改单/PRINT)→ README;
- 07-23 工具链打磨:IntelliSense 兼容(variant 聚合初始化需显式 `Action{}` 包装)、`CMAKE_EXPORT_COMPILE_COMMANDS`、FetchContent 重配置 80s→0.4s(`FETCHCONTENT_UPDATES_DISCONNECTED`)、停止跟踪生成文件;
- 07-24 注释全量翻译中文;GTC 重命名为 GFD(CTP 标准:Good For Day);
- 07-25 删无用 include;main loop 捕获 parse 异常不再崩溃;
- 07-27 PRINT 增强为显示逐单 id/qty。

**决策与理由**:
- `std::map` + `std::deque` 刻意朴素:作为后续迭代的优化基线,每次演进能讲出"为什么、提升多少";
- 事件用 caller-owned `std::vector<Event>&` 复用缓冲,参考 low-latency-matching-engine,避免热路径构造 vector;
- `orders_` 用 `unordered_map` 管理订单生命周期提供稳定地址,PriceLevel 存指针;
- CLI 协议(SUBMIT/CANCEL/MODIFY/MARKET/PRINT)完全仿照 low-latency-matching-engine 的命令格式;
- 价格整数化(分),避免浮点比较陷阱(Simple-HFT-Engine 的 dev_blog Lesson 2 佐证)。

**数据与证据**:
- 41 个 GoogleTest 全过:price_level 8 + orderbook 21 + parser 12;
- 回放 fixture `tests/fixtures/basic_orders.txt` 跑通;
- FetchContent 重配置 80s → 0.4s(commit 1bd996b)。

**踩坑/遗留**:
- FetchContent 默认每次重配置联网检查 → 配置巨慢(已修);
- IntelliSense/clangd 对 variant 聚合初始化的兼容问题 → 用显式 `Action{}` 规避(根因未深究);
- **遗留未修**:parser 对未知订单类型 token 静默按 GFD 处理(只识别 IOC/FOK),应改为报错(已列入 Stage 0 任务);
- **遗留未修**:README 协议表 GTC 残留(2026-09-24 已随 ROADMAP 建立修复);
- `InvalidPrice`/`InvalidQuantity` 拒绝原因已定义未启用(Stage 1 随 tick 定价处理)。

**下一步**:
- (当时)进入 Iteration 2:平坦数组、tick 定价 —— 实际停滞 2 个月;
- (现在)按 ROADMAP.md Stage 0 收尾工程基座,再进 Stage 1。

### 2026-09-24 | Stage 0 | 重建路线:撮合引擎项目升级为交易系统路线,建立过程记录

**做了什么**:
- 全面盘点现状:Iteration 1 已完成(41 测试),原 5 迭代路线只走完第 1 个,7-27 后停滞;确认全仓无过程记录文件、无 CI、无 lint、无 benchmark;
- 调研三个参考项目的可借鉴点(Simple-HFT 的演进叙事与研究对比、low-latency 的基准方法论与测试组织、order-matching 的分片架构与反馈复盘文档);
- 制定 `ROADMAP.md`(v1):8 阶段路线(Stage 0–7 + 可选扩展),每阶段带可客观检验的胜利标准 Gate;建立 §0 跨对话使用协议(新会话只读 ROADMAP + 本文件即可接上);
- 建立本文件并回填 Iteration 1 历史;
- 修复 README:GTC→GFD 残留、Roadmap 节替换为指向 ROADMAP 的简表。

**决策与理由**:
- 总路线从"撮合引擎 5 迭代"升级为"交易系统 8 阶段":原 Iter 5(CTP 网关)不足以覆盖"可实操交易系统"(缺行情接入/策略/风控/OMS/回测/监控),重排为 Stage 4–7;
- 撮合引擎角色重新定位:回测模拟器核心 + 低延迟 C++ 训练场(实盘不需要自建撮合),写进 ROADMAP §1.2 防止方向迷失;
- 方向性决策三则按推荐默认设定(用户未作答,ROADMAP §3 可推翻):市场先币安(实操门槛最低,网关可插拔,CTP 留可选扩展)、策略层 C++ 全栈、实盘为 Stage 7 可选且前置条件锁死;
- 多标的 Exchange 层从原 Iter 5 **提前**到 Stage 1:回测与实时都需要,越晚改结构越痛;
- WAL+快照(Stage 3)识别为三个参考项目的共同空白区,作为自研差异化重点;
- 阶段推进铁律:Gate 全过才进下一阶段;量级只做节奏参考不设死线(Iteration 1 实际节奏证明日期无意义)。

**数据与证据**:
- ROADMAP.md v1(约 400 行,8 阶段 × 任务清单/Gate/常见坑);
- 本文件建立,含 1 条回填条目;
- git tag 计划:`stage-N-clear` 序列(本条目所属 commit 之后开始执行)。

**踩坑**:
- 无(纯文档变更)。

**下一步**:
- Stage 0 剩余:CI、clang-format/tidy、parser 静默 GFD bug(详见状态块)。
