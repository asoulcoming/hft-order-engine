# 2026-09-25 | Stage 0 | README 中文化

**做了什么**:
- README.md 全文翻译为中文,与全仓中文文档(ROADMAP/JOURNAL/docs/代码注释)统一;技术名词(命令、GFD/IOC/FOK、CMake、testnet 等)保留英文。

**决策与理由**:
- README 自 Iteration 1 起就是英文(当时照 GitHub 开源惯例与英文参考项目所写),但仓库其余文档与注释均为中文,语言不统一;本项目受众是自己 + 国内求职场景,中文优先;
- 若日后需要面向国际观众的英文版(如公开作品集),可另建 `README.en.md` 并互链,不影响现在。

**数据与证据**:
- 纯文档变更,41 测试全过不受影响。

**踩坑**:
- 无。

**下一步**:
- Stage 0 剩余:CI、clang-format/tidy、parser 静默 GFD bug。
