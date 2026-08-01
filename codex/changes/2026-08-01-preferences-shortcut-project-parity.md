# Preferences、Shortcut Mapper 与 Project Panel 对齐

日期：2026-08-01

## 目标基线

- 原版：Notepad++ v8.4.6 Win32。
- 移植版：当前 Qt / Scintilla 5 / Lexilla 主线。
- Windows 100% DPI 以原版截图尺寸为几何基线；150% DPI 用于检查缩放、截断和重叠。
- 对齐行为和代码职责，不把 Win32 消息机械翻译成 Qt 事件。

## Preferences

- `PreferenceDlg` 仍是总对话框，19 个分类继续由独立 `makePage_*()` 构建，和原版子对话框职责对应。
- 客户区固定为 `830x372`，左侧分类列表、分隔线、右侧页面和底部 Close 按原版坐标关系布置。
- General 恢复 Localization、Tool Bar、Tab Bar、Status Bar 和 Menu 分区；界面语言从错误的 Language 页迁回 General。
- Editing 恢复显示、自动换行、光标/滚动和编辑选项的横向分组。
- Language 恢复“语言菜单 + 制表符设置”职责，不再冒充界面语言设置页。
- 配置仍通过现有 `NppGUI` / `ScintillaViewParams` 写回；没有增加 v8.4.6 配置中不存在的 XML 属性。

## Shortcut Mapper

- 新增 `src/WinControls/Grid/ShortcutMapper.*`，路径和职责与原版 `WinControls/Grid/ShortcutMapper` 对应。
- `MainWindow` 只负责收集菜单命令、显示对话框和在变更后刷新宏/编辑器；表格、过滤、冲突检查、修改、清空和写回由 `ShortcutMapper` 负责。
- 恢复 5 个标签：Main menu、Macros、Run commands、Plugin commands、Scintilla commands。
- 恢复行号、Name/Shortcut/Category 表头、冲突区、Filter 和 Modify/Clear/Delete/Close 命令带。
- 主菜单命令按真实菜单树顺序列出；写回继续使用 `NppParameters::writeShortcuts()`，不改变 `shortcuts.xml` 结构。

## Project Panel

- `Project 1/2/3` 恢复为三个独立 `QDockWidget`，不再合并到一个 Dock 的三个标签中。
- 每个 `ProjectPanel` 内部恢复 Workspace/Edit 两页；Workspace 树包含独立工作区根节点。
- Dock 默认目标宽度为 207 px；显示时重新应用宽度，避免隐藏 Dock 的 size hint 把面板撑宽。
- `WorkspaceDocument` 的加载、保存、相对路径和 Find in Projects 信号保持不变；合成工作区根节点不写入 workspace XML。

## 本地化

- `NativeLangSpeaker::changeDlgLang()` 增加表格表头和树顶层条目的翻译支持。
- 简体中文资源补齐 Preferences 新控件、Shortcut Mapper、Project Panel 三个工程标题及页签。

## 验证

- Release 全量构建通过。
- CTest：`29/29` 通过。
- UI 捕获：`build/ui-pixel-parity-final/`。
- 已捕获主窗口、Project Panel、Shortcut Mapper 五页、Find 五页和 Preferences 十九页。

Qt 和 Win32 对字体栅格、边框绘制及系统主题度量的实现不同，因此跨平台不能承诺截图逐字节相同；Windows 100% DPI 的客户区尺寸、控件层级和工作流是当前像素对照基线。
