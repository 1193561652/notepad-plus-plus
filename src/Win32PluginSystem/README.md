# Win32 Plugin System

本目录用于实现 Notepad++ v8.4.6 Windows 插件 ABI 兼容层，只在 Windows
构建中启用。目录内允许直接使用 Windows API、Win32 类型和 Windows SDK
头文件，不要求为这些实现增加非 Windows 桩代码。

## 职责

- 发现并加载原版目录结构中的 Windows `.dll` 插件。
- 兼容原版插件六个导出函数及其生命周期。
- 转换和转发 `NPPM_*`、`NPPN_*` 与 `SCI_*` 消息。
- 隔离 Win32 `HWND`、菜单、工具栏、Dock 和通知结构等平台语义。
- 在单个插件初始化失败时回滚该插件注册，不影响其余插件。

## 非职责

- 插件清单、下载、安装、更新和卸载仍由
  `src/MISC/PluginsManager` 与 `src/WinControls/PluginsAdmin` 负责。
- Linux/macOS 原生插件 ABI 不放入本目录。
- 不向通用业务层暴露 Win32 类型或头文件。

## 依赖边界

- 本目录可以依赖 `MISC/PluginsManager` 提供的平台无关模型和宿主服务。
- 主窗口只通过稳定的宿主 facade 与本目录交互。
- Win32 头文件和实现必须留在本目录，并使用 Windows 构建条件隔离。
- 顶层 CMake 仅在 `WIN32` 为真时进入本目录；Linux 和 macOS 不配置、不编译
  本目录中的任何源码。
- 新源码必须显式加入本目录 `CMakeLists.txt` 的
  `WIN32_PLUGIN_SYSTEM_SOURCES`，不得加入顶层通用 `SOURCES`。
- 在 ABI 行为和生命周期未验证前，不把实验接口声明为稳定接口。

## 计划结构

```text
src/Win32PluginSystem/
  Win32MainWindowAdapter.*
  Win32MainEditorAdapter.*
  Win32SubEditorAdapter.*
  Win32PluginManager.*
  LegacyPluginAdapterWin.*
  LegacyPluginLoaderWin.*
  LegacyMessageRouterWin.*
  LegacyDockAdapterWin.*
  LegacyPluginRecoveryWin.*
```

`Win32PluginManager` 已由 `MainWindow` 在 Windows 启动期间创建，当前持有主窗口、
永久主编辑器和永久副编辑器。主窗口适配器映射 Qt 主窗口的真实 HWND；主/副编辑器
适配器各映射一个由管理器创建的 Win32 原生代理 HWND。`DocTabView` 标签只映射
Buffer，切换标签通过 `SCI_SETDOCPOINTER` 更换两个永久编辑器显示的文档。

两个编辑器代理窗口均为主窗口的隐藏 `WS_CHILD` 窗口，位置 `(0, 0)`、大小 `1×1`，
不设置 `WS_VISIBLE`。窗口过程从 `GWLP_USERDATA` 取得主/副 editor adapter；adapter
处理已支持的 `SCI_*`，未知消息回落到 `DefWindowProcW`。管理器析构时先解除映射，
再销毁两个窗口。

三个适配层分别承担以下稳定身份：

- `Win32MainWindowAdapter`：绑定 `QMainWindow`，作为 `NPPM_*` 主窗口消息入口。
- `Win32MainEditorAdapter`：映射主永久 `ScintillaEditView` 与主代理 HWND。
- `Win32SubEditorAdapter`：映射副永久 `ScintillaEditView` 与副代理 HWND。

`Win32MainWindowAdapter` 使用 Win32 subclass 直接处理
`NPPM_GETCURRENTSCINTILLA`。`Win32EditorMessageAdapter` 是主/副 adapter 的共用实现，
只对白名单消息原样调用 Scintilla 5 消息入口；消息编号、指针宽度和
`WPARAM/LPARAM/LRESULT` 语义与 mimeTools 2.8 使用的接口一致。

## 最小 ABI 加载验证

- `Win32PluginInterface.h` 保持 v8.4.6 的 `NppData`、`FuncItem`、快捷键结构和
  六导出函数签名。
- `Win32PluginManager::loadPlugin()` 使用 `LoadLibraryW/GetProcAddress`，按原版顺序
  校验六个导出，并把主窗口、主编辑器代理和副编辑器代理三个 HWND 通过
  `setInfo(NppData)` 传入 DLL。
- 管理器析构时先发送 `NPPN_SHUTDOWN`，再按逆序 `FreeLibrary`。

`Win32PluginManager::loadPlugins()` 使用 `PluginArtifactResolver` 扫描 Plugins Admin
和 updater 共用的 `NppPath/plugins` 安装目录。当前兼容测试由 `MainWindow` 传入
`mimeTools` 文件夹白名单，因此目录中即使存在其他插件也只加载官方
`mimeTools 2.8 x64`。

CTest 先用 `npp-plugin-updater` 和标准安装计划安装已校验的官方 ZIP，再验证安装
收据、目录发现、ABI 初始化、`MIME Tools` 名称、命令表和关闭生命周期。人工集成
验证还使用同一 updater 从官方 GitHub Release 完成了真实网络下载和事务安装。
当前白名单覆盖 mimeTools v2.8 实际使用的九个消息：
`SCI_GETSELECTIONSTART/END`、`SCI_GETSELTEXT`、`SCI_TARGETFROMSELECTION`、
`SCI_GETTARGETTEXT`、`SCI_SETTARGETSTART/END`、`SCI_REPLACETARGET` 和
`SCI_SETSEL`。真实 DLL 的 Base64 Encode 已在主/副视图通过，URL Encode 在主视图
通过，并覆盖插件分配的输入/输出字节缓冲区。

`mimeTools` 白名单仅用于当前 Win32 ABI 接入测试。扩展消息路由后应移除白名单，按
插件管理器发现结果加载所有兼容插件。
