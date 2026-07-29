# 2026-07-28 UI 完善验证

## 结果

- 浅色 UI 捕获退出码 0。
- 深色 UI 捕获退出码 0。
- 最终深色 UI 捕获退出码 0。
- 完整构建成功，CTest 22/22。

## 覆盖范围

- 1100x760 主窗口和完整标准工具栏。
- Project Panels Dock。
- Shortcut Mapper 四个页面。
- Find/Replace 五个页面。
- Preferences 十九个页面及 Auto-Completion 滚动区域。

## 视觉修复记录

1. UDL 无默认图标时被渲染成文字工具按钮：
   补入 v8.4.6 `userDefineDlg_off.ico`。
2. 深色 GroupBox 标题对比度不足：
   增加明确标题颜色。
3. Preferences 首屏部分列表项未使用深色文字规则：
   增加 `QListWidget::item` 状态样式。
4. 滚动条箭头区域出现未着色棋盘格：
   明确 add/sub page、line 和 arrow 样式。

## 产物

- `light/`：初次浅色矩阵。
- `dark/`、`dark-final/`、`dark-final-2/`：问题定位与迭代证据。
- `dark-accepted/`：最终验收截图。
