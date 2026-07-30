# 2026-07-30 插件管理首轮移植

## 范围

本次只实现插件管理主线及跨平台插件文件定位，不实现 Windows 原版插件 ABI
兼容层或新插件 API。Windows 清单采用 GitHub `nppPluginList/v1.5.4` 的三份
原始 JSON；非 Windows 平台在本平台包清单形成前保持空列表。

## 代码变更

- 新增统一的 `PluginArtifactResolver`，供管理和现有加载器共同使用。
- 新增插件版本、兼容区间、旧版本映射和 JSON 清单解析。
- 新增 Available、Updates、Installed、Incompatible 数据模型及 Qt 对话框。
- 新增结构化更新计划、ZIP 路径防护、SHA-256 校验和独立外部更新器。
- Plugins 菜单新增 Plugins Admin 和打开插件目录。
- CMake 构建并随主程序产生 `npp-plugin-updater`，测试构建新增
  `plugin-admin-tests`。

## 行为边界

- Windows：`plugins/<name>/<name>.dll`。
- Linux：`plugins/<name>/linux/<name>.so`。
- macOS：`plugins/<name>/macos/<name>.dylib`。
- 不增加 CPU 架构目录，目标平台/架构由各自清单及插件包区分。
- 安装、更新和卸载均在主程序退出后执行，完成后重启。
- 首版安全范围仅包括包 SHA-256 以及必要的路径、解压和写入正确性检查。

## 未完成项

- 将 GitHub v1.5.4 JSON 与 v8.4.6 官方发行包内 DLL 资源逐字节复核。
- 不可写程序目录的权限提升启动与非特权重启。
- Windows 构建、文件版本、PowerShell ZIP 与完整交互验证。
- Linux/macOS 发布包对 `unzip` 运行时依赖的处理。
- Windows 原版插件加载兼容与新跨平台 API。
