# Notepad++ v8.4.6 插件调查：x86 O–S 批次

## 1. 范围和口径

本文件是 **x86-only** 首轮调查，只使用
`third_party/nppPluginList/catalog/windows/pl.x86.json`，按 `folder-name` 不区分大小写
排序，范围从 `OpenSelection` 到 `SurroundSelection`，共 32 项。

- JSON 版本、包 SHA-256、作者、主页和包地址以仓库内 x86 JSON 为准。
- 只有与 JSON 指定版本对应的 tag/commit 才用于 API 定级；只有二进制、只有
  其他版本源码或尚未定位到对应 revision 的插件保持 `U`，没有反汇编。
- API 集合来自业务源码调用点，不把 SDK 头文件或自动生成接口中定义的全部常量
  当成实际调用。
- A/B/C/D/U 是“Qt 主窗口 HWND＋两个原生 Scintilla 代理 HWND”原则下的初评，
  不是 Windows x86 实机验证结论。
- “维护状态”只描述公开仓库可观察活动；本轮没有向作者确认。

## 2. x86 JSON 基线

| `folder-name` | 显示名称 | x86 版本 | 包 SHA-256 |
| --- | --- | --- | --- |
| `OpenSelection` | OpenSelection | `1.1.3.0` | `b51b53d4110898f66c264d4fac2faeb52aefd59a26072da9eab69fc031bd3106` |
| `Papyrus` | Papyrus Script Lexer | `0.4.0.27` | `619f7ac822d5fecee375d05e282728da906c2e7f9f74c1bacfd11beee431dca1` |
| `PlanetCNCNpp32` | Expression calculator | `3001.21.1123.1` | `67213223df17b234be75b8ada4f7bdbd3c05db2e2d00390b7effc4bb814bc94d` |
| `PlantUmlViewer` | PlantUML Viewer | `1.3.0.7` | `601b5f2bfe48f4c9e6404f7b6b51f9c093042517f269699fa51b02e7c6ae8e94` |
| `PoorMansTSqlFormatterNppPlugin` | Poor Man's T-Sql Formatter | `1.6.13.31502` | `a4a130b8163a323fcccdb1793c7a544c430e50fbeb87bf708cd196ddbce4682e` |
| `pork2sausage` | Pork to Sausage | `2.2` | `21ee354564268b75f766d114e8bc12baea9e1a5941a55d3994aa5daa0fb56dcd` |
| `PreviewHTML` | Preview HTML | `1.3.2.0` | `b7538d5b4c61e7232ad6befe3a5eb1799544a4de741d4acd896716e9273f260b` |
| `PyNPP` | PyNPP | `1.2` | `e5ac10066afdf1bde3f5b1b1219b9556989dc43c0cbcf7a8e2dc81b0e84bd67f` |
| `Python Indent` | Python Indent | `1.0.0.4` | `ceef38255c06cc20acb7b4a3fedcc8e88f0cc83700fd7fe89ba8cb5a258cca0d` |
| `PythonScript` | PythonScript | `2.0.0.0` | `6c5d4a51e511710b892f3ff3f443c0d7763e686cfd7ffb9888789c8ce09318b9` |
| `qkNppReverseLines` | Reverse Lines | `1.0.0.0` | `8d1f3b45dcb56e9f66b0f4fad66e6b3cd6f730f5116997d978e589e1e5dc73e0` |
| `QuickOpenPlugin` | QuickOpenPlugin | `1.1` | `43d57c67a30a777076a4d4083b9c6c914898028d21d0094c8f54cd58ffa803de` |
| `QuickText` | QuickText | `0.2.5.1` | `8fa56545d59bbc94d85808cd6ff91666165cb861645d630e9d37f6836fbb14f6` |
| `RandomValuesNppPlugin` | Random Values | `0.2.1` | `65f4fd342da9541801af1fb2a55c223d7708c5a0906d153606ef3c6b28ba0f64` |
| `rdmd-en-x86` | RDMD for Notepad++ (English) | `0.1.0.2` | `ad00a5596a89dee788c4d192e93b23448f9a079b38a556a7fbe46d5cef2b6858` |
| `rdmd-ja-x86` | RDMD for Notepad++ (Japanese) | `0.1.0.2` | `8728fc8df7572cd1752fc1cc792e8c51f0967007862014cb222b8ae9488dc803` |
| `RegexTrainer` | Regex Trainer | `1.0.0` | `2ae9b1115414da77379fafec2dc11277200dab6b73b55dc9636bb30a9d5f6503` |
| `Remove Duplicate Lines` | Remove Duplicate Lines | `1.3.0.0` | `afdf2e1e2c9e2fe01a528c40873327796d6960e2107721eaf9274dbdef1d675a` |
| `RestApiToText` | RestApiToText | `1.3.1.0` | `725a49d61b1323485b475ce8b03ccaa8335cb004f55b2463f6b0e3a084a5286d` |
| `RunMe` | RunMe | `1.4.1.0` | `2b0197fbf1dc5927726cf6c5801f1c74693ee6f2011194a83369bb82ee0b16aa` |
| `SecurePad` | SecurePad | `2.4` | `6ebd61bb4c0ad49968a387beb8ebe891f7686abee823c9c1d1f8e60d67d19fea` |
| `selectNLaunch` | Select N' Launch | `2.1` | `bb6e5eeb6707ac469e0cecd36b021349d8a9b223c7c5351777cfdba8463cca69` |
| `SelectQuotedText` | SelectQuotedText | `1.0.0` | `798b6624432a62adbfd9ef514d3ce8e0a423c22d2fe1b030eca2b1fdf7ae6533` |
| `SelectToClipboard` | Select to Clipboard | `1.0.3` | `3bbc396ef98445e81b7499fc9b3817476e7752536c62d70c43a67aac1e82b200` |
| `SessionMgr` | Session Manager | `1.4.4` | `77661b508700768b08f6056c6f3fd1d1e7bfbce031f9d9bdf4c213033b4de2f1` |
| `SherloXplorer` | SherloXplorer | `0.3` | `abb3bd754d3ca2da6215800f1cbbe1e9731404faea4514cee03feea4aaba2be0` |
| `ShtirlitzNppPlugin` | Shtirlitz | `1.1.2` | `5cbdfa3fec96e478b4002a5fc2b68b60a9a4f90a401d645845c99a5a784bb5a6` |
| `SourceCookifier` | Source Cookifier | `0.7.3` | `ccc0e93ddb580511b5c2b4a3363713ef0cd96187515e972362bf5b6b235430de` |
| `SpeechPlugin` | SpeechPlugin | `0.4.0.0` | `9b647ba92834a961e980dfd886ace54fc8f972b01ff917cef0ba3f47361e5b40` |
| `SpellChecker` | Spell-Checker | `1.3.3.0` | `4306120f5dad3952a24d2a38bcd611e1c15c713d6a2a1cfab3b847588f6eb859` |
| `SQLinFormNpp` | SQLinForm | `5.3.35` | `0c5d009c5904628c25728975c55b61f62d4d47575a9e062669412ec700cc0f75` |
| `SurroundSelection` | SurroundSelection | `1.4.1` | `38ef3528794a5e54224580844ce8cb6d92e1991662b40d330f41402bc2dc3a7e` |

## 3. 有对应版本源码的插件

### 3.1 `Papyrus` 0.4.0.27

- **源码/许可证/维护**：仓库 tag `v0.4.0`，revision
  `3977d024b2d3`，插件文件版本 0.4.0.27；GPL-3.0。仓库在 2026 年仍有提交。
- **实际 NPP/通知**：`NPPM_ALLOCATECMDID`、`DOOPEN`、
  `GETCURRENTBUFFERID/VIEW/LANGTYPE`、`GETCURRENTDOCINDEX`、
  `GETBUFFERIDFROMPOS/GETBUFFERLANGTYPE`、路径/语言/配置目录、保存和状态栏；
  `NPPN_READY/BEFORESHUTDOWN/BUFFERACTIVATED/LANGCHANGED`。
- **实际 SCI/SCN**：文本范围、搜索 Target、Word、Style、Indicator、Annotation、
  扩展 Style、`SCI_GETDOCPOINTER`、`SCI_COLOURISE`；处理
  `SCN_MODIFIED/UPDATEUI/HOTSPOTCLICK/HOTSPOTDOUBLECLICK`。
- **Win32/Dock/进程**：错误列表原生 Dock；原生设置/颜色对话框；Papyrus
  编译器子进程、计时器和异步结果；同时直接操作主、副两个 SCI HWND。
- **双代理难点/初评**：指针结构、Annotation/Indicator/Style、双视图文档映射、
  高频通知和 Dock 生命周期均需覆盖；外部编译器本身不要求宿主模拟。`C`，建议
  以新 API 移植 lexer、编译服务和错误面板，不扩大为完整 Win32 Dock 模拟。
- **证据**：[v0.4.0](https://github.com/blu3mania/npp-papyrus/tree/v0.4.0)
  的 `src/Plugin/Plugin.cpp`、`Lexer/*`、`CompilationErrorHandling/*`、
  `Compiler/*`。
- **未验证**：六导出、编译器发现/关闭、通知顺序、主副视图和 x86 实机。

### 3.2 `PlantUmlViewer` 1.3.0.7

- **源码/许可证/维护**：tag `1.3.0.7`，revision `46ae43e732d0`，MIT；仓库
  在 2026 年仍有提交。
- **实际 API**：`NPPM_DMMREGASDCKDLG/DMMHIDE/DMMSHOW`、
  `GETEDITORDEFAULTBACKGROUNDCOLOR`、`GETMENUHANDLE`、
  `SETMENUITEMCHECK`；经 gateway 读取当前路径以及
  `SCI_GETLENGTH/GETTEXT`。无业务通知处理。
- **Win32/Dock/运行时**：WinForms Dock，直接 `EnableMenuItem(HMENU)`；
  调用附带的 PlantUML JAR/Java，含刷新任务、HTTP 检查更新、剪贴板和导出图片。
- **双代理难点/初评**：SCI 读取容易代理，但 Qt 主窗口没有原版 HMENU/Docking
  结构；WinForms Dock 和 Java 生命周期不是有限消息转发能可靠覆盖。`C`，保留
  PlantUML 业务逻辑，以新面板 API 重写 UI/宿主调用。
- **证据**：[1.3.0.7](https://github.com/Fruchtzwerg94/PlantUmlViewer/tree/1.3.0.7)
  的 `PlantUmlViewer.cs`、`Forms/PreviewWindow.cs`。
- **未验证**：JRE 搜索、渲染取消/退出竞态、DPI、六导出和实机 Dock。

### 3.3 `PoorMansTSqlFormatterNppPlugin` 1.6.13.31502

- **源码/许可证/维护**：tag `1.6.13`，revision `7880f999b278`，AGPL-3.0；
  上游最后可见提交在 2019 年，低频/近似停止。
- **实际 API**：`NPPM_GETPLUGINSCONFIGDIR`；
  `SCI_GETSELTEXT/GETTEXT/GETCURRENTPOS`、`REPLACESEL/SETTEXT/SETSEL`。
  通过 NppPluginNET 获取当前 SCI HWND；没有业务通知。
- **Win32/运行时**：.NET/WinForms 设置和确认框；格式化器是托管业务库，无
  Dock、Hook、direct function、后台线程或子进程证据。
- **双代理难点/初评**：主要是变长输出缓冲区、UTF-8 字节长度和 CLR 异常边界。
  `A`，Windows 上优先直接兼容 DLL。
- **证据**：[1.6.13](https://github.com/TaoK/PoorMansTSqlFormatter/tree/1.6.13)
  的 `PoorMansTSqlFormatterNppPlugin/Main.cs`。
- **未验证**：程序集内部版本与 JSON 四段版本、非 ASCII、大文档、六导出。

### 3.4 `pork2sausage` 2.2

- **源码/许可证/维护**：tag `v2.2`，revision `ba00cfda04e7`，GPL-2.0；
  仓库 2025 年仍有活动。
- **实际 API**：`NPPM_GETPLUGINSCONFIGDIR/GETCURRENTSCINTILLA/DOOPEN`；
  `SCI_GETSELECTIONSTART/END/GETSELTEXT`、`CLEAR/ADDTEXT/REPLACESEL/SETSEL`。
- **Win32/进程/线程**：读取配置后用 `CreateProcess` 执行任意配置命令，以
  三个 Win32 线程等待进程和读取 stdout/stderr；原生消息框，无 Dock。
- **双代理难点/初评**：同步 SCI 指针缓冲区容易代理；风险主要是后台线程、
  进程管道、退出清理和不可信配置，而非宿主 HWND。`B`，Windows DLL 可在进程
  隔离和关闭行为验证后兼容。
- **证据**：[v2.2](https://github.com/npp-plugins/pork2sausage/tree/v2.2)
  的 `src/Pork2Sausage.cpp`、`src/Process.cpp`。
- **未验证**：插件卸载时存活线程、超大 stdout/stderr、编码、六导出。

### 3.5 `PyNPP` 1.2

- **源码/许可证/维护**：tag `v1.2`，revision `8b9ea8ddf0cc`，README 声明
  GPL-3.0（部分模板源码头为 GPL-2.0-or-later，需许可证复核）；2019 年后无
  可见活动。
- **实际 API**：`NPPM_GETPLUGINSCONFIGDIR`、`SAVECURRENTFILE`、
  `GETFULLCURRENTPATH`、`GETCURRENTSCINTILLA`；
  `SCI_ENSUREVISIBLE/GOTOLINE`。
- **Win32/进程**：原生选项/帮助/跳行对话框；枚举 `C:\python*`，用
  `CreateProcess` 启动外部 Python，并操作进程句柄。无 Dock。
- **双代理难点/初评**：SCI 转发简单，主窗口只作对话框 owner；路径和
  CreateProcess 由 DLL 自己处理。`A`，Windows 上直接兼容，跨平台功能应改用
  进程服务而不是复用 DLL。
- **证据**：[v1.2](https://github.com/mpcabd/PyNPP/tree/v1.2) 的
  `PluginDefinition.cpp`、`globals.cpp`、`OptionsDlg.cpp`。
- **未验证**：进程关闭/句柄释放、Python 路径空格、六导出和错误崩溃边界。

### 3.6 `PythonScript` 2.0.0.0

- **源码/许可证/维护**：tag `v2.0.0`，revision `accd4b14d774`，GPL-2.0；
  项目在 2026 年仍活跃。
- **实际宿主能力**：动态命令 ID/快捷键、主菜单、配置和 N++ 目录、toolbar、
  `NPPM_CREATESCINTILLAHANDLE/DESTROYSCINTILLAHANDLE`、Dock、
  `MODELESSDIALOG`、`MSGTOPLUGIN`，以及脚本可调用的大范围 `NPPM_*`。
  Python 暴露几乎完整 SCI 接口和通知回调，不应按头文件中的常量全集误判为每个
  内置脚本都调用，但 API 表面本身就是公开能力。
- **Win32/Dock/线程/运行时**：嵌入 CPython/Boost.Python；Console Dock 创建
  新 Scintilla HWND并 subclass；主窗口和输入控件也 subclass；动态重写菜单；
  Python 执行线程、异步中断、子进程 stdout/stderr 线程和插件间消息。
- **双代理难点/初评**：任意脚本可保存 HWND、调用任意 SCI 并注册回调；新建
  Scintilla、窗口 subclass、HMENU 和回调重入无法由两个固定代理完整模拟。`C`，
  属于重要插件，应为新 API/跨平台脚本运行时单独设计，不完整模拟旧窗口树。
- **证据**：[v2.0.0](https://github.com/bruderstein/PythonScript/tree/v2.0.0)
  的 `PythonScript/src/{ScintillaWrapper,ConsoleDialog,MenuManager,PythonHandler,
  ProcessExecute}.cpp`。
- **未验证**：脚本 API 全量矩阵、Python ABI/依赖 DLL、异常隔离、退出竞态。

### 3.7 `qkNppReverseLines` 1.0.0.0

- **源码/许可证/维护**：tag `v1.0.0.0`，revision `c15eb32e465f`，GPL-3.0；
  2019 年后无可见活动。
- **实际 API**：`NPPM_GETCURRENTSCINTILLA`；`SCI_GETEOLMODE/GETSELTEXT`、
  `GETSELECTIONSTART/END`、`GETCURRENTPOS/GETTEXTLENGTH`、
  `SETTARGETSTART/END/REPLACETARGET/SETSEL/SELECTALL`；仅处理
  `NPPN_SHUTDOWN`。
- **Win32/双代理/初评**：除同步 `SendMessage` 和 About 消息框，无 Dock、
  Hook、线程、direct function。指针输出和目标替换语义可由代理覆盖。`A`。
- **证据**：[v1.0.0.0](https://github.com/querykuma/qkNppReverseLines/tree/v1.0.0.0)
  的 `src/PluginDefinition.cpp`。
- **未验证**：矩形/多选区、UTF-8 和空文档边界、六导出。

### 3.8 `QuickText` 0.2.5.1

- **源码/许可证/维护**：tag `0.2.5.1`，revision `fc21782d3b8d`，
  GPL-2.0-or-later；该 tag 即 2022 年最后可见提交，低频。
- **实际 NPP/通知**：`GETCURRENTSCINTILLA/LANGTYPE`、
  `GETLANGUAGENAME/PLUGINSCONFIGDIR/SHORTCUTBYCMDID`、`DOOPEN`；
  `NPPN_SHUTDOWN`，`SCN_CHARADDED/MODIFIED/UPDATEUI/AUTOCCOMPLETED`。
- **实际 SCI**：Word/selection/TextRange，缩进、Tab/EOL、多选区、
  `AUTOCSHOW/CANCEL`、自动完成分隔符、`REGISTERIMAGE`、Style 颜色和
  `REPLACESEL`。
- **Win32/双代理/初评**：无 Dock、Hook、direct function；需要精确的高频通知、
  autocomplete 图像和多选区语义。`B`，增加可复用通知/自动完成覆盖后兼容。
- **证据**：[0.2.5.1](https://github.com/vinsworldcom/nppQuickText/tree/0.2.5.1)
  的 `QuickText.cpp`。
- **未验证**：通知重入、非 ASCII snippet、双视图、六导出。

### 3.9 `RandomValuesNppPlugin` 0.2.1

- **源码/许可证/维护**：tag `0.2.1`，revision `aae173d87c96`；对应 revision
  未找到明确许可证文件/声明，许可证未知；仓库 2023 年仍有活动。
- **实际 API**：`NPPM_ADDTOOLBARICON_FORDARKMODE`、
  `GETPLUGINSCONFIGDIR/DOOPEN`；当前 SCI 的 `SCI_REPLACESEL/NEWLINE`。
  无业务通知。
- **Win32/UI**：.NET WinForms 生成、设置和 About 对话框；剪贴板/浏览器只在
  About；无 Dock、线程和 direct function。
- **双代理难点/初评**：文本替换容易代理，Toolbar 暗色图标结构和 HICON 所有权
  需有限实现。`B`。
- **证据**：[0.2.1](https://github.com/BdR76/RandomValuesNPP/tree/0.2.1)
  的 `RandomValuesNppPlugin/Main.cs`、`Tools/SettingsBase.cs`。
- **未验证**：许可证、换行设置、HICON 生命周期、六导出和 CLR 边界。

### 3.10 `Remove Duplicate Lines` 1.3.0.0

- **源码/许可证/维护**：包发布 tag 为 `1.3.0.2`，revision
  `5e6f4a5109e8`，源码 About/程序集对应 1.3.0.0；未找到明确许可证，许可证
  未知；仓库 2023 年仍有提交。
- **实际 API**：当前 SCI 的 `SCI_GETSELTEXT/CLEARSELECTIONS/REPLACESEL`；
  无业务 `NPPM_*`/通知。WinForms 消息框，无 Dock/线程/direct function。
- **双代理难点/初评**：仅选区输出缓冲区和 UTF-8 字节长度。`A`。
- **证据**：[1.3.0.2](https://github.com/gurikbal/Remove_dup_lines/tree/1.3.0.2)
  的 `Remove dup lines/Remove dup lines/Main.cs`。
- **未验证**：包内程序集哈希与源码构建、许可证、EOL 保留、多字节、六导出。

### 3.11 `RestApiToText` 1.3.1.0

- **源码/许可证/维护**：对应发布 commit `c254d40`；仓库未提供明确许可证，
  许可证未知；最后可见活动为 2022 年。
- **实际 API**：`NPPM_GETCURRENTSCINTILLA/MENUCOMMAND`；
  `SCI_GETLENGTH/GETSELECTIONSTART/END/GETSELTEXT`、
  `SETSELECTIONSTART/END/SETTEXT`；`NPPN_SHUTDOWN`。
- **Win32/网络**：原生对话框和 `ShellExecute`；同步 WinINet
  `InternetOpen/InternetConnect/HttpOpenRequest` 发 HTTP(S)，将响应写到新文档。
  无 Dock、Hook、direct function。
- **双代理难点/初评**：同步 SCI 和 `IDM_FILE_NEW` 可代理；网络阻塞/响应大小与
  退出稳定性需要防护，但不是窗口结构问题。`B`。
- **证据**：[commit c254d40](https://github.com/eljefe7000/RestApiToText/tree/c254d40)
  的 `RestApiToText/PluginDefinition.cpp`。
- **未验证**：TLS/代理/超时、错误响应、大响应、六导出和许可证。

### 3.12 `SecurePad` 2.4

- **源码/许可证/维护**：tag `v2.4`，revision `c7b2ac94bccd`，源码头声明
  GPL-2.0-or-later；仓库在 2026 年仍有活动。
- **实际 API**：`NPPM_GETCURRENTSCINTILLA`；全文/选区的
  `SCI_GETTEXTLENGTH/GETTEXT/SETTEXT`、`GETSELECTIONSTART/END`、
  `GETTEXTRANGE/REPLACESEL`；`NPPN_SHUTDOWN`。
- **Win32/UI**：密码资源对话框、消息框、`VirtualAlloc`，内置 Blowfish；
  无 Dock（仓库模板 `DockingFeature` 未被业务注册）、线程、direct function。
- **双代理难点/初评**：`Sci_TextRange` 指针、缓冲区和二进制/文本长度必须精确；
  主窗口只作 owner。`A`，Windows 直接兼容。
- **证据**：[v2.4](https://github.com/DominicTobias/SecurePad/tree/v2.4)
  的 `PluginDefinition.cpp`、`blowfish.cpp`。
- **未验证**：加密格式兼容、Unicode/空字节、密钥内存清理、六导出。

### 3.13 `selectNLaunch` 2.1

- **源码/许可证/维护**：tag `v2.1`，revision `f37ddc8b01ee`，GPL-2.0；
  仓库 2024 年仍有活动。
- **实际 API**：`NPPM_GETPLUGINSCONFIGDIR/GETCURRENTSCINTILLA/DOOPEN`；
  `SCI_GETSELECTIONSTART/END/GETSELTEXT`。
- **Win32/进程**：依据配置把选区写临时文件，`ShellExecute` 交给关联程序；
  原生消息框，无 Dock、Hook、线程和 direct function。
- **双代理难点/初评**：仅选区输出缓冲区；外部启动由插件执行。`A`。
- **证据**：[v2.1](https://github.com/npp-plugins/selectnlaunch/tree/v2.1)
  的 `src/SelectNLaunch.cpp`。
- **未验证**：临时文件清理、恶意配置/路径、编码、六导出。

### 3.14 `SelectQuotedText` 1.0.0

- **源码/许可证/维护**：tag `v1.0.0`，revision `b921ec0addf8`，GPL-2.0；
  仓库 2024 年仍有活动。
- **实际 API**：`NPPM_GETCURRENTSCINTILLA`；
  `SCI_GETCURRENTPOS/GETLEXER/GETSTYLEAT/WORDSTARTPOSITION/
  WORDENDPOSITION/SETSELECTIONSTART/SETSELECTIONEND`。
- **Win32/UI**：原生 About 对话框和 `ShellExecute`；无 Dock、线程、Hook、
  direct function。
- **双代理难点/初评**：需要 lexer/style 查询和选区设置，均为同步标量。`A`。
- **证据**：[v1.0.0](https://github.com/ffes/selectquotedtext/tree/v1.0.0)
  的 `SelectQuotedText.cpp`、`DlgAbout.cpp`。
- **未验证**：不同 lexer/嵌套引号、双视图、六导出。

### 3.15 `SelectToClipboard` 1.0.3

- **源码/许可证/维护**：tag `v1.0.3`，revision `d38ddc293a57`，GPL-2.0；
  该 tag 即 2022 年最后可见提交。
- **实际 API/通知**：`NPPM_GETCURRENTBUFFERID/GETCURRENTSCINTILLA/
  GETPLUGINSCONFIGDIR`，`NPPN_SHUTDOWN`，`SCN_UPDATEUI`；SCI 包括编码/EOL、
  line/position、普通/矩形选区、`GETTEXTRANGE`。
- **Win32**：直接 `OpenClipboard/EmptyClipboard/SetClipboardData`，注册
  Column Select 格式；无 Dock、Hook、线程或 direct function。
- **双代理难点/初评**：通知频率、矩形选区逐行输出和 UTF-8→UTF-16 转换；剪贴板
  由 DLL 自己处理。`B`。
- **证据**：[v1.0.3](https://github.com/KubaDee/SelectToClipboard/tree/v1.0.3)
  的 `src/PluginDefinition.cpp`、`src/SelectToClipboard.cpp`。
- **未验证**：大选区、剪贴板占用失败、矩形选择、多字节、六导出。

### 3.16 `SessionMgr` 1.4.4

- **源码/许可证/维护**：tag `v1.4.4`，revision `1d6063799cef`，GPL-3.0；
  2015 年后无可见提交，停止维护。
- **实际 NPP/通知**：`GETNPPVERSION/GETWINDOWSVERSION/GETNPPDIRECTORY/
  GETPLUGINSCONFIGDIR`、`SAVECURRENTSESSION/LOADSESSION`、buffer→路径/位置、
  `ACTIVATEDOC/MENUCOMMAND/MSGTOPLUGIN/SETSTATUSBAR`；处理 ready/shutdown、
  buffer、文档顺序、打开/保存/关闭前后、语言等大量 `NPPN_*`。
- **SCI/Win32**：恢复位置使用 `SCI_GOTOLINE`，处理 margin/savepoint 通知；
  多个原生资源对话框、ListBox/ComboBox 控件消息和文件系统读写，无 Dock。
- **双代理难点/初评**：SCI 很少，难点是完整 session XML/文档事件顺序、
  buffer-view 映射和插件间消息。`B`，这些是可复用宿主服务，不应模拟额外窗口树。
- **证据**：[v1.4.4](https://github.com/chcg/npp-session-manager/tree/v1.4.4)
  的 `src/SessionMgr.cpp`、`System.cpp`、`DlgSessions.cpp`。
- **未验证**：Notepad++ 8.4.6 session 语义、双视图恢复、异常退出、六导出。

### 3.17 `SpeechPlugin` 0.4.0.0

- **源码/许可证/维护**：tag `v0.4.0`，revision `b0870d4ff407`，GPL-2.0；
  仓库在 2026 年仍有活动。
- **实际 API**：`NPPM_GETCURRENTSCINTILLA`；全文/选区的
  `SCI_GETTEXTLENGTH/GETTEXT/GETSELECTIONSTART/END/GETTEXTRANGE`；
  `NPPN_SHUTDOWN`。
- **Win32/外部依赖**：调用 Windows SAPI/COM 发音，原生消息框；无 Dock、Hook、
  direct function。语音 COM 由 DLL 自己管理，不需要宿主模拟。
- **双代理难点/初评**：文本缓冲区和 UI 线程 COM apartment；消息覆盖本身简单。
  `A`，Windows 原版 DLL 兼容；跨平台另用 Qt/系统语音服务。
- **证据**：[v0.4.0](https://github.com/chcg/SpeechPlugin/tree/v0.4.0)
  的 `SpeechPlugin.cpp`。
- **未验证**：SAPI 缺失/语音切换、超长文本、关闭时仍在发音、六导出。

### 3.18 `SurroundSelection` 1.4.1

- **源码/许可证/维护**：tag `v1.4.1`，revision `dfb6805abcfb`，GPL-2.0；
  最后可见提交为 2023 年，低频。
- **实际 API/通知**：`NPPM_GETCURRENTSCINTILLA/GETPLUGINSCONFIGDIR/
  SETMENUITEMCHECK`；`NPPN_READY/BUFFERACTIVATED/SHUTDOWN`，
  `SCN_FOCUSIN/FOCUSOUT`。Scintilla wrapper 使用多选区、Target、
  `BEGIN/ENDUNDOACTION` 和替换/追加选区消息。
- **Win32/Hook**：按当前 UI 线程安装 `WH_KEYBOARD` hook，读取键盘状态和布局，
  `VkKeyScanEx` 将字符转虚拟键；原生 About，无 Dock/进程。
- **双代理难点/初评**：固定代理可转发文本操作，但键盘 hook 依赖真实 UI 线程、
  焦点通知、布局和按键吞噬时序；错误会影响整个应用输入。`B`，仅在准确转换
  Focus/Buffer 通知并做崩溃隔离后尝试 Windows DLL，跨平台应改用新输入事件 API。
- **证据**：[v1.4.1](https://github.com/dail8859/SurroundSelection/tree/v1.4.1)
  的 `src/Main.cpp`、`src/ScintillaEditor.*`。
- **未验证**：Qt 原生窗口线程是否一致、IME/AltGr、双视图焦点、hook 卸载。

## 4. 无对应版本源码或源码证据不足

以下项目均保持 `U`。表中“可见材料”不是 API 结论。

| 插件 | 可见材料与维护/许可证信息 | 保持 U 的原因及后续证据 |
| --- | --- | --- |
| `OpenSelection` 1.1.3.0 | 官方主页仓库仅归档 x86/x64/ARM64 ZIP 与 README；2024 年仍更新其他包；许可证未知 | 对应版本只有二进制，未反汇编。需作者源码或 Windows 外部行为/API trace；应用范围待定。 |
| `PlanetCNCNpp32` 3001.21.1123.1 | GitHub 仓库 revision `207361f` 只提交 `PlanetCNCNpp32.zip/64.zip`；主页仍存在；许可证未知 | 无源码；不从计算器功能推断 SCI/窗口调用。需源码或受控实机 trace。 |
| `PreviewHTML` 1.3.2.0 | Fossil 项目主页/指定版本 ZIP；本轮未取得能与 `v1.3.2.0-32` 对齐的源码 revision | 即使公开站点可能含历史，也尚未建立 revision→包证据。下一轮应导出 Fossil manifest、许可证和源码。 |
| `Python Indent` 1.0.0.4 | SourceForge 项目和版本 ZIP；许可证、维护状态未知 | 尚未取得对应版本源码，不使用其他 Python-indent 实现推断。 |
| `QuickOpenPlugin` 1.1 | SourceForge 主页；JSON 仓库 URL 的文件名为 “V1.2” 而 JSON 版本是 1.1 | 版本元数据矛盾且未取得源码。先核对包文件版本，再找 source archive。 |
| `rdmd-en-x86` 0.1.0.2 | GitLab 项目和英语发布包；许可证/维护未核验 | 本轮未取得对应 revision；英语和日语包须分别核对资源差异，不能只分析一个二进制。 |
| `rdmd-ja-x86` 0.1.0.2 | 同一 GitLab 项目和日语发布包 | 同上。若源码相同，也要记录本地化资源/导出包差异后才能合并结论。 |
| `RegexTrainer` 1.0.0 | GitHub “Descriptions” 仓库只含说明、截图和 Release ZIP，2023 年更新包哈希；源码/许可证未知 | 对应版本只有二进制；不反汇编。需作者源码或实机行为/API trace。 |
| `RunMe` 1.4.1.0 | `NppPlugins` 仓库仅归档各版本 ZIP 与 README；许可证未知 | 无对应源码，不因“运行命令”功能推断进程/API。 |
| `SherloXplorer` 0.3 | SourceForge `sourcecookifier/other plugins` 二进制包；许可证/维护未知 | 尚未找到 0.3 对应源码。名称和来源接近 SourceCookifier 不等于实现相同。 |
| `ShtirlitzNppPlugin` 1.1.2 | GitHub revision `bc34dea`/`b90ae9d` 只有 DLL、ZIP、配置、数据和辅助 EXE；许可证未知，2019 年后无活动 | 明确是无源码发布；不反汇编。若重要，可从原版应用观察解码工作流后复刻。 |
| `SourceCookifier` 0.7.3 | SourceForge 版本包；源码/许可证/维护未核验 | 本轮未取得可复现的 0.7.3 source archive；其浏览/索引 UI 可能复杂，但不能据此定级。 |
| `SpellChecker` 1.3.3.0 | SourceForge 老版本 UNI DLL 包；主页表明为旧插件，许可证/源码 revision 未核验 | 未建立 1.3.3.0 对应源码证据；不要与 DSpellCheck 混同。 |
| `SQLinFormNpp` 5.3.35 | 商业 SQLinForm 官网提供插件包，未找到公开对应源码或许可证 | 无源码且可能为商业闭源；不反汇编。先调查使用范围和厂商合作可能，再决定外部行为复刻价值。 |

## 5. 汇总与宿主能力方向

| 初评 | 数量 | 插件 |
| --- | ---: | --- |
| A | 8 | Poor Man's T-Sql Formatter、PyNPP、Reverse Lines、Remove Duplicate Lines、SecurePad、Select N' Launch、SelectQuotedText、SpeechPlugin |
| B | 7 | pork2sausage、QuickText、Random Values、RestApiToText、SelectToClipboard、SessionMgr、SurroundSelection |
| C | 3 | Papyrus、PlantUML Viewer、PythonScript |
| D | 0 | 无 |
| U | 14 | 第 4 节全部 |

本批最终统计为 `A=8, B=7, C=3, D=0, U=14`，合计 32。

有限兼容层优先实现/验证的公共能力：

1. 当前视图选择、输入/输出缓冲区和 `Sci_TextRange` 的严格同步转发；
2. `NPPN_READY/BUFFERACTIVATED/SHUTDOWN` 与常用 `SCN_*` 的来源 HWND、顺序、
   重入和频率；
3. 配置目录、路径、语言、打开/保存以及 session/buffer/view 映射；
4. Toolbar 图标、菜单勾选、状态栏等有限 UI 服务；
5. 对插件异常、后台线程、网络和子进程退出竞态的隔离。

不应为本批插件扩大为完整模拟的范围：

- 原版 HMENU/Win32 Dock 窗口树；
- 允许任意插件 subclass Qt 主窗口或代理 SCI HWND；
- 为 PythonScript 暴露原版全部 Win32/Scintilla 内部结构；
- 为单个键盘插件建立全局通用 Windows Hook 模拟。

## 6. 批次级未验证项

- 所有 32 个包均未在 Windows x86 + Notepad++ v8.4.6 或 Qt 端加载；
- 未逐包核验六个标准导出、Unicode 导出、附带 DLL/运行时和安装后文件布局；
- 未将源码构建产物与 JSON 包 SHA-256 做二进制等价验证；
- 尚未测试插件异常是否能被宿主拦截，in-process DLL 的访问冲突通常不能由普通
  C++ 异常处理可靠恢复；
- `U` 项需要源码 revision 或受控外部行为/API trace；仍然不进行反汇编。
