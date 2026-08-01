# 最终 UI 对照验证

日期：2026-08-01

## 结果

- 原版 v8.4.6 浅色 100% 捕获成功。
- 原版 v8.4.6 深色 100% 捕获成功。
- Qt 简体中文浅色/深色 100%/150% 四组捕获均退出 `0`。
- Qt 完整构建成功，CTest `29/29` 通过。
- Qt 启动标题为 `new 1 - Notepad++`。
- Preferences 单一“关闭”按钮及其简体中文资源验证通过。

## 本地构建产物

截图位于 `build/ui-final-audit/`，不提交到 Git：

- `original-light-100/`
- `original-dark-100/`
- `qt-light-100/`
- `qt-light-150/`
- `qt-dark-100/`
- `qt-dark-150/`
- `preferences-contact-sheet.png`

## 捕获范围

- 主窗口和 Project Panel。
- Shortcut Mapper。
- Find、Replace、Find in Files、Find in Projects、Mark。
- 19 个 Preferences 分类页。
- 浅色和深色主题。
- Qt 100% 和 150% DPI。

完整结论见 `codex/analysis/2026-08-01-final-ui-parity-audit.md`。

