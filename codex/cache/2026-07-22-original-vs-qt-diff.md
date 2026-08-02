# 2026-07-22 原版与 Qt 版功能差异缓存

## 本地缓存目的

保存本轮对原版 Notepad++ v8.4.6 与 `./` 的功能点、功能差异和逻辑差异分析，供后续修改前快速恢复上下文。

## 已对照源码

原版重点：

- `PowerEditor/src/Notepad_plus.cpp`
- `PowerEditor/src/NppIO.cpp`
- `PowerEditor/src/NppCommands.cpp`
- `PowerEditor/src/NppBigSwitch.cpp`
- `PowerEditor/src/Parameters.*`
- `PowerEditor/src/ScintillaComponent/Buffer.*`
- `PowerEditor/src/ScintillaComponent/FindReplaceDlg.*`
- `PowerEditor/src/MISC/PluginsManager/*`
- `PowerEditor/src/menuCmdID.h`
- `PowerEditor/src/WinControls/Preference/*`
- `PowerEditor/src/WinControls/FileBrowser/*`
- `PowerEditor/src/WinControls/DocumentMap/*`
- `PowerEditor/src/WinControls/ProjectPanel/*`

Qt 版重点：

- `./src/MainWindow.*`
- `./src/Parameters.*`
- `./src/MISC/FileManager.*`
- `./src/ScintillaComponent/Buffer.*`
- `./src/ScintillaComponent/ScintillaEditView.*`
- `./src/ScintillaComponent/FindReplaceDlg.*`
- `./src/WinControls/TabBar/DocTabView.*`
- `./src/WinControls/DockingWnd/*`
- `./src/WinControls/Preference/PreferenceDlg.*`
- `./src/MISC/PluginsManager/*` 与 `./src/WinControls/PluginsAdmin/*`

## 差异摘要

- 原版是完整 Windows/Scintilla/Win32 插件应用，Qt 版是核心功能子集。
- Qt 版已有主窗口、编辑器包装、文件/Buffer、标签、配置、查找替换、偏好设置、停靠面板和插件接口。
- 原版 Buffer 模型远复杂于 Qt 版，涉及 Scintilla Document、引用计数、文件监控、备份、编码/EOL、语言检测。
- 原版配置覆盖面远大于 Qt 版；完全读写兼容需要专门投入。
- 原版插件接口不能被当前 Qt `IPlugin` 直接兼容。
- 搜索、宏、UDL、打印、项目面板、上下文菜单、工具栏配置、深色模式等仍未达到原版完整行为。

## 重要待办

1. 建立配置 XML 节点级对照表。
2. 建立 FileManager/Buffer 生命周期调用链图。
3. 建立菜单命令 ID 到 QAction/slot 的映射表。
4. 建立查找替换行为对照表。
5. 明确插件“预留接口”与“原版兼容 ABI”之间的边界。
