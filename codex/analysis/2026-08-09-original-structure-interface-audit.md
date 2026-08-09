# 原版结构、接口与逻辑一致性审计

日期：2026-08-09

## 基线与优先级

审计基线为 Git tag `v8.4.6:PowerEditor/src`。判定顺序为：

1. 优先保持项目目录、模块所有者和类职责一致。
2. 在职责一致的前提下，尽量保持类名、入口和生命周期接口一致。
3. Qt signal/slot、对象父子所有权和 `QDockWidget` 等底层机制允许替代 Win32
   message、HWND 和 Dock 容器实现，但不得改变业务状态所有者和调用方向。

## 本轮结构归位

| 原版位置 | Qt 版归位后位置 | 责任映射 |
| --- | --- | --- |
| `EncodingMapper.*` | `src/EncodingMapper.*` | 编码编号和 codec 映射 |
| `localization.*` | `src/localization.*` | `NativeLangSpeaker` |
| `ScintillaComponent/Buffer.*` | `ScintillaComponent/Buffer.*`、`FileManager.*` | Buffer 状态及 FileManager 生命周期 |
| `ScintillaComponent/DocTabView.*` | `ScintillaComponent/DocTabView.*` | 主/副永久编辑器的标签映射 |
| `ScintillaComponent/AutoCompletion.*` | `ScintillaComponent/AutoCompletionParser.*` | API XML 解析；运行部分仍由编辑视图调用 |
| `WinControls/DocumentMap/` | `WinControls/DocumentMap/documentMap.*` | Document Map 面板 |
| `WinControls/FileBrowser/` | `WinControls/FileBrowser/fileBrowser.*` | File Browser 面板 |
| `WinControls/FunctionList/` | `WinControls/FunctionList/functionListPanel.*`、`functionParser.*` | Function List UI 与解析 |
| `WinControls/ToolBar/` | `WinControls/ToolBar/ToolbarIconTheme.*` | 工具栏图标主题 |
| `MISC/RegExt/` | `MISC/RegExt/FileAssociationModel.*` | Windows 文件关联模型 |
| `WinControls/Preference/preferenceDlg.*` | 同路径和大小写 | Preferences 所有者和生命周期 |

`WinControls/DockingWnd/` 现在只保留 `DockingManager`，不再把各业务面板错误地
归入布局管理器目录。空的 `WinControls/TabBar/` 已删除；原版的 `DocTabView` 本来
就属于 `ScintillaComponent`。

## 核心接口与逻辑对照

| 原版结构 | Qt 当前结构 | 结论 |
| --- | --- | --- |
| `NppParameters` 单例拥有 GUI、Session、语言、样式和快捷键状态 | 同名 `NppParameters`，TinyXml 读写原版 XML，Qt 私有窗口状态隔离到 `qtState.ini` | 所有者和主要入口一致 |
| `BufferID` 为 Buffer 指针；`MainFileManager` 管理 Buffer | 保留 `BufferID`、`BUFFER_INVALID`、`MainFileManager` 和单例 `FileManager` | 接口形态接近；Qt 字符串和时间类型替代 Win32 类型 |
| 主/副 `DocTabView` 与主/副永久 `ScintillaEditView` | 两个永久编辑器；标签切换只执行 `SCI_SETDOCPOINTER` | 生命周期和调用逻辑一致 |
| `FindReplaceDlg`、`PreferenceDlg` 由主控制器长期持有 | `MainWindow` 延迟创建并长期持有 QWidget 对话框 | 所有者一致；Qt 为避免启动成本采用安全的延迟实例化 |
| `DockingManager` 注册和管理业务 Dock | 同名管理器封装 `QDockWidget`，调用者不直接管理 Dock 生命周期 | 职责和入口一致，底层实现不同 |
| 原版 `PluginsManager` 处理 Windows 六导出和消息 | 包管理在 `MISC/PluginsManager`；Windows ABI 在 `Win32PluginSystem`；二者共用路径和宿主状态 | 按平台隔离约束形成可接受拆分 |
| Win32 `WM_COMMAND`、`WM_NOTIFY`、`SCNotification` | Qt action/signal 驱动业务，Windows 插件边界恢复原消息 ABI | 行为顺序一致，分发机制允许不同 |

本轮同时把 `MainWindow` 暴露的文档、命令、编码和状态栏插件服务改为平台中性命名。
`Win32MainWindowAdapter` 负责把 `NPPM_*` 翻译为这些宿主服务，业务接口不再出现
`ForWin32Plugin` / `FromWin32Plugin`。仅 Windows 测试需要的管理器访问器继续受
`Q_OS_WIN` 限定。

## 仍存在的结构差异

### 主控制器实现分片

原版将职责分散在 `Notepad_plus.cpp`、`NppCommands.cpp`、`NppIO.cpp` 和
`NppNotification.cpp`，并以 `Notepad_plus_Window` 封装 Win32 窗口。Qt 当前的
`MainWindow.cpp` 同时承担上述四类方法，文件规模约 7600 行。这是当前最明显的
结构差异。

本轮没有机械拆分该文件，原因是其中的文件生命周期、命令 action、通知、插件和
局部 helper 互相引用；在没有先建立分片边界测试时直接移动定义会扩大回归面。
后续应作为独立纯结构批次实施：

- `Notepad_plus`：状态所有者、初始化、主/副视图和顶层协调；
- `NppCommands`：QAction 命令入口和命令状态；
- `NppIO`：打开、保存、关闭、监视、最近文件和 Session 文件流程；
- `NppNotification`：Scintilla、标签、焦点和插件生命周期通知；
- `MainWindow` 或 Qt 平台窗口层：只保留 `QMainWindow` 事件和控件承载。

该拆分必须先增加方法所有者/调用顺序测试，且不得借结构调整改变行为。

### 合理的平台差异

- Qt 使用 `main.cpp`，不使用只适用于 Win32 GUI subsystem 的 `winmain.cpp`。
- Qt 对象父子关系和 `QObject` 自动销毁替代原版显式 HWND `destroy()` 链。
- QWidget 对话框、菜单和 Dock 接口使用对象指针和稳定 `objectName`，替代资源 ID
  与 HWND；原版 XML 和 command ID 仍保持兼容。
- 当前实验性 Qt `IPlugin` 不是原版 ABI，也不是冻结的跨平台 ABI；不得据此改变
  Windows 原版插件兼容层。

## 验证要求

- 所有移动必须在 Windows 和大小写敏感平台使用完全相同的 include 路径。
- Release 全目标构建和全部 CTest 必须通过。
- UI、配置语料、永久双视图、Dock 和真实插件回归必须继续覆盖。
