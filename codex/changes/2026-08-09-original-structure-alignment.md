# 原版项目结构对齐

## 修改

- 将编码、本地化、FileManager、DocTabView、自动完成解析器、文件关联、工具栏主题
  和三个内置面板归位到 v8.4.6 对应职责目录。
- 恢复原版文件大小写 `WinControls/Preference/preferenceDlg.*`。
- `DockingWnd` 只保留 DockingManager，业务面板回到各自 `WinControls` 模块。
- 插件宿主服务使用平台中性方法名；Win32 消息翻译继续隔离在
  `Win32PluginSystem`。
- 更新 CMake、测试 include 和代码索引。

## 行为

本批次不改变菜单、编辑器、配置、Dock、插件和文件生命周期行为。类实现只移动，
公开插件宿主方法只做同步重命名。

## 剩余边界

`MainWindow.cpp` 仍合并原版 `Notepad_plus`、`NppCommands`、`NppIO` 和
`NppNotification` 职责。该项需要独立的纯结构重构和调用顺序测试，不在本批次机械
拆分。完整审计见
`codex/analysis/2026-08-09-original-structure-interface-audit.md`。

## 验证

- Release 全目标构建通过。
- CTest `37/37` 通过。
- 记录：
  `codex/validation/2026-08-09-original-structure-alignment/README.md`。
