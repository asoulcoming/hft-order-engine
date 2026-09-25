# 开发过程记录(JOURNAL)· 索引

> 本文件是过程记录的**索引与状态锚点**:顶部"当前状态块" + 条目模板 + 条目索引。
> 每条记录单独一个文件,存放在 `docs/journal/YYYY-MM-DD-主题.md`,**只增不改**(写错了在新条目里更正并注明,不删旧条目)。
> 与 ROADMAP.md 的分工:ROADMAP 回答"要去哪、怎么算到了",过程记录回答"已经发生了什么、为什么"。
> 记录风格融合两个参考:order-matching-engine 的 `FeedbackAndImprovements.md`(决策日志:问题→方案→量化闭环)与 Simple-HFT-Engine 的 `docs/dev_blog.md`(犯错史:瓶颈→依据→修复→收益)。

## 当前状态块(每次工作会话结束时更新)

- **最后更新**:2026-09-25
- **当前阶段**:Stage 0 — 工程基座与记录体系(进行中)
- **已完成(本阶段)**:ROADMAP.md 建立、过程记录建立(含 Iteration 1 回填)、README 修复、仓库整理(移除参考项目 submodule、过程文档拆分为 `docs/journal/` 单文件、删除 superpowers 残留),全部已推送
- **下一步**:Stage 0 剩余任务 —— ① GitHub Actions CI(build+ctest,ASan/UBSan job)② .clang-format/.clang-tidy 并全仓 format ③ 修 parser 静默 GFD bug + 补测试
- **状态权威说明**:本块 > ROADMAP.md §4 > README.md;新对话开工先读本块与索引中最新的 1–2 条记录

## 条目模板

新条目三步:① 复制下方模板到 `docs/journal/YYYY-MM-DD-主题.md` 并填写;② 在文末索引表追加一行;③ 更新顶部状态块。

```markdown
# YYYY-MM-DD | Stage N | 一句话主题

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

## 条目索引(新条目追加在表末)

| 日期 | 记录 | 主题 |
|---|---|---|
| 2026-07-22~27 | [iteration1 回填](docs/journal/2026-07-22-iteration1.md) | Iteration 1:Make it Work(std::map+std::deque 正确性基线,41 测试) |
| 2026-09-24 | [roadmap-replan](docs/journal/2026-09-24-roadmap-replan.md) | 路线重定:升级为 8 阶段交易系统 ROADMAP,建立过程记录 |
| 2026-09-25 | [references-submodule](docs/journal/2026-09-25-references-submodule.md) | 文档推送;参考项目 submodule(当天即撤销,见下一行) |
| 2026-09-25 | [docs-reorg](docs/journal/2026-09-25-docs-reorg.md) | 仓库整理:移除 submodule、拆分过程文档、删 superpowers 残留 |
| 2026-09-25 | [readme-chinese](docs/journal/2026-09-25-readme-chinese.md) | README 中文化,统一仓库文档语言 |
