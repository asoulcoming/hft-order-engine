# 2026-09-25 | Stage 0 | 文档推送;参考项目以 submodule 固化进仓库

> 注意:本条描述的 submodule 方案**当天即被用户撤销**(仓库保持干净,参考项目用本地兄弟目录),详见同日的《仓库整理》条目。保留本条作为决策演变的完整记录。

**做了什么**:
- 推送文档提交到 origin/main(cf0bccd..ab61bc2);
- 三个参考项目以 git submodule 加入 `references/` 目录(Simple-HFT-Engine、low-latency-matching-engine、order-matching-engine);
- ROADMAP.md 中所有 `../xxx` 兄弟目录路径引用改为 `references/xxx`,§5 头部说明同步更新;
- README.md References 节更新为 submodule 说明(`git clone --recursive` / `git submodule update --init --recursive`)。

**决策与理由**:
- submodule vs 复制源码 vs 仅留链接:submodule 把参考依赖变成仓库的一部分(换电脑一条 clone 命令即得)、不复制代码无许可问题、且**锁定参考时的 commit 版本**(参考项目以后更新不会悄悄改变 ROADMAP 里引用的行号与内容);
- submodule URL 用 HTTPS 而非 SSH:换新机器没有配 SSH key 也能拉取。

**数据与证据**:
- `.gitmodules` 三条记录;`git push` 成功(cf0bccd..ab61bc2)。

**踩坑**:
- 无。

**下一步**:
- (已被同日《仓库整理》条目取代)。
