# 编辑器 3D 边框修复

日期：2026-08-01

## 原因

原版 v8.4.6 通过 `ScintillaEditView::setBorderEdge()` 为浅色编辑器启用
`WS_EX_CLIENTEDGE`，形成下沉 3D 边框；深色模式改用单线 `WS_BORDER`。官方 Qt
`ScintillaEditBase` 构造函数则明确设置 `QFrame::NoFrame`，移植层此前没有恢复该
行为，Preferences 中的“无边缘”控件也未接入配置。

## 修改

- 在 `ScintillaEditView` 适配层实现 `setBorderEdge()`，不修改第三方 Scintilla。
- 浅色模式使用更接近 `WS_EX_CLIENTEDGE` 的 `QFrame::WinPanel | QFrame::Sunken`，深色模式使用
  `QFrame::Box | QFrame::Plain`，禁用时恢复 `QFrame::NoFrame`。
- 读取和保守写回 `ScintillaPrimaryView borderEdge`，默认值保持原版的启用状态。
- Preferences 的“无边缘”复选框加载和保存实际配置；应用后立即刷新全部编辑视图。
- 恢复 `borderWidth` 的 `0-30px` 配置、首选项滑块和 DocTabView 外部间距；默认
  2px，不再让编辑器边框直接贴住标签页面板。
- UI 捕获增加浅色和深色 frame 断言。

## 验证

- 浅色/深色、100%/150% DPI UI 捕获通过。
- v8.4.6 和当前用户配置语料读写测试通过。
- Debug 全目标构建成功，CTest `31/31` 通过。
