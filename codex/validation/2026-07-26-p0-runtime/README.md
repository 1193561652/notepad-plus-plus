# P0 运行时 UI 验证

验证日期：2026-07-26

## 执行方式

- 使用 Debug 构建的 `ui-parity-capture.exe` 实例化真实 `MainWindow`、
  `FindReplaceDlg` 和 `PreferenceDlg`。
- 自动打开 Project Panels 和 Shortcut Mapper。
- 自动遍历 5 个查找页、19 个首选项页和 4 个 Shortcut Mapper 页。
- 所有截图保存在本目录。

## 结果

- UI 捕获程序退出码：0。
- 主窗口、Project Panels、Find in Projects、标签匹配、备份和自动插入页面均成功显示。
- Shortcut Mapper 初次验证发现动作平铺且混入编码/语言动作。
- 已修复为 Main menu、Macros、Run commands、Scintilla commands 四个分类页。
- Main menu 仅列出具有原版 Notepad++ command ID 的已实现动作。
- 宏和 Run commands 从当前 `shortcuts.xml` 正确显示名称和快捷键。
- 当前用户配置没有 ScintillaKeys 条目，因此该页为空；v8.4.6、
  v7.8.1 和当前配置的 XML 往返仍由 CTest 语料验证。
- Auto-Completion 页面滚动后，六类 Auto-insert 选项完整可见。

## 自动化验证

- 完整 Debug 构建成功。
- CTest `18/18` 通过。

## 仍需用户直接操作

- URL 点击后系统默认应用的实际打开行为。
- 输入法下的字符/标签自动插入及撤销体验。
- 备份目录选择和失败确认框。
- Shortcut Mapper 修改后点击 OK、重启并观察快捷键恢复。
- Project Panel 使用真实 workspace 的加载、编辑、保存。
- Find/Replace in Projects 对真实项目文件的结果跳转和替换确认。
