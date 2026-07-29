# 插件系统功能分析

## 功能定位

插件系统近期不作为完整功能目标，但代码中需要保留接口和平台隔离边界，避免后续实现时破坏主线架构。

## 当前实现入口

- `./src/PluginSystem/IPlugin.h`
- `./src/PluginSystem/PluginManager.h`
- `./src/PluginSystem/PluginManager.cpp`
- `./src/MainWindow.h`
- `./src/MainWindow.cpp`
- `./CMakeLists.txt`

## 当前接口

`IPluginHost` 提供：

- 当前活动编辑视图。
- 打开文件。
- 当前文件路径。
- 主窗口指针。

`IPlugin` 提供：

- 插件名称。
- 插件版本。
- 初始化。
- 清理。
- 插件贡献的菜单动作。

动态库导出函数类型：

- `createPlugin`
- `destroyPlugin`

## 阶段七更新

- 动态库后缀继续按 Windows/Linux/macOS 隔离。
- 扫描同时支持 `plugins/` 根目录和 `plugins/<plugin-name>/` 一级子目录。
- 使用规范路径去重，避免同一插件重复加载。
- 主窗口析构时显式调用 `cleanup`、`destroyPlugin` 和动态库卸载。
- 仍不宣称兼容原版 Notepad++ 插件 ABI，也不实现 Plugin Admin。

## 已知约束

- 插件近期不实现完整功能。
- 需要预留接口。
- 平台相关动态库加载必须隔离。
- Windows / Linux / macOS 动态库后缀应分别为 `.dll` / `.so` / `.dylib`。

## 当前构建策略

- `ENABLE_PLUGIN_SYSTEM` 默认关闭，符合插件不属于近期目标的约束。
- 显式使用 `-DENABLE_PLUGIN_SYSTEM=ON` 可构建并启用现有 Qt 插件接口。
- 当前 `IPlugin` 是 Qt 化扩展边界，不等同于 Notepad++ 原生插件 ABI。

## 与原版差异

- 原版插件接口包含 `PluginInterface.h`、`Notepad_plus_msgs.h`、`NppData`、`FuncItem`、`ShortcutKey` 和大量 `NPPM_*` 消息。
- 原版插件可获取 Notepad++ 和 Scintilla 的窗口句柄，发送消息控制菜单、状态栏、文档、编码、格式、dock 面板等。
- 原版 `PluginsManager` 负责扫描、加载、菜单初始化、快捷键、通知、Plugin Admin 和 docking 恢复。
- Qt 版 `IPlugin` 只提供 Qt 对象级接口：当前视图、打开文件、当前文件路径、主窗口和菜单动作。
- 因此 Qt 版当前只能视为“插件接口预留”，不能视为“原版插件兼容”。

## 后续索引任务

- 对比原版 `PowerEditor/src/MISC/PluginsManager/`。
- 明确平台适配层边界。
- 设计插件接口保留方案，但不进入近期功能实现。
