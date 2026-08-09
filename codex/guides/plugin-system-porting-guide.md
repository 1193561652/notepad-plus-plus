# 插件系统移植指导

## 1. 文档目的

本文用于指导 Notepad++ for Qt 后续插件系统的设计、实现和验收。目标是在不污染
编辑器业务层的前提下，建立可维护的跨平台插件能力，并尽可能保留原版 Notepad++
v8.4.6 的使用习惯。

本文不是“让 Windows 插件 DLL 在所有平台直接运行”的方案。原版插件 ABI 公开了
`HWND`、Win32 消息、`TCHAR`、调用约定和插件自建 Win32 窗口，因此：

- Windows 可以通过专用适配层兼容原版二进制 ABI。
- Linux 和 macOS 不能直接加载现有 Windows 插件 DLL。
- 跨平台插件必须使用新的稳定 ABI，或针对 Qt 移植版重新编译。
- 不在 Linux/macOS 上模拟 `HWND` 或完整 Win32 消息系统。

行为兼容优先于接口逐字复制。能跨平台表达的原版能力应保持工作流和结果一致，
平台专属能力则由平台适配层处理。

### 1.1 项目级移植策略

插件按价值、复杂度和兼容成本分为三类处理：

1. **简单且常用的插件**：主程序在 Windows 上提供有限兼容层，支持经过验证的
   插件加载、常用 `NPPM_*`、`SCI_*` 和基础通知，使选定的原版 DLL 可以直接
   运行。
2. **常用但难以兼容的插件**：基于新的跨平台插件 API 修改插件源码，保留核心
   业务逻辑，重写宿主调用和 Win32 UI。JSON Viewer、ComparePlus、XML Tools、
   DSpellCheck、HexEditor 等类型的插件优先按此路线评估。
3. **不常用且移植成本高的插件**：不提供官方移植；新 API 和 SDK 稳定后允许
   社区自行实现或维护。

总体原则为：

> 有限兼容旧插件，正式发展新插件 API，移植常用复杂插件，放弃低价值长尾插件。

旧插件兼容层和新插件 API 必须调用同一套 Host Services，不得形成两套文档、
编辑器、命令或事件实现。

插件分类不是永久标签。评估时至少记录：

- 用户需求和实际使用量。
- 插件源码、许可证及维护状态。
- 构建是否可复现，依赖和运行时能否跨平台。
- 使用的 `NPPM_*`、`SCI_*`、通知、Dock 和 Win32 UI 范围。
- 安全风险、移植工作量及后续维护成本。

对外状态统一使用“直接兼容、部分兼容、已移植、社区维护、不支持”，并同时标明
插件版本、CPU 架构、主程序版本、测试平台和已知缺失功能。“部分兼容”不能作为
没有明细的笼统承诺。

## 2. 已核验基线

### 2.1 原版 v8.4.6

权威源码：

- `v8.4.6:PowerEditor/src/MISC/PluginsManager/PluginInterface.h`
- `v8.4.6:PowerEditor/src/MISC/PluginsManager/PluginsManager.h`
- `v8.4.6:PowerEditor/src/MISC/PluginsManager/PluginsManager.cpp`
- `v8.4.6:PowerEditor/src/MISC/PluginsManager/Notepad_plus_msgs.h`
- `v8.4.6:PowerEditor/src/WinControls/DockingWnd/Docking.h`

原版插件导出以下入口：

- `isUnicode`
- `setInfo`
- `getName`
- `getFuncsArray`
- `beNotified`
- `messageProc`

宿主通过 `NppData` 向插件提供主窗口及两个 Scintilla 窗口句柄；通过 `FuncItem`
注册命令、初始勾选状态和快捷键；通过 `NPPM_*` 消息提供文档、视图、菜单、
状态栏、Dock、配置目录和插件间通信；通过 `NPPN_*` 与 Scintilla 通知广播生命
周期和编辑器事件。

原版目录约定为 `plugins/<插件目录>/<插件目录>.dll`，并排除
`plugins/Config`。加载顺序的关键点是：解析并校验导出、调用 `setInfo`、取得
`FuncItem`、分配命令 ID、建立菜单，随后在主程序就绪时发送 `NPPN_READY`。
关闭阶段在动态库卸载前发送关闭通知。

原版存在两层兼容性检查：

1. **插件列表元数据检查**：Plugin Admin 从 `nppPluginList` 数据读取插件版本、
   `npp-compatible-versions` 和可选的 `old-versions-compatibility`。前者声明
   当前仓库版本适用的 Notepad++ 版本闭区间；后者把一段旧插件版本映射到一段
   Notepad++ 兼容版本。区间端点为空表示不限制，两个端点都为空则按兼容处理。
2. **动态库加载检查**：加载前检查 DLL CPU 架构，加载后检查 `isUnicode` 和
   必需导出函数；lexer 插件还检查 Lexilla 导出及其 XML 配置。这些检查失败时
   插件不会进入正常注册流程。

原版不通过插件目录名区分 CPU 架构。Notepad++ 的 x86、x64 和 ARM64 安装包分别
携带对应构建目录中的 `plugins/Config/nppPluginList.dll`，因此 Plugin Admin
看到的仓库列表已经按发行架构准备。实际加载插件时，`getBinaryArchitectureType`
映射 DLL、读取 PE 头的 `FileHeader.Machine`，再与当前 Notepad++ 编译时确定的
`IMAGE_FILE_MACHINE_I386`、`IMAGE_FILE_MACHINE_AMD64` 或
`IMAGE_FILE_MACHINE_ARM64` 比较；不一致就拒绝加载。这是分发筛选之后的第二道
运行时保护。

当 Plugin Admin 元数据可用时，`PluginsManager::loadPlugins` 会在加载 DLL 前用
主程序文件版本、插件 DLL 文件版本和仓库声明进行匹配。不兼容插件进入单独列表
且不加载；若仓库有更高且兼容的版本，可进入更新列表。没有 Plugin Admin
元数据、插件不在仓库中或元数据未声明限制时，原版只能依赖动态库层面的基础检查。

这不是运行能力探测：原版不会静态分析插件实际使用了哪些 `NPPM_*`、`SCI_*`、
Dock 或内部窗口，也不能保证通过版本区间和导出检查的插件运行时一定兼容。

### 2.2 当前 Qt 主线

当前入口：

- `src/MISC/PluginsManager/IPlugin.h`
- `src/MISC/PluginsManager/PluginManager.h`
- `src/MISC/PluginsManager/PluginManager.cpp`
- `src/MainWindow.h`
- `src/MainWindow.cpp`
- `CMakeLists.txt`

当前实现使用 `QLibrary` 加载 `.dll`、`.so` 或 `.dylib`，查找
`createPlugin`/`destroyPlugin`，并直接跨动态库边界传递 `QString`、`QList`、
`QAction`、`QMainWindow` 和 C++ 虚接口。它适合作为功能原型，但存在以下限制：

- ABI 绑定编译器、标准库、Qt 主版本、Qt 构建选项和编译配置。
- 没有 ABI 版本协商或能力协商。
- 没有原版命令 ID、快捷键、通知、消息和 Dock 语义。
- 插件异常可越过模块边界，加载失败也没有结构化诊断。
- 插件目录固定为应用目录下的 `plugins/`，尚未统一到配置路径策略。
- `ENABLE_PLUGIN_SYSTEM` 默认关闭，尚无完整插件测试。

因此，不应把当前 `IPlugin` 直接冻结为公开的长期 ABI。

## 3. 总体架构

插件系统分为三层：

```text
编辑器业务与状态
        │
        ▼
Plugin Host Services（平台无关、只暴露受控能力）
        │
        ├── Cross-platform ABI Adapter ── Qt/Linux/macOS/Windows 新插件
        │
        └── Legacy N++ ABI Adapter Win ── Windows 原版插件
```

### 3.1 跨平台稳定 ABI

长期公开边界使用版本化 C ABI，而不是 Qt/C++ ABI。建议唯一入口类似：

```c
NppQtResult nppqt_plugin_query(
    uint32_t host_abi_version,
    const NppQtHostApi *host,
    NppQtPluginApi *plugin);
```

具体命名可以在阶段 0 冻结，但必须遵守：

- 公开头文件能以 C 编译，不包含 Qt、STL 或项目内部头文件。
- 使用固定宽度整数、显式长度、UTF-8 字节串和 POD 结构。
- 结构体包含 `struct_size` 和 `abi_version`，尾部只追加字段。
- 宿主和插件以函数表、不可解引用的句柄通信。
- 内存由分配方释放；跨模块释放必须调用分配方提供的释放函数。
- 回调返回错误码；C++ 异常不得跨 ABI 边界。
- 入口声明统一封装导出可见性和调用约定。
- 新能力通过能力位或可选服务表协商，不通过猜测版本启用。
- 所有句柄定义失效时机，禁止缓存已关闭的文档、视图或面板句柄。

Qt 便利封装可以作为 SDK 单独提供，但它只能包装 C ABI，不能取代 C ABI。

### 3.2 Windows 原版 ABI 适配器

Windows 专用适配器负责：

- 按 v8.4.6 签名解析六个原版导出。
- 提供真实的主窗口和两个 Scintilla 兼容窗口句柄。
- 将已支持的 `NPPM_*` 映射到平台无关 Host Services。
- 将菜单命令、Scintilla 通知和 `NPPN_*` 事件按原版顺序转发。
- 适配动态命令 ID、marker 分配、插件间消息和 Dock 注册。
- 对尚未支持的消息返回原版约定的失败值，并记录一次可诊断日志。

此适配器必须位于 `Q_OS_WIN` 平台目录中。业务层不得出现 `HWND`、`WPARAM`、
`LPARAM` 或 `SendMessage`。

兼容层采用接口白名单，不以覆盖全部 v8.4.6 插件接口为目标。白名单中的每个
`NPPM_*`、`NPPN_*` 和特殊 `SCI_*` 用法必须由目标插件、自动测试或明确的 SDK
需求证明。插件一旦依赖复杂 Dock、窗口子类化、工具栏内部结构、主窗口控件层级
或特殊编辑视图，应优先转为源码移植，不继续扩大 Win32 模拟范围。

“能够加载”不代表“兼容”。只有通过消息、通知、菜单、快捷键、编辑和 Dock
验收矩阵的插件，才可列入兼容清单。

### 3.3 Host Services

Host Services 是两个 ABI 适配器共用的核心，按领域拆分：

- Application：版本、路径、语言、主题和主线程调度。
- Documents：打开、关闭、保存、激活、枚举、路径和脏状态。
- Views：活动视图、主/副视图、文档位置和视图切换。
- Editor：受控的 Scintilla 消息入口及通知订阅。
- Commands：命令注册、执行、勾选、启用状态和快捷键。
- UI：菜单、工具栏、状态栏、对话框和 Dock 面板。
- Events：应用、文档、视图、语言、主题和关闭事件。
- Storage：插件专属配置目录和安全的路径解析。
- Messaging：插件间按插件 ID 定向发送的版本化消息。
- Resources：动态命令 ID、marker、indicator 等有限资源分配。

Host Services 只能依赖稳定的编辑器 facade，不能让插件直接持有 `MainWindow`、
`Buffer`、`DocTabView` 或 `ScintillaEditView` 指针。

新 API 至少应提供文档与编辑器访问、命令和菜单注册、事件通知、面板和自定义
视图、设置、文件系统及进程服务。文件系统和进程能力由宿主服务封装并形成可审计
调用，不直接向插件泄露内部对象。

## 4. 兼容边界与规则

### 4.1 文本和路径

- 跨平台 ABI 一律使用 UTF-8 与显式长度，不传 `wchar_t` 或 `TCHAR`。
- 文件路径由宿主规范化，但不得擅自解析、改写或升级用户配置内容。
- 插件安装目录与可写配置目录分离。
- `plugins/Config` 兼容目录继续保留；新 ABI 优先给每个插件分配独立配置子目录。
- 便携模式、`-settingsDir=` 和平台默认目录统一通过 `ConfigPathResolver`，
  不在 PluginManager 中重新实现路径优先级。

### 4.2 插件身份和目录

新插件清单至少包含稳定插件 ID、显示名、版本、ABI 范围、入口库和能力声明。
显示名不能作为唯一身份。目录建议保持：

```text
plugins/
  <plugin-id>/
    manifest.json
    <platform library>
    resources/
  Config/
    <plugin-id>/
```

在迁移期继续识别原版 `plugins/<name>/<name>.dll`。根目录散落动态库仅作为当前
原型的临时兼容，不应成为新格式。

扫描必须确定性排序，使用规范路径去重，拒绝目录穿越和符号链接逃逸，并把
“未找到”“格式错误”“架构不符”“ABI 不符”“依赖缺失”区分记录。

### 4.3 生命周期

统一状态机：

```text
Discovered → LibraryLoaded → Queried → Initialized → Ready
                                              │
                       Unloading ← ShuttingDown
```

要求：

- `query` 阶段不能访问尚未就绪的 UI。
- `initialize` 失败必须按逆序释放已取得资源。
- `Ready` 只发送一次，且在菜单、文档和视图初始化完成后发送。
- 关闭事件分为“可取消的关闭开始”和“不可逆的最终关闭”。
- 最终关闭后不再发送普通通知。
- 先断开回调和 UI，再释放插件对象，最后卸载动态库。
- 宿主退出期间禁止插件注册新异步任务。
- 所有 UI 调用只能在主线程；跨线程请求通过 Host Services 排队。

插件回调周围必须建立异常和重入保护。捕获异常可以防止普通 C++ 异常传播，但
不能把进程内插件视为沙箱；访问越界等故障仍可能终止宿主。

### 4.4 命令、菜单和快捷键

- 插件注册稳定的本地命令键，宿主分配运行期整数 ID。
- 命令显示名可本地化，但持久化匹配不能只依赖显示名。
- 菜单结构由描述数据构建，宿主拥有最终 `QAction` 生命周期。
- 支持命令启用、禁用、勾选和单选组状态。
- 原版 `shortcuts.xml` 的 `PluginCommands` 继续兼容读取和保守写回。
- 快捷键冲突使用现有 Shortcut Mapper 规则，不由插件静默覆盖。
- 卸载插件时保留未知插件的快捷键配置，不能删除用户数据。

### 4.5 事件和 Scintilla

先定义平台无关事件，再映射到 `NPPN_*`。事件负载只包含稳定句柄和 POD 数据。
至少覆盖：

- 应用就绪、关闭开始、关闭取消、最终关闭。
- 文件打开前/后/失败、保存前/后、关闭前/后。
- 活动 Buffer、语言、只读状态、标签顺序和主题变化。
- Scintilla 修改、光标、缩放、UI 更新等插件常用通知。

通知必须记录顺序、线程和是否允许重入。高频 Scintilla 通知应支持按类别订阅，
避免无条件广播造成性能回退。

新 ABI 可提供受控的 Scintilla 消息调用接口以保留编辑器语义，但必须校验视图
句柄、指针参数和缓冲区长度。不得用 `QRegularExpression` 替代插件发起的
Scintilla 正则操作。

### 4.6 Dock 和插件 UI

Qt 原生 Dock 由宿主创建 `QDockWidget`、保存状态并拥有外层容器。插件只提供
内容或声明式面板描述。

需要区分两种稳定性级别：

- 与主程序使用完全相同 Qt 构建的新插件，可通过 SDK 提供 `QWidget` 内容；
  这是源码/构建兼容层，不承诺长期二进制兼容。
- 需要长期稳定 ABI 的插件，不跨边界传 `QWidget*`，使用宿主控件描述、Web
  内容或未来的进程外 UI 协议。

Windows 原版插件 Dock 由 Legacy Adapter 接收 `tTbData`，并把真实 Win32
子窗口嵌入或托管到 Qt 窗口。该工作必须单独做原型验证，不能影响跨平台接口。

Dock 持久化键至少包含插件 ID 和面板 ID；缺少插件时保留布局信息，重装后可恢复。

### 4.7 自定义文档和视图

自定义视图不同于 Dock 面板。HexEditor 一类插件本质上提供独立的二进制编辑器，
不应把原 DLL 强行塞入兼容层，也不能只把一个 `QWidget` 放入普通文本标签页。

跨平台 API 需要单独定义自定义文档/视图扩展点，至少处理：

- 文档类型识别、Buffer 身份和主/副视图关系。
- 打开、保存、另存、重载、关闭和脏状态。
- 会话恢复、标签标题、路径变化和外部文件变化。
- 选择、复制、撤销、查找及命令启用状态。
- 大文件访问方式和内存上限。
- 普通文本视图与自定义视图之间是否允许及如何切换。

宿主仍负责标签、会话、文件冲突处理和关闭工作流；插件负责格式专属的数据模型和
编辑 UI。自定义视图协议应在基础 ABI 稳定后独立设计，不能为了 HexEditor 提前
扩大 ABI v1 的最小范围。

## 5. 安全和可靠性

插件是进程内本机代码，默认拥有与编辑器相同的用户权限。界面必须明确这一事实。

- 不自动下载或加载来源未知的插件。
- 首次加载记录来源和文件哈希。
- 保持原版安全启动能力（`-noPlugin` 必须始终可用）。
- 插件安装、更新和卸载在退出主程序后执行；首版保持原版清理/解压流程。
- 首版对下载插件包执行 SHA-256 校验，不增加其他密码学安全校验。
- 日志不得包含文档内容、密钥或完整敏感路径；详细路径仅在用户主动诊断时显示。
- 单个插件初始化失败不能阻止其他插件加载。
- 检测到上次启动在插件加载阶段异常时，下一次提供隔离该插件的恢复路径。

进程隔离可作为后续增强，但不是首版实现的前置条件。需要文件系统、网络或进程
权限控制时，应设计进程外插件协议，不能宣称进程内插件已被沙箱化。

## 6. 建议模块划分

后续插件运行时应在原版目录职责上继续演进，不再建立顶层 `PluginSystem`：

```text
src/MISC/PluginsManager/
  abi/
    NppQtPluginAbi.h
  PluginRegistry.*
  PluginLoader.*
  PluginHostServices.*
  PluginCommandRouter.*
  PluginEventBus.*
  PluginStorage.*
  PluginDiagnostics.*
  updater/
    PluginArchiveExtractor.*
    PluginUpdateExecutor.*
    PluginUpdaterMain.cpp
  sdk/
    QtPluginSdk.*

src/WinControls/PluginsAdmin/
  PluginAdminModel.*
  PluginAdminDialog.*
  QtPluginUiAdapter.*
  PluginDockService.*

src/Win32PluginSystem/
  LegacyPluginAdapterWin.*
  LegacyPluginLoaderWin.*
  LegacyMessageRouterWin.*
  LegacyDockAdapterWin.*
  LegacyPluginRecoveryWin.*
```

`MainWindow` 只负责建立宿主 facade、提供菜单/Dock 挂载点和转发明确的应用事件。
插件扫描、ABI 解析、错误处理、资源所有权和卸载顺序都留在
`MISC/PluginsManager`；插件管理界面只留在 `WinControls/PluginsAdmin`；原版
Windows DLL 插件兼容实现隔离在 `Win32PluginSystem`。

顶层 CMake 只在 `WIN32` 条件内进入 `src/Win32PluginSystem`。该目录可以直接
依赖 Windows SDK 和调用 Windows API；Linux/macOS 不配置、不编译其中源码，
也不为该兼容层维护平台桩实现。新增源文件只登记在目录内的 `CMakeLists.txt`。

首个重构步骤应把现有 `IPluginHost` 从 `MainWindow` 继承关系中移出，并用组合的
`PluginHostServices` 适配器替代。旧 `IPlugin` 在迁移期可作为实验接口保留，
但要明确标记为不稳定，最终通过新 ABI 的 Qt SDK 实现。

## 7. 当前实施阶段

### 已完成基线：插件包管理

Plugin Admin、清单/兼容模型、退出后安装/更新/卸载、SHA-256、ZIP 防护、事务
回滚、Windows UAC 和普通权限重启已经实现并测试。它不再列为未来阶段；详细状态
见 `codex/features/plugin-system.md`。

### 阶段 0：规划与状态归一（已完成）

- 旧阶段编号已按当前依赖重排。
- 插件包管理与插件加载运行保持独立。
- Windows 原版 ABI 有限兼容先行，跨平台版本化 C ABI 是长期主线。
- 不完整模拟 Notepad++ Win32 窗口树，不在 Linux/macOS 运行 Windows DLL。

### 阶段 1：x86 调查与代理评价（已完成）

- v8.4.6 x86 清单 169/169 项完成身份、版本、源码、许可证/维护、API、依赖、
  Win32/UI、线程/运行时、双代理风险和 A/B/C/D/U 初评。
- 当前 64 项有对应版本源码证据；105 项证据不足并严格保持 `U`，未反汇编。
- 统一矩阵：`codex/analysis/plugins-v846/api-dependency-matrix.md`。
- 自动校验保证 JSON、inventory、importance 和六个字母批次恰好覆盖同一 169 项。

阶段 1 当时不表示任何真实 DLL 已兼容；后续阶段现已完成首批真实 DLL 验证。

### 阶段 2：安全加载基础（已完成，预检延期）

- 已完成六导出校验、结构化加载日志、异常启动恢复、`-noPlugin` 和注册资源回滚。
- 日志使用 JSON Lines；加载 DLL 前原子写入当前插件标记，正常成功或失败时清除。
  若进程在加载期间退出，下次启动跳过该插件一次、显示恢复提示并清除标记。
- 已用无效/有效测试 DLL 连续加载验证失败注册不占用插件槽位和命令 ID；
  `-noPlugin` 回归同时覆盖命令行解析、宿主不创建加载器和不生成加载日志。
- PE 架构和依赖预检按当前决策暂不实现，后续单独推进；错误架构和缺依赖测试随之延期。

### 阶段 3：Windows 原版 ABI 与双代理骨架（已完成）

- 实现 `NppData`、`FuncItem`、命令 ID、菜单和快捷键。
- 为主/副视图建立生命周期稳定的代理 HWND，先实现已批准的同步 SCI 白名单。
- Win32 类型只存在于 `src/Win32PluginSystem/` 和必要的平台桥接代码。
- 已完成永久主/副 Scintilla 视图、三个代理 HWND、命令/快捷键、同步 SCI 白名单和
  原生 DockingManager 接入。

### 阶段 4：通知和 Host Services（进行中）

- 转换 `NPPN_*` 与 `SCNotification`，覆盖 Buffer、文件、语言、主题和关闭顺序。
- 把可跨平台能力收敛到组合式 `PluginHostServices`，不继续让 MainWindow 实现 ABI。
- 验证双视图、clone、高频通知、线程和回调重入。

### 阶段 5：首批真实插件（部分完成）

- A 类先用 `mimeTools` 验证最小命令插件。
- B 类用 `JsonTools`、`NPPJSONViewer`、`XMLTools` 验证文本、Dock 和运行时。
- 每个插件单独批准消息增量；失败不得留下 QAction、命令、回调或 Dock。
- mimeTools、Reverse Lines、Remove Duplicate Lines、SelectQuotedText、BracketsCheck、
  SecurePad、Code Alignment、JSON Viewer、JsonTools、Converter 和 NppPluginDemo 已通过
  官方 DLL 的有限兼容回归；XMLTools 尚未进入实现。

### 阶段 6：跨平台 ABI v1

- 基于真实语料冻结版本化 C ABI 的错误、所有权、线程和服务发现规则。
- Qt/C++ 只作为 SDK 包装，不跨 ABI 传递 Qt/STL 类型或异常。
- Windows、Linux、macOS 使用同一源码测试插件重新编译验证。

### 阶段 7：复杂插件源码移植

- `ComparePlus`、`DSpellCheck`、`Explorer` 走新 Host Services 和 Qt UI。
- `HexEditor` 使用自定义文档/视图协议，不兼容原 DLL。
- direct call、主窗口/Tab subclass 和任意脚本宿主不得反向扩大旧兼容层。

### 阶段 8：跨平台生态与发布

- 建立 Linux/macOS 原生清单和包，发布 SDK、示例、兼容清单和迁移说明。
- 完成架构包、签名、macOS quarantine/公证和目标桌面 UI 验证。

### 阶段 9：加固和可选隔离

- 建立加载性能、故障恢复和隐私审计。
- 评估未知插件预检、脚本插件或进程外协议，不改变 ABI v1。

## 8. 测试策略

### 8.1 自动测试

至少提供以下测试插件：

- `minimal`：一个命令和一个事件订阅。
- `lifecycle`：记录所有生命周期顺序。
- `editor`：读取/修改文档并监听 Scintilla。
- `dock`：创建和恢复面板。
- `failure-*`：缺入口、错误 ABI、初始化失败、回调异常、重复 ID。

测试分层：

- 纯逻辑：manifest、版本协商、资源分配、状态机、路径和命令映射。
- 进程级：真实动态库加载、失败回滚、退出卸载和安全启动。
- UI：菜单、快捷键、Dock、主题、双视图和持久化。
- 兼容：Windows 上运行选定的 v8.4.6 插件语料。

所有插件测试使用隔离配置目录和临时插件目录，不写入真实用户配置。

### 8.2 平台矩阵

最低矩阵：

| 平台 | 编译 | 新 ABI | UI/Dock | 原版 ABI |
| --- | --- | --- | --- | --- |
| Windows x64 | Debug + Release | 必测 | 必测 | 必测 |
| Ubuntu x64 | Debug + Release | 必测 | offscreen + 交互抽查 | 不适用 |
| macOS | Release，必要时 Debug | 必测 | CI + 真机抽查 | 不适用 |

没有 macOS 设备时，使用 CI 的 macOS runner 完成编译、单测和无头启动；发布前仍需
安排真机检查菜单、快捷键、Dock、动态库加载、签名和 Gatekeeper。CI 通过不能替代
最终的签名、公证和真实桌面交互验证。

### 8.3 每阶段验证命令

```bash
cmake -S . -B build -DBUILD_TESTING=ON -DENABLE_PLUGIN_SYSTEM=ON
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

插件功能默认关闭的构建也必须单独执行，确认未引入非插件行为变化：

```bash
cmake -S . -B build-no-plugins -DBUILD_TESTING=ON -DENABLE_PLUGIN_SYSTEM=OFF
cmake --build build-no-plugins -j 4
ctest --test-dir build-no-plugins --output-on-failure
```

## 9. 禁止事项

- 不把当前 Qt/C++ `IPlugin` 当作已稳定公开 ABI。
- 不在业务层散布平台后缀、`QLibrary`、Win32 类型或消息编号。
- 不以宏复制两套业务逻辑；平台差异放入适配器。
- 不为 Linux/macOS 实现伪 `HWND` 来宣称 Windows DLL 兼容。
- 不让插件直接拥有宿主菜单、主窗口、编辑视图或 Buffer 的生命周期。
- 不允许异常、STL 容器、Qt 对象或由另一模块释放的内存穿过稳定 C ABI。
- 不为单个插件无限扩大 Windows 兼容白名单；超过边界后转为源码移植评估。
- 不因插件缺失或版本变化删除 `shortcuts.xml`、Dock 布局或未知配置。
- 不使用 `QRegularExpression` 替代插件请求的 Scintilla 搜索语义。
- 不在加载失败后留下半注册命令、快捷键、事件订阅或面板。
- 不在缺少完整 Windows 回归测试时修改原版 ABI 路由。
- 不在未验证 SHA-256 和用户确认的情况下安装远程插件。

## 10. 当前实施主流程

阶段 0、1、阶段 2 的非延期项及阶段 3 已完成，阶段 5 已有首批真实插件语料。当前
主切片是阶段 4：补齐由真实插件证明需要的常用 `NPPN_*`、Buffer、文件、语言和主题
生命周期，再从这些语料收敛组合式 Host Services。PE 架构/依赖预检继续按既有决策
延期；不得借通知补齐为设想中的插件无限扩大消息白名单。

每一阶段先产生可审查的调查或验证记录，再修改兼容原则或实现代码。不得在未核验
插件真实 API 使用情况时，为设想中的插件扩大兼容层。

## 11. 方案讨论记录

本节记录已经确认的设计决策。后续讨论形成结论时，必须在实现前更新本节；如果
推翻已有结论，应保留原记录并标记“已替代”，同时链接新决策。

### 2026-07-30：移植总体策略

状态：**已确认**

- `PS-001`：插件采用三类分流：Windows 有限兼容、常用复杂插件源码移植、低价值
  长尾插件不提供官方移植。
- `PS-002`：Windows 兼容层只服务选定的简单常用插件，不完整模拟 Notepad++
  Win32 窗口结构。
- `PS-003`：Windows 原版 DLL 不在 Linux/macOS 直接运行；跨平台必须重新编译
  或移植。
- `PS-004`：新插件 API 是长期主线，旧兼容层与新 API 共用 Host Services。
- `PS-005`：依赖复杂 Dock、窗口子类化、内部工具栏或特殊视图的插件优先源码
  移植。
- `PS-006`：HexEditor 通过新 API 的自定义文档/视图能力实现，不强行兼容原 DLL。
- `PS-007`：公开插件状态必须区分直接兼容、部分兼容、已移植、社区维护和不支持。

### 2026-07-30：ABI v1 初始提案

状态：**讨论中**

- `PS-008`（提案）：跨平台长期二进制边界采用版本化 C ABI；Qt/C++ 只作为 SDK
  封装。其入口、服务发现和数据所有权将在首个议题中确认。

### 2026-07-30：插件系统顶层职责

状态：**已确认**

- `PS-009`：插件系统在产品功能层面先分为“插件管理”和“插件加载运行”两部分。
- `PS-010`（已由 `PS-018` 部分替代）：插件管理覆盖插件发现/下载、安装、更新、
  卸载和兼容性检查，而不只指在线下载；原先列入的单插件启用/禁用不进入首版。
- 插件加载运行负责扫描本地插件、加载与初始化、注册贡献、提供宿主能力、发送
  通知，以及关闭时清理和卸载。
- 插件描述、兼容元数据和 Host Services 是两部分共用的基础，不必单独列为第三个
  面向用户的功能模块。

### 2026-07-30：插件管理移植与跨平台目录

状态：**已确认**

- `PS-011`（已确认）：完整保留原版 Plugin Admin 的产品能力和主要工作流，把插件
  列表、兼容判断、安装、更新、卸载及安装状态实现为平台无关核心；原版 Win32
  对话框、`gup.exe`、管理员权限提升和 Authenticode 实现不机械照搬，分别使用
  Qt UI 与平台服务适配。
- `PS-012`（已确认）：Windows 继续识别原版目录
  `plugins/<plugin-name>/<plugin-name>.dll`；其他平台在同一个原版插件目录下
  进入对应的平台目录查找动态库，不增加 CPU 架构目录。
- 目录形式：

  ```text
  plugins/
    <plugin-id>/
      <plugin-name>.dll
      linux/
        <plugin-name>.so
      macos/
        <plugin-name>.dylib
      resources/
  ```

- 延续原版思想，由不同平台/架构的插件列表和下载包提供正确二进制，安装目录只
  保存当前目标的产物；加载器仍检查 PE、ELF 或 Mach-O 与当前进程是否匹配，
  防止手工安装错误架构。CPU 架构属于 artifact 元数据和加载校验，不进入目录名。
- 插件管理与插件加载必须共用同一个 artifact 解析器，不能分别实现文件查找规则。
- `PS-013`（已确认）：不兼容插件保留已安装文件、禁止加载并显示原因，提供更新
  或卸载选择，不自动删除用户已经安装的插件。
- `PS-014`（已确认）：插件管理尽量保留一套原版逻辑。只有少量语法或 API 差异
  时，在局部底层 helper 中使用编译宏；只有权限提升、签名验证、退出后替换等
  具有实质语义差异的能力才建立小型平台适配，不扩大成平行业务实现。
- `PS-015`（已确认）：沿用原版按目标平台和 CPU 架构分别发布插件列表及插件包
  的方式；单个下载包不携带其他平台或架构的动态库。
- `PS-016`（已确认）：沿用原版退出后更新流程。安装、更新和卸载先请求退出主
  程序，由外部更新流程执行解压或清理，完成后重新启动；首版不做运行中热替换。
- `PS-017`（已由 `PS-022` 替代）：原提案准备保留原版插件列表和更新程序签名；
  首版按后续决定只进行插件包 SHA-256 校验。
- `PS-018`（已确认）：首版不增加持久化的单插件启用/禁用功能，保持 v8.4.6
  行为。保留 `-noPlugin` 作为本次启动全局跳过全部插件的安全入口；停止加载单个
  插件通过卸载完成。
- `PS-019`（已确认）：插件安装根目录保持原版逻辑，为程序目录下的 `plugins/`。
  便携和普通模式不改变插件二进制根目录；普通模式的插件可写配置仍进入用户配置
  目录，便携模式进入程序目录。程序目录不可写时，由退出后更新流程处理所需权限。
- `PS-020`（已确认）：首版把各平台/架构对应的插件列表 JSON 编译在主程序中，
  不从 `nppPluginList.dll` 或外部 JSON 加载；以后再单独讨论动态更新和 DLL
  载体。现已直接归档 GitHub `nppPluginList/v1.5.4` tag 的 x86、x64、ARM64
  原始 JSON；Windows 构建按当前 CPU 架构选择对应清单。Linux/macOS 在有本平台
  原生包清单之前继续使用空清单，不能下载这些 Windows DLL 包。
- `PS-021`（已确认）：实现独立的跨平台更新程序，保持原版清理、解压、退出后
  操作和完成后重启流程。公共逻辑保持一份，只有必要的平台 API 使用局部宏或小型
  helper。
- `PS-022`（已确认）：首版只校验下载插件包的 SHA-256，不验证插件列表、更新
  程序或动态库签名。JSON 编译进主程序，包期望哈希来自该内置列表。路径合法性、
  解压目录穿越防护和文件写入检查属于基本正确性校验，不因本决定取消。

#### v8.4.6 原版管理行为核验

- **插件列表**：x86、x64、ARM64 发行包分别携带对应的
  `plugins/Config/nppPluginList.dll`。Release 版先验证该模块和 `gup.exe` 的
  Windows 签名，再从 DLL 资源读取 JSON；Debug 版可直接读取
  `nppPluginList.json`。
- **插件包**：列表中的每项包含 `folder-name`、`version`、`repository` 和
  `id`。安装/更新时把插件目录名、下载地址和 `id` 交给 `gup.exe`，由对应架构
  的插件列表选择相应包。`id` 在原版结构中定义为插件包 SHA-256。
- **安装、更新、卸载时机**：三种操作都要求先退出 Notepad++。安装向 `gup.exe`
  传递 `-unzipTo`，更新传递 `-unzipTo -clean`，卸载传递 `-clean`；操作完成后
  重新启动 Notepad++。v8.4.6 主程序代码没有实现运行中热替换。
- **启用/禁用**：Plugin Admin 没有持久化的单插件启用/禁用操作。原版只提供
  `-noPlugin`，用于本次启动全局跳过全部插件；单个插件只能通过移除而停止加载。
- **兼容失败**：仓库元数据判定不兼容的插件不加载并进入不兼容列表；另一方面，
  DLL 架构、导出或初始化检查失败时，原版会询问用户是否直接删除该 DLL。后者与
  已确认的 `PS-013` 不同，本项目按 `PS-013` 保留文件并禁用加载。
- **安全校验**：Release 版验证插件列表模块和 `gup.exe` 的 Windows 数字签名；
  插件条目的 `id` 是包 SHA-256，并随下载地址传给外部更新器。签名校验的具体
  平台实现不能直接移植到 Linux/macOS。

Windows 新 API 插件的识别方式及 manifest 属于插件加载/API 议题，不再作为插件
管理部分的未决项。

#### 插件管理移植实现状态

2026-08-01 已完成管理主线及事务更新收口：

- `PluginArtifactResolver` 统一插件管理与现有加载器的动态库定位规则：Windows
  使用原版路径，Linux/macOS 使用插件目录内的平台子目录，不增加架构目录。
- `PluginCatalog` 解析 v8.4.6 的清单字段、版本闭区间和旧版本兼容映射；单条坏
  记录按原版行为跳过。Windows 内置 v1.5.4 三架构清单。
- `PluginAdminModel` 形成 Available、Updates、Installed、Incompatible 四类
  数据；Windows 可回退读取 DLL 文件版本，跨平台管理器写入安装收据保存版本。
- Qt `PluginAdminDialog` 保留原版四页、搜索、描述、批量勾选和操作前退出确认
  工作流。
- 独立 `npp-plugin-updater` 在主程序退出后下载、校验 SHA-256、检查 ZIP 路径和
  平台二进制；全部包预检成功后事务替换目录，失败时回滚，成功后写收据并重启。
- 更新计划拒绝无效/重复插件目录、非 HTTP(S)/本地文件地址和非法 SHA-256。

Windows 构建现在可以按架构生成 Available 和 Updates；Linux/macOS 因没有本地
原生插件包清单，仍保持这两页为空。Installed 和卸载按本地动态库发现结果工作。
Windows PowerShell 真实 ZIP 路径已有自动化覆盖；不可写程序目录会通过 UAC
启动更新器，并用桌面 shell 普通 token 重启。真实公网下载、UAC 确认框和完整
UI 手工操作仍按发布验证矩阵执行，不再作为管理代码未实现项。

- `PS-029`（已更新）：采用 GitHub `nppPluginList/v1.5.4` tag 的三架构 JSON，
  来源 commit、条目数和哈希见清单调查文档。后续仍需与 v8.4.6 官方发行包中的
  `nppPluginList.dll` 资源对比，发现差异时以发行包为准。
- `PS-030`（已确认）：逐插件调查当前只覆盖 x86 清单 `pl.x86.json` 中的 169
  个插件。x64 和 ARM64 JSON 继续作为原始资料保留，但不进入本轮统计、源码/API
  调查或兼容评级；代理窗口实测也以 Windows x86 为当前目标。
- `PS-031`（阶段 1 已完成）：x86 清单 169 项均已形成静态首版记录，统计为
  A 13、B 23、C 25、D 1、U 107。逐项证据和待验证项见
  `codex/analysis/plugins-v846/`。`U` 表示没有足够的对应版本源码证据，不等于
  不兼容；本轮遵守约定未反汇编。统一 API、依赖和双代理风险矩阵已经形成，
  但评级仍须在 Windows x86 上通过六导出、消息集合、双视图、通知顺序和故障
  边界实测复核。
- `PS-032`（重要度首评）：兼容难度和插件重要度必须分别评价。x86 169 项的
  重要度首评为高 20、中 94、低 55，并为每项记录置信度、社区证据或功能判断。
  当前没有统一可靠的安装量数据，不能用 GitHub star、是否仍在官方清单或单个
  帖子直接代替用户数量。详细矩阵见
  `codex/analysis/plugins-v846/importance.md`。
- 处理优先级首先关注“高重要度 × B/C”：B 类用于验证有限旧兼容层，C 类用于
  提炼新 API 和源码移植需求；高重要度 U 类优先补充源码、社区使用量和 Windows
  行为证据。低重要度插件默认不得反向推动宿主模拟完整 Win32 内部结构。

### 2026-07-30：插件加载总体方向与故障边界

状态：**部分确认**

- `PS-023`（已确认）：Windows 加载器尽量兼容原版插件；难以兼容但重要的插件
  使用本项目的新 API 重写；难以兼容且价值较低的插件不提供官方兼容。
- `PS-024`（讨论中）：插件加载器必须以避免或尽量减少插件导致的主程序崩溃为
  目标，并为失败提供明确诊断和下次启动恢复能力。但原版插件是主进程内本机代码，
  对任意不兼容、错误或恶意 DLL 无法承诺绝对故障隔离。
- 需要分别处理以下故障阶段：
  - 加载前：平台、架构、版本、已知兼容状态、文件完整性和依赖检查。
  - 动态库加载：`DllMain`/加载器异常、缺少依赖、错误导出和初始化失败。
  - 正常回调：命令、通知、消息和关闭回调中的 C++ 异常、Windows SEH、重入和
    非法返回数据。
  - 不可完全拦截：堆或栈破坏、死锁、无限循环、异步线程崩溃、主动结束进程。
- 待讨论的保护级别：
  - 是否只自动加载兼容列表中经过验证的插件。
  - 未知或手工安装插件是否先在辅助进程中预检。
  - 启动时记录正在加载/回调的插件，异常退出后是否自动隔离嫌疑插件。
  - Windows 回调边界是否同时使用 C++ 异常保护和 SEH。
  - 哪些情况需要进程外插件宿主；进程外运行不能自然兼容依赖 `HWND` 指针参数、
    高频 `SCI_*` 或复杂 Dock 的原版 DLL。

### 2026-07-30：Windows 原版插件兼容工作流程

状态：**已确认**

- `PS-025`：按以下顺序推进插件调查、兼容评价和实现：

  1. 调查插件列表 JSON 中的全部插件。对于不在 JSON 中的插件，除非应用特别
     广泛，否则不纳入正式统计。
  2. 分析每个插件实际调用的 Notepad++、Scintilla 和 Win32 API，以及源码、
     UI、运行时、依赖和平台移植难点。
  3. 以 Windows 上创建两个假的原生 Scintilla 窗口、使用现有主窗口 HWND，并由
     本项目截获和处理相关窗口消息为基本兼容模型，重新判断每个插件的兼容难度。
  4. 根据代理窗口分析结果修改插件兼容原则，再用修改后的原则重新评价所有候选
     插件。
  5. 实现能够兼容的插件所需宿主能力，并逐项验证目标插件。
  6. 新 API、难兼容但重要插件的源码移植、不兼容插件稳定性专项测试等后续工作，
     在上述结果形成后再决定。

  当前进度：步骤 1-4 的静态调查与原则修订已完成；步骤 5 尚未开始。安全加载
  基础作为 `PS-026` 规定的前置门槛，对应当前阶段 2。

#### 调查产物

- 插件清单：插件 ID、名称、版本、来源、架构、维护状态、源码和使用范围依据。
- 调查基线固定为 Notepad++ v8.4.6 对应的 x86 插件列表，只使用其中的插件身份、
  版本和包信息；实际代理窗口验证优先从 Windows x86 开始。
- API 使用矩阵：六个原版导出、`NPPM_*`、`NPPN_*`、`SCI_*`、Win32 API、
  Dock、窗口层级、子类化、后台线程、子进程和第三方运行时。
- 难点记录：无源码、编译依赖、原生 UI、指针消息、直接函数、特殊视图、lexer、
  安装辅助程序及平台专属能力。
- 无源码插件明确标注，不进行反汇编。无源码且重要的插件特别标注，后续可以根据
  用户可观察的功能和交互，通过本项目新 API 或内置能力进行行为复刻。
- 兼容评价：直接兼容、增加有限消息后兼容、需要新 API/源码移植、不支持，以及
  对应证据和未验证项。

#### 代理窗口评价原则

- `NppData._nppHandle` 使用 Qt 主窗口已有的 Windows HWND，`NPPM_*` 通过主窗口
  原生事件入口转发到宿主实现。
- `_scintillaMainHandle` 和 `_scintillaSecondHandle` 使用两个生命周期稳定的原生
  代理 HWND，分别映射主、副 `ScintillaEditView`。
- 代理窗口按白名单同步转发 `SCI_*`，保持同进程指针和返回值语义；不能只按消息号
  声称兼容，必须核验缓冲区、结构体、编码和重入行为。
- 依赖真实 Scintilla 窗口类、可视区域、父子窗口层级、窗口子类化、
  `SCI_GETDIRECTFUNCTION`/`SCI_GETDIRECTPOINTER` 或内部控件遍历的插件单独标为
  高风险，不能假设两个代理 HWND 足以兼容。
- Qt/Scintilla 事件需要转换为原版 `SCNotification`/`NPPN_*`，通知内容、顺序和
  生命周期也属于兼容评价范围。

`PS-026`（已确认）：在步骤 5 开始加载真实插件前，至少完成架构/导出检查、调用
边界异常保护、当前加载插件日志和 `-noPlugin` 恢复入口；完整的不兼容插件压力
测试仍可保留在步骤 6。这样不改变既定工作顺序，只建立调查和兼容开发所需的最低
安全基线。

- `PS-027`（已确认）：插件调查以 Notepad++ v8.4.6 对应的插件列表为基线，不以
  当前最新插件生态替换历史基线。
- `PS-028`（已确认）：无源码插件只标注源码缺失和重要程度，不进行反汇编；重要
  插件可在后续依据应用的外部可观察行为进行功能复刻。

## 12. 待讨论议题

插件调查、API 矩阵、代理窗口评价和兼容原则修订已经完成。进入阶段 2 前后需要按
实现依赖依次确认以下议题：

1. 安全加载的自动加载范围、辅助进程预检、异常恢复和 SEH 保护级别。
2. Windows 原版兼容接口的首批白名单。
3. 新 API 的公开形态和最小 Host Services。
4. 难兼容但重要插件的首批源码移植名单。
5. Qt 面板 SDK 和稳定 ABI 的边界。
6. 自定义文档/视图协议。

每个议题应记录：候选方案、最终选择、选择理由、明确不做的内容、兼容影响、验证
方法和仍未解决的问题。
