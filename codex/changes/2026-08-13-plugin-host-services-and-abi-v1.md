# Plugin Host Services 与跨平台 ABI v1

## 原版结构对照

以仓库 `v8.4.6` tag 的
`PowerEditor/src/MISC/PluginsManager/{PluginsManager,PluginInterface}` 为基线：

- 插件管理器保留发现、动态库生命周期、六导出、命令表和通知职责。
- 文档、Buffer、编辑器、会话、编码、状态栏和 Dock 行为继续由主控制器及既有模块实现。
- 平台适配器只翻译 ABI 类型和调用服务，不复制业务逻辑。

`MainWindow` 不再继承跨 DLL 的 Qt/C++ `IPluginHost`。新增
`PluginHostServices` 薄门面；`MainWindowPluginHostServices` 只委托已有主控制器操作。
Win32 原版插件适配器也使用同一门面，但 HWND、SCI 和 NPP 消息仍隔离在
`src/Win32PluginSystem`。

## ABI v1

公共定义集中在单一头文件：
`src/CrossPlatformPluginSystem/PluginInterface.h`。接口沿用原版六导出形态，使用 C ABI、
固定宽度整数、UTF-8 和显式 `struct_size`，不暴露 Qt、STL、异常或系统窗口句柄。

`NppPluginHostInfo` 在 `nppSetInfo` 时传递系统类型、CPU 架构、系统名称和版本、软件名称
和版本、插件安装/配置路径，以及当前文件、打开文件和日志函数。

ABI v1 只定义命令、`READY`/`SHUTDOWN` 和最小宿主函数。编辑器、Buffer、Dock、主题和
后台任务等能力待真实复杂插件需求证明后追加，不在首版预先扩张。

## 包边界

ABI v1 使用独立平台目录：

```text
plugins/<id>/cross-platform/windows/<id>.dll
plugins/<id>/cross-platform/linux/<id>.so
plugins/<id>/cross-platform/macos/<id>.dylib
```

这样 Windows 探测新 ABI 时不会执行原版插件 DLL 的 `DllMain`。两类插件共用
`pluginsEnabled.xml`；同目录同时存在两类二进制时优先 ABI v1，不再重复加载原版 DLL。
Plugins Admin 能显示仅含 ABI v1 二进制的已安装插件。

## 验证

- 不依赖 Qt 的共享库测试插件验证六导出、环境信息、当前路径、打开文件、菜单命令和
  `READY` 单次通知。
- Debug 全目标构建通过。
- 完整 CTest `47/47` 通过，包括 27 个原版 DLL 的首次启动与重启共存测试。
