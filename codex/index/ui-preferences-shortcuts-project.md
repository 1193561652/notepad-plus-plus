# Preferences / Shortcut Mapper / Project Panel 索引

## 入口

- `src/Preferences/PreferenceDlg.h|cpp`：Preferences 总对话框、19 个页面构建器、设置装载与写回。
- `src/WinControls/Grid/ShortcutMapper.h|cpp`：快捷键管理 UI、过滤、冲突检测和 `shortcuts.xml` 模型写回。
- `src/WinControls/ProjectPanel/ProjectPanel.h|cpp`：Workspace/Edit UI 与工程树编辑。
- `src/WinControls/ProjectPanel/WorkspaceDocument.h|cpp`：workspace XML 数据模型。
- `src/MainWindow.cpp::setupAuxiliaryPanels()`：三个 Project Dock 的创建与信号连接。
- `src/MainWindow.cpp::createMenus()`：Shortcut Mapper 和 Project Panel 菜单入口。
- `src/NativeLangSpeaker.cpp::changeDlgLang()`：标签、按钮、表格表头和树顶层条目本地化。
- `resources/nativeLang/chineseSimplified.xml`：三个区域的简体中文资源。

## 修改约束

- Preferences 页面职责以 v8.4.6 `preferenceDlg.*` / `preference.rc` 为准。
- Shortcut Mapper 必须保持独立类；不要把表格和写回逻辑重新放回 `MainWindow`。
- Project 1/2/3 必须保持独立 Dock，但共享 `ProjectPanel` 实现。
- 合成的 Workspace 根节点不是 workspace XML 数据节点，保存时只序列化它的子项目。
- 不为 UI 新状态自动增加配置属性；只有 v8.4.6 已有字段才可写入兼容 XML。

## 验证入口

- `tests/UiParityCapture.cpp`
- `tests/ui/CaptureUiMatrix.ps1`
- `ctest --test-dir build -C Release --output-on-failure`
- 当前截图缓存：`build/ui-pixel-parity-final/`

## 2026-08-01 Preferences 最终结构

- `PreferenceDlg` 保持 v8.4.6 的 19 页顺序，页面固定在 660x320 内容区，并使用原资源 DLU 比例定位。
- 新增静态控件必须具有稳定 `objectName`；Qt 专属静态文本可使用语言 XML 的 `source` 回退。
- `NativeLangSpeaker::changeDlgLang()` 的源文本回退覆盖 Label、GroupBox、Button 和 ComboBox 条目。
- Searching 页面直接读写 `NppGUI` 的 v8.4.6 字段，不借用 `FindHistory` 状态。
- `Searching`、`FinderConfig` 写回前必须确认节点原本存在，避免配置自动升级。
- 最新 UI 捕获目录：`build/ui-preferences-verify-final/`。
