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

## 加载故障恢复

- 加载器在用户配置目录的 `plugin-load/` 中写入 JSON Lines 结构化日志。
- 调用 `LoadLibraryW` 前先原子写入 `plugin-load-in-progress.json`；普通成功或失败均清除。
- 标记在异常退出后保留。下一次启动会跳过对应插件一次、记录 `recovery-skip` 并提示用户。
- 插件导出或命令表注册失败时，释放模块、恢复命令 ID 检查点且不加入已加载集合。
- `-noPlugin` 启动不创建本系统，因此不会创建代理 HWND 或加载状态文件。
- PE 架构与依赖预检当前明确延期，不属于本轮实现。

## 依赖边界

- 本目录可以依赖 `MISC/PluginsManager` 提供的平台无关模型和宿主服务。
- 主窗口只通过稳定的宿主 facade 与本目录交互。
- Win32 头文件和实现必须留在本目录，并使用 Windows 构建条件隔离。
- 顶层 CMake 仅在 `WIN32` 为真时进入本目录；Linux 和 macOS 不配置、不编译
  本目录中的任何源码。
- 新源码必须显式加入本目录 `CMakeLists.txt` 的
  `WIN32_PLUGIN_SYSTEM_SOURCES`，不得加入顶层通用 `SOURCES`。
- 在 ABI 行为和生命周期未验证前，不把实验接口声明为稳定接口。

### Scintilla 结构 ABI

- 发给代理编辑器 HWND 的普通消息保持插件使用的当前 Win32 指针宽度结构。
- `SCI_GETDIRECTFUNCTION` 返回的宿主 thunk 转换旧 direct 插件使用的
  `Sci_PositionCR=long` 结构。
- 不得把旧结构转换移动到公共 HWND 路径：XMLTools 3.1.1.13 在该路径使用
  `intptr_t`，DoxyIt 0.4.4 和 ElasticTabstops 1.3.1 则需要旧 direct 转换。

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

## mimeTools 2.8 兼容验证

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
收据、目录发现、ABI 初始化、`MIME Tools` 名称、命令表、菜单和关闭生命周期。当前
白名单覆盖 mimeTools v2.8 实际使用的九个消息：
`SCI_GETSELECTIONSTART/END`、`SCI_GETSELTEXT`、`SCI_TARGETFROMSELECTION`、
`SCI_GETTARGETTEXT`、`SCI_SETTARGETSTART/END`、`SCI_REPLACETARGET` 和
`SCI_SETSEL`。

`MainWindow` 将 DLL 返回的 `FuncItem` 表加入 Plugins 菜单，保留 4 个分隔符和全部
14 个可操作项。真实 DLL 直接执行 12 个文本转换：Base64 的 7 项、quoted-printable
的 2 项和 URL 的 3 项。回归测试覆盖每项的成功转换、空选区不修改、主/副永久视图、
目标替换、选择区和插件分配的字节缓冲区。

mimeTools 的 URL 源码在计算 `SCI_GETSELTEXT` 输出缓冲区时假定长度包含 NUL；Qt
Scintilla 的返回值是精确文本长度。仅在该 DLL 的 3 个 URL 命令进行长度查询时，adapter
额外返回 NUL 长度，避免插件自身少分配 2 字节。复制文本仍返回真实长度。

官方 x64 DLL 的 SAML Decode 使用无输入边界的 `tinf_uncompress()`，About 使用内嵌
Win32 modeless dialog；两条路径在 Qt 宿主中均可复现进程崩溃。兼容层只替代这两个
命令：SAML 使用 zlib 的 raw-DEFLATE 解码，并保持原插件的 URL/Base64/inflate 错误文案、
200000 字节上限、选区与替换语义；About 使用非模态 Qt 对话框显示原资源中的作者、版本
和 GPL 信息。其余命令不由宿主重实现。

当前白名单只接纳已经完成 API 审计和真实 DLL 回归的同步消息型插件。扩展通知、Hook、
Dock 和运行时边界后，应逐项扩大兼容集合；不能仅因插件已经安装就自动加载。

## Simple synchronous plugin corpus

The audited load set now also includes Reverse Lines, Remove Duplicate Lines,
SelectQuotedText, BracketsCheck, SecurePad, and Code Alignment. These plugins
remain on the same compatibility boundary as mimeTools: the DLL receives the
three stable HWND values and its synchronous `NPPM_*`/`SCI_*` calls are handled
by the adapters. No plugin editing algorithm is reimplemented in Qt.

Poor Man's T-SQL Formatter is installed but skipped because its .NET 2.0 image
requires a process-wide CLR activation decision. BetterMultiSelection 1.5 is
admitted after the editor receivers gained `SCI_GETDIRECTFUNCTION` and
`SCI_GETDIRECTPOINTER` pass-through to the permanent Scintilla Qt instances.
Its READY initialization, Hook enable/disable, main-editor direct calls, clean
unload, and coexistence with XMLTools are covered; physical keyboard behavior
remains a manual Windows check. BracketsCheck's direct
`GetMenu`/`CheckMenuItem` state synchronization is also deferred; attaching an
HMENU to the Qt main window is not an acceptable local workaround.

## Configurable plugin enablement

Plugin loading is controlled by `pluginsEnabled.xml` in the active application
configuration directory. The stable key is the plugin folder name. Only an
entry with `enabled="yes"` is passed to either the Win32 compatibility manager
or the cross-platform plugin manager; a missing file, missing entry, disabled
entry, malformed XML, or invalid attribute leaves the plugin unloaded.

The Plugins Admin Installed page exposes a separate Enabled checkbox between
the plugin name and version. Changes are atomically persisted immediately and
apply on the next application launch. This user-controlled list replaces the
hard-coded folder list, but enabling a plugin is not a compatibility guarantee:
an incompatible in-process DLL may still terminate the host.

## Dock adapter and JSON Viewer 1.41

`Win32PluginDockAdapter` and `Win32NativeDockHost` implement the v8.4.6 Dock
ABI without moving Win32 types into the generic UI layer. The adapter maps
`tTbData`, DMM messages, DMN notifications, and modeless dialogs. The host is
the sole owner of the plugin client HWND and restores its original parent and
styles before plugin DLL unload.

The official JSON Viewer 1.41 x64 DLL is installed through the updater fixture.
Runtime coverage executes Show JSON Viewer, Format JSON, and Compress JSON;
verifies JSON language selection, Dock show/hide/float/redock, direct HWND
parenting, exact host sizing, and shutdown cleanup. Qt 5 creates four new
native Qt widgets for this Dock: host, QDockWidget, close button, and float
button. No editor, splitter, central widget, or higher container is promoted.

## JsonTools 3.2.0

The official x64 CLR4/WinForms DLL is installed from the pinned corpus and
loaded unchanged. Its narrow additional surface is handled here: current-path,
filename, file-new, file-open, append-text, goto-line, goto-position, toolbar
modification, and file-before-close notifications.

JsonTools intentionally registers its tree with function index `4` as the Dock
ID. The host-assigned command ID is copied into the managed function table by
`NPPN_TBMODIFICATION` and is used separately for menu check state. Do not merge
these identifiers in the adapter. Registering a Dock also makes it current so a
new WinForms panel does not remain behind an existing tabified Dock.
## JsonTools deep matrix and simple corpus additions

The real JsonTools 3.2.0 DLL is covered for Settings, RemesPath query and
assignment, JSON Lines, YAML output, tree-to-source navigation, and its exact
4 MB partial/full tree behavior. The upstream `Run tests` command is not
portable because `JsonGrepperTests.cs` calls `GetFiles()` on the author's
absolute test directory without guarding a missing directory. The host does
not synthesize that path or replace the plugin command.

The audited Windows corpus also includes the official nppConverter 4.4.0 and
NppPluginDemo 4.2 x64 packages. They use the existing package installer,
six-export loader, message receivers, and DockingManager. Their source-proven
editor additions are limited to `SCI_ADDTEXT` and `SCI_ENSUREVISIBLE`.

## EditorConfig 0.4.0 and AutoSave 1.6.1.0

The official x64 DLLs are pinned and installed unchanged by the corpus fixture.
EditorConfig uses the standard buffer notifications and Scintilla tab, indent,
EOL, fold, and text-range messages. AutoSave installs its own window procedure
on the real main-window HWND and uses `NPPM_SAVECURRENTFILEAS` for timestamped
and recovery copies. The latter preserves the original `asCopy` behavior so a
copy does not rename or clean the active buffer.

Do not relay every Qt native window message to all plugin `messageProc`
callbacks. Qt emits a different destruction stream, and the audited plugin
corpus can block during shutdown under that policy. These two plugins do not
require it: EditorConfig uses notifications and AutoSave owns a WndProc hook.

`win32-configuration-plugins` covers both real DLLs together. Focus-loss and
minute-timer behavior additionally require an interactive Windows desktop;
set `NPP_QT_TEST_REAL_FOCUS=1` only in that environment.

## Session and document policy plugins

The unchanged SessionMgr 1.4.4 DLL uses the host's existing session XML
implementation through `NPPM_SAVECURRENTSESSION` and `NPPM_LOADSESSION`.
Buffer position and application-directory queries are also implemented with
the v8.4.6 return-value and UTF-16 buffer conventions.

AutoCodepage 1.2.4 and AutoEolFormat 1.0.2 retain their independent INI files
and request original menu command IDs. File-open notifications must finish
with `NPPN_FILEOPENED` before `NPPN_BUFFERACTIVATED`, matching the original
host's event order.

nppAutoDetectIndent 2.3 uses the Scintilla 5 direct API and native
`Sci_TextRange`. The editor adapter selects that ABI only while invoking this
plugin; older audited DLLs continue to receive the legacy Scintilla 4 range
translation.
