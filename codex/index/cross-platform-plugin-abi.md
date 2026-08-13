# 跨平台插件 ABI 代码索引

## 公共接口

- `src/CrossPlatformPluginSystem/PluginInterface.h`：ABI v1 唯一公共头文件。
- `src/CrossPlatformPluginSystem/README.md`：插件作者的导出、所有权、线程和包布局说明。

## 宿主实现

- `src/MISC/PluginsManager/PluginManager.*`：发现 ABI v1 动态库、解析六导出、建立命令表、
  发送生命周期通知并逆序卸载。
- `src/MISC/PluginsManager/PluginHostServices.*`：平台中性宿主服务门面；实现只委托
  `MainWindow` 和 `NppParameters` 的既有行为。
- `src/MISC/PluginsManager/PluginArtifactResolver.*`：原版二进制路径和 ABI v1 独立平台
  路径。
- `src/Win32PluginSystem/Win32MainWindowAdapter.*`：NPPM/Win32 类型到同一宿主服务的
  薄适配，不拥有业务逻辑。
- `src/MainWindow.cpp::setupPluginSystem()`：读取统一启用配置，先加载 ABI v1，再把未由
  ABI v1 占用的目录交给 Windows 原版 DLL 管理器。

## 测试

- `tests/plugins/CrossPlatformAbiPlugin.cpp`：纯 C ABI 形态的最小真实共享库插件。
- `tests/CrossPlatformPluginTests.cpp`：环境、版本、回调、命令和生命周期验证。
- `tests/PluginAdminTests.cpp`：ABI v1 平台目录发现和已安装页识别。

修改公共头文件后必须至少运行 `cross-platform-plugin-tests`、`plugin-admin-tests`、
Win32 插件共存测试和完整 CTest。`NppPluginHostInfo` 新字段只能追加并通过
`struct_size` 判断；连续的 `NppPluginFuncItem` 数组改变布局时必须升级 ABI 版本。
