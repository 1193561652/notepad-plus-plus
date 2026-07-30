# 插件系统功能分析

## 功能定位

插件系统分为插件管理和插件加载运行。当前已接通插件管理首轮实现；原版插件兼容
加载、新跨平台 ABI 和真实插件清单仍按移植指导分阶段推进。

完整的后续实施边界、ABI 分层、阶段计划和测试矩阵见
`codex/guides/plugin-system-porting-guide.md`。

## 当前实现入口

- `./src/PluginSystem/IPlugin.h`
- `./src/PluginSystem/PluginManager.h`
- `./src/PluginSystem/PluginManager.cpp`
- `./src/PluginSystem/PluginArtifactResolver.h`
- `./src/PluginSystem/PluginCatalog.h`
- `./src/PluginSystem/PluginAdminModel.h`
- `./src/PluginSystem/PluginAdminDialog.h`
- `./src/PluginSystem/PluginUpdatePlan.h`
- `./src/PluginSystem/PluginUpdaterMain.cpp`
- `./src/PluginSystem/PluginArchiveExtractor.cpp`
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

## 插件管理首轮实现（2026-07-30）

- Plugins 菜单提供 Plugins Admin 和打开插件目录入口。
- Plugin Admin 提供 Available、Updates、Installed、Incompatible 四页、搜索、
  描述、批量选择以及安装/更新/卸载确认。
- 内置清单解析保持 v8.4.6 字段与版本区间语义；已原样归档 GitHub
  `nppPluginList/v1.5.4` 的 x86、x64、ARM64 JSON。Windows 按进程架构选择，
  Linux/macOS 保持空清单以避免误装 Windows DLL。
- Windows 插件文件为
  `plugins/<name>/<name>.dll`；Linux 为
  `plugins/<name>/linux/<name>.so`；macOS 为
  `plugins/<name>/macos/<name>.dylib`。不增加 CPU 架构目录。
- 管理器与加载器共用 `PluginArtifactResolver`，排除 `plugins/Config`。
- 安装、更新和卸载写入结构化计划，主程序正常退出后由
  `npp-plugin-updater` 执行；远程包只按已确认范围校验 SHA-256。
- 更新器拒绝目录穿越和符号链接，更新前先完成下载、哈希和解压检查，完成后写入
  `.npp-package.json` 安装收据并重启主程序。
- Windows 可从 DLL 版本资源识别手工安装插件版本；Linux/macOS 的已管理版本从
  安装收据取得，缺少收据时版本显示为 Unknown。

当前 Ubuntu 已完成编译和管理核心针对性测试。Windows 构建、UAC/不可写目录、
PowerShell ZIP、真实网络包和完整 UI 工作流尚待验证。

## 已知约束

- Windows 清单尚未在 Windows 实机完成真实下载和安装验证；Linux/macOS 尚无
  对应的原生插件包清单。
- Unix 更新器当前依赖系统 `unzip` 命令；发布打包必须验证或明确提供该依赖。
- 程序目录不可写时的权限提升启动与降权重启尚未实现；当前会明确失败，不能用于
  需要管理员权限的系统级安装目录。
- 当前 Qt/C++ `IPlugin` 仍只是原型接口，不是冻结的跨平台 ABI。

## 当前构建策略

- `ENABLE_PLUGIN_SYSTEM` 默认关闭，符合插件不属于近期目标的约束。
- 显式使用 `-DENABLE_PLUGIN_SYSTEM=ON` 可构建并启用现有 Qt 插件接口。
- Plugin Admin 和外部更新器不依赖 `ENABLE_PLUGIN_SYSTEM`，以便先完成管理工作。
- 当前 `IPlugin` 是 Qt 化扩展边界，不等同于 Notepad++ 原生插件 ABI。

## 与原版差异

- 原版插件接口包含 `PluginInterface.h`、`Notepad_plus_msgs.h`、`NppData`、`FuncItem`、`ShortcutKey` 和大量 `NPPM_*` 消息。
- 原版插件可获取 Notepad++ 和 Scintilla 的窗口句柄，发送消息控制菜单、状态栏、文档、编码、格式、dock 面板等。
- 原版 `PluginsManager` 负责扫描、加载、菜单初始化、快捷键、通知、Plugin Admin 和 docking 恢复。
- Qt 版 `IPlugin` 只提供 Qt 对象级接口：当前视图、打开文件、当前文件路径、主窗口和菜单动作。
- 因此 Qt 版当前只能视为“插件接口预留”，不能视为“原版插件兼容”。

## 后续索引任务

- 原版 `PowerEditor/src/MISC/PluginsManager/` 的导出、加载顺序、消息、通知和
  Dock 边界已经核验。
- 后续按移植指导中的阶段 0 开始冻结契约；当前 Qt/C++ `IPlugin` 不作为稳定 ABI。
- 跨平台稳定 C ABI 与 Windows 原版 ABI 适配器分别实施，不在 Linux/macOS
  模拟 Win32 插件二进制环境。
