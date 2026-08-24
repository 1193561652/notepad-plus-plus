# Notepad++ v8.4.6 x86 插件调查：I–M

## 1. 范围与方法

本文件只覆盖 `third_party/nppPluginList/catalog/windows/pl.x86.json` 按 `folder-name`
不区分大小写排序后从 `ImgTag` 到 `MZC8051` 的 23 项。版本、下载地址和
SHA-256 只采用 x86 JSON，不合并 x64/ARM64 条目。

调查日期为 2026-07-30。源码结论以清单版本对应的官方 tag/提交为准；SDK
头文件、生成的完整消息枚举和未被业务代码调用的包装函数不计为“实际使用”。
找不到对应版本源码时不检查 DLL、不反汇编，API 和六导出均记为未核验。本轮没有
在 Windows x86 加载插件，因此等级是静态初评。

等级含义：A 为当前主窗口 HWND 和双 Scintilla 代理模型可直接覆盖；B 为补充
有限、可复用能力后可覆盖；C 为原 DLL 不宜兼容且应通过源码/新 API 移植的重要
插件；D 为低价值高成本、不建议官方支持；U 为证据不足。

## 2. 汇总结论

| 插件 | x86 版本 | 对应版本源码 | 主要风险 | 初评 |
| --- | --- | --- | --- | --- |
| ImgTag | 2.0.1 | 未找到 | 文件对话框和文本插入均未核验 | U |
| IndentByFold | 0.7.3 | 有 | 保存 direct function/pointer | C |
| iTimeTrack | 3.0.0 | 未找到 | 网络、后台计时和隐私边界未核验 | U |
| jN | 2.2.185.8 | 有 | JavaScript/ActiveX/Win32、Dock、direct pointer | C |
| JSFunctionViewer | 1.1.0 | 有 | .NET/WinForms Dock、外部进程 | C |
| JSLintNpp | 0.8.3.119 | 未找到 | JS 运行时和结果 UI 未核验 | U |
| JsMapParser.NppPlugin | 4.2 | 有 | .NET/WinForms Dock、通知驱动解析 | C |
| JSMinNPP | 1.2205.0 | 未定位精确 revision | 文本/Dock/API 仅有其他版本源码 | U |
| JsonTools | 3.2.0 | 有 | WinForms Dock、同步文本缓冲和 CLR 边界 | B |
| LanguageHelp | 1.7.5.0 | 无公开源码 | CHM/HLP/PDF 启动方式未核验 | U |
| lexamples | 1.0.0.0 | 未找到 | 外部 lexer ABI | U |
| LightExplorer | 2.0.0.0 | 未找到 | 文件树 Dock/拖放/监控未核验 | U |
| Linefilter3 | 1.0.0.0 | 未找到 | 自定义结果窗口和文本范围未核验 | U |
| Linter | 0.1.0.0 | 有 | 子进程、实时通知和 indicator | B |
| LocationNavigate | 0.4.8.1 | 未找到 | 高频位置历史/修改跟踪未核验 | U |
| LuaScript | 0.11 | 有 | Lua、全量 SCI API、direct pointer、自建 SCI Dock | C |
| MarkdownViewerPlusPlus | 0.8.2 | 有 | .NET Dock、HTML/PDF、线程池/WebBrowser | C |
| MenuIcons | 1.2.5 | 无公开源码 | 主/上下文菜单内部结构未核验 | U |
| Merge files in one | 1.2.0.0 | 有 | .NET/WinForms、同步读文件 | B |
| mimeTools | 2.8 | 有 | 选择区缓冲区和原生 About 对话框 | A |
| MultiClipboard | 2.1.0.0 | 未找到 | 剪贴板、鼠标/键盘和弹出菜单未核验 | U |
| MusicPlaye_1.0.11x86r | 1.0.0.3 | 有 | .NET Dock、Windows 媒体栈 | B |
| MZC8051 | 0.0.1 | 有 | 编译器子进程、Dock、字符通知 | B |

## 3. 逐项记录

### 3.1 ImgTag

- **清单身份**：`ImgTag` 2.0.1，SHA-256
  `0381685bc8410c301dc492e52c5fc99abdb55368ce18822f67e2fb31cf88886c`；
  SourceForge 包 `ImgTag_binary_unicode_2.0.1.zip`，通过文件选择器插入 HTML
  `IMG` 标签。
- **源码/维护**：未找到与 2.0.1 对应的公开源码；许可证、revision、构建和维护
  状态未知，不反汇编 DLL。
- **API/UI/代理结论**：六导出、`NPPM/NPPN/SCI`、文件对话框参数、路径编码和
  插入方式全部未核验。**U**；若取得精确源码，重点检查选择区替换和原生
  `GetOpenFileName`。未确认其重要性，不列行为复刻候选。

### 3.2 IndentByFold

- **清单身份**：`IndentByFold` 0.7.3，SHA-256
  `125f054921e4acb4226fa04dbe5f6b970bdae366e2bdca0ceeada03a1ac62fed`。
- **源码/许可证/维护**：官方 `ffes/indentbyfold` tag `v0.7.3`，提交
  `dbb4c6fabcee`（2019-11-10）；GPL-2.0。C++/Visual Studio，当前维护低频。
- **实际契约**：六导出；用 `NPPM_GETCURRENTSCINTILLA`、
  `GETCURRENTLANGTYPE`、`GETFULLCURRENTPATH`、`GETCURRENTBUFFERID`、
  `GETPLUGINSCONFIGDIR`、`GETMENUHANDLE`、`MAKECURRENTBUFFERDIRTY`；
  处理 `BUFFERACTIVATED/FILESAVED/LANGCHANGED/READY/SHUTDOWN` 及
  `SCN_UPDATEUI/CHARADDED/AUTOCSELECTION/PAINTED/AUTOCCANCELLED/MODIFIED`。
- **SCI/Win32**：读写 fold level、行缩进、选择、文本范围、自动完成和 undo；
  `Sci_TextRange` 是双向指针结构。初始化对活动 SCI 请求并保存
  `SCI_GETDIRECTFUNCTION`、`SCI_GETDIRECTPOINTER`；仅帮助链接使用
  `ShellExecute`，无 Dock。
- **双代理/初评**：普通同步消息和通知可代理，但代理 HWND 无法返回指向 Qt
  编辑器核心且保持 Scintilla ABI 的 direct function/pointer。**C**，建议用新
  编辑器 API 移植缩进算法；不为单插件暴露伪 direct pointer。未验证双视图切换、
  多选区、通知重入和二进制六导出。

### 3.3 iTimeTrack

- **清单身份**：`iTimeTrack` 3.0.0，SHA-256
  `b40ddd0e9d679d39acc1f731f07a646cf42f404e4470fb30673f091175a73024`；
  将文件工作时间提交至 `itimetrack.com`。
- **源码/维护**：JSON 所列 GitHub 仓库当前不可取得，未找到 3.0.0 对应源码；
  许可证、构建和维护未知。不反汇编发布包。
- **API/外部依赖**：宿主消息、文档/项目映射、计时线程、网络协议、凭据存储、
  离线/退出行为均未核验。**U**；联网和隐私能力不能仅由描述推断为兼容。若后续
  认定重要，先取得源码或公开协议，再决定新 API 实现。

### 3.4 jN

- **清单身份**：`jN` 2.2.185.8，SHA-256
  `55acd2d2e56dd4fcc7c471060ddcf01c4bb46bd627c977d3b5be14b298ad8cd0`。
- **源码/许可证/维护**：官方 `sieukrem/jn-npp-plugin` tag `2.2.185.8`，
  提交 `786ed48d420a`（2022-08-29）；GPL-3.0 或商业许可。包含 JavaScript
  引擎、Win32/ActiveX 扩展和脚本库。
- **实际 API**：处理 `READY/SHUTDOWN/FILEOPENED/FILECLOSED/FILESAVED/`
  `BUFFERACTIVATED/LANGCHANGED/READONLYCHANGED` 及
  `SCN_UPDATEUI/CHARADDED/DOUBLECLICK/MODIFIED/MARGINCLICK/ZOOM` 等；
  使用 `NPPM_DMMREGASDCKDLG/DMMSHOW/DMMHIDE/DMMUPDATEDISPINFO`、
  `GETNBOPENFILES`。编辑封装覆盖文本、行、选择、indicator、可见行等大量 SCI。
- **高风险依赖**：调用并保存 `SCI_GETDIRECTFUNCTION/POINTER`，还直接取得
  `SCI_GETCHARACTERPOINTER`；创建原生 Dock/HTML 对话框，向主窗口发送
  `WM_COMMAND`，脚本可访问 ActiveX 和任意 Win32 API。脚本行为使消息集合原则上
  不封闭。
- **双代理/初评**：有限消息白名单无法给任意脚本提供原版 HWND/ActiveX/SCI
  语义。**C**；若保留脚本能力，应基于新 API 重新设计受控宿主和 UI 服务。
  不模拟完整 Win32 窗口树。未验证脚本调试器、HTML 引擎、异常脚本和退出稳定性。

### 3.5 JSFunctionViewer

- **清单身份**：`JSFunctionViewer` 1.1.0，SHA-256
  `78ed11e72bd10d4decd19ff976637c921318151fefb4b8ef243a34b51a02cde5`。
- **源码/许可证/维护**：官方 `davidsover/nppJSFunctionViewer` tag `v1.1.0`，
  提交 `37cd1f2973`（2021-12-24）；GPL-3.0；C#/.NET Framework，维护低频。
- **实际 API/UI**：`GETPLUGINSCONFIGDIR`、`GETFULLCURRENTPATH`、
  `ADDTOOLBARICON`、`DMMREGASDCKDLG/DMMSHOW/DMMHIDE`；
  `SCI_GETSELTEXT` 使用插件分配的非托管输出缓冲。处理
  `FILEBEFORELOAD/BEFORESHUTDOWN/SHUTDOWN`；WinForms Dock、剪贴板，并以
  `Process.Start("notepad++.exe", ...)` 打开外部来源文件。
- **双代理/初评**：SCI 文本读回可同步转发，但 WinForms Dock 的 HWND 所有权和
  强编码宿主进程名不是纯代理问题。**C**，建议复用 JS 分析逻辑并用新面板/文档
  API 移植。未验证外部文件定位、两个视图、32 位 CLR 加载和进程启动失败。

### 3.6 JSLintNpp

- **清单身份**：`JSLintNpp` 0.8.3.119，SHA-256
  `8306c76dbbb7ea07f22b613ddbedc60f1eaf74c286136bba92acd593a6557336`。
- **源码/维护**：未找到与 SourceForge 0.8.3.119 对应、可核验的源码；许可证、
  JSLint 运行时版本和维护状态未知，不反汇编。
- **API/代理结论**：文档读取、结果标记、错误窗口、脚本运行时和线程均未核验。
  **U**；后续需精确源码，或在 Windows x86 只做受控消息追踪。不能因功能类似
  Linter 就复用其结论。

### 3.7 JsMapParser.NppPlugin

- **清单身份**：`JsMapParser.NppPlugin` 4.2，SHA-256
  `a42f6f1496a652b37b47bb4d304e6b1dbf22bb74181a345b3b391016a73e8d7c`。
- **源码/许可证/维护**：官方 `megaboich/js-map-parser` tag `4.2`，提交
  `6a0da15115c7`（2017-12-29）；未找到明确顶层许可证，C#/.NET Framework，
  已长期无发布。
- **实际 API**：`GETCURRENTSCINTILLA`、`GETFULLCURRENTPATH`、
  `GETPLUGINSCONFIGDIR`、`GETNPPVERSION`、`SETMENUITEMCHECK`、
  `ADDTOOLBARICON`、`DMMREGASDCKDLG/DMMSHOW/DMMHIDE`；SCI 使用
  `GETCURRENTPOS/LINEFROMPOSITION/GETCOLUMN/POSITIONFROMLINE/GOTOPOS/`
  `GRABFOCUS`。处理 `READY/TBMODIFICATION/FILESAVED/BUFFERACTIVATED/`
  `SHUTDOWN` 和 `SCN_CHARADDED`。
- **UI/代理/初评**：解析树是 WinForms Dock，通知触发重新解析，跳转依赖焦点。
  文本读取方式还需第二轮核验。普通消息可代理，但跨平台面板和 .NET UI 不可复用。
  **C**，建议移植解析核心并重写面板。未验证许可证、计时器重入、大文件和解析异常。

### 3.8 JSMinNPP

- **清单身份**：`JSMinNPP`（显示名 JSTool）1.2205.0，SHA-256
  `a5e6c812f0d53159e0099acede148cb2f2fa71dcd9567b2d73cb4acde31c28f7`。
- **源码状态**：找到 `sunjw/jstoolnpp` 公开 GPL-2.0 仓库，但远端没有
  1.2205.0 tag，当前 revision 不能作为 2022 清单版本证据。因此对应版本源码
  记为“未定位”，不把当前源码中的 Dock、toolbar 或 SCI 调用归给 1.2205.0。
- **结论**：六导出、精确 API、JSON 视图、线程和附带运行时均未核验，**U**。
  下一步需从发布归档定位 source snapshot/commit；JSTool 使用广泛时可标重点，
  但在补证据前不定 C。

### 3.9 JsonTools

- **清单身份**：`JsonTools` 3.2.0，SHA-256
  `8371a76462adb1afd231a3f655a16e66a0097ae799178d4a5a2388ae51c052fd`。
- **源码/许可证/维护**：官方 `molsonkiko/JsonToolsNppPlugin` tag `v3.2.0`，
  提交 `7d9a94388adb`（2022-09-19）；Apache-2.0；项目持续维护。C#/.NET。
- **实际宿主能力**：`GETPLUGINSCONFIGDIR`、`GETFULLCURRENTPATH`、
  `GETFULLPATHFROMBUFFERID`、`GETCURRENTBUFFERID`、`GETBUFFERLANGTYPE`、
  `SETCURRENTLANGTYPE`、`MENUCOMMAND`、`DMMREGASDCKDLG`、
  `SETMENUITEMCHECK`；处理 `FILEBEFORECLOSE` 和 `SCN_CHARADDED`。
- **SCI/UI**：排除 NppPlugin.NET 网关中未被业务代码调用的通用包装后，对应版本
  实际使用全文读写、追加和跳转等有限同步消息；WinForms JSON Tree Dock、文件
  对话框和 CLR 导出桥仍需宿主处理。未发现业务代码调用
  `SCI_GETDIRECTFUNCTION/POINTER`。
- **双代理/初评**：同步文本缓冲可由代理覆盖，主要增量是有限 Dock 生命周期和
  CLR/WinForms 加载边界。按重点插件深查结论统一为 **B**；Windows 可先验证旧
  DLL，跨平台仍应将解析/查询核心接入新文档和面板 API。未验证大 JSON、
  正则/查询超时、CLR 异常隔离和六导出。

### 3.10 LanguageHelp

- **清单身份**：`LanguageHelp` 1.7.5.0，SHA-256
  `be410c694abeef2469d34c98c16ec39387650275907cb4966cd5503618f20afd`。
- **源码状态**：维护者仓库 `francostellari/NppPlugins` 只提供
  `LanguageHelp_dll_1v75_x32.zip` 等二进制包，未找到对应源码；许可证未知，
  不反汇编。
- **API/Win32/结论**：描述确认按光标关键字启动 CHM/HLP/PDF，并可注入主菜单
  或上下文菜单；具体 `NPPM/SCI`、Shell/HTML Help API 和菜单结构均未核验。
  **U**。若后续认定重要，可按可观察行为复刻；不能据描述认定菜单代理可覆盖。

### 3.11 lexamples

- **清单身份**：`lexamples` 1.0.0.0，SHA-256
  `0703a38b16c574562159526e909588a98ee76b3213d9f01b0089ab70283c101f`；
  Makefile 和 MIB/ASN.1 外部 lexer 包。
- **源码/ABI**：未找到精确公开源码；许可证、导出符号、Lexilla/Scintilla ABI、
  `SCI_LOADLEXERLIBRARY` 路径和通知均未核验，不反汇编。
- **结论**：双代理窗口不能单独解决外部 lexer 动态库 ABI，**U**。需先取得源码
  和导出契约，再判断直接纳入 Lexilla、源码移植或不支持。

### 3.12 LightExplorer

- **清单身份**：`LightExplorer` 2.0.0.0，SHA-256
  `0c6f915ac926e12acdc6bff30307549cb074f97bebd04205a350cced8931463a`。
- **源码状态**：未找到 SourceForge 2.0 对应源码；许可证、维护、六导出未核验。
- **依赖/结论**：公开描述只确认 dockable 文件浏览器；树控件、shell 图标、
  文件监控、拖放、上下文菜单、`NPPM_DOOPEN` 等均不能由描述视为实际调用。
  **U**；不反汇编。若取得源码，优先评价 UI/文件系统服务而非 SCI 代理。

### 3.13 Linefilter3

- **清单身份**：`Linefilter3` 1.0.0.0，SHA-256
  `7adbed33c702c57dffa40aab356d7f549fc5b7d2852cfd9b05c3681b041dc0ea`。
- **源码状态**：未找到对应公开仓库/源码；许可证、维护状态、六导出未知。
- **API/UI/结论**：描述只确认过滤文本并显示到新窗口；文本读取编码、增量更新、
  自定义窗口类型和线程均未核验。**U**；需要精确源码，不能按普通选择区插件
  推断为 A/B。

### 3.14 Linter

- **清单身份**：`Linter` 0.1.0.0，SHA-256
  `5ebbd8d01a2342e1916974a15e10280fecf61c41bf073d269d15a5c3fa316b49`。
- **源码/许可证/维护**：官方 `deadem/notepad-pp-linter` tag `v0.1.0.0`，
  提交 `91a82793ce7a`（2019-01-27）；MIT；C++，维护停止/低频。
- **实际 API**：`NPPM_GETCURRENTSCINTILLA/GETCURRENTDIRECTORY/GETFILENAME/`
  `GETEXTPART/GETPLUGINSCONFIGDIR/DOOPEN`；处理 `READY/BUFFERACTIVATED/`
  `SHUTDOWN` 及 `SCN_MODIFIED/UPDATEUI/PAINTED/FOCUSIN/FOCUSOUT`。
  SCI 使用 `GETTEXT/GETLENGTH/GETLINE/LINELENGTH/POSITIONFROMLINE` 和
  indicator 配置/填充/清除。
- **进程/代理/初评**：用 `CreateProcess` 运行用户配置的 checkstyle 兼容
  linter，并读取输出；未发现 direct pointer、Dock 或窗口 subclass。**B**：
  有限 SCI/通知可代理，另需稳定、可取消的进程服务和退出清理。未验证长任务并发、
  输出编码、恶意命令配置、双视图和高频启动节流。

### 3.15 LocationNavigate

- **清单身份**：`LocationNavigate` 0.4.8.1，SHA-256
  `c4ebd8eaef1a82ee7668980488726d9335ef1d5f5c7b22afa992d6a603156734`。
- **源码状态**：未找到 SourceForge 清单版本源码；许可证和维护未知。
- **API/风险/结论**：功能描述确认需要持续记录浏览/修改点并跨会话调整位置，但
  `SCN_MODIFIED/UPDATEUI` 字段、buffer ID、文件切换、marker/indicator 和状态
  持久化均未核验。**U**；不反汇编。后续 Windows 消息追踪必须特别验证高频通知
  顺序、关闭文档和两视图来源。

### 3.16 LuaScript

- **清单身份**：`LuaScript` 0.11，SHA-256
  `ab523219ba00844cc88f92d0a3edce1148f404d3d2786df0a7256fc25760eec2`。
- **源码/许可证/维护**：官方 `dail8859/LuaScript` tag `v0.11`，提交
  `066aeb615c08`（2021-03-15）；GPL-2.0；内置 Lua 和 SciTE Lua 扩展层。
- **实际能力**：向脚本暴露大范围 `NPPM_*`、`NPPN_*`、`SCI_*` 和
  `SCN_*`；自身处理文档/语言/关闭等宿主通知并提供 Console Dock。初始化取得并
  保存 `SCI_GETDIRECTFUNCTION/POINTER`，脚本接口覆盖文本、样式、lexer、
  indicator、marker、自动完成等几乎全部 Scintilla 能力。
- **Win32/UI**：创建额外 Scintilla 控件作为 console，注册原生 Dock，修改控件
  window style，自身对话框使用 subclass；Lua 扩展还允许发送任意数值消息。
- **双代理/初评**：固定白名单、Qt SCI 映射与返回函数指针都无法满足原 DLL；
  脚本可调用集合也不是封闭的。**C**；作为重要脚本插件，应移植 Lua 运行时并在
  新 API 上明确能力/线程/安全边界。未验证脚本 ABI、用户脚本兼容率和崩溃隔离。

### 3.17 MarkdownViewerPlusPlus

- **清单身份**：`MarkdownViewerPlusPlus` 0.8.2，SHA-256
  `c62cce888f6a580c2e958480bd6bbed0e2c80f15ec1c422643650ed1eb1d0e3c`。
- **源码/许可证/维护**：官方 `nea/MarkdownViewerPlusPlus` tag `0.8.2`，
  提交 `1a3dddc52591`（2018-04-29）；MIT；C#/.NET，版本线已老旧。
- **实际 API**：`GETCURRENTSCINTILLA/GETPLUGINSCONFIGDIR`、
  `ADDTOOLBARICON`、`DMMREGASDCKDLG/DMMSHOW/DMMHIDE`、
  `SETMENUITEMCHECK`；处理 `TBMODIFICATION/SHUTDOWN/BUFFERACTIVATED` 和
  `SCN_UPDATEUI/MODIFIED`。通过 SCI 获取当前文本/位置，具体调用子集需复核。
- **UI/线程/依赖**：WinForms Dock，HtmlRenderer/PdfSharp，导出 HTML/PDF，
  `WebBrowser` 打印；线程池异步载入本地图片，并启动外部文件/URL。
- **双代理/初评**：通知和文本读取可代理，但原生 Dock、HTML/PDF 栈、打印与异步
  图片生命周期不应成为旧兼容层职责。**C**；重要插件应复用 Markdown 逻辑并用
  新面板/导出服务移植。未验证大文档节流、图片线程退出、WebBrowser/COM 和双视图。

### 3.18 MenuIcons

- **清单身份**：`MenuIcons` 1.2.5，SHA-256
  `f503e1398703997dbdbdebdb659c99e490d01493d9ef1a121e9e7f15ec60824c`。
- **源码状态**：`francostellari/NppPlugins` 提供精确 x86 二进制和主题资源，
  未找到源码；许可证、六导出和 revision 未核验，不反汇编。
- **风险/结论**：描述确认它给主菜单和上下文菜单设置图标，因此行为依赖菜单对象，
  但 `NPPM_GETMENUHANDLE`、Win32 `HMENU` API、菜单遍历和重载时机均不能在没有
  源码时断言。**U**。Qt 菜单不是原版 Win32 菜单树；若需求成立，应原生实现主题
  功能，不为该 DLL 模拟整个菜单结构。

### 3.19 Merge files in one

- **清单身份**：`Merge files in one` 1.2.0.0，SHA-256
  `ea58934446fb94fca1c1d39db53a91f1ccb76ec17957782ebd8a90a134ee47f7`。
- **源码/许可证/维护**：官方 `gurikbal/Merge-files-in-one` tag `1.2.0.0`，
  提交 `71a370632c1d`（2019-11-03）；未找到明确许可证；C#/.NET，维护停止。
- **实际 API/UI**：`GETCURRENTSCINTILLA/GETPLUGINSCONFIGDIR/GETFILENAME/`
  `MENUCOMMAND(IDM_FILE_NEW)`；`SCI_GETLENGTH/GETTEXT/SETTEXT` 使用
  `StringBuilder/string` 输出输入缓冲；处理 `TBMODIFICATION/SHUTDOWN`，另有
  自定义的 buffer activated 路径。WinForms 模态 UI，同步遍历文件。
- **双代理/初评**：有限消息和缓冲可同步代理，UI 作为 Windows 子对话框可运行。
  **B**；补齐 Unicode 缓冲和菜单命令即可试验旧 DLL，跨平台更适合按新文档服务
  小规模重写。未验证大文件内存、异常文件、无许可证风险和二进制通知行为。

### 3.20 mimeTools

- **清单身份**：`mimeTools` 2.8，SHA-256
  `551e46d64e953fd9d9e7d4ab7e01a69fd4deadd13238ac6a093bcb877726ed07`。
- **源码/许可证/维护**：官方 `npp-plugins/mimetools` tag `v2.8`，提交
  `eedd053a3f68`（2022-05-13）；GPL-3.0；C/C++，Notepad++ 官方生态维护。
- **实际 API**：只用 `NPPM_GETCURRENTSCINTILLA` 选择视图；SCI 为
  `GETSELECTIONSTART/END/GETSELTEXT`、`TARGETFROMSELECTION`、
  `GETTARGETTEXT`、`SETTARGETSTART/END`、`REPLACETARGET`、`SETSEL`。
  输入/输出均为插件分配的字节缓冲。未发现实际 `NPPN_*`、direct pointer、
  Dock、线程或子进程。
- **UI/代理/初评**：仅 About 资源对话框；Base64/URL/quoted-printable/SAML
  逻辑与宿主无关。**A**：同步指针缓冲转发即可覆盖 Windows 原 DLL，跨平台也易于
  重编译。未验证空/矩形/多选择区、超大缓冲、无效编码输入和六导出二进制。
- **2026-08-02 动态验证**：Qt 兼容层已处理 `NPPM_GETCURRENTSCINTILLA` 及上述九个
  `SCI_*`。官方 2.8 x64 DLL 的 Base64 Encode 在主/副永久视图通过，URL Encode
  在主视图通过；六导出、安装收据、插件分配字节缓冲、target replacement 和
  选择区恢复均已覆盖。空/矩形/多选择区及超大缓冲仍未覆盖。

### 3.21 MultiClipboard

- **清单身份**：`MultiClipboard` 2.1.0.0，SHA-256
  `fba2177939eae03056b0baeb724fd73faabb95298cca4beea91fe0bc19c3df56`。
- **源码状态**：未找到 SourceForge 2.1 对应源码；许可证、维护、六导出未知。
- **风险/结论**：描述确认 10 个历史缓冲、Ctrl/Shift+V、中键和弹出菜单；具体
  clipboard format、键盘/鼠标 hook、SCI 替换、窗口 subclass、线程和退出清理
  均未核验。**U**；不反汇编。若源码仍缺失但功能被认定重要，可考虑 Qt 剪贴板
  历史的行为复刻，不能为此默认允许任意 hook。

### 3.22 MusicPlaye_1.0.11x86r

- **清单身份**：目录 `MusicPlaye_1.0.11x86r`，显示名 MusicPlayer，JSON 版本
  1.0.0.3，发布包名指向 1.0.11，SHA-256
  `0961a0238198fd2245b39a2303483aa0ddb028532db26dd6e0101ee0e5c406fb`。
- **源码/许可证/维护**：官方 `gallettube/MusicPlayer` revision
  `e9b4add2e3e0`，源码/包版本标识 1.0.11、程序集 1.0.0.*；无 tag，和 JSON
  的精确构建号不能完全对应。未找到明确许可证；最后提交 2018-02-02。
- **实际 API/UI**：`GETPLUGINSCONFIGDIR`、`ADDTOOLBARICON`、
  `DMMREGASDCKDLG/DMMSHOW`，处理 `TBMODIFICATION/SHUTDOWN`；WinForms Dock。
  未发现实际 SCI 文本操作。媒体播放依赖 Windows/.NET 媒体组件，格式支持为
  WAV/MP3/AIFF/WMA。
- **双代理/初评**：Windows 上只需有限 Dock/toolbar 生命周期，**B**；跨平台不应
  复用 Windows 媒体实现，且低重要度可暂缓。因 revision 不精确，等级需 Windows
  包实测确认。未验证 COM/codec、暂停关闭、音频线程和资源释放。

### 3.23 MZC8051

- **清单身份**：`MZC8051` 0.0.1，SHA-256
  `fd8b7aeb3e6bfb0467b3fb46b4a01beb660a6a54d6f9867191982c6c6fa8a8e9`。
- **源码/许可证/维护**：官方 `Jiangshan00001/npp_MZC8051` tag `0.0.1`，
  提交 `4bafbcf78706`（2020-08-19）；GPL-3.0；C++，维护低频，附 8051 编译工具。
- **实际 API**：`GETPLUGINSCONFIGDIR/DOOPEN/GETCURRENTLANGTYPE/`
  `GETCURRENTSCINTILLA/MODELESSDIALOG/DMMREGASDCKDLG`；处理 `SHUTDOWN` 和
  `SCN_CHARADDED`。SCI 使用 `GETCURRENTPOS/GETTEXTRANGE`、
  `BEGIN/ENDUNDOACTION/REPLACESEL/SETSEL`，另有 `GOTOLINE/ENSUREVISIBLE`。
- **进程/UI/代理**：通过 `ShellExecuteEx` 运行编译器/命令，打开生成的 make、
  main 和 log 文件；有原生 Dock/GoToLine 对话框。`Sci_TextRange` 需同步结构体
  写回，未见 direct pointer。
- **初评**：**B**（Windows 原 DLL）：有限消息、Dock、通知和进程启动可复用实现；
  跨平台需核验编译器可移植性后再决定源码移植。未验证工具链来源、命令注入、
  子进程退出、路径编码、自动插入重入和两个视图。

## 4. 批次级结论与后续验证

### 4.1 兼容层需求

- A/B 插件共同需要：活动视图查询、同步 SCI 输入/输出缓冲、`Sci_TextRange`、
  配置目录、`DOOPEN`、菜单命令、toolbar、有限 Dock/modeless 生命周期，以及
  `NPPN_*`/`SCN_*` 的来源和顺序转换。
- 进程服务应覆盖 Linter 和 MZC8051 的启动、输出/退出、取消和宿主关闭清理；不要
  把任意 shell 执行混入文档服务。
- `SCI_GETDIRECTFUNCTION/POINTER` 是 IndentByFold、jN、LuaScript 的明确阻断项。
  不应返回跨 ABI 的伪函数/对象指针，也不应让脚本绕过宿主服务边界。
- Dock 能力应作为新旧 API 共用核心服务，但不能因此模拟 Notepad++ 全部 Win32
  窗口层级。Qt 跨平台插件应使用新面板 API。

### 4.2 Windows x86 必测

1. 在 Notepad++ v8.4.6 原版记录六导出、命令数组、通知顺序和主/副视图行为。
2. 在双代理宿主分别验证 mimeTools、Linter、Merge files in one、MusicPlayer 和
   MZC8051；对所有指针输出加入长度、空指针和异常保护。
3. 对 C 级 DLL 验证加载前静态能力拒绝或加载失败隔离；尤其禁止执行 direct
   function/pointer、任意脚本消息或不受控窗口 subclass 后继续运行。
4. 对所有 U 项先找精确源码；无源码插件只做公开行为和受控消息追踪，不反汇编。
5. 验证宿主退出时 Dock、CLR、线程池、子进程、媒体组件和脚本运行时是否能在
   卸载前停止；不兼容插件的异常不得使主程序退出崩溃。

### 4.3 证据索引

除 x86 JSON 外，本批次的源码证据为以下公开仓库/tag：`ffes/indentbyfold`
`v0.7.3`、`sieukrem/jn-npp-plugin` `2.2.185.8`、
`davidsover/nppJSFunctionViewer` `v1.1.0`、`megaboich/js-map-parser` `4.2`、
`molsonkiko/JsonToolsNppPlugin` `v3.2.0`、`deadem/notepad-pp-linter`
`v0.1.0.0`、`dail8859/LuaScript` `v0.11`、
`nea/MarkdownViewerPlusPlus` `0.8.2`、`gurikbal/Merge-files-in-one`
`1.2.0.0`、`npp-plugins/mimetools` `v2.8` 和
`Jiangshan00001/npp_MZC8051` `0.0.1`。MusicPlayer 使用无 tag revision
`e9b4add2e3e0`，故已明确保留版本不确定性；LanguageHelp/MenuIcons 仓库只作为
“公开发行物无源码”证据。
