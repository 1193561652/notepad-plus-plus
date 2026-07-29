# 本地缓存：大文件模式方案决策

缓存日期：2026-07-26

## 结论

- 大文件模式延期决策已解除。
- 第一版选择方案 A：完整复刻 Notepad++ v8.4.6。
- 固定阈值为 200 MiB，达到阈值后静默降级。
- 不增加可配置阈值、非模态提示或按 Buffer 优化。
- 方案 A 的缺陷已接受并记录。
- 方案 B 仅作为后续可选优化，不进入当前实现范围。

## 重要实现约束

- 方案 A 不等于只增加阈值和功能开关。
- Qt 当前 `readAll -> QString -> setText` 不符合原版分块加载模型。
- 实现必须包含带大文件选项的 Scintilla 文档、分块加载和分块保存。
- 约 2 GiB 分支必须核验 QScintilla `long` API 和 Scintilla position 宽度。
- 不能绕过 `QsciDocument` 生命周期直接替换文档指针。

## 入口

- 决策：`codex/decisions/2026-07-26-large-file-mode-v846-parity.md`
- 功能索引：`codex/features/large-file-mode.md`
- 旧延期记录：
  `codex/decisions/2026-07-25-deferred-large-file-and-boost-regex.md`

