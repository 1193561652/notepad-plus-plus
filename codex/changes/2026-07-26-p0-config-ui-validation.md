# 2026-07-26 P0 配置与 UI 验证变更

## 代码

- 新增 `tests/ConfigCorpusTests.cpp` 和 14 个 CTest XML 语料用例。
- `Parameters.cpp` 改为保守原位更新 config、FindHistory、最近文件和 Session。
- 新增 `tests/UiParityCapture.cpp` 与 `ui-parity-capture` 构建目标。
- 新增 `tests/ui/CaptureUiMatrix.ps1`，用于原版外部窗口基线采集。
- 新增 `MISC/UiFont.h`；Windows 优先采用 Segoe UI 9 pt。
- 主窗口标题移除 Qt 后缀，查找对话框标题改为原版的 `Find`。
- 调整 Find 与 Preferences 的初始尺寸。

## 验证

- 应用和全部测试目标构建通过。
- 配置语料 14/14 通过。
- Qt 100% / 150% UI 矩阵各生成 28 个文件。
- v8.4.6 原版成功回读 Qt 写回后的隔离配置。

详细报告：
`codex/analysis/2026-07-26-p0-config-ui-validation.md`

