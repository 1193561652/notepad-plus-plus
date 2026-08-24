# Notepad++ v8.4.6 重点插件调查（仅 x86）

## 1. 范围和方法

本批次只以 `third_party/nppPluginList/catalog/windows/pl.x86.json` 为清单基线，不比较 x64 或
ARM64。调查对象为 ComparePlus、DSpellCheck、HexEditor、JsonTools、
NPPJSONViewer 和 XMLTools。

调查遵循以下边界：

- 版本、SHA-256、下载地址和兼容区间只取 x86 JSON。
- 源码只使用清单版本对应的官方 tag/revision；未进行反汇编，也没有用较新版本
  的实现替代基线版本。
- API 清单是对应 revision 实际业务源码的静态调用摘要，排除了 SDK 头文件中
  “仅定义、未调用”的常量。
- 维护状态核对到 2026-07-30，以官方仓库公开历史为依据。
- 本轮未下载或运行插件二进制，六个导出、构建可复现性、运行时行为和崩溃边界
  仍需 Windows x86 实测。

等级 A/B/C/D/U 评价的是指导文档规定的“Qt 主窗口原生 HWND＋两个 Scintilla
代理 HWND”在 **Windows 上兼容原 DLL** 的可行性。跨平台路线单独记录；某插件
在 Windows 初评为 B，不表示其 DLL 能在 Linux/macOS 运行。

## 2. 汇总结论

| 插件 | x86 清单版本 | 源码定位 | Windows 代理初评 | 跨平台建议 | 核心阻断点 |
| --- | --- | --- | --- | --- | --- |
| ComparePlus | 1.0.0 | `cp_1.0.0` / `cd943fa` | C | 新 API 源码移植 | `SCI_GETDIRECTFUNCTION/POINTER`、主窗口/标签栏/状态栏 subclass、双视图深度控制 |
| DSpellCheck | 1.4.24 | `v1.4.24` / `59ddfbb` | C | 新 API 源码移植 | 主窗口 subclass、坐标/上下文菜单、后台下载、WinInet、原生 UI |
| HEX-Editor | 0.9.12.0 | `0.9.12` / `67350aa` | C | 新 API 自定义二进制视图 | 动态创建额外 Scintilla、主窗口/控件 subclass、独立十六进制视图语义 |
| JSON Tools | 3.2.0 | `v3.2.0` / `7d9a943` | B | 复用解析核心，重写 UI/宿主层 | .NET Framework 4、WinForms Dock、托管/非托管导出桥 |
| JSON Viewer | 1.41 | `v1.41` / `f857389` | B | 复用 RapidJSON 逻辑，重写 Qt 树视图 | 原生 Dock/TreeView 和 Win32 对话框 |
| XML Tools | 3.1.1.13 | `3.1.1.13` / `de76065` | B | 新 API 移植并替换 XML 引擎/UI | MSXML 6 COM、MFC 属性页、原生对话框 |

优先级判断：

1. ComparePlus 和 HEX-Editor 不应推动旧兼容层去模拟 Scintilla direct-call 或完整
   Notepad++ 窗口结构。
2. DSpellCheck 的文本标记能力可以成为新 API 的公共需求，但主窗口 subclass、
   异步网络和 Win32 UI 不应进入通用兼容层。
3. JSON Viewer、JsonTools、XMLTools 可以先作为 Windows Dock、同步缓冲区和常用
   通知的兼容层样本；其跨平台版本仍应改源码。

## 3. ComparePlus

### 3.1 身份、源码和维护

| 项目 | 结论 |
| --- | --- |
| x86 清单 | `ComparePlus` 1.0.0，SHA-256 `d8640fca448c6f739837a8b9245172884290dd73812782b988b4ab013329e290` |
| 包 | `ComparePlus_1.0.0_x86.zip`，兼容区间 `[8.4.2,]` |
| 官方源码 | [pnedev/comparePlus](https://github.com/pnedev/comparePlus)，tag [`cp_1.0.0`](https://github.com/pnedev/comparePlus/tree/cp_1.0.0)，commit `cd943fa`（2022-09-01） |
| 许可证 | GPL-3.0，见该 tag 的 `license.txt` |
| 维护状态 | 活跃；官方仓库 2026 年仍有代码提交 |
| 主要依赖 | Win32 common controls；Diff against Git/SVN 随包带 `libgit2.dll`、`sqlite.dll`；C++ 工作线程 |
| 构建 | 有 CMake 和 VS2017 工程；x86 基线未在本轮复现 |

### 3.2 宿主和编辑器调用

主要 `NPPM_*`：

- 文档/视图：`NPPM_GETCURRENTSCINTILLA`、`NPPM_GETCURRENTBUFFERID`、
  `NPPM_GETFULLCURRENTPATH`、`NPPM_GETFULLPATHFROMBUFFERID`、
  `NPPM_GETBUFFERLANGTYPE`、`NPPM_SETBUFFERLANGTYPE`、`NPPM_DOOPEN`。
- 主程序控制：`NPPM_MENUCOMMAND`、`NPPM_GETMENUHANDLE`、
  `NPPM_HIDETABBAR`、`NPPM_GET/SETLINENUMBERWIDTHMODE`、
  `NPPM_SETMENUITEMCHECK`。
- UI：`NPPM_DMMREGASDCKDLG`、`NPPM_ADDTOOLBARICON_FORDARKMODE`、
  `NPPM_GETENABLETHEMETEXTUREFUNC`。

主要通知：

- `NPPN_READY`、`NPPN_BUFFERACTIVATED`、`NPPN_FILEBEFORECLOSE`、
  `NPPN_FILESAVED`、`NPPN_LANGCHANGED`、`NPPN_WORDSTYLESUPDATED`、
  `NPPN_DARKMODECHANGED`、`NPPN_TBMODIFICATION`、
  `NPPN_BEFORESHUTDOWN`、`NPPN_SHUTDOWN`。
- `SCN_MODIFIED`、`SCN_UPDATEUI`、`SCN_PAINTED`、`SCN_MARGINCLICK`、
  `SCN_ZOOM`。

主要 `SCI_*`：

- **高风险 direct-call**：`SCI_GETDIRECTFUNCTION` 和两个视图分别调用
  `SCI_GETDIRECTPOINTER`，随后以函数指针和文档私有指针直接调用 Scintilla。
- 双视图同步：`SCI_GET/SETZOOM`、`SCI_GET/SETFIRSTVISIBLELINE`、
  `SCI_VISIBLEFROMDOCLINE`、`SCI_DOCLINEFROMVISIBLE`、`SCI_LINESONSCREEN`。
- 差异呈现：marker、annotation、indicator、fold 和 margin 消息族。
- 文本和编辑：`SCI_GETTEXT`、`SCI_SETTEXT`、`SCI_INSERTTEXT`、
  `SCI_DELETERANGE`、`SCI_APPENDTEXT`、selection、line/position 消息族。

### 3.3 Win32、Dock 和线程

- 在 `Compare.cpp` 中替换 Notepad++ 主窗口 WndProc，并直接修改主/副标签栏与
  状态栏的 window style；还依赖原版菜单句柄和命令 ID。
- Navigation Bar 用 `NPPM_DMMREGASDCKDLG` 注册原生 Dock，并创建 Win32
  scrollbar；设置、颜色选择、About、正则忽略等均为资源对话框。
- 比较引擎使用 `std::thread` 并行处理；进度窗另用 `CreateThread`，用
  `PostMessage` 回到原生窗口。

### 3.4 双代理评价和路线

两个代理能处理普通同步 `SCI_*` 和缓冲区写回，但不能安全返回 Qt/QScintilla
内部的 Scintilla direct function/pointer。即使伪造函数指针，也必须保持每个
视图、文档切换、重入和调用约定一致，等同暴露内部实现。主窗口、标签栏和状态栏
subclass 又要求原版窗口层级，不符合有限兼容层边界。

- 初评：**C**
- 推荐：保留比较算法和 Git/SVN 业务逻辑，以新 API 重写文档配对、差异标记、
  双视图同步、Dock 和进度 UI。
- 不应扩大的兼容范围：direct-call 仿真、原版标签栏/状态栏 HWND、任意主窗口
  subclass。
- 未验证：x86 包所带依赖版本、线程取消/关闭行为、全部 FuncItem 和快捷键。

## 4. DSpellCheck

### 4.1 身份、源码和维护

| 项目 | 结论 |
| --- | --- |
| x86 清单 | `DSpellCheck` 1.4.24，SHA-256 `d037d6995b9df66bff0d1b32d40b9331cc985b01029f788619a57ae4a9e58c96` |
| 包 | `DSpellCheck_x86.zip`；x86 条目未声明 N++ 兼容区间 |
| 官方源码 | [Predelnik/DSpellCheck](https://github.com/Predelnik/DSpellCheck)，tag [`v1.4.24`](https://github.com/Predelnik/DSpellCheck/tree/v1.4.24)，commit `59ddfbb`（2022-05-18） |
| 许可证 | GPL-2.0，见 `License.txt` |
| 维护状态 | 活跃；官方仓库 2026 年仍有维护提交 |
| 主要依赖 | Hunspell、可选 Aspell、win-iconv、minizip、FTP client、WinInet、WinSock、COM/native spell service |
| 构建 | CMake 带依赖源码；x86 基线未在本轮复现 |

### 4.2 宿主和编辑器调用

主要 `NPPM_*`：

- `NPPM_GETCURRENTSCINTILLA`、`NPPM_GETFULLCURRENTPATH`、
  `NPPM_GETNBOPENFILES`、`NPPM_GETOPENFILENAMES*`、
  `NPPM_ACTIVATEDOC`、`NPPM_SWITCHTOFILE`。
- `NPPM_GETPLUGINSCONFIGDIR`、`NPPM_GETNPPDIRECTORY`、
  `NPPM_GETCURRENTDIRECTORY`、`NPPM_DOOPEN`。
- `NPPM_ALLOCATECMDID`、`NPPM_ADDTOOLBARICON`、
  `NPPM_SETMENUITEMCHECK`、`NPPM_MODELESSDIALOG`。

通知为 `NPPN_READY`、`NPPN_SHUTDOWN`、`NPPN_BUFFERACTIVATED`、
`NPPN_LANGCHANGED`、`NPPN_TBMODIFICATION`，以及 `SCN_MODIFIED`、
`SCN_UPDATEUI`、`SCN_ZOOM`、`SCN_FOLDINGSTATECHANGED`。

主要 `SCI_*`：

- 拼写范围和文本：`SCI_GETTEXT`、`SCI_GETTEXTRANGE`、`SCI_GETLINE`、
  `SCI_GETSELTEXT`、`SCI_FINDTEXT`、selection/line/position 消息族。
- 标记：indicator current/style/color/value/fill/clear；少量 marker。
- 替换：target、`SCI_REPLACETARGET`、`SCI_REPLACESEL`、undo action。
- 鼠标定位：`SCI_CHARPOSITIONFROMPOINT[CLOSE]`、
  `SCI_POINTX/YFROMPOSITION`、`SCI_TEXTHEIGHT`。
- 未发现业务源码使用 `SCI_GETDIRECTFUNCTION` 或 `SCI_GETDIRECTPOINTER`。

### 4.3 Win32、线程和外部服务

- 用 `SetWindowSubclass` subclass 主窗口，拦截/合成上下文菜单；suggestion button
  依赖编辑器 HWND、坐标、焦点和 `PostMessage`。
- 设置、词典下载、移除词典、语言选择、Aspell 配置等均是 Win32 原生对话框和
  控件。
- 词典下载使用异步任务、WinInet/FTP、进度回调和窗口消息；native speller
  路径调用 COM 初始化。
- 高频 `SCN_MODIFIED/UPDATEUI` 驱动拼写扫描与 indicator 更新，必须控制关闭期
  任务取消和通知重入。

### 4.4 双代理评价和路线

文本读取、范围替换和 indicator 可成为通用 Host Services；但上下文菜单体验依赖
主窗口 subclass、编辑器窗口坐标和异步消息。若为旧 DLL 支持这些行为，兼容层会
从消息转发扩张到 Qt/Win32 输入、菜单和窗口生命周期耦合。词典下载线程和网络
错误也扩大了崩溃面。

- 初评：**C**
- 推荐：源码移植；复用 Hunspell/Aspell 和分词业务逻辑，使用新 API 的文本快照、
  indicator、事件和主线程调度，网络与对话框改用 Qt。
- 不应扩大的兼容范围：任意主窗口 subclass、Qt 编辑器坐标的 Win32 伪装、
  WinInet 代理。
- 未验证：异步任务是否跨线程直接发送宿主/SCI 消息、关闭等待上界、安装包内
  字典/运行时布局。

## 5. HEX-Editor

### 5.1 身份、源码和维护

| 项目 | 结论 |
| --- | --- |
| x86 清单 | `HexEditor` 0.9.12.0，SHA-256 `266be61339df9d7a781ca6f5853ac03db1e35c3c6c1c8bcb2349c619f981c781` |
| 包 | `HexEditor_0.9.12_Win32.zip`；x86 条目未声明 N++ 兼容区间 |
| 源码 | [chcg/NPP_HexEdit](https://github.com/chcg/NPP_HexEdit)，tag [`0.9.12`](https://github.com/chcg/NPP_HexEdit/tree/0.9.12)，commit `67350aa`（2022-02-02） |
| 来源说明 | 仓库 README 明确称其为 SourceForge 原源码的非官方 GitHub 仓库；它仍是清单 homepage/release 仓库 |
| 许可证 | GPL-2.0，见 `HexEditor/license.txt` |
| 维护状态 | 活跃；该仓库 2026 年仍有维护提交 |
| 构建 | VS2003 工程及 Win32 资源；基线构建未复现 |

### 5.2 宿主和编辑器调用

主要 `NPPM_*`：

- `NPPM_CREATESCINTILLAHANDLE` / `NPPM_DESTROYSCINTILLAHANDLE` 被多处使用，
  用于临时/子编辑器，而不只是两个宿主视图。
- `NPPM_ENCODESCI` / `NPPM_DECODESCI`、`NPPM_GETCURRENTSCINTILLA`、
  `NPPM_GETCURRENTDOCINDEX`、`NPPM_GETOPENFILENAMES*`、`NPPM_DOOPEN`。
- `NPPM_GETPLUGINSCONFIGDIR`、`NPPM_SETSTATUSBAR`、
  `NPPM_SETMENUITEMCHECK`、`NPPM_ADDTOOLBARICON_DEPRECATED`。
- 还调用非公开 `NPPM_INTERNAL_GETMENU`，不应纳入公共兼容白名单。

主要通知：

- `NPPN_READY`、`NPPN_FILEOPENED`、`NPPN_FILECLOSED`、
  `NPPN_TBMODIFICATION`。
- `SCN_MODIFIED`、`SCN_SAVEPOINTREACHED`。

主要 `SCI_*` 是文本、selection、target/search/replace、undo、clipboard 和
line/position 消息族；多处使用 `SCI_GETTEXTRANGE` 等指针结构。未发现
`SCI_GETDIRECTFUNCTION/POINTER`，但插件会取得并操纵额外真实 Scintilla HWND。

### 5.3 原生视图和窗口依赖

- 核心 HEXDialog 是 owner-data Win32 ListView，自行呈现地址、十六进制和文本列；
  它不是宿主文本编辑器的简单显示模式。
- `setInfo` 后 subclass Notepad++ 主窗口；HEXDialog、list、编辑框、combo 和
  hyperlink 均有 WndProc subclass。
- 自建查找/替换、转到、pattern、column、compare、options 等对话框；直接处理
  list notification、custom draw、剪贴板、键盘和焦点。
- 额外 Scintilla HWND 用于编码转换、搜索和模式输入，生命周期与原生子窗口相连。

### 5.4 双代理评价和路线

两个固定代理只能表示主/副文本视图，无法满足“按需创建并返回真实 Scintilla
HWND”的调用，也无法表达独立二进制文档视图。实现这些旧消息会把宿主演化为
Scintilla 窗口工厂和完整 Win32 控件宿主，且仍不能解决二进制文档模型。

- 初评：**C**
- 推荐：按新 API 实现自定义二进制编辑视图；复用十六进制解析、搜索和编辑逻辑，
  不把二进制内容伪装为普通文本。
- 不应扩大的兼容范围：`NPPM_INTERNAL_GETMENU`、任意 Scintilla HWND 创建、
  原版控件层级或 subclass。
- 未验证：大文件策略、编码转换边界、undo/保存一致性、异常关闭的数据恢复。

## 6. JSON Tools（JsonTools）

### 6.1 身份、源码和维护

| 项目 | 结论 |
| --- | --- |
| x86 清单 | `JsonTools` 3.2.0，SHA-256 `8371a76462adb1afd231a3f655a16e66a0097ae799178d4a5a2388ae51c052fd` |
| 包 | `Release_x86.zip`，兼容区间 `[8.4.1,]` |
| 官方源码 | [molsonkiko/JsonToolsNppPlugin](https://github.com/molsonkiko/JsonToolsNppPlugin)，tag [`v3.2.0`](https://github.com/molsonkiko/JsonToolsNppPlugin/tree/v3.2.0)，commit `7d9a943`（2022-09-19） |
| 许可证 | Apache-2.0，见 `LICENSE.md` |
| 维护状态 | 活跃；官方仓库 2026 年仍持续提交 |
| 运行时/依赖 | C#、.NET Framework 4.0、System.Windows.Forms、System.Drawing；NppPlugin.NET DllExport/Mono.Cecil 构建桥 |
| 构建 | VS C# 工程明确提供 x86 target；未在本轮复现导出和打包 |

### 6.2 宿主和编辑器调用

排除 NppPlugin.NET 自动生成的完整常量表后，3.2.0 业务代码实际范围较小：

- `NPPM_GETPLUGINSCONFIGDIR`。
- `NPPM_GETCURRENTSCINTILLA`（基础设施在访问时选择主/副句柄）。
- `NPPM_DMMREGASDCKDLG`、`NPPM_SETMENUITEMCHECK`。
- 通知：`NPPN_TBMODIFICATION`、`NPPN_FILEBEFORECLOSE`、`NPPN_SHUTDOWN`。
- 实际编辑器方法映射为 `SCI_GETLENGTH`、`SCI_GETTEXT`、`SCI_SETTEXT`、
  `SCI_APPENDTEXT`、`SCI_GOTOLINE`；均为同步调用，其中 GET/SET TEXT 带托管与
  原生缓冲区封送。
- 未发现业务代码使用 Scintilla direct function/pointer。

插件把 JSON tree 注册为 WinForms 原生 Dock；解析、lint、pretty print、
JSON Lines、RemesPath 和 schema 逻辑均为托管 C#。本版本未发现后台线程或
子进程的业务调用。

### 6.3 双代理评价和路线

两个代理可覆盖该版本实际使用的同步 SCI 子集；主要 Windows 兼容工作是保持
指针缓冲区封送、通知时机和让 WinForms HWND 能作为 Qt Dock 的原生子窗口。
这属于数量有限的复用能力，因此 Windows 初评不是 C。不过 CLR 加载、DllExport
和 WinForms 都不是跨平台 API 的可接受长期边界。

- 初评：**B**
- Windows 路线：在隔离进程/异常保护策略明确后，验证 .NET 导出加载和原生 Dock。
- 跨平台路线：复用 Apache-2.0 的 parser/RemesPath 业务思想或源码，重写为项目
  选定语言与 Qt UI；不能把 .NET Framework/WinForms 纳入新 ABI。
- 未验证：六导出实际生成结果、CLR 异常是否越过导出边界、Dock 关闭顺序、
  大文档全量 `GETTEXT/SETTEXT` 的延迟和内存峰值。

## 7. JSON Viewer（NPPJSONViewer）

### 7.1 身份、源码和维护

| 项目 | 结论 |
| --- | --- |
| x86 清单 | `NPPJSONViewer` 1.41，SHA-256 `d7da0e00102ca11d30114727ef623e38baa14f05321be694fe83e71a44f766a5` |
| 包 | `NPPJSONViewer_Win32.zip`；x86 条目未声明 N++ 兼容区间 |
| 官方源码 | [kapilratnani/JSON-Viewer](https://github.com/kapilratnani/JSON-Viewer)，tag [`v1.41`](https://github.com/kapilratnani/JSON-Viewer/tree/v1.41)，commit `f857389`（2022-09-05） |
| 许可证 | MIT，见 `LICENSE` |
| 维护状态 | 活跃；官方仓库 2026 年仍有提交 |
| 主要依赖 | 内嵌 RapidJSON、Win32 TreeView/Dock/资源对话框 |
| 构建 | VS 工程和 Makefile；x86 基线未在本轮复现 |

### 7.2 API 和 UI

实际宿主范围：

- `NPPM_GETCURRENTSCINTILLA`、`NPPM_DMMREGASDCKDLG`、
  `NPPM_MODELESSDIALOG`、`NPPM_SETCURRENTLANGTYPE`。
- `NPPN_SHUTDOWN`。
- `SCI_GETLENGTH`、`SCI_GETSELECTIONSTART/END`、`SCI_GETSELTEXT`、
  `SCI_REPLACESEL`、`SCI_SETSEL`、`SCI_GETEOLMODE`、
  `SCI_GETTABWIDTH`、`SCI_GETUSETABS`。
- 未发现 direct function/pointer、后台线程或子进程。

JSON tree 是原生 Dock 对话框中的 Win32 TreeView；About/hyperlink 等控件有
WndProc subclass。格式化会读取选区、替换文本并把当前语言设为 JSON。

### 7.3 双代理评价和路线

同步 selection/replace 消息和标量设置可以由代理完整写回。Windows 的主要新增
能力是一个受控的原生 Dock 容器及 `NPPM_DMMREGASDCKDLG` 生命周期，而不是模拟
Scintilla 内部。Dock 子窗口自身的 Win32 subclass 不要求宿主模拟 Notepad++
控件层级。

- 初评：**B**
- Windows 路线：把它作为原生 Dock、选区缓冲区和关闭通知的最小验证样本。
- 跨平台路线：保留 RapidJSON/树构建逻辑，使用 Qt tree/dock 和新文档 API。
- 未验证：TreeView 大数据性能、重复显示/隐藏、关闭时窗口销毁顺序、全部命令和
  快捷键。

## 8. XML Tools

### 8.1 身份、源码和维护

| 项目 | 结论 |
| --- | --- |
| x86 清单 | `XMLTools` 3.1.1.13，SHA-256 `9521d91be847a9c9fcfc6cb6ea5455fd7dfe840f4f12a8fd95e5137116dbd6c3` |
| 包 | `XMLTools-3.1.1.13-x86.zip`；x86 条目未声明 N++ 兼容区间 |
| 官方源码 | [morbac/xmltools](https://github.com/morbac/xmltools)，tag [`3.1.1.13`](https://github.com/morbac/xmltools/tree/3.1.1.13)，commit `de76065`（2022-03-31） |
| 许可证 | GPL-3.0，见 `LICENSE` |
| 维护状态 | 活跃；官方仓库 2026 年仍有代码提交 |
| 主要依赖 | MSXML 6 COM DOM/SAX/XSLT、MFC/ATL、QuickXml/SimpleXml/StringXml |
| 构建 | VS2019 `v142`、Windows 10 SDK、Unicode；x86 基线未在本轮复现 |

### 8.2 宿主和编辑器调用

主要 `NPPM_*`：

- 文档：`NPPM_GETCURRENTSCINTILLA`、`NPPM_GETCURRENTBUFFERID`、
  `NPPM_GETFULLPATHFROMBUFFERID`、`NPPM_GETCURRENTDOCINDEX`、
  `NPPM_GETNBOPENFILES`、`NPPM_ACTIVATEDOC`。
- 路径/编码/语言：`NPPM_GETPLUGINHOMEPATH`、
  `NPPM_GETPLUGINSCONFIGDIR`、`NPPM_GETBUFFERENCODING`、
  `NPPM_SETBUFFERENCODING`、`NPPM_GET/SETCURRENTLANGTYPE`。
- UI：`NPPM_MENUCOMMAND`、`NPPM_SETSTATUSBAR`、
  `NPPM_ADDTOOLBARICON_FORDARKMODE`。

通知：

- `NPPN_READY`、`NPPN_SHUTDOWN`、`NPPN_BUFFERACTIVATED`、
  `NPPN_FILEOPENED`、`NPPN_FILEBEFORECLOSE`、
  `NPPN_FILEBEFORESAVE`、`NPPN_TBMODIFICATION`。
- `SCN_CHARADDED`（tag autoclose）、`SCN_MODIFIED`、`SCN_UPDATEUI`。

主要 `SCI_*`：

- 全文/选区/行读取与替换：`SCI_GETTEXT`、`SCI_GETTEXTRANGE`、
  `SCI_GETLINE`、`SCI_GETSELTEXT`、`SCI_SETTEXT`、`SCI_REPLACESEL`。
- selection/caret/line/scroll：`SCI_SETSEL`、`SCI_GOTOLINE/POS`、
  `SCI_SETFIRSTVISIBLELINE`、`SCI_SETXOFFSET`、`SCI_SHOWLINES`。
- annotation 用于错误报告；另有 EOL、tab、wrap、margin 和 undo 消息。
- 未发现业务源码使用 Scintilla direct function/pointer。

### 8.3 Windows/XML 引擎依赖

- `MSXMLHelper.cpp` 直接 `CoCreateInstance` MSXML 6 的 DOM、FreeThreaded DOM、
  SAX reader、XSL template 和 schema cache；验证、XPath、XSLT 和错误对象均用
  COM 接口。
- Options 使用 MFC property grid；XPath、XSLT、schema file、error report、
  About 等是原生对话框。
- 没有 Dock，也未发现业务后台线程。宿主交互主要是同步消息和通知。
- MSXML 的 external entity、DTD、XSLT script、URL retrieval 等选项具有安全
  含义；跨平台替换解析器时不能只比较 pretty-print 输出。

### 8.4 双代理评价和路线

Windows 原 DLL 的宿主消息都能由有限同步代理表达，COM/MFC 依赖由插件自身承担，
所以 Windows 初评为 B；这不要求宿主模拟 MSXML。跨平台则完全不同：MSXML 不能
移植，Qt UI 也不能承载 MFC 对象，必须替换 XML engine 并建立行为语料。

- 初评：**B**
- Windows 路线：补齐列出的文档、编码、状态栏、annotation 和通知白名单后实测。
- 跨平台路线：新 API 源码移植；选择跨平台 XML/XSD/XPath/XSLT 栈，并按
  3.1.1.13 的安全选项和错误定位建立兼容测试。
- 不应扩大的兼容范围：在宿主实现 COM/MSXML 或 MFC 仿真。
- 未验证：包内 VC runtime/附带 DLL、COM apartment 初始化、外部实体和网络访问
  默认值、异常 COM 错误能否越过插件边界。

## 9. 公共宿主能力建议

由这六个插件共同证明、适合进入新 API/有限兼容层候选的能力：

- 按主/副/当前视图进行同步文本、选区、line/position、target/replace 调用。
- 显式长度的输入/输出缓冲区封送和 UTF-8/宿主编码转换。
- 文档枚举、buffer ID/路径、激活视图、语言和编码。
- marker、indicator、annotation 的受控分配和清理。
- `NPPN_READY/SHUTDOWN/BUFFERACTIVATED/FILE*` 与常用 `SCN_*` 的顺序转换。
- 命令勾选、工具栏图标、状态栏和受控 Dock 注册。
- 主线程调度、可取消后台任务和关闭超时（主要由 DSpellCheck/ComparePlus 证明）。
- 自定义编辑视图（HEX-Editor 证明需求，但应只存在于新 API）。

明确不进入公共兼容层：

- `SCI_GETDIRECTFUNCTION`、`SCI_GETDIRECTPOINTER` 或返回 QScintilla 内部指针。
- 任意 Notepad++/标签栏/状态栏/Scintilla WndProc subclass 的兼容承诺。
- 非公开 `NPPM_INTERNAL_*`。
- 为插件创建任意真实 Scintilla HWND。
- WinInet、COM、MFC、WinForms 或 CLR 作为宿主 API。

## 10. 后续 Windows x86 验证项

- [ ] 对六个清单包复核 SHA-256、目录布局、附带 DLL 和 PE x86 架构。
- [ ] 核验六个标准导出、Unicode 返回值、FuncItem 数量/名称/快捷键。
- [ ] 在原版 Notepad++ v8.4.6 x86 建立行为基线和关闭顺序日志。
- [x] 对 B 类验证 JSON Viewer 的 Dock/选区和 JsonTools CLR/Dock。
- [ ] 验证 XMLTools 的编码、annotation、tag autoclose 和 COM 异常。
- [ ] 对 C 类只做安全加载/拒绝、异常隔离和诊断验证；不得因“能显示菜单”改判。
- [ ] 验证插件初始化、通知和命令异常不会让宿主崩溃；记录可恢复和必须重启情形。
- [ ] 验证高频通知性能、指针缓冲区越界保护、关闭超时和后台任务取消。

## 11. 主要证据

| 编号 | 证据 | 支持范围 | 限制 |
| --- | --- | --- | --- |
| E-01 | `third_party/nppPluginList/catalog/windows/pl.x86.json` | 版本、包、SHA、兼容区间 | 不证明运行兼容 |
| E-02 | [ComparePlus `cp_1.0.0`](https://github.com/pnedev/comparePlus/tree/cp_1.0.0) | direct-call、双视图、Dock、subclass、线程 | 未运行 x86 包 |
| E-03 | [DSpellCheck `v1.4.24`](https://github.com/Predelnik/DSpellCheck/tree/v1.4.24) | indicator、通知、subclass、网络和依赖 | 未运行 x86 包 |
| E-04 | [NPP_HexEdit `0.9.12`](https://github.com/chcg/NPP_HexEdit/tree/0.9.12) | 额外 Scintilla、HEX view、原生 UI | 仓库自述为原源码镜像/延续 |
| E-05 | [JsonTools `v3.2.0`](https://github.com/molsonkiko/JsonToolsNppPlugin/tree/v3.2.0) | .NET/WinForms、实际窄 API 集、Dock | 2026-08-09 已验证 CLR4、格式化/压缩、Settings、RemesPath、JSON Lines、YAML、树跳转和 4 MB 阈值；`Run tests` 仍有上游绝对路径缺陷 |
| E-06 | [JSON Viewer `v1.41`](https://github.com/kapilratnani/JSON-Viewer/tree/v1.41) | RapidJSON、Dock、selection API | README 标题仍写 1.40，结论以 tag 源码为准 |
| E-07 | [XMLTools `3.1.1.13`](https://github.com/morbac/xmltools/tree/3.1.1.13) | MSXML、MFC UI、宿主/SCI 调用 | 未运行 COM 场景 |

### 11.1 JsonTools 后续复核

2026-08-09 对官方 x86/x64 包、六导出、CLR 元数据和当前 Qt 适配层进行了复核。
除 6.2 原记录外，可达业务路径还需要 `NPPM_GETFULLCURRENTPATH`、
`NPPM_GETFILENAME`、`NPPM_MENUCOMMAND(IDM_FILE_NEW)`、`NPPM_DOOPEN` 和
`SCI_GOTOPOS`。源码中的 `JsonGrepper` 网络/线程代码在 v3.2.0 菜单中不可达。
等级仍为 B；有限增量、真实 DLL 和深层功能矩阵均已实施，完整结果见
`jsontools-v320-compatibility-evaluation.md`。
