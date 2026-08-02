# 原版模块布局与边栏光标修复

日期：2026-08-02

## 结构调整

- Preferences 从顶层 `src/Preferences` 迁移到
  `src/WinControls/Preference`，与原版 `WinControls/Preference` 对齐。
- 插件运行接口、清单、更新计划和更新器迁移到
  `src/MISC/PluginsManager`。
- Plugin Admin 的对话框与展示模型迁移到
  `src/WinControls/PluginsAdmin`。
- 打印实现从 Qt 移植期的顶层 `src/Printing/NotepadPlusPrinter.*` 归位并改名为
  原版结构对应的 `src/ScintillaComponent/Printer.*`；打印行为保持不变。
- 不再使用顶层 `src/PluginSystem`；后续插件 ABI 与加载器继续在
  `MISC/PluginsManager` 内演进，UI 保持在 `WinControls/PluginsAdmin`。
- 已删除空的 `src/Printing`、`src/PluginSystem`、`src/Preferences`，以及根目录下
  未被构建使用的空 `include`、`cmake` 目录。
- 新建 `src/Win32PluginSystem`，作为原版 Windows DLL 插件 ABI 兼容层的独立
  边界；当前先记录职责和计划结构，不接入未实现源码。
- 新增 `Win32PluginManager`；Windows 主窗口启动时传入主窗口和主/副编辑器
  宿主，管理类保存对象及 HWND。非 Windows 构建不配置这些源码。

## 行号边栏光标

- Scintilla 5.3.0 核心对 margin 默认返回 `Cursor::reverseArrow`，应用层也为
  书签边栏设置 `SC_CURSORREVERSEARROW`。
- 原 Qt 平台层 `PlatQt.cpp` 未处理 `Cursor::reverseArrow`，因此回退为
  `Qt::ArrowCursor`，显示成普通左向指针。
- 修复位于 Scintilla Qt 平台适配层：增加右向自定义指针，热点位于右上角；
  不在 `ScintillaEditView` 中重复判断边栏坐标。

## 验证

- 全目标 Debug 构建成功。
- `large-file-mode-tests` 通过，验证书签边栏使用右向指针，文本区恢复 I-beam。
- `ui-parity-capture` 通过，验证行号边栏真实鼠标移动后使用右向指针。
- 完整 CTest `31/31` 通过。
