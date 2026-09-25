# 2026-09-25 | Stage 0 | Stage 0 完成:工程基座就位,CI 全绿,过 Gate

**做了什么**:
- 修 parser 静默 GFD bug(`SUBMIT ... GTC` 等未知订单类型现抛 ParseError,由 main loop 捕获)+ 2 个新测试,过时测试名 `ParseSubmitGtc` 更名为 `ParseSubmitDefaultsToGfd`(41 → 43);
- `.clang-format`(LLVM 基调 + 左绑定引用/指针 + 顶格访问修饰符,对齐仓库既有风格)+ `.clang-tidy`(bugprone/performance/modernize/readability 子集,排除项及理由写在文件里)+ 全仓一次性格式化(include 排序、单行 if 展开);
- GitHub Actions CI 三任务:构建+测试(gcc/Release)、ASan+UBSan(clang/Debug)、lint(clang-format 钉 23.1.1 + clang-tidy 走 compile_commands);
- 修 `gtest_discover_tests` 串注册问题(见踩坑 1,重要);
- 配好本地 VS Code 一键构建/运行/调试(`.vscode/`,gitignored)。

**决策与理由**:
- clang-format 版本对齐:本地 brew 23.1.2,PyPI 无该版本;实测 23.1.1 对全仓格式化输出与 23.1.2 逐字节一致 → CI 钉 23.1.1,消除本地/CI 格式漂移;
- gtest 发现模式 POST_BUILD → PRE_TEST:规避并行构建下的注册串写(踩坑 1);
- clang-tidy 只在 CI 跑:本地无 clang-tidy,装 brew llvm 太重;检查集从子集起步,避免告警风暴;
- 已知编译警告(missing-field-initializers)不在本次修:非本次引入,记入遗留清单。

**数据与证据(Gate 逐项)**:
- CI 全绿:最新 run(commit 2d26cdb)workflow 徽章 passing;lint 任务在首个 run 已单独绿(clang-tidy 零告警,一次通过);
- 本地:build 退出 0 / ctest **43/43** 通过(全新目录并行构建验证)/ `clang-format --dry-run -Werror` 退出 0;
- 测试数 41 → 43;两轮 CI 失败(sysctl 缺 -w、ASan 任务)均已定位并修复。

**踩坑**:
1. **gtest_discover_tests 串注册(本阶段最大收获)**:全新目录并行构建(CMake 4.4.0)后,三个测试目标的注册文件**全部写成 OrderBookTest 的列表**(二进制本身正确,`--gtest_list_tests` 各自输出正常)。错误条目 = 错误二进制 + 不匹配的 filter → 跑 0 个测试也报 PASS,**静默稀释测试覆盖**;ASan 任务的 Build 步骤也曾因此挂掉(POST_BUILD 发现在构建期运行 sanitizer 二进制)。疑似机制:三者 EXTRA_ARGS 为空 → 哈希同为 `e3b0c442`,并行 POST_BUILD 发现互踩。修复:改 PRE_TEST(ctest 串行发现),全新并行构建精确注册 43 个正确命名测试。防复发:见"全绿但测试数可疑"时先 `ctest -N` 对数;
2. Linux procps `sysctl` 设值必须带 `-w`,漏写被当成读不存在的键,整步失败;
3. 未认证 GitHub API 限流 60 次/小时,超出后用 workflow 徽章 `badge.svg`(公开、不限流)确认结论;
4. 遗留:`src/book/order_book.cpp:130,136`、`tests/test_price_level.cpp` 有 missing-field-initializers 编译警告(原有),Stage 1 顺手清。

**四维清单**:
- 安全:N/A(纯工程基座,无资金/权限面);
- 生命周期:CI 无常驻状态;
- 性能:N/A;
- 失败态:CI 三轮失败均可见、可定位、已修复;parser 非法输入有明确报错路径(main loop 捕获打 stderr)。

**下一步**:
- Stage 1 启动:tick 定价与价格校验、平坦数组价格档位、多标的 Exchange 层、golden replay + invariant 随机化测试(ROADMAP §5 Stage 1)。
