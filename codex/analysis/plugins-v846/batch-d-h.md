# Notepad++ v8.4.6 x86 插件调查：D–H

## 1. 范围与方法

本文件只覆盖 `third_party/nppPluginList/catalog/windows/pl.x86.json` 按 `folder-name`
不区分大小写排序后从 `DarkTheme` 到 `HugeFiles` 的 23 项。版本、下载地址和
SHA-256 均以该 JSON 为准；未合并 x64/ARM64 条目。

调查日期为 2026-07-30。源码结论来自清单版本对应的官方 tag/提交；只出现在 SDK
头文件中的常量不算作插件使用。没有找到对应版本源码时不检查 DLL、导入表或进行
反汇编，API 项统一记为“未核验”。本轮也没有在 Windows x86 上加载插件，所以
所有等级都是静态初评。

等级沿用调查模板：A 为现有主窗口和双 Scintilla 代理模型可直接覆盖，B 为补充
有限公共能力后可覆盖，C 为重要插件应源码移植/新 API 重写，D 为低价值高成本且
不建议官方支持，U 为证据不足。

## 2. 汇总结论

| 插件 | x86 版本 | 对应版本源码 | 主要风险 | 初评 |
| --- | --- | --- | --- | --- |
| DarkTheme | 1.0 | 有 | subclass 主窗口、主题绘制拦截 | D |
| dbgpPlugin | 0.0.13.27 | 未找到 | DBGP 网络、IDE UI 均未核验 | U |
| DiscordRPC | 1.4.262.1 | 有 | Discord Game SDK、后台线程 | A |
| DoxyIt | 0.4.4 | 有 | 编辑通知、原生设置对话框 | B |
| DSpellCheck | 1.4.24 | 有 | 高频通知、指示器、复杂 UI/词典 | C |
| ElasticTabstops | 1.3.1 | 有 | 高频重算、Tab stop/marker/indicator | B |
| EnhanceAnyLexer | 1.1.3 | 有 | direct function/pointer、窗口类名 | C |
| ERPHelper | 1.1.2 | 有 | .NET/WinForms Dock、Saxon、SOAP | C |
| Explorer | 1.9.5.0 | 有 | 主窗口 subclass、复杂 Dock/控件/拖放 | C |
| ExtSettings | 1.2.1 | 未找到 | Scintilla 设置范围未核验 | U |
| FallingBricks | 1.1.0.0 | 未找到 | 原生游戏窗口实现未核验 | U |
| FileSwitcher | 1.0.3.0 | 未找到 | 文档枚举、键盘/焦点实现未核验 | U |
| FingerText | 0.5.60 | 未找到对应版本 | snippet/hotspot 与按键接管未核验 | U |
| FWDataViz | 2.6.1.0 | 有 | direct function/pointer、复杂 Dock/视图 | C |
| GedcomLexer | 0.5.0.170 | 未找到 | 外部 lexer ABI | U |
| GitSCM | 1.4.7.1 | 有 | Dock、窗口 hook、子进程 | C |
| GmodLua | 1.5 | 未找到 | 外部 lexer ABI | U |
| GOnpp | 1.2.0.0 | 未找到 | gocode/go 子进程、自动完成 | U |
| GotoLineCol | 2.4.2.0 | 有 | Dock/原生 UI、命令行通知 | B |
| GrepBugsPluginNpp | 1.0.0 | 有 | .NET 4、网络、全文件枚举 | B |
| HexEditor | 0.9.12.0 | 有 | 自建 Scintilla/二进制编辑模型 | C |
| HTMLTag_unicode | 1.3.5.0 | 未定位精确提交 | 文本范围、标签解析 API 未核验 | U |
| HugeFiles | 0.1.1.0 | 有 | .NET/WinForms Dock、大文件分块 | B |

## 3. 逐项记录

### 3.1 DarkTheme

- **清单身份**：`DarkTheme`，1.0，SHA-256
  `704eb5438934a11fb0b47396a2bbd1d4bc2e6365a6be6bee01606be386e4a1ec`；
  功能是反转默认浅色主题的颜色。
- **源码状态**：有，官方
  `MyTDT-Mysoft/DarkTheme-Npp-Plugin` tag `v1.0`，提交
  `6e69810f0d0e`；FreeBASIC 源码，许可证未明确核验。仓库同时提交了 DLL，
  本调查只读 `DarkTheme.bas`。
- **契约/API**：实现六个原版导出；确认使用
  `NPPM_GETPLUGINSCONFIGDIR`、`NPPM_GETCURRENTLANGTYPE`、
  `NPPM_SETCURRENTLANGTYPE`、`NPPM_SETMENUITEMCHECK`、
  `NPPM_ADDTOOLBARICON`，处理 `NPPN_READY`、`NPPN_TBMODIFICATION`。
- **Win32/Dock/SCI**：未确认 SCI 消息或 Dock；启用时用
  `SetWindowLongPtr(GWLP_WNDPROC)` 替换主窗口过程，并在自定义 `WndProc`
  中处理主题/绘制消息。主窗口 HWND 不是单纯消息目的地。
- **双 HWND 评价**：两个 Scintilla 代理对核心功能没有帮助；Qt 主窗口 HWND
  的 subclass 会绕过 Qt 事件边界，且主题绘制对象不是原版 Win32 控件树。
- **初评**：**D**。功能价值较低而 hook 风险高；不应为它模拟原版窗口绘制
  结构。Windows 可单独验证 DLL 是否能安全拒绝/禁用，跨平台不建议官方移植。
- **未验证**：所有命令、具体被截获消息、禁用时恢复窗口过程、异常卸载。

### 3.2 dbgpPlugin

- **清单身份**：`dbgpPlugin`，0.0.13.27，SHA-256
  `9ab506a6eaa765b152ff1b7886e2173a7537788a205ea932cf92c08b4e2093b0`；
  SourceForge 说明其通过 DBGP/XDebug 把 Notepad++ 变成 PHP 调试 IDE。
- **源码状态**：未找到与 0.0.13.27 对应、可核验的官方源码；许可证、六导出、
  构建和维护状态均未核验。不反汇编清单 DLL。
- **API/Win32/Dock**：`NPPM_*`、`NPPN_*`、`SCI_*`、网络线程、断点 UI 和
  Dock 用法全部未核验。功能说明只能确认存在 DBGP 网络依赖，不能据此推断接口。
- **双 HWND 评价与初评**：证据不足，**U**。后续需找到精确源码或在 Windows
  x86 通过消息追踪黑盒验证；如果仍无源码但被认定重要，可列行为复刻候选。

### 3.3 DiscordRPC

- **清单身份**：`DiscordRPC`，1.4.262.1，SHA-256
  `b1522d3f2eca45508d13013dc0462c4f3c697bac49ffa75a903e1f7fd98f7c1d`；
  向 Discord 显示当前编辑文件。
- **源码状态**：有，官方 `Zukaritasu/notepadpp_rpc` tag `v1.4`，提交
  `c287cd6b819d`；GPL-3.0。版本资源的第四段构建号由构建生成，tag 是清单
  1.4.262.1 的发布源码基线。
- **契约/API**：实现六导出；实际确认的宿主 API 只有
  `NPPM_GETCURRENTLANGTYPE`，通知只处理 `NPPN_SHUTDOWN`。未确认 SCI、
  Dock、窗口层级检查或 subclass。
- **Win32/外部依赖**：使用 Discord Game SDK、Win32 `HANDLE`/线程、
  `PathFileExists` 和文件状态 API；线程调用 Discord 回调，不跨线程调用 SCI。
- **双 HWND 评价**：主窗口代理只需同步实现一个语言查询；Scintilla 代理未被
  使用。关闭通知必须先停止线程并销毁 Discord core。
- **初评**：**A**（仅 Windows 原 DLL）。跨平台应重编译/替换系统与 Discord
  SDK 封装，但不需要扩大旧插件消息层。
- **未验证**：六导出和命令数组的二进制实测、Discord 未运行时行为、退出竞态。

### 3.4 DoxyIt

- **清单身份**：`DoxyIt`，0.4.4，SHA-256
  `9ed3a7c101114c9c7498fa31478157f805e16fb7e0225fa0622e0d41295344ae`；
  生成 Doxygen 注释。
- **源码状态**：有，官方 `dail8859/DoxyIt` tag `v0.4.4`，提交
  `bcef24a32b65`；GPL-3.0。
- **契约/API**：六导出；确认
  `NPPM_GETCURRENTSCINTILLA`、`GETPLUGINSCONFIGDIR`、`GETCURRENTLINE`、
  `GETCURRENTLANGTYPE`、`GETLANGUAGENAME`、`GETFILENAME`、
  `SETMENUITEMCHECK`、`MODELESSDIALOG`；处理 `NPPN_READY`、
  `BUFFERACTIVATED`、`LANGCHANGED`、`SHUTDOWN` 及 `SCN_UPDATEUI`、
  `CHARADDED`、`FOCUSIN`、`FOCUSOUT`。
- **SCI/UI**：确认在两个 SCI HWND 上配置 indicator
  `SCI_INDICSETSTYLE/ALPHA/OUTLINEALPHA`；文本读取/替换由源码包装函数完成，
  尚需第二轮列出全部消息。使用资源对话框、modeless 注册和对话框内 hyperlink
  control subclass；没有确认 subclass Scintilla 或主窗口。
- **双 HWND 评价**：同步 SCI 转发和通知来源区分可覆盖编辑功能；原生对话框可
  作为 Windows 插件子窗口运行。需保证高频 `SCN_UPDATEUI/CHARADDED` 时序。
- **初评**：**B**。补齐有限文本/indicator 消息和 modeless 生命周期后可测试
  原 DLL；跨平台可复用解析逻辑并重写设置 UI。
- **未验证**：完整 SCI 包装调用集、Unicode/多选区、命令数组、通知重入。

### 3.5 DSpellCheck

- **清单身份**：`DSpellCheck`，1.4.24，SHA-256
  `d037d6995b9df66bff0d1b32d40b9331cc985b01029f788619a57ae4a9e58c96`；
  Hunspell/Aspell 拼写检查、词典管理和拼写 UI。
- **源码状态**：有，官方 `Predelnik/DSpellCheck` tag `v1.4.24`，提交
  `59ddfbb4ada4`；GPL-3.0。详细专项结论由重点插件调查文件维护，本节只给批次
  广度结论。
- **确认 API**：大量文档/路径/菜单消息，包括 `NPPM_GETCURRENTSCINTILLA`、
  `GETFULLCURRENTPATH`、`GETOPENFILENAMES*`、`GETPLUGINSCONFIGDIR`、
  `DOOPEN`、`SWITCHTOFILE`、`MODELESSDIALOG`、`ADDTOOLBARICON`；
  处理 `READY`、`BUFFERACTIVATED`、`LANGCHANGED`、`TBMODIFICATION`、
  `SHUTDOWN` 以及 `SCN_MODIFIED/UPDATEUI/ZOOM`。
- **SCI/UI**：确认文本范围/样式读取、indicator/marker、查找、坐标换算和
  编辑消息；使用原生设置/词典 UI、上下文菜单与下载功能。未在首版发现
  `SCI_GETDIRECTFUNCTION/POINTER` 的实际调用。
- **双 HWND 评价**：消息本身大多可转发，但高频增量拼写、指示器资源冲突、
  popup/坐标语义、词典线程和复杂 UI 使“能加载”远低于功能兼容。
- **初评**：**C**。属于重要复杂插件，应优先基于新 API 移植源码；Windows
  原 DLL 兼容仅作为受限实验。
- **未验证**：以重点专项调查为准，特别是线程、Aspell DLL、词典下载与右键 UI。

### 3.6 ElasticTabstops

- **清单身份**：`ElasticTabstops`，1.3.1，SHA-256
  `4ed9a765181e8398c0b5acf465281451a5388a7f35277324f9d5a297011d5c4e`。
- **源码状态**：有，官方 `dail8859/ElasticTabstops` tag `v1.3.1`，提交
  `66042eefd8da`；GPL-3.0。
- **契约/API**：六导出；确认 `NPPM_GETCURRENTSCINTILLA`、
  `GETPLUGINSCONFIGDIR`、`GETEXTPART`、`GETFULLPATHFROMBUFFERID`、
  `DOOPEN`、`SETMENUITEMCHECK`；处理 `READY`、`BUFFERACTIVATED`、
  `FILESAVED`、`SHUTDOWN` 和 `SCN_MODIFIED/UPDATEUI/ZOOM`。
- **SCI/UI**：确认 `SCI_GETLINECOUNT`、`CLEARTABSTOPS`，以及 margin、
  marker、indicator 配置；核心计算代码还通过 Scintilla 包装读取行和测量文本。
  只有 About/config 原生对话框；hyperlink subclass 限于自身对话框控件。
- **双 HWND 评价**：无 direct pointer 或编辑器 subclass 证据；稳定映射活动
  SCI HWND、完整转发 tab-stop 消息和高频通知即可覆盖主要算法。
- **初评**：**B**。有限 SCI 白名单可兼容 Windows DLL；跨平台源码移植成本也
  较低。未验证大文档性能、两个视图同时更新和 marker/indicator ID 冲突。

### 3.7 EnhanceAnyLexer

- **清单身份**：`EnhanceAnyLexer`，1.1.3，SHA-256
  `69a548dc25abf416fa0bcea9c3182853c07901005a544a880075f9bb7cd5e416`。
- **源码状态**：有，官方 `Ekopalypse/EnhanceAnyLexer` tag `v1.1.3`，提交
  `1bf2191656de`；MIT；V 语言和 MinGW 构建。
- **契约/API**：六导出；确认文档/视图/语言/路径/菜单消息：
  `GETCURRENTVIEW`、`GETPLUGINSCONFIGDIR`、`GETBUFFERLANGTYPE`、
  `GETLANGUAGENAME`、`GETFULLPATHFROMBUFFERID`、`GETCURRENTBUFFERID`、
  `GETCURRENTDOCINDEX`、`GETBUFFERIDFROMPOS`、`DOOPEN`、`MENUCOMMAND`、
  `GETNPPVERSION`、`SETMENUITEMCHECK`；处理 `READY`、`FILESAVED`、
  `BUFFERACTIVATED`、`LANGCHANGED`、`SHUTDOWN` 和
  `SCN_UPDATEUI/MODIFIED/MARGINCLICK`。
- **高风险能力**：`setInfo` 立即对两个 SCI HWND 请求
  `SCI_GETDIRECTFUNCTION` 和 `SCI_GETDIRECTPOINTER` 并长期保存；扫描中还使用
  `SCI_GETRANGEPOINTER`。另用 `FindWindowExW` 查找类名
  `splitterContainer` 并依赖其可见性判断单/双视图。
- **双 HWND 评价**：普通代理 HWND 不能返回真实、可调用的 Scintilla direct
  function/pointer，也不应伪造原版窗口类层级；这是硬阻断。
- **初评**：**C**。算法和配置可移植，但必须改为 Host Editor 服务/安全范围
  读取；不应为该插件暴露 Qt 编辑器内部指针或模拟 `splitterContainer`。
- **未验证**：V runtime/GC 跨 DLL 生命周期、正则性能和 indicator 资源冲突。

### 3.8 ERPHelper

- **清单身份**：`ERPHelper`，1.1.2，SHA-256
  `b58e77a96bc89e9951869c2b0dc83db7025ba92b2394a8d36ab07f398c82191a`；
  XSL 转换和 Workday SOAP 辅助。
- **源码状态**：有，官方 `swhitley/ERPHelper` tag `v1.1.2`，提交
  `50cd302494fc`；许可证未明确核验。C#/.NET 插件，依赖
  NppPluginNET、Saxon-HE、WinForms 和网络服务。
- **契约/API**：NppPluginNET 生成六导出。业务代码确认
  `NPPM_DMMREGASDCKDLG`、`NPPM_DMMSHOW`、`NPPM_MENUCOMMAND`；SCI gateway
  使用全文/target range、`SCI_SETTEXT`、annotation、goto line 等。通知回调为空。
- **Win32/Dock/线程**：WinForms `Handle` 直接注册为 N++ Dock 子窗口；启动
  后台线程预热 Saxon，表单执行 Workday SOAP 调用。原生窗口、CLR 和网络生命周期
  都超出双 SCI 转发。
- **双 HWND 评价与初评**：编辑文本可由代理覆盖，但 WinForms Dock 与运行时是
  主难点，**C**。若被认定常用，应保留 Saxon/SOAP 业务逻辑，使用新进程/网络
  服务和 Qt 面板重写；不扩张 legacy Dock 到完整 WinForms 宿主。
- **未验证**：包内依赖、凭据存储、TLS/代理、关闭后台线程和错误隔离。

### 3.9 Explorer

- **清单身份**：`Explorer`，1.9.5.0，SHA-256
  `e6e4a2388bbc1203e01de9b868a8de429b4b0712535987dd2a4ec29301d14bdd`；
  文件浏览、收藏、会话和拖放面板。
- **源码状态**：有，官方 `oviradoi/npp-explorer-plugin` tag `v1.9.5`，提交
  `1c4fa22b4edf`；GPL-3.0。
- **宿主 API**：确认 Dock/打开/会话/菜单/路径/文件枚举等广泛消息：
  `DMMREGASDCKDLG`、`DMMVIEWOTHERTAB`、`DOOPEN`、`GETFULLCURRENTPATH`、
  `GETCURRENTDIRECTORY`、`GETNBOPENFILES`、`GETOPENFILENAMES`、
  `GETNBSESSIONFILES`、`GETSESSIONFILES`、`LOADSESSION`、
  `SAVECURRENTSESSION`、`MENUCOMMAND`、`MODELESSDIALOG`、
  `MSGTOPLUGIN`；处理文件打开/关闭、READY、工具栏和样式通知。
- **Win32/Dock**：在主窗口上 `SetWindowLongPtr(GWLP_WNDPROC)` 做 subclass；
  自建 Dock、TreeView/ListView/Toolbar/Rebar、系统 image list、context menu、
  shell 操作、COM/OLE 拖放，并对自身树控件再 subclass。
- **双 HWND 评价**：主要问题不是 SCI，而是主窗口 hook 和庞大的原生控件层；
  Qt 主 HWND 无法提供原版窗口过程及 Dock 内部结构。
- **初评**：**C**。文件浏览器很有价值，应以新 UI/文件服务移植；不应为它模拟
  N++ 原生主窗口、Rebar、TreeView 或 COM 控件树。
- **未验证**：所有消息的参数细节、网络路径、符号链接、shell 扩展和卸载恢复 hook。

### 3.10 ExtSettings

- **清单身份**：`ExtSettings`，1.2.1，SHA-256
  `5a6d2de1d6d8327c144a3e7a48a3eab79747bfdd662941fe6bf82cf0d1d6a969`；
  设置偏好页未提供的 Scintilla 选项。
- **源码状态/API**：清单指向 SourceForge 二进制包；未找到 1.2.1 对应源码，
  不反汇编。六导出、具体 `SCI_*`、通知、原生 UI 和保存方式均未核验。
- **双 HWND 评价与初评**：功能表明会访问 SCI，但不能据描述推断消息集合，
  **U**。后续优先找源码；若只涉及可复用设置消息，可能降为 A/B。

### 3.11 FallingBricks

- **清单身份**：`FallingBricks`，1.1.0.0，SHA-256
  `5bace8e94c194f2d9a916cfc63d2e0bdbd4cab157de9cf0b3970a44cc4acf37b`；
  独立 Tetris 类小游戏对话框。
- **源码状态/API**：未找到对应版本源码；不反汇编。除功能说明可确认有可暂停的
  UI 窗口外，宿主消息、窗口/定时器和关闭行为均未核验。
- **双 HWND 评价与初评**：SCI 很可能不是核心，但证据不足，**U**。若后续证实
  仅六导出和普通模态窗口，可再评 A；本轮不因“看起来简单”越级。

### 3.12 FileSwitcher

- **清单身份**：`FileSwitcher`，1.0.3.0，SHA-256
  `c539b6e19d3524eea8df5fa02f2fefe84104e2b2170159555a00ec9a3715cc56`；
  按文件名、路径或 tab index 键盘切换 buffer。
- **源码状态/API**：未找到清单版本源码；不反汇编。文档枚举/激活、快捷键、
  对话框、焦点和 tab 顺序的具体实现均未核验。
- **双 HWND 评价与初评**：**U**。需确认是否只用
  `GETOPENFILENAMES/SWITCHTOFILE`，还是 hook 键盘、标签栏或主窗口。

### 3.13 FingerText

- **清单身份**：`FingerText`，0.5.60，SHA-256
  `eed1470179624edae57b29ba0971bfa22e568ab51d2896edc90d5ac968fa88c5`；
  支持多 hotspot 的 snippet 插件，并建议把 Tab 从 Scintilla 原命令解绑。
- **源码状态/API**：公开项目存在，但本轮未取得可证明与 0.5.60 完全对应的源码
  快照；因此按未核验处理，不混用其他版本结论，也不反汇编。
- **双 HWND 评价与初评**：多选区/hotspot、Tab 键接管、文本替换和通知时序均
  未核验，**U**。这是潜在常用插件，找到精确源码后应优先补查。

### 3.14 FWDataViz

- **清单身份**：`FWDataViz`，2.6.1.0，SHA-256
  `9f87480d23d343fb1feac837558a348340c083c983894fbf9fc4f5de3fcb4dc5`；
  固定宽度数据的字段、折叠、跳转、复制/提取及可视化面板。
- **源码状态**：有，官方 `shriprem/FWDataViz` tag `v2.6.1.0`，提交
  `fc6f39025578`；GPL-3.0。
- **宿主 API**：确认 `ALLOCATECMDID`、`DMMREGASDCKDLG/SHOW/HIDE`、
  `DOOPEN`、`GETCURRENTSCINTILLA`、`GETFULLCURRENTPATH`、
  `GETFULLPATHFROMBUFFERID`、`GETMENUHANDLE`、`GETPLUGINHOMEPATH`、
  `GETPLUGINSCONFIGDIR`、暗色/编辑器颜色/版本消息以及工具栏和菜单状态；
  处理 READY、BUFFERACTIVATED、FILEBEFORECLOSE、TB/DARKMODE、SHUTDOWN 和
  `SCN_UPDATEUI`。
- **高风险 SCI/UI**：明确请求并缓存 `SCI_GETDIRECTFUNCTION`、
  `SCI_GETDIRECTPOINTER`；大量文本、样式、折叠、margin、marker、calltip 和
  坐标消息。自建大型 Dock 面板及多个资源对话框/原生控件。
- **双 HWND 评价**：普通 SCI HWND 转发不能提供 direct function/pointer；
  即使补齐，Qt 侧直接指针也破坏隔离。原生 Dock UI 也是独立迁移工作。
- **初评**：**C**。重要业务可通过新 Editor/自定义视图/Dock API 移植；禁止为
  兼容 DLL 暴露内部 Scintilla 指针。
- **未验证**：direct call 的线程、所有 SCI 参数、超大数据性能和配置兼容。

### 3.15 GedcomLexer

- **清单身份**：`GedcomLexer`，0.5.0.170，SHA-256
  `9c2050429afbe1121151e426ea3fa4ed5b062a02bf95346ab7f9c6dd4af3126e`；
  GEDCOM 语法着色、错误标记和折叠。
- **源码状态/API**：未找到清单 revision 170 的可核验源码；不反汇编。其属于
  lexer 类插件，但具体采用旧 Lexilla 导出还是普通插件加 lexer、XML 配置和
  Scintilla 接口均未核验。
- **双 HWND 评价与初评**：**U**。必须单独核对 lexer 导出、XML 与 v8.4.6
  Lexilla ABI，不能按普通六导出插件判断。

### 3.16 GitSCM

- **清单身份**：`GitSCM`，1.4.7.1，SHA-256
  `1754f9b7291a64bfd817ec521463971f093aa0fbf454217429ef7022f83ef570`；
  Git/TortoiseGit GUI。
- **源码状态**：有，官方 `vinsworldcom/nppGitSCM` tag `1.4.7.1`，提交
  `0f0bc44fa8c9`；GPL-3.0。
- **API/通知**：确认 `NPPM_GETCURRENTDIRECTORY`、
  `GETFULLCURRENTPATH`、`GETCURRENTSCINTILLA`、`DOOPEN`、
  `GETPLUGINSCONFIGDIR`、`DMMREGASDCKDLG`、`MODELESSDIALOG`、
  `SETMENUITEMCHECK`、`ADDTOOLBARICON`；处理 READY、BUFFERACTIVATED、
  FILEOPENED/FILESAVED、TBMODIFICATION、SHUTDOWN。SCI 只确认当前位置、
  行号和默认样式颜色。
- **Win32/Dock/进程**：复杂 Dock 内含 Toolbar/ListView/Pager/context menu，
  对面板窗口做 `GWLP_WNDPROC` hook；用 `CreateProcess` 启动 Git，并可调用
  TortoiseGit/ShellExecute。
- **双 HWND 评价**：SCI 很简单，但 Dock/hook/子进程输出和关闭生命周期并不
  能仅靠两个代理解决。
- **初评**：**C**。应通过新进程服务和 Qt Git 面板移植；不为其模拟原生
  Toolbar/ListView/Pager 层级。未验证 Git 编码、长命令、凭据和进程取消。

### 3.17 GmodLua

- **清单身份**：`GmodLua`，1.5，SHA-256
  `fe753ae0cb0f585d9bef3fd882f4f7eceb7d0e3014bcd0f12cd5aab323de0d7f`；
  Garry's Mod Lua lexer。
- **源码状态/API**：未找到 1.5 对应源码；不反汇编。lexer 导出、配置 XML、
  六导出和 Scintilla/Lexilla ABI 均未核验。
- **双 HWND 评价与初评**：**U**。与 GedcomLexer 一样须走 lexer 专项检查，
  不能从“语法高亮”描述推断兼容。

### 3.18 GOnpp

- **清单身份**：`GOnpp`，1.2.0.0，SHA-256
  `cec990ef66d9d2264e1066585016b955cfa04f48a11535cf5245c07e5e959c85`；
  调用 `gocode` 完成自动补全/calltip，并执行 `go fmt/test/install/run`。
- **源码状态/API**：未找到 1.2 对应源码；不反汇编。只能确认外部 Go 和
  gocode 进程依赖，不能确认 SCI 自动完成、calltip、文本替换、工作目录和进程
  捕获的接口。
- **双 HWND 评价与初评**：**U**。需源码或 Windows 消息跟踪；若重要，建议
  最终使用新进程服务重写，不直接继承未审计的命令执行方式。

### 3.19 GotoLineCol

- **清单身份**：`GotoLineCol`，2.4.2.0，SHA-256
  `f663bcfb5fb151e91800dd51cf25c8755cb8823155e44aec6b504651bb01cf9d`；
  按行及字节/字符列导航并显示字符编码。
- **源码状态**：有，官方 `shriprem/Goto-Line-Col-NPP-Plugin` tag
  `v2.4.2.0`，提交 `3a4906a45f0d`；GPL-3.0。
- **宿主/通知**：确认当前 SCI/buffer/编码/文件名/路径/命令行、配置目录、
  N++/Windows 版本、暗色和颜色消息，Dock 注册/显示、modeless、工具栏/菜单；
  处理 BUFFERACTIVATED、TB/DARKMODE、`NPPN_CMDLINEPLUGINMSG`、SHUTDOWN 和
  `SCN_UPDATEUI`。
- **SCI/UI**：确认当前位置、行列/行端、tab width、字符读取、跳转、calltip、
  caret/可视策略等同步消息；无 direct function/pointer 证据。原生 Dock 和多个
  对话框依赖 Win32 controls/dark-mode 辅助代码，但未 subclass 主窗口/SCI。
- **双 HWND 评价**：SCI 转发可覆盖核心导航；需增加命令行通知和有限 Dock/
  modeless 支持。坐标/焦点语义只用于自身面板，风险可隔离。
- **初评**：**B**。Windows 原 DLL 可作为 Dock 兼容层代表插件测试；跨平台
  可移植核心列换算并重写 UI。未验证 UTF-8/DBCS/组合字符列规则和双视图。

### 3.20 GrepBugsPluginNpp

- **清单身份**：`GrepBugsPluginNpp`，1.0.0，SHA-256
  `0971e9eee0032f9c5f408f3c2d8176b2efc929677442683e6c79b77d3b62cf86`；
  下载 GrepBugs 规则并扫描全部打开文件，要求 .NET 4+。
- **源码状态**：有，官方 `foospidy/GrepBugsPluginNotepadPlusPlus` tag
  `v1.0`，提交 `a5406ae80b28`；许可证未明确核验。C#/NppPluginNET。
- **确认 API**：`NPPM_DOOPEN`、`GETNBOPENFILES`、
  `GETOPENFILENAMES`、`GETPLUGINSCONFIGDIR`、活动 SCI 查询；
  `SCI_GOTOLINE`、`GETLENGTH`、`GETTEXT`。使用输出缓冲区和原生数组指针。
- **Win32/运行时**：WinForms/.NET 4，HTTP 下载 JSON 规则；业务代码扫描打开
  文件并显示结果 UI。首版未确认主窗口/SCI subclass 或 direct pointer。
- **双 HWND 评价**：同步消息和输出数组可实现，但必须精确保持 x86 结构/缓冲区
  语义；CLR 和网络异常需要隔离。
- **初评**：**B**（Windows 原 DLL）。有限消息集可覆盖；跨平台更适合用新网络
  服务和面板 API 重写。未验证规则来源现状、TLS、UI/Dock 注册和大文件性能。

### 3.21 HexEditor

- **清单身份**：`HexEditor`，0.9.12.0，SHA-256
  `266be61339df9d7a781ca6f5853ac03db1e35c3c6c1c8bcb2349c619f981c781`。
- **源码状态**：有，官方 `chcg/NPP_HexEdit` tag `0.9.12`，提交
  `67350aaa8f63`；GPL-2.0。详细专项结论由重点插件调查文件维护。
- **确认高风险能力**：除当前/两个原有 SCI 外，调用
  `NPPM_CREATESCINTILLAHANDLE`、`DESTROYSCINTILLAHANDLE`、
  `ENCODESCI/DECODESCI`，并维护自己的十六进制编辑器、文档同步和二进制状态；
  使用大量文本/选择/查找/undo SCI 消息和原生 UI。
- **双 HWND 评价**：两个代理只代表主/副文本视图，无法表达插件创建的第三个
  Scintilla 和独立二进制文档模型；强行兼容会把适配层扩展成自定义编辑器宿主。
- **初评**：**C**。按既定原则通过新 API 的自定义编辑视图实现，不兼容原 DLL
  的全部内部结构。未验证项以专项调查为准。

### 3.22 HTMLTag_unicode

- **清单身份**：`HTMLTag_unicode`，1.3.5.0，SHA-256
  `f93b4758b71918920b3bb927fc566012d1f586a9bfa0fb7368836b2135b0eed5`；
  HTML/XML 标签跳转/选择及实体、JS 字符编码转换。
- **源码状态**：官方 Bitbucket 仓库存在，NEWS 确认 1.3.5 发布于
  2022-08-18，但本轮浅克隆未定位到清单版本的精确提交；因此不使用当前 1.6
  源码推断 1.3.5 API，不反汇编发布 DLL。
- **API/双 HWND/初评**：六导出、SCI 文本范围、解析、UI 和通知均未核验，
  **U**。后续应获取 2022-08-18 对应历史提交后再评；不能因功能看似文本处理
  直接判 A/B。

### 3.23 HugeFiles

- **清单身份**：`HugeFiles`，0.1.1.0，SHA-256
  `c888874c3c4f610dd826fce279a156cf38a6e12167ae5da19d694ef4f25ef116`；
  大文件按可配置分隔符和大小分块读取、预览和导航。
- **源码状态**：有，官方 `molsonkiko/HugeFiles` tag `v0.1.1.0`，提交
  `f8bfab5687cd`；GPL-3.0；C#/NppPluginNET、WinForms。
- **确认 API**：业务代码使用 `NPPM_GETPLUGINSCONFIGDIR`、
  `DMMREGASDCKDLG`、`DMMSHOW/DMMHIDE`、`SETMENUITEMCHECK`，处理
  `NPPN_FILEBEFORECLOSE`；通过 gateway 打开/激活 chunk buffer 并写入文本。
  模板 SDK 中的大量常量不计为实际调用。
- **Win32/Dock/文件**：WinForms Dock/设置/文件选择器；按块直接读文件并用
  临时编辑 buffer 展示，关闭时清理 `Chunker`。无 direct function/pointer 或
  主窗口 subclass 证据。
- **双 HWND 评价**：文本 buffer 操作可通过普通 SCI 转发；需要有限 Dock、
  新建/激活 buffer 和关闭通知。大文件读取不依赖原版控件树。
- **初评**：**B**（Windows 原 DLL）。跨平台可保留 chunk 算法并重写 Qt UI；
  若发展为正式大文件能力，应走新自定义视图 API，而不是长期把临时 buffer 当作
  文档模型。
- **未验证**：实际 gateway 消息全集、超 2/4 GB 偏移、编码、内存峰值、文件
  变化、异常关闭和 CLR 卸载。

## 4. 对兼容层设计的批次反馈

本批次支持把旧 DLL 兼容层继续限定为白名单：

- A/B 候选共同需要当前视图/文档/路径、打开与激活文档、配置目录、菜单状态、
  基本 SCI 文本/位置/indicator/marker 消息和原版时序通知。
- `NPPM_MODELESSDIALOG` 与基础 Dock 可作为 Windows 专用的有限公共能力评估，
  但不得由此承诺模拟原版 Dock 内部控件树。
- `SCI_GETDIRECTFUNCTION`、`SCI_GETDIRECTPOINTER`、
  `SCI_GETRANGEPOINTER` 和 `NPPM_CREATESCINTILLAHANDLE` 是明确升级到 C 路线的
  信号；适配器不得返回 Qt 编辑器内部指针或伪造可调用函数指针。
- 主窗口/Scintilla subclass、窗口类名与父子层级检查也是 C/D 信号，不应通过
  扩大 Win32 模拟范围解决。
- 无源码项目保持 U，直到取得精确版本源码或完成 Windows x86 非反汇编运行观察。

## 5. Windows x86 后续验证项

1. 校验每个包 SHA-256、DLL 文件名、Unicode/架构及六导出，不加载失败插件。
2. 记录 `getFuncsArray` 命令数、快捷键和初始勾选状态。
3. 用消息日志核对本文件列出的 `NPPM_*`、`SCI_*` 和通知，并补齐输出缓冲区/
   指针结构的长度、编码和返回值。
4. 分别验证主/副视图、切换视图、关闭/重启和插件异常后的宿主稳定性。
5. 对 A/B 候选逐功能验收；对 C/D/U 插件验证“拒绝或禁用后宿主不崩溃”，不把
   成功 `LoadLibrary` 当作兼容。
