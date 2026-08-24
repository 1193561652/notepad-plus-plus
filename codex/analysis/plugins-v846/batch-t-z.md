# Notepad++ v8.4.6 插件调查：T–Z（仅 x86）

## 1. 范围、计数和方法

本批次只使用 `third_party/nppPluginList/catalog/windows/pl.x86.json`（版本 `1.5.4`，架构
`32`）作为清单和版本基线。按 `inventory.md` 的排序，本文件覆盖第 148～169
项中分配给 T–Z 批次的 17 项：

`TagLEET`、`TagsView`、`TakeNotes`、`Tidy2`、`TopMost`、`Translate`、
`urlPlugin`、`VisualStudioLineCopy`、`WakaTime`、`WebEdit`、`WindowManager`、
`WLangLexer`、`XBrackets`、`XMLTools`、`XPatherizerNPP`、
`ZenCoding-Python`、`zoomdisabler`。

任务候选看似有 18 项，是因为还包含 `_CustomizeToolbar`；该项是 inventory 第
23 项，已经归入 A–C 批次，本文件不重复。17 项与 `_CustomizeToolbar` 合计 18
项，各批次并集仍以 x86 清单的 169 个唯一 `folder-name` 为准。

调查边界：

- JSON 中的版本、SHA-256、包地址和主页按原文记录；这 17 项均未声明
  `npp-compatible-versions`。
- 只有找到与清单版本相符的公开源码，才根据实际业务调用给出 A/B/C/D 等级；
  未找到相符源码的一律保持 `U`，没有反汇编 DLL。
- SDK 头文件和生成的 C# Scintilla 接口中“只定义、未调用”的消息不计入 API
  使用情况。
- 本轮是静态首版调查，未下载或运行 x86 二进制，也未验证六个导出、崩溃边界及
  构建可复现性。
- 等级只评价 Windows 上“Qt 主窗口原生 HWND＋两个 Scintilla 代理 HWND”兼容
  原 DLL 的难度；Linux/macOS 仍需源码移植或新 API。

## 2. 汇总结论

| 插件 | x86 版本 | 对应版本源码 | 主要宿主/API 与平台依赖 | Dock/线程/运行时 | 双代理初评 |
| --- | --- | --- | --- | --- | :---: |
| TagLEET | 1.3.2.0 | 未找到 | 未核验 | 未核验 | U |
| TagsView | 1.0.3 | 未找到 | 未核验 | 未核验 | U |
| TakeNotes | 1.2.3.0 | 未找到；清单主页仅定位到二进制归档仓库 | 未核验 | 未核验 | U |
| Tidy2 | 0.2 | 未找到 | 未核验 | 未核验 | U |
| TopMost | 1.4.0.0 | 未找到；清单主页仅定位到二进制归档仓库 | 未核验 | 未核验 | U |
| Translate | 3.1.1.0 | 未找到 | 未核验 | 未核验 | U |
| urlPlugin | 1.2.0.0 | 有，tag `1.2.0.0` | 常用文本/选区消息、文件打开、新建文档、原生设置对话框 | 无 Dock；未见后台线程 | B |
| VisualStudioLineCopy | 1.0.0.2 | 未找到 | 未核验 | 未核验 | U |
| WakaTime | 4.2.3 | 有，tag `4.2.3` | 文件路径、保存/修改通知；.NET Framework、WinForms | 后台 Task/Timer、下载 Python/CLI、子进程和网络 | B |
| WebEdit | 2.1 | 未找到 | 未核验 | 未核验 | U |
| WindowManager | 1.2.2.0 | 未找到 | 未核验 | 未核验 | U |
| WLangLexer | 4.1.0.16 | 未找到 | 未核验 | 未核验 | U |
| XBrackets | 1.3.1 | 有，tag `v131` | 文本/选区、字符通知、配置；直接替换主窗口 WndProc | 无 Dock/线程；原生设置对话框 | C |
| XMLTools | 3.1.1.13 | 有，tag `3.1.1.13` | 常用文本 API；MSXML 6 COM、MFC/Win32 UI | 无 Dock；未见核心后台线程 | B |
| XPatherizerNPP | 2.10 | 未找到 | 未核验 | 未核验 | U |
| ZenCoding-Python | 0.7.0.1 | 有，仓库 HEAD 与资源版本一致 | `NPPM_MSGTOPLUGIN` 调用 PythonScript；菜单、语言和文件通知 | PythonScript/Python 运行时依赖 | C |
| zoomdisabler | 1.2.0 | 清单主页仓库不可访问，未找到 | 未核验 | 未核验 | U |

本批次统计：`B=3`、`C=2`、`U=12`。没有足够证据的插件没有按功能名称猜测
API 或等级。

## 3. 有对应版本源码的插件

### 3.1 urlPlugin 1.2.0.0

| 项目 | 结论 |
| --- | --- |
| JSON 记录 | SHA-256 `6ab0ed27036f40eccc529ce6aa4373b2ddf71a39a4133d1243cb1e158f9b3dd8` |
| 源码 | [SinghRajenM/nppURLPlugin](https://github.com/SinghRajenM/nppURLPlugin)，tag `1.2.0.0`，commit `dc60f6c`（2022-03-25） |
| 许可证 | GPL-2.0（tag 内 `LICENSE`） |
| 功能 | 对当前选区或光标所在单词执行 URL 编码/解码，并可在新文档中格式化 JSON |
| 初评 | **B** |

实际业务调用：

- `NPPM_GETCURRENTSCINTILLA`、`NPPM_GETFULLCURRENTPATH`、`NPPM_DOOPEN`、
  `NPPM_MENUCOMMAND`、`NPPM_SETMENUITEMCHECK`、
  `NPPM_ADDTOOLBARICON_FORDARKMODE`、`NPPM_GETENABLETHEMETEXTUREFUNC`；
  静态对话框基础类还使用 `NPPM_MODELESSDIALOG`。
- `NPPN_TBMODIFICATION`、`NPPN_BUFFERACTIVATED`、`NPPN_SHUTDOWN`。
- `SCI_GETLENGTH`、`SCI_GETSELECTIONSTART/END`、`SCI_GETCURRENTPOS`、
  `SCI_WORDSTARTPOSITION`、`SCI_WORDENDPOSITION`、`SCI_GETTEXTRANGE`、
  `SCI_SETSELECTIONSTART/END`、`SCI_REPLACESEL`、`SCI_SETTEXT`。
- `SCI_GETTEXTRANGE` 带 `Sci_TextRange` 输入/输出指针；未发现业务源码调用
  `SCI_GETDIRECTFUNCTION` 或 `SCI_GETDIRECTPOINTER`。

Win32/UI：

- Settings、About 使用资源对话框和原生控件；Settings 是需通过
  `NPPM_MODELESSDIALOG` 登记的非模态窗口。
- URL 控件会 subclass 自己的静态文本控件，但没有 subclass Notepad++ 主窗口或
  Scintilla；仓库内通用 `TabBar.cpp` 的窗口改写没有被本插件业务实例化。
- 未发现 Dock、自定义编辑视图或后台线程。

两个代理可同步处理它使用的普通文本、选区和指针结构消息，主窗口入口可处理文件
和菜单消息。B 级难点是保持 `Sci_TextRange` 布局/编码、原生非模态对话框生命周期
和主题函数返回值；不需要模拟原版控件树。跨平台建议保留编码算法，用新 API 或 Qt
重写设置 UI。

未验证：x86 包六个导出、全部 FuncItem、格式化 JSON 所用库及异常输入、对话框
关闭顺序、构建可复现性和 Windows x86 运行。

### 3.2 WakaTime 4.2.3

| 项目 | 结论 |
| --- | --- |
| JSON 记录 | SHA-256 `d144a82dc3e50b4451049b0292e5dc0175f6ef69e399be2625b61b3e36683fe7` |
| 源码 | [wakatime/notepadpp-wakatime](https://github.com/wakatime/notepadpp-wakatime)，tag `4.2.3`，commit `ec6a137`（2021-11-13） |
| 许可证 | BSD-3-Clause（tag 内 `LICENSE.txt`） |
| 运行时 | C#/.NET Framework 4.5、DllExport/ILMerge、WinForms、Python 与 WakaTime CLI |
| 初评 | **B** |

排除生成的 PluginInfrastructure 常量后，业务代码使用：

- `NPPM_GETNPPVERSION`、`NPPM_GETCURRENTSCINTILLA`、
  `NPPM_GETFULLCURRENTPATH`、`NPPM_GETFULLPATHFROMBUFFERID`、
  `NPPM_ADDTOOLBARICON`。
- `NPPN_FILESAVED`、`NPPN_SHUTDOWN`，以及 `SCN_MODIFIED` 的插入文本事件。
  编辑器网关仅用于取得当前视图；业务不依赖大范围 `SCI_*` 或 direct-call。
- WinForms 提供 API key 和设置窗口；未使用 Dock，也未见主窗口/Scintilla
  subclass。

线程和外部依赖是主要风险：初始化及 heartbeat 会进入 `Task.Run`，另有
`System.Timers.Timer`；依赖管理用 `WebClient` 下载 Python/CLI 并用
`Process.Start` 执行版本检查和 heartbeat。关闭时会停止 timer，但后台任务、
下载和子进程的完成/取消上界需实测。

两个代理足以提供它的当前编辑器和通知来源，常用路径消息也容易同步实现，因此旧
DLL 初评为 B；但加载托管导出 DLL、CLR 初始化失败、后台任务关闭期回调和外部程序
都必须纳入隔离及 Windows 实测，不能仅以 API 数量判为 A。跨平台更适合保留
heartbeat 业务协议，改用新 API 的文件事件、进程和网络服务。

未验证：发布 DLL 的 CLR/导出布局、32 位 .NET 运行环境、代理通知中的 buffer ID、
CLI 下载校验、代理/离线行为、进程退出及异常是否越过导出边界。

### 3.3 XBrackets Lite 1.3.1

| 项目 | 结论 |
| --- | --- |
| JSON 记录 | SHA-256 `385588c50edde84fbb78617d6118d1ffea116883d81ef8f74dc3bd789a2e8742` |
| 源码 | [d0vgan/npp-XBracketsLite](https://github.com/d0vgan/npp-XBracketsLite)，tag `v131`，commit `ff82c29`（2022-02-12） |
| 许可证 | tag 中未找到独立许可证文件，待上游确认 |
| 功能 | 根据字符输入、文件类型和设置自动补全括号/引号 |
| 初评 | **C** |

实际调用包括：

- `NPPM_GETCURRENTSCINTILLA`、`NPPM_GETFULLCURRENTPATH`、
  `NPPM_GETCURRENTDIRECTORY`、`NPPM_GETFILENAME`、`NPPM_GETNAMEPART`、
  `NPPM_GETEXTPART`、`NPPM_GETCURRENTWORD`、`NPPM_GETNPPDIRECTORY`、
  `NPPM_GETPLUGINSCONFIGDIR`、`NPPM_GETMENUHANDLE`、
  `NPPM_MAKECURRENTBUFFERDIRTY`。
- `NPPN_BUFFERACTIVATED`、`NPPN_FILEOPENED`、`NPPN_FILESAVED`、
  `NPPN_READY`、`NPPN_SHUTDOWN` 和 `SCN_CHARADDED`。
- 文本/选区族：`SCI_GETCHARAT`、`SCI_GETCURRENTPOS`、
  `SCI_GETSELTEXT`、`SCI_GETTEXT[ LENGTH]`、`SCI_GETTEXTRANGE`、
  `SCI_GETSELECTION*`、`SCI_SETSEL`、`SCI_REPLACESEL`、
  `SCI_BEGIN/ENDUNDOACTION` 等。还取得 `SCI_GETDOCPOINTER`，但本轮未发现
  direct function/pointer 调用。

核心阻断点不是普通 `SCI_*`，而是 `NPPN_READY` 后用
`SetWindowLongPtr(GWLP_WNDPROC)` 直接替换 Notepad++ 主窗口 WndProc，以截获
`WM_COMMAND` 和原版宏运行私有消息 `WM_USER + 4002`，关闭时再恢复原过程。设置也是
Win32 资源对话框。

两个 Scintilla 代理能承载文本操作与 `SCN_CHARADDED`，但现有 Qt 主窗口 HWND 不应
允许插件任意替换宿主 WndProc；私有宏消息又绑定原版内部实现。因此按既定边界判
C。推荐源码移植：将字符通知、事务编辑、当前语言/文档和宏状态做成显式服务，用
Qt 设置页替换资源对话框，不为该插件扩展任意主窗口 subclass 兼容。

未验证：主窗口 hook 与 Qt nativeEvent 的实机冲突、宏录制/回放顺序、多选和矩形
选区、非 UTF-8 code page、关闭时 WndProc 恢复及 x86 构建。

### 3.4 XML Tools 3.1.1.13

该插件的完整证据和结论已经写入
[`batch-priority-complex.md`](batch-priority-complex.md#6-xml-tools)，本处
只保留批次索引，避免复制后产生两份结论。

- JSON SHA-256：
  `9521d91be847a9c9fcfc6cb6ea5455fd7dfe840f4f12a8fd95e5137116dbd6c3`
- 源码：[morbac/xmltools](https://github.com/morbac/xmltools)，tag
  `3.1.1.13`，commit `de76065`（2022-03-31），GPL-3.0。
- 主要消息：当前视图/缓冲区/路径/编码/语言、菜单/状态栏，普通文本、选区、
  annotation、滚动及 `SCN_CHARADDED/MODIFIED/UPDATEUI`；未发现 Scintilla
  direct-call。
- 主要难点：MSXML 6 COM（DOM/SAX/XSLT/schema）、MFC Property Grid 和多组
  Win32 对话框。两个代理可支持编辑器消息，不能解决 COM XML 引擎和原生 UI 的
  跨平台问题。
- 初评：**B**。Windows 可作为常用消息和同步指针缓冲区的兼容样本；跨平台应
  保留 XML 业务语义并替换 XML 引擎/UI。

未验证项沿用重点调查文件，包括 x86 构建、MSXML 安全选项、外部实体/网络访问、
大文档性能、对话框生命周期及异常边界。

### 3.5 Zen Coding - Python 0.7.0.1

| 项目 | 结论 |
| --- | --- |
| JSON 记录 | SHA-256 `a781ee5c76ebc543b412a41e024c2d96f9a6edcae591f27868e190a48bc7bba6` |
| 源码 | [bruderstein/ZenCoding-Python](https://github.com/bruderstein/ZenCoding-Python)，commit `d291792`（2011-03-22）；资源文件声明版本 `0.7.0.1` |
| 许可证 | 仓库未找到独立许可证文件，待上游确认 |
| 运行时 | 原生桥接 DLL＋随包 Python 脚本；依赖另一插件 `PythonScript.dll` |
| 初评 | **C** |

原生桥接层实际使用 `NPPM_GETPLUGINSCONFIGDIR`、
`NPPM_SETMENUITEMCHECK`、`NPPM_GETSHORTCUTBYCMDID`、
`NPPM_GETBUFFERLANGTYPE`、`NPPM_GETFULLPATHFROMBUFFERID` 和
`NPPM_MSGTOPLUGIN`；处理 `NPPN_READY`、`NPPN_SHORTCUTREMAPPED`、
`NPPN_BUFFERACTIVATED`、`NPPN_LANGCHANGED`、`NPPN_FILESAVED`。它把
`PythonScript_Exec` 包装进 `CommunicationInfo`，按目标模块名
`PythonScript.dll` 发送给 PythonScript 插件，实际编辑逻辑由
`ZenCodingPython/` 脚本通过该运行时完成。

因此两个代理不是主要问题：必须先兼容复杂的 PythonScript 插件及其插件间 ABI、
目标模块发现、结构体布局和同步所有权，之后才谈得上运行本插件。为一个附属插件
把完整 PythonScript/旧 Python 运行时纳入有限兼容层不合适，判 C。推荐在新 API
下移植脚本逻辑或采用受控脚本服务；如果未来先移植 PythonScript，再单独复评旧
DLL 直兼容。

未验证：发布包内 PythonScript 依赖声明及脚本布局、Python 版本、脚本调用的全部
编辑器 API、`NPPM_MSGTOPLUGIN` 返回/异常语义、无依赖时行为和 x86 构建。

## 4. 保持 U 的 12 项

下列条目都在 x86 JSON 中，但本轮没有找到能可靠对应清单版本的源码。按项目规则
不反汇编、不根据名称猜测调用，API、Win32、Dock、线程和双代理难点均标为未核验。

| 插件 | 版本 | JSON SHA-256 | 源码核验结果 | 后续非反汇编证据 |
| --- | --- | --- | --- | --- |
| TagLEET | 1.3.2.0 | `bdf948c12b336630809391575b47457dcf34c3e20c944ec98b396bbc3cb8ce8e` | SourceForge 主页未定位到对应源码 | 项目文档、作者发布的源码包、Windows x86 运行观察 |
| TagsView | 1.0.3 | `481561400b1ee1d87c72bf39283412fdc2912a8fc6ad72a5fd96ac71fa27fb67` | SourceForge 主页未定位到对应源码 | 项目文档、包内容清单、Windows Dock/ctags 行为观察 |
| TakeNotes | 1.2.3.0 | `ba0cc59dbe61376951633a3d22a55d3d6ed153ad1d842447cdd2ce1222c245ba` | 清单主页 `NppPlugins` 是发布二进制归档，未定位源码 | README、作者可提供的源码、Windows UI/文件行为观察 |
| Tidy2 | 0.2 | `59cf24719009f0a62ad4414faa9b5dac7dfd637c12a13c024492fee1a5eca3ab` | Google Code 归档入口未核到对应源码 revision | 归档项目元数据、源代码归档、包内许可/依赖说明 |
| TopMost | 1.4.0.0 | `4e68fdd5ac499b2bbe885d3d6283d8d43e610cfa6e60f306f42ecdb9f8438e5a` | 清单主页 `NppPlugins` 是发布二进制归档，未定位源码 | README、Windows 窗口置顶行为观察 |
| Translate | 3.1.1.0 | `f5b84324f494ffbfb125ea44d33dc71d82e03696366452805b9ee88ccf506b99` | SourceForge 主页未定位到对应源码 | 项目文档、网络服务说明、Windows 运行观察 |
| VisualStudioLineCopy | 1.0.0.2 | `f50568de4a488f37667b93b6b6a14a95165a3bd5636e8f4bcbcab586d95df95e` | SourceForge 主页未定位到对应源码 | 项目文档、剪贴板行为观察 |
| WebEdit | 2.1 | `60622e73f01a1fdb5462c2ba416649c404c39db18100826301f4ad74b4e7da46` | SourceForge 发布页未定位到对应源码 | 项目说明、命令/选区行为观察 |
| WindowManager | 1.2.2.0 | `edfcfa2d82ea0eaa68beb3dcba58c3c830c313705a25d249222c1d3a85f5ddfc` | SourceForge 发布页未定位到对应源码 | 文档、窗口/文档列表 UI 行为观察 |
| WLangLexer | 4.1.0.16 | `f7108be1ced52b98553e80674ed5fd493445ce668c2444648477632c0c0da0d3` | SourceForge 项目入口未定位到对应 revision 源码 | lexer 文档、包内配置/许可、语言高亮行为观察 |
| XPatherizerNPP | 2.10 | `92e8393a2bf94b27a8084d8d84734616a56ea51f877bf5cc62b6d494642e76b5` | Google Code 归档入口未核到对应源码 revision | 归档源码、依赖说明、XPath UI 行为观察 |
| zoomdisabler | 1.2.0 | `eb49c4fce581fa63b024a3a0454ccc49d082b9524c0cc9a0d5c25544e264da55` | JSON 指向的 GitHub 仓库当前不可访问，未找到源码 | 包内容、作者镜像、Windows 缩放行为观察 |

其中没有项目既被当前计划标为“重要”又明确无源码，因此本批次暂不提出行为复刻
候选。若后续使用量证据改变重要性，应先标注，再通过公开文档和应用观察建立行为
规范。

## 5. Windows x86 后续验证项

1. 对 17 个 JSON 包核对 SHA-256、目录布局、DLL 文件名、依赖 DLL 和六个导出；
   不从导入表推导 API 清单。
2. 在原版 v8.4.6 记录 FuncItem、默认快捷键、通知顺序、主/副视图切换和退出行为。
3. 对 urlPlugin 验证 `Sci_TextRange`、非模态 Settings、主题函数和新文档命令。
4. 对 WakaTime 验证 CLR 加载失败、离线/代理/下载失败、Python/CLI 子进程以及退出
   时 Task/Timer 是否仍回调宿主。
5. 对 XBrackets 验证主窗口 WndProc hook 是否破坏 Qt native event，特别是宏录制
   私有消息；加载不兼容时必须拒绝或隔离，不能允许半初始化 hook 留存。
6. 对 XMLTools 验证 MSXML/MFC 运行时、外部实体和网络访问、大文档、错误 annotation
   及关闭期 COM 生命周期。
7. 对 ZenCoding-Python 分别测试未安装、版本不匹配和可用的 PythonScript，确认
   `CommunicationInfo` 所有权、同步返回和脚本异常边界。
8. 对 U 项只做公开行为观察和稳定性测试；仍不反汇编。加载失败、缺少依赖、导出
   异常和关闭超时应由宿主记录并尽量避免拖垮主程序。
