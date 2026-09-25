# 2026-09-25 | Stage 0 | 仓库整理:移除参考项目 submodule、拆分过程文档、删除 superpowers 残留

**做了什么**:
- 移除三个参考项目的 git submodule(`references/` 与 `.gitmodules` 删除,清理 `.git/modules`);
- ROADMAP 所有参考路径从 `references/` 改回 `../` 兄弟目录;README References 节改为三条 clone 命令(参考项目不属于本仓库,需要时看本地 HFT 目录下的兄弟仓库);
- 过程文档拆分:JOURNAL.md 改为"当前状态块 + 条目模板 + 索引",每条记录单独存 `docs/journal/YYYY-MM-DD-主题.md`;
- 删除 `.superpowers/`(本地工作目录,gitignored)与 `docs/superpowers/`(Iteration 1 的 superpowers 实施计划,一次性脚手架,git 历史永久可找回)。

**决策与理由**:
- 移除 submodule(推翻当天早些时候的方案,用户拍板):参考项目只是学习资料,放进主仓库"感觉奇怪"、让仓库变乱;代价是换电脑需手动 clone 三个兄弟仓库——clone 命令固化在 README References,ROADMAP 路径统一用 `../`,可接受;
- 过程文档按条目拆文件(用户要求):单文件 JOURNAL 会随条目数无限膨胀,拆分后每次只读需要的条目,索引承担导航;状态块仍留在 JOURNAL.md 顶部,跨对话协议(ROADMAP §0)不变;
- 删除 superpowers 脚手架:Iteration 1 已完成,设计实质内容在 `docs/design/`,实施计划是给子代理的一次性工单,保留徒增文档噪音;git 历史(提交 ab61bc2 之前)可随时找回。

**数据与证据**:
- 仓库根目录文档:README.md / ROADMAP.md / JOURNAL.md(索引)三个;`docs/` = design 1 篇 + journal 4 篇;
- 41 个测试全过(纯文档与结构整理,无代码变更)。

**踩坑**:
- `git rm` 移除 submodule 后 `.gitmodules` 残留为空文件,需 `git rm -f .gitmodules` 再清一次;submodule 的对象存储在 `.git/modules/references/`,要手动 `rm -rf` 才算清干净。

**下一步**:
- Stage 0 剩余:CI、clang-format/tidy、parser 静默 GFD bug。
