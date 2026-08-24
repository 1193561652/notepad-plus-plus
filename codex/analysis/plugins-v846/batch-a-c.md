# Notepad++ v8.4.6 插件调查：x86 A–C 批次

## 1. 范围和口径

本文件是 **x86-only** 首轮调查，只使用
`third_party/nppPluginList/catalog/windows/pl.x86.json`，不比较、不推断 x64 或 ARM64。
范围按 `folder-name` 不区分大小写排序，包括数字和下划线开头的条目，截止
`CustomLineNumbers`，共 24 项。

调查遵守以下限制：

- 版本、SHA-256、作者、主页和功能描述来自仓库内的 x86 JSON。
- 有公开源码时，只把能对应到 JSON 版本的 tag/commit 作为定级证据。
- 源码仓库只有更新版本、仓库已失效或只发布二进制时，不把其他版本的调用当作
  基线事实；不下载后二进制反汇编，等级保持 `U`。
- 下列 API 是首轮源码扫描确认的业务调用集合或高风险调用，不是 SDK 头文件中
  “出现过”的常量全集。后续进入兼容实现前仍须逐调用复核。
- 所有等级均为“主窗口 HWND＋双 Scintilla 代理 HWND”原则下的初评，尚未在
  Windows x86 上加载验证。

## 2. x86 JSON 基线

| `folder-name` | 显示名称 | x86 版本 | 包 SHA-256 |
| --- | --- | --- | --- |
| `3P` | 3P - Progress Programmers Pal | `1.8.7` | `9c566a9083ef66a0ce93a3ce5f55977faea559b5b0993e37a1461b87f4aeb6f0` |
| `_CustomizeToolbar` | Customize Toolbar | `5.3` | `979ed6f192b05a4c2c07f2397a03f7954f5e0e01d92494b6de5540f2e312f710` |
| `ActiveX` | ActiveX Plugin | `1.1.8.7` | `8a6b9dadbf2ec37d5c60a12a5445f0eec2ef00e6eaa80452925789fd73950193` |
| `AnalysePlugin` | AnalysePlugin | `1.13.49.0` | `a5ffe695a7ffc49983ef55a4d5705c522acb12ab7491476639b292ed411b1162` |
| `AutoCodepage` | AutoCodepage | `1.2.4` | `b41c1090fb917f0ebe09ed14073ed8f233e063551bbe7c77a8e5801c8b8f79fd` |
| `AutoEolFormat` | AutoEolFormat | `1.0.2` | `fb11ea9b5af3385cc8e9a5b0ff0762261f48c80a592cd4421d4c32e42ec141b7` |
| `AutoSave` | AutoSave | `1.6.1.0` | `505e4c5ae157db658eb6da2dd7e89d87856cc211ec7f9655926803d205bf471f` |
| `BetterMultiSelection` | BetterMultiSelection | `1.5` | `cfd2b102fce7fbe734439c179e10b63cf883552560db9e8634093b6bc7b33594` |
| `BigFiles` | BigFiles - Open Very Large Files | `0.1.3` | `27E2761DA531C90BA855971F939DF34BFA211B18A863E3B5D907A48615D531D7` |
| `BookmarksDook` | Bookmarks@Dook | `2.3.3.0` | `ccc215e90322c25b2eede9a730fd36450e9b14eb4d67bd80e4608e3cad301bcb` |
| `BracketsCheck` | BracketsCheck | `1.2.2` | `5055cca557d668222478445ec99be580e63c62b774eb6efaa3753a0c171d145c` |
| `CADdyTools` | CADdyTools | `1.1.3.7` | `7ccbfb712e3b6757137f4921de6e6f4f83f9c258248137690c0dd5de9262c5dd` |
| `ccc` | PHP Autocompletion | `1.4.1` | `86d29d7a7998eec33cff35af54071ecf767c014048755cfbc0e54c1fda9d73d8` |
| `CodeAlignmentNpp` | Code Alignment | `14.1.107` | `F731F5B018A2C56FDBEFE0B13641FF4B8A94838260AED2078FA1333356B8024A` |
| `CodeStats` | Code::Stats | `1.0.1` | `e82def2cb807eddba5591281bc2f980bb6f99c50497380738ad5a9131d1a8246` |
| `ColorPicker` | Don Rowlett Color Picker | `2.3` | `db861fd46649c92b2d0b048c3e298a9cba76e610dc5ec8bdf90bcada598744f3` |
| `ColumnTools` | ColumnTools | `1.4.4.1` | `0c356bc202147f8de806fa4fd676a122eb799d8d68f8c9ecfe978c6c5d91c8ab` |
| `Comment Wrap` | Comment Wrap | `1.0.0.5` | `8b557136493cc8339d8c16b03be561141d81af3c1ab4feb0f7fade4e2d17e555` |
| `ComparePlugin` | Compare | `2.0.2` | `ea2f4cd6627c1b902f700a43b03b38f725e67136c8ce00ac3620ecc03417332a` |
| `ComparePlus` | ComparePlus | `1.0.0` | `d8640fca448c6f739837a8b9245172884290dd73812782b988b4ab013329e290` |
| `CSScriptNpp` | CS-Script - C# Intellisense | `2.0.2.0` | `57a98787f2b61666ae371f514a4825adfa308a24767b329d13374c4208ec3530` |
| `CSVLint` | CSV Lint | `0.4.5.4` | `a76ce970893922e19df1a0b0d3fce04127178999efd23fdaf26d4892fba3aa3f` |
| `CsvQuery` | CsvQuery | `1.2.9` | `0defb9c0bb3c2d6628a248b10f7cc0844a363ecee19ad224bf09d1797d4f906e` |
| `CustomLineNumbers` | CustomLineNumbers | `1.1.7` | `00ef75241132b0dc8b051de039915752689eb925a5c9606413806357403f2e6d` |

## 3. 有对应版本源码的插件

### 3.1 `3P` 1.8.7

- **源码/许可证/类别**：有源码，官方 tag `v1.8.7`，GPL-3.0；.NET 4.6.1
  OpenEdge ABL IDE、自动完成、代码/文件浏览、编译与 PROLINT 集成。
- **已确认 NPP API**：`NPPM_GETCURRENTSCINTILLA`、
  `NPPM_GETCURRENTBUFFERID`、`NPPM_GETFULLCURRENTPATH`、
  `NPPM_GETOPENFILENAMES*`、`NPPM_GET/LOAD/SAVECURRENTSESSION`、
  `NPPM_DOOPEN`、`NPPM_SWITCHTOFILE`、`NPPM_MENUCOMMAND`、
  `NPPM_GETPLUGINSCONFIGDIR`、`NPPM_DMMREGASDCKDLG`、
  `NPPM_DMMHIDE/DMMSHOW/DMMUPDATEDISPINFO`、`NPPM_MODELESSDIALOG`、
  `NPPM_ADDTOOLBARICON`、`NPPM_SETMENUITEMCHECK`。
- **已确认通知/SCI**：处理 `NPPN_READY`、`NPPN_SHUTDOWN`、
  `NPPN_BUFFERACTIVATED`、文件打开/保存/关闭前后、语言和样式变化，以及
  `SCN_CHARADDED`、`SCN_MODIFIED`、`SCN_UPDATEUI`、Dwell、Margin 等。
  文本、选区、自动完成、CallTip、Marker、Indicator、Margin、折叠和样式消息
  范围很广；明确取得 `SCI_GETDIRECTFUNCTION` 和
  `SCI_GETDIRECTPOINTER`。
- **Win32/Dock/运行时**：WinForms/Yamui 多 Dock 面板及弹窗；主窗口
  `SetWindowLong` subclass；线程级 `SetWindowsHookEx`；窗口枚举；
  外部 OpenEdge 编译/运行进程和文件系统操作。
- **双 HWND 难点**：代理 HWND 无法返回真实 Scintilla direct function/pointer；
  主窗口 subclass、Hook 和 WinForms Dock 生命周期也超出消息转发。
- **初评**：`C`；建议保留业务逻辑，以新 API 重写宿主、编辑器直连和 UI。
- **证据**：[tag v1.8.7](https://github.com/jcaillon/3P/tree/v1.8.7) 中
  `3PA/NppCore/SciApi.cs`、`Npp.cs`、`NotificationsPublisher.cs`、
  `WindowsCore/WinHook.cs`、`CoreWindowsHook.cs`、
  `MainFeatures/CodeExplorer`、`FileExplorer`、`Pro/ProExecution.cs`。
- **未验证**：六导出、发布包依赖、所有外部工具关闭行为和 x86 实机加载。

### 3.2 `BetterMultiSelection` 1.5

- **源码/许可证/类别**：有源码，tag `v1.5`，GPL-2.0；多选区光标移动。
- **已确认 API**：`NPPM_GETCURRENTSCINTILLA`、
  `NPPM_GETPLUGINSCONFIGDIR`、`NPPM_SETMENUITEMCHECK`；处理
  `NPPN_READY/SHUTDOWN/BUFFERACTIVATED` 和
  `SCN_CHARADDED/FOCUSIN/FOCUSOUT`。向当前视图同步发送
  `SCI_CHARLEFT/RIGHT`、`SCI_WORDLEFT/RIGHT`、行移动、换行、删除及其 Extend
  变体。
- **Win32/Dock**：不使用 Dock 或窗口 subclass，但在 `NPPN_READY` 中通过
  `SetWindowsHookEx(WH_KEYBOARD, ..., GetCurrentThreadId())` 安装 UI 线程键盘 Hook。
  `setInfo()` 还会向主编辑器请求 `SCI_GETDIRECTFUNCTION` 和
  `SCI_GETDIRECTPOINTER`，后续所有编辑操作均通过 Scintilla direct-function ABI
  执行，而不是逐条 `SendMessage(SCI_*)`。
- **当前 Qt 故障根因**：Win32 编辑器适配层传给插件的是隐藏消息接收 HWND；它没有
  处理上述两个 direct 接口，因此插件保存空函数指针，并在 `NPPN_READY` 调用
  `SCI_AUTOCSETMULTI` 时解引用。Scintilla 5 Qt 本体已经实现两个 direct 接口，兼容
  层可从真实编辑器透传函数和实例指针。
- **双 HWND/输入难点**：还需验证主副视图切换、`SCN_FOCUSIN/OUT`、
  `NPPN_BUFFERACTIVATED`，以及线程键盘 Hook 与其他插件共存时的链式调用和卸载顺序。
- **修订初评**：原版 DLL 可兼容，最小入口是透传 Scintilla direct ABI；完成后仍须
  对多选移动、连续输入、双视图和与 XMLTools 同时加载进行独立进程验证。
- **证据**：[tag v1.5](https://github.com/dail8859/BetterMultiSelection/tree/v1.5)
  的 `src/Main.cpp`。
- **未验证**：通知顺序、连续多选输入、双视图切换和六导出。

### 3.3 `BigFiles` 0.1.3

- **源码/许可证/类别**：有源码，tag `v0.1.3.x86`，GPL-2.0；以分页缓冲预览
  超大文件。
- **已确认 API**：`NPPM_MENUCOMMAND(IDM_FILE_NEW)`、
  `NPPM_GETCURRENTBUFFERID/GETCURRENTSCINTILLA`、
  `NPPM_GETPLUGINSCONFIGDIR`、`NPPM_DOOPEN`、`NPPM_SETSTATUSBAR`、
  `NPPM_ADDTOOLBARICON`；处理 `NPPN_BUFFERACTIVATED`、文件打开/关闭/保存、
  `NPPN_TBMODIFICATION/SHUTDOWN`；使用 `SCI_CLEARALL`、
  `SCI_APPENDTEXT`、`SCI_SETTEXT`、`SCI_GETLINECOUNT`、`SCI_GOTOLINE`。
- **Win32/Dock**：Win32 文件选择和文件映射/读取逻辑；无 Dock/direct function。
- **双 HWND 难点**：`FileTracker` 保存当时的 Scintilla HWND，并把 N++ 临时
  buffer 与外部大文件页绑定；文档被移动到另一视图时，保存的“视图 HWND”可能
  不再代表该文档。
- **初评**：`B`；需增加并验证 buffer→当前视图的稳定映射和状态栏语义。
- **证据**：[tag v0.1.3.x86](https://github.com/superolmo/BigFiles/tree/v0.1.3.x86)
  的 `src/FileTracker.cpp`、`v3.cpp`、`PluginDefinition.cpp`、
  `Configuration.cpp`。
- **未验证**：大于 2/4 GiB 文件、移动到另一视图、文件映射释放和异常关闭。

### 3.4 `BracketsCheck` 1.2.2

- **源码/许可证/类别**：有源码，tag `v1.2.2`，Unlicense/公有领域声明；文本
  或选区括号平衡检查。
- **已确认 API**：当前 Scintilla 视图的 `SCI_GETLENGTH`、`SCI_GETTEXT`、
  `SCI_GETSELTEXT`、`SCI_GETSELECTIONSTART`，通过插件分配的缓冲区取回文本。
- **Win32/Dock**：C#/.NET 导出层和 WinForms 消息框；未发现 Dock、窗口结构
  遍历、subclass、Hook 或 direct function 的业务调用。
- **双 HWND 难点**：输出缓冲区写回和 ANSI/UTF-8 文本长度必须完全模拟
  Scintilla；其余仅需正确选择当前代理。
- **初评**：`A`。
- **证据**：[tag v1.2.2](https://github.com/niccord/BracketsCheck/tree/v1.2.2)
  的 `BracketsCheck/Main.cs`。
- **未验证**：多字节字符长度、超大选区、六导出和 .NET 运行时异常边界。

### 3.5 `CADdyTools` 1.1.3.7

- **源码/许可证/类别**：有源码，tag `1.1.3.7`，GPL-2.0；CADdy 坐标和测量
  文本转换，含多个工具面板。
- **已确认 NPP/SCI**：文件枚举、打开/切换/保存/另存、路径、语言、配置目录、
  菜单命令、Toolbar，以及 `NPPM_DMMREGASDCKDLG/DMMSHOW`。文本操作经
  ScintillaGateway；还使用未公开的 `NPPM_INTERNAL_CLEARINDICATOR`。
- **Win32/Dock**：多个 WinForms Dock 子窗口；直接取得 `HMENU`，递归枚举
  插件子菜单并调用 `GetMenuItemCount/GetMenuItemID/EnableMenuItem` 动态启用
  命令。
- **双 HWND 难点**：Qt 主窗口 HWND 不提供原版 Win32 `HMENU`/子菜单结构，
  不能只靠截获 `NPPM_*` 满足该假设；多个原生 Dock 还需完整生命周期验证。
- **初评**：`C`；技术上不宜扩张为原版菜单树模拟，建议源码移植；是否列为官方
  移植对象仍需重要性数据。
- **证据**：[tag 1.1.3.7](https://github.com/MarioRosi/CADdyTools/tree/1.1.3.7)
  的 `CADdyTools/Main.cs`、`Tools/ClassNPPTools.cs`、
  `PluginInfrastructure/NotepadPPGateway.cs`。
- **未验证**：所有转换命令涉及的 SCI 集合、Dock 关闭通知及第三方组件。

### 3.6 `CodeAlignmentNpp` 14.1.107

- **源码/许可证/类别**：有源码，tag `v14.1`；许可证文件仅写明 “AS IS”，
  再分发权限需法务复核；按字符对齐文本。
- **已确认 API**：`NPPM_GETPLUGINSCONFIGDIR`、
  `NPPM_GETCURRENTLANGTYPE`、`NPPM_GETEXTPART`；处理
  `NPPN_TBMODIFICATION/SHUTDOWN`。SCI 使用
  `GETLINECOUNT/GETSELECTIONNSTART/END/GETCURRENTPOS/GETCOLUMN`、
  `LINEFROMPOSITION/POSITIONFROMLINE/GETLINEENDPOSITION/GETLINE`、
  `GETUSETABS/GETTABWIDTH`、`BEGINUNDOACTION/INSERTTEXT/ENDUNDOACTION`。
- **Win32/Dock**：C# 导出层和同步 `SendMessage`；未发现 Dock、窗口遍历、
  subclass、Hook 或 direct function 的业务调用。
- **双 HWND 难点**：`SCI_GETLINE` 输出缓冲区和多选区索引语义；当前视图映射。
- **初评**：`A`。
- **证据**：[tag v14.1](https://github.com/cpmcgrath/codealignment/tree/v14.1)
  的 `CodeAlignment.Npp/Implementations/{Document,Line,Edit}.cs`、
  `Main.cs`、`UnmanagedExports.cs`。
- **未验证**：JSON 文件版本 `14.1.107` 与 tag 内程序集文件版本逐字匹配、
  非 ASCII 对齐、矩形/多选区和许可证。

### 3.7 `CodeStats` 1.0.1

- **源码/许可证/类别**：有源码，tag `v1.0.1`，BSD-3-Clause；编码行为统计和
  Code::Stats 网络上报。
- **已确认 API**：`NPPM_GETFULLCURRENTPATH`、
  `NPPM_GETCURRENTLANGTYPE`、`NPPM_GETLANGUAGENAME/DESC`、
  `NPPM_SETSTATUSBAR`、`NPPM_ADDTOOLBARICON`；处理 `NPPN_READY`、
  `NPPN_LANGCHANGED/SHUTDOWN/TBMODIFICATION` 和高频
  `SCN_CHARADDED/SCN_MODIFIED`。
- **Win32/线程/网络**：WinForms 设置；`System.Timers.Timer`、`Task.Run`、
  `WebClient`；另有子进程辅助类。没有 Dock/direct function 证据。
- **双 HWND 难点**：消息本身可转发，但高频通知、后台网络任务和关闭时尚未发送
  的统计需要异常隔离；必须确认后台线程不会调用 HWND。
- **初评**：`B`；有限通知和状态栏能力外，还要验证线程/关闭边界。
- **证据**：[tag v1.0.1](https://github.com/p0358/notepadpp-CodeStats/tree/v1.0.1)
  的 `CodeStats/CodeStatsPackage.cs`、`Constants.cs`、`RunProcess.cs`。
- **未验证**：离线/超时、退出竞态、代理配置和网络服务现状。

### 3.8 `ColumnTools` 1.4.4.1

- **源码/许可证/类别**：对应源码 tag `1.4.4.1`；仓库中未找到明确顶层许可证，
  许可证未知；当前列高亮和水平标尺。
- **已确认 API**：`NPPM_GETCURRENTSCINTILLA`、
  `NPPM_GETPLUGINSCONFIGDIR`、`NPPM_ISTABBARHIDDEN`、
  `NPPM_SETMENUITEMCHECK`；处理 `NPPN_READY/SHUTDOWN`、文件/Buffer/样式变化、
  `SCN_UPDATEUI/ZOOM`。大量 Edge、Margin、字体/文本尺寸、矩形选区和虚拟空格
  SCI 消息作用于两个视图。
- **Win32/UI**：枚举/持有原版主副 Tab HWND，对两个 Tab 控件
  `SetWindowLongPtr(GWLP_WNDPROC)` subclass；使用 `TCM_ADJUSTRECT`，创建水平
  标尺窗口，并给主窗口发送 `WM_SIZE` 触发布局。
- **双 HWND 难点**：难点不在 SCI 转发，而在插件假设真实 Scintilla 与 Tab
  控件具有原版 Win32 层级、坐标和布局；Qt Tab 不具备该窗口结构。
- **初评**：`C`；水平标尺应基于新 UI API/Qt 编辑器装饰能力重写。
- **证据**：[tag 1.4.4.1](https://github.com/vinsworldcom/nppColumnTools/tree/1.4.4.1)
  的 `NppHorizontalRuler.cpp`、`HorizontalRuler.cpp`、
  `PluginDefinition.cpp`、`ColumnTools.cpp`。
- **未验证**：许可证、DPI、多视图布局和关闭时恢复 subclass。

### 3.9 `ComparePlugin` 2.0.2

- **源码/许可证/类别**：有源码，tag `v2.0.2`，GPL-3.0；双文件差异比较。
- **已确认 NPP API**：文档/buffer 定位和激活、路径、编码/语言、打开文件、
  菜单/工具栏、配置目录、`NPPM_DMMREGASDCKDLG`；处理关闭、保存、Buffer、
  Toolbar、样式通知。
- **已确认 SCI**：两个视图的大范围文本、行、可见区、缩放、Marker、
  Indicator、Margin、滚动、折叠和编辑消息，含输出结构
  `SCI_GETTEXTRANGE`；处理 `SCN_MODIFIED/UPDATEUI/PAINTED/ZOOM`。
- **Win32/Dock**：导航 Dock 和多个原生对话框；枚举主窗口子控件，按
  `ToolbarWindow32`、`SysTabControl32` 类名查找工具栏/标签，直接发送
  `TB_ENABLEBUTTON`；自定义控件 subclass；比较进度窗口使用键盘 Hook。
- **双 HWND 难点**：SCI 同步转发本身可实现，但原版 Toolbar/Tab 子窗口层级在
  Qt 主窗口下不存在；导航条、同步滚动和高频通知顺序也须精确复刻。
- **初评**：`C`；Compare 属常用复杂插件，应移植源码，不能为它完整模拟原版
  主窗口控件树。
- **证据**：[tag v2.0.2](https://github.com/pnedev/compare-plugin/tree/v2.0.2)
  的 `src/Compare.cpp`、`NppHelpers.cpp`、`Engine/Engine.cpp`、
  `NavDlg/NavDialog.cpp`、`ProgressDlg/ProgressDlg.cpp`。
- **未验证**：所有比较模式、临时文件恢复、取消/关闭竞态和实际性能。

### 3.10 `ComparePlus` 1.0.0

- **源码/许可证/类别**：有源码，tag `cp_1.0.0`，GPL-3.0；增强型双文件比较。
- **已确认 API/通知**：在 Compare 2.0.2 基础上还使用暗色模式、命令行、
  行号宽度、隐藏 Tab、深浅色 Toolbar 图标等消息；处理
  `NPPN_READY/LANGCHANGED/DARKMODECHANGED` 等。
- **已确认 SCI 高风险**：大范围比较、Marker/Indicator/Annotation/隐藏行及
  编辑操作；明确取得一个 `SCI_GETDIRECTFUNCTION` 和主副视图各自的
  `SCI_GETDIRECTPOINTER`，后续大量调用绕过代理 HWND。
- **Win32/Dock**：导航 Dock；主窗口和状态栏 WndProc subclass；枚举并修改
  Toolbar、两个 Tab 和状态栏内部控件；键盘 Hook；原生设置/进度/颜色控件。
- **双 HWND 难点**：direct pointer/function 是硬阻断，代理 HWND 无法返回
  QScintilla 内部对象；主窗口/状态栏 subclass 和内部控件树也不能稳定模拟。
- **初评**：`C`；按既定原则优先以新 API 移植。
- **证据**：[tag cp_1.0.0](https://github.com/pnedev/comparePlus/tree/cp_1.0.0)
  的 `src/Compare.cpp`（含 direct SCI 初始化）、`NppHelpers.cpp`、
  `Engine/Engine.cpp`、`NavDlg/NavDialog.cpp`、`ProgressDlg/ProgressDlg.cpp`。
- **未验证**：所有新比较模式、注解/隐藏行与 Qt Scintilla 的等价表现。

### 3.11 `CSScriptNpp` 2.0.2.0

- **源码/许可证/类别**：有源码，tag `v2.0.2.0`，MIT；C# 脚本执行、Roslyn
  IntelliSense 和调试环境。
- **已确认 API/SCI**：Dock 显示隐藏/注册、Toolbar、菜单命令、保存文件、
  版本、命令勾选；处理 Buffer/文件/Ready/Shutdown/Toolbar 和
  `SCN_CHARADDED/DWELL/MARGIN/SAVEPOINT`。文本、选区、Indicator、Marker、
  替换和自动完成消息范围广。
- **Win32/Dock/运行时**：多个 WinForms Dock（Project、Output、Debug、
  Watch/Locals 等）和弹窗；直接给主窗口发送 `WM_COMMAND`；读取/写入原版状态栏
  子 HWND；Roslyn/Syntaxer、CS-Script、调试器和多个子进程，含异步输出。
- **双 HWND 难点**：即便基础 SCI 可转发，复杂 Dock、状态栏内部 HWND、
  调试/进程生命周期和 C# 自动完成 UI 已超过有限兼容层的合理范围。
- **初评**：`C`；如需求足够高，保留脚本/语言服务逻辑并用新 API 重写宿主与 UI。
- **证据**：[tag v2.0.2.0](https://github.com/oleg-shilo/cs-script.npp/tree/v2.0.2.0)
  的 `src/CSScriptNpp/Plugin.cs`、`ProjectPanel.cs`、`OutputPanel.cs`，
  以及 `src/CSScriptIntellisense/Syntaxer.cs`、`NppEditor.cs`、
  `Interop/Npp.cs`。
- **未验证**：发布包附带运行时、调试器协议、进程退出和目标 .NET 版本。

### 3.12 `CSVLint` 0.4.5.4

- **源码/许可证/类别**：有源码，tag `0.4.5.4`，GPL-3.0；CSV/定宽数据语法
  高亮、校验、转换和元数据生成。
- **已确认 API**：配置/路径、语言切换、菜单、深浅色 Toolbar、
  `NPPM_DMMREGASDCKDLG/DMMHIDE/DMMSHOW`；处理 Buffer、语言变化和文件关闭。
- **SCI/自定义 Lexer**：使用 ScintillaGateway 操作文档；明确调用
  `SCI_PRIVATELEXERCALL`、`SCI_SETKEYWORDS`、`SCI_SETIDENTIFIERS`，并带
  自定义 lexer 配置/对象交互。
- **Win32/Dock**：WinForms Dock 和多种模态数据转换窗口。
- **双 HWND 难点**：Dock 可以单独适配，但 `SCI_PRIVATELEXERCALL` 的对象/
  指针语义不能由通用消息代理安全猜测；还涉及 lexer 与宿主 Scintilla 版本绑定。
- **初评**：`C`；建议以新 API 移植并为语言/诊断建立正式能力。
- **证据**：[tag 0.4.5.4](https://github.com/BdR76/CSVLint/tree/0.4.5.4)
  的 `CSVLintNppPlugin/Main.cs`、`PluginInfrastructure/Lexer.cs`、
  `NotepadPPGateway.cs`。
- **未验证**：lexer 二进制契约、所有数据类型/编码、大文件和 Dock 关闭。

## 4. 无对应版本源码或证据不足的插件

本节全部初评为 `U`。功能描述能说明调查方向，但不能证明具体调用。

| 插件 | 源码状态与类别 | 可确认依赖 | 双 HWND 主要待查 | 证据与未验证项 |
| --- | --- | --- | --- | --- |
| `_CustomizeToolbar` 5.3 | SourceForge 只确认到发布包；工具栏定制 | JSON 明确其修改 N++、附加及其他插件的 Toolbar 按钮；精确 API 未核验 | 很可能依赖原版 Toolbar 控件，但不得仅凭描述判定具体窗口类/API | [项目页](https://sourceforge.net/projects/npp-customize)；源码、许可证、六导出、Toolbar 枚举/subclass 全部未核验 |
| `ActiveX` 1.1.8.7 | 未找到对应源码；通过 ActiveX/COM 控制 N++ | JSON 明确 ActiveX 自动化和外部脚本/进程调用；NPPM/SCI 未核验 | COM 对象模型如何映射到主 HWND、是否直接转发任意消息未知 | [项目页](https://sourceforge.net/projects/nppactivexplugin/)；源码、COM 注册、线程模型、安全边界未核验 |
| `AnalysePlugin` 1.13.49.0 | 未取得对应源码；多模式日志分析 | API、通知、UI、Dock 均未核验 | 结果窗口是否 Dock、是否使用 Marker/Indicator/大缓冲区未知 | [项目页](https://sourceforge.net/projects/analyseplugin)；版本源码、许可证和实机行为未核验 |
| `AutoCodepage` 1.2.4 | 未取得对应源码；文档代码页自动切换 | 描述确认响应加载、重命名、语言和激活事件；具体 `NPPN_*`/`NPPM_SETBUFFERENCODING` 未以源码确认 | buffer ID、通知顺序及保存时机未知 | [项目页](https://sourceforge.net/projects/autocodepage)；源码、通知集合、无损写回未核验 |
| `AutoEolFormat` 1.0.2 | 未取得对应源码；自动 EOL 格式 | 描述确认加载、保存、重命名、激活工作流；具体通知/SCI 未确认 | 是否用 `SCI_CONVERTEOLS`、何时标脏未知 | [项目页](https://sourceforge.net/projects/autoeolformat)；源码、通知顺序和保存递归未核验 |
| `AutoSave` 1.6.1.0 | 官方 GitHub 仓库该目录只有发布 ZIP/README，无源码；定时/失焦保存 | README 确认 `NPPM_GETPLUGINSCONFIGDIR`；其余 API 未核验 | 失焦判断、保存消息、Timer 与关闭竞态未知 | [官方仓库](https://github.com/francostellari/NppPlugins/tree/main/AutoSave)；许可证为自定义文本，六导出和调用未核验 |
| `BookmarksDook` 2.3.3.0 | 发布链接对应 commit `a649a621...` 的仓库快照未提供可分析源码；书签面板 | JSON 只确认 “Bookmarks panel”；Dock/Marker API 未确认 | Dock 子窗口、Marker 分配、文件切换和持久化未知 | [官方仓库](https://github.com/Dook1/Bookmarks-Dook)；对应版本源码、许可证和 API 未核验 |
| `ccc` 1.4.1 | JSON 所列 GitHub 仓库当前不可访问；PHP 类自动完成和 Dock 类列表 | 描述确认自动完成弹窗、Dock 和跳转；具体 SCI/NPPM 未核验 | 自动完成 UI 是否依赖 Scintilla 原生 popup、Dock/索引线程未知 | JSON 主页 `github.com/StanDog/npp-phpautocompletion` 当前返回仓库不存在；不使用镜像或反编译补结论 |
| `ColorPicker` 2.3 | 未取得对应源码；颜色选择和代码插入 | API/SCI/Win32 对话框未核验 | 插入选区、颜色控件和剪贴板语义未知 | [发布页](https://sourceforge.net/projects/npp-plugins/files/ColorPicker/)；源码、许可证、调用未核验 |
| `Comment Wrap` 1.0.0.5 | 未取得对应源码；输入时自动换行注释 | 描述确认需编辑通知，但具体 `SCN_*`/SCI 未核验 | 高频输入重入、语言识别和撤销分组未知 | [项目页](https://sourceforge.net/projects/kered13-notepad-plugins/)；源码、通知、编码和撤销未核验 |
| `CsvQuery` 1.2.9 | 官方仓库可访问，但无 tag；当前 `master`（2025 commit）不能当作 1.2.9 源码 | 当前版显示 WinForms Dock、NPPM/SCI gateway 和内置 C# SQLite，但不用于基线定级 | 1.2.9 的 Dock、SQL 线程、文本读取/结果文档能力需匹配发布 commit | [官方仓库](https://github.com/jokedst/CsvQuery)；需定位 1.2.9 发布 commit/source archive 后重查 |
| `CustomLineNumbers` 1.1.7 | SourceForge 只确认到对应 x86 发布包和版本历史，未找到可对应 1.1.7 的源码 | 功能说明确认修改行号边距显示；具体 NPPM/SCI、通知和窗口调用均未核验 | [SourceForge v1.1.7](https://sourceforge.net/projects/customlinenumbers/files/v1.1.7/)；不反汇编，需取得对应源码或在 Windows 做受控消息追踪 |

## 5. 首轮结论

| 等级 | 插件 |
| --- | --- |
| `A` | `BetterMultiSelection`、`BracketsCheck`、`CodeAlignmentNpp` |
| `B` | `BigFiles`、`CodeStats` |
| `C` | `3P`、`CADdyTools`、`ColumnTools`、`ComparePlugin`、`ComparePlus`、`CSScriptNpp`、`CSVLint` |
| `D` | 无；本轮没有足够使用量证据把任何插件判为低价值 |
| `U` | `_CustomizeToolbar`、`ActiveX`、`AnalysePlugin`、`AutoCodepage`、`AutoEolFormat`、`AutoSave`、`BookmarksDook`、`ccc`、`ColorPicker`、`Comment Wrap`、`CsvQuery`、`CustomLineNumbers` |

该分布只说明首轮技术证据，不是支持承诺。尤其：

- `A/B` 仍必须在 Windows x86 上验证六导出、初始化、菜单、通知和正常关闭。
- `C` 中 Compare/ComparePlus 的内部控件枚举和 ComparePlus/3P 的 direct SCI
  是明确阻断项，不应扩大兼容层去模拟。
- `U` 不等于不兼容；只有取得对应源码或完成允许的原版运行观察后才能复评。

## 6. 后续验证清单

- [ ] 在隔离的 Windows x86 环境逐个校验下载包 SHA-256 和 PE 架构。
- [ ] 校验六个必需导出、`isUnicode`、`FuncItem` 数量/名称/快捷键。
- [ ] 对 A/B 插件记录实际 `SendMessage` 白名单并加入兼容层自动测试。
- [ ] 验证主副视图切换、文档移动到另一视图、通知来源 HWND 和重入顺序。
- [ ] 对所有输出缓冲区消息验证 x86 指针宽度、结构布局、编码和边界长度。
- [ ] 用异常边界测试加载失败、命令回调、通知回调和关闭回调不导致宿主崩溃。
- [ ] 为 U 类继续查找对应版本源码；无源码插件只做应用级行为观察，不反汇编。
- [ ] 补充使用量/重要性证据后，再决定 C 类源码移植优先级和是否出现 D 类。
