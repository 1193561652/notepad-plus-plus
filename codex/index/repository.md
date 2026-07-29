# 仓库索引

## 目标

本仓库用于将 Notepad++ v8.4.6 移植为 Qt 跨平台版本。仓库根目录是唯一开发
主线，原版实现保留在 Git tag `v8.4.6` 中。

## 主要目录

- `src/`：Qt 主线代码。
- `resources/`：兼容配置、语言、图标和解析规则。
- `tests/`：自动测试和仓库内 v8.4.6 配置语料。
- `third_party/qscintilla/`：QScintilla 2.13.3 和 Scintilla 源码。
- `third_party/boostregex/`、`third_party/lexilla/`：编辑器核心适配依赖。
- `codex/`：代码知识库和代码索引目录。

## 已确认约束

- 行为一致优先于代码逐行一致。
- 配置文件要求完全读写兼容 Notepad++ 原有 XML 格式和目录结构。
- 插件系统不属于近期目标，但必须保留接口和平台隔离边界。
- 功能范围需要在分析后再指定有限范围。
- 每个功能完成后至少验证；阶段内按风险决定是否构建。

## 迁移状态

2026-07-29 已从旧工作区迁入当前 `qt-port` 分支。迁移决策见
`codex/decisions/2026-07-29-repository-root-migration.md`。

## 参考顺序

1. 先查阅 `codex/` 已有索引。
2. 使用 `git show v8.4.6:<path>` 查阅原版实现，确认目标行为。
3. 核验当前分支真实源码和调用关系。
4. 在仓库根目录进行局部修改和验证。
