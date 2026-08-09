# Windows 初始窗口外框位置修复

## 问题

原版 v8.4.6 的 `AppPosition` 来自 `WINDOWPLACEMENT.rcNormalPosition`，其 `x/y/width/height` 表示完整窗口外框。Qt 版曾用顶层窗口 `setGeometry()` 恢复该矩形，并在关闭时用 `geometry()` 保存客户区矩形。

当默认配置为 `x=0, y=0` 时，Qt 把客户区左上角放在屏幕原点，Windows 标题栏和边框因此位于可视工作区之外；调整窗口大小触发系统重新计算非客户区后，边框才出现。

## 修复

- 恢复时读取 `QWindow::frameMargins()`，从配置的外框尺寸扣除边框后设置客户区大小，并使用 `move()` 设置外框左上角。
- 关闭普通窗口时使用 `frameGeometry()` 写回 `AppPosition`，保持与 v8.4.6 的 `rcNormalPosition` 语义一致。
- UI 回归断言窗口外框左上角位于当前屏幕可用区域内。

## 验证

- `notepadpp-qt` 和 `ui-parity-capture` 构建通过。
- 插件安装 fixture、100% UI 运行测试和核心行为测试 `3/3` 通过。
- 2026-08-08 人工验证确认：全新配置启动后，窗口首次显示即具有完整边框和标题栏，无需再调整窗口大小。
