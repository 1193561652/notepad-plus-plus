# 仓库根目录迁移决策

日期：2026-07-29

## 决策

- `notepad-plus-plus` Git 仓库根目录成为 Qt 移植唯一工作区。
- 不再使用旧的外部独立 Qt 主线目录。
- 原版 Notepad++ v8.4.6 通过本仓库 tag `v8.4.6` 查阅。
- QScintilla 2.13.3、Scintilla、Boost.Regex 适配层和 LexUser 作为
  `third_party/` 源码依赖随仓库管理。
- CMake 构建和配置语料测试不得依赖旧工作区兄弟目录。

## 原版查阅

```bash
git show v8.4.6:PowerEditor/src/Notepad_plus.cpp
```

需要目录级浏览时可创建临时只读 worktree，但不得把原版工作树混入当前分支。

## 影响

- 所有构建命令从仓库根执行。
- `codex/`、`AGENTS.md`、源码、资源和测试均位于同一 Git 仓库。
- 旧目录名只允许出现在本决策之前的历史 Git 提交中。
