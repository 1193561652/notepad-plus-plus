# Notepad++ v8.4.6 x86 插件调查：NativeLang～NWScript-Npp

日期：2026-07-30
状态：首轮广度调查；共 50 项

## 1. 范围、证据边界和等级规则

本文件只调查 `resources/pluginList/windows/pl.x86.json`（列表版本 `1.5.4`，
`arch=32`）中按 `folder-name` 忽略大小写排序后从 `NativeLang` 到
`NWScript-Npp` 的 50 项。版本、功能描述、包地址和主页均以该 x86 JSON 为准，
不引用 x64/ARM64 条目补全结论。

本轮没有下载插件 DLL，也不做反汇编。公开主页或 GitHub 仓库只证明“存在项目
页面”，不自动证明清单版本的源码可得；只有实际取得并检出对应版本源码的项目
才定 A/B/C/D。其余项目保持 `U`，下面对功能、运行时或 Windows 依赖的描述若仅
来自 JSON，均明确标为“清单声明”，不能当作源码 API 结论。

表中缩写：

- `N/S`：`NPPM_*`/`NPPN_*` 与 `SCI_*`；
- `UI/Dock`：原生 UI、Dock、窗口层级、subclass/hook 等；
- `T/R`：线程、子进程、网络、脚本或托管运行时；
- “双代理难点”按现有“主窗口 HWND＋两个 Scintilla 代理 HWND”模型评价。

## 2. 逐项首轮调查

| # | 插件（x86 版本） | 源码状态与功能 | 可确认 N/S | Win32、Dock、T/R | 双代理难点与等级 | 证据；未验证项 |
| ---: | --- | --- | --- | --- | --- | --- |
| 1 | `NativeLang` 1.1.0.0 | 未找到对应版本源码；为其他插件翻译菜单/对话框 | 均未核验 | 必然涉及插件菜单/对话框文本，但具体 Win32 入口未核验 | 需要宿主暴露菜单命令和窗口文本；**U** | x86 JSON、SourceForge 主页；六导出、消息、窗口枚举和编码未验证 |
| 2 | `NavigateTo` 1.12.7.0 | 有公开仓库，未取得并核对对应版本源码；按文件名、路径、符号快速切换并预览 | 未核验 | 清单可确认有交互式搜索 UI；Dock、线程未核验 | 文档枚举/激活可代理；实时预览、原生控件与焦点语义待查；**U** | x86 JSON、`young-developer/nppNavigateTo` 及 `v.1.12.7` 发布链接；源码 tag、API 未验证 |
| 3 | `NewFileBrowser` 0.1.3 | 未找到对应版本源码；模板化新文件并含内置浏览器 | 未核验 | 清单明确含浏览器；浏览器内核、COM、Dock 未核验 | 自定义浏览器不是 SCI 转发问题，可能需新视图 API；**U** | x86 JSON、SourceForge `locationnav`；二进制依赖与窗口模型未验证 |
| 4 | `NotepadStarterPlugin` 2.3.3.0 | 有公开仓库，未核对对应源码；替换系统 Notepad 启动入口 | 未核验 | 清单明确依赖 Windows 系统集成，包可能含 `NotepadStarter.exe`；权限/注册表细节未核验 | 双代理基本无助于系统替换；跨平台无等价需求；**U** | x86 JSON、`lygstate/NotepadStarter` 2.3.3.0 发布链接；安装行为、UAC、导出未验证 |
| 5 | `nppAutoDetectIndent` 2.3 | 有公开仓库，未取得对应版本源码；打开文档时检测 tab/space 与宽度 | 未核验 | UI/线程未核验 | 预计需要文档通知、行读取、tab/indent 设置，均可由有限代理表达；证据不足，**U** | x86 JSON、`Chocobo1/nppAutoDetectIndent` 2.3 发布链接；精确 N/S 和性能未验证 |
| 6 | `NppAutoIndent` 1.2.0.0 | 未找到对应版本源码；C 风格语言智能缩进 | 未核验 | UI/线程未核验 | 预计高度依赖字符通知、行/括号查询和 selection；通知时序需实测；**U** | x86 JSON、SourceForge；精确消息、重入、编码未验证 |
| 7 | `NppBplistPlugin` 1.3.0.0 | 有公开仓库，未取得对应版本源码；查看/编辑 binary plist | 未核验 | 解析库、转换策略及 UI 未核验 | 若替换当前文档可代理；若提供自定义二进制视图则需新视图 API；**U** | x86 JSON、`azerg/NppBplistPlugin` 1.3.0.0 发布链接；源码 tag、文件保存语义未验证 |
| 8 | `NppCalc` 1.5 | 未找到对应版本源码；400 余函数的表达式计算器 | 未核验 | 清单声明 RS-232、TCP/IP、文件、压缩/密码能力；线程/网络模型未核验 | 基本文本插入可代理，但外设/网络与复杂运行时不属于窗口代理；**U** | x86 JSON、SourceForge；源码、依赖、权限和异常边界未验证 |
| 9 | `nppConverter` 4.4.0 | tag v4.4，commit `43667bd`；ASCII/Hex 和 Conversion Panel | `NPPM_GETCURRENTSCINTILLA`、`GETPLUGINSCONFIGDIR`、`DOOPEN`、Dock/modeless；选区、替换、`SCI_ADDTEXT` | 原生资源 Dock，无后台线程 | 官方 x64 DLL 已验证双向转换、配置和面板插入，有限 Dock 兼容，**B** | v4.4 源码、官方包 SHA-256 与真实 DLL 自动回归 |
| 10 | `nppcrypt` 1.0.1.6 | 有公开仓库，未取得对应源码；加解密、hash、随机数、Base16/32/64 | 未核验 | 密码库、随机源、原生对话框未核验 | 文本输入输出可代理；密码 UI、二进制数据和缓冲区边界需查；**U** | x86 JSON、`jeanpaulrichter/nppcrypt` 1.0.1.6 发布链接；算法依赖、API、敏感数据清理未验证 |
| 11 | `NppDocShare` 0.1.0.0 | 有公开仓库，但清单版本与包名 `0.1.13` 不一致，未取得对应源码；双机实时协同编辑 | 未核验 | 清单明确网络通信；线程、协议、同步模型未核验 | 高频远端编辑、线程切换、撤销/selection 一致性远超简单同步转发；**U** | x86 JSON、`chcg/NppDocShare` 发布包；版本错配、协议、重入和关闭未验证 |
| 12 | `NppEditorConfig` 0.4.0 | 有公开仓库，未取得对应版本源码；发现并应用 `.editorconfig` | 未核验 | EditorConfig 解析库/文件遍历；UI、线程未核验 | 路径查询、buffer 通知和 tab/EOL 设置应可有限代理；文件监控与通知顺序待查；**U** | x86 JSON、`editorconfig/editorconfig-notepad-plus-plus` v0.4.0 发布链接；源码 tag、精确消息未验证 |
| 13 | `NppEventExec` 0.9.0 | 有公开仓库，未取得对应版本源码；把 N++ 事件映射为 NppExec 脚本 | 未核验 | 清单明确插件间协作/脚本触发；可能依赖 `NPPM_MSGTOPLUGIN`，但未由源码确认 | 关键是完整 NPPN 时序和插件间 ABI，不是 SCI 转发；**U** | x86 JSON、`MIvanchev/NppEventExec` v0.9.0 发布链接；事件集合、NppExec 协议和错误隔离未验证 |
| 14 | `NppExec` 0.8.2 | 有公开仓库，未取得并核验 `v082` 源码；命令、脚本和控制台 | 未核验 | 清单明确子进程/脚本；控制台大概率为原生 Dock，但本轮未以源码确认 | 进程 I/O、Dock、宏变量、插件消息和关闭行为构成高风险；证据不足仍为 **U** | x86 JSON、`d0vgan/nppexec` v082 发布链接；对应源码、精确 N/S、跨线程调用未验证 |
| 15 | `NppExport` 0.4.0.0 | 有公开仓库，未取得对应源码；导出 HTML/RTF 及富文本剪贴板 | 未核验 | 清单明确 Windows clipboard；RTF/HTML 格式与 UI 未核验 | 样式/全文读取可代理，富剪贴板属于平台服务；**U** | x86 JSON、`chcg/NPP_ExportPlugin` 0.4.0 发布链接；样式 SCI、剪贴板所有权和大文档未验证 |
| 16 | `NppFavorites` 1.0.0.1 | 有公开仓库，未取得对应源码；收藏夹 | 未核验 | 原生列表/Dock、配置持久化未核验 | 文档打开/路径服务可代理；若 Dock 则需受控原生容器；**U** | x86 JSON、`heldersepu/nppfavorites` 1.0.0.1.21 发布链接；版本对应关系、消息/UI 未验证 |
| 17 | `NPPFSIPlugin` 0.1.1 | 有公开仓库页面，未取得对应源码；内嵌 F# Interactive | 未核验 | 清单明确依赖单独安装 F#；脚本运行时/子进程、承载 UI 未核验 | 运行时托管、控制台 I/O 和停止语义超出双代理；**U** | x86 JSON、`ppv/NPPFSIPlugin` 历史下载链接；源码版本、.NET/F# 版本、Dock 未验证 |
| 18 | `NppFTP` 0.29.10 | 有公开仓库，未取得对应版本源码；FTP/FTPS/FTPES/SFTP | 未核验 | 清单明确网络和 TLS/SSH；通常需要文件树/队列 UI，但未以源码确认 | 网络线程、远端缓存、Dock/对话框、关闭取消是主要难点；**U** | x86 JSON、NppFTP 官网、`ashkulz/NppFTP` v0.29.10 包；对应源码/API/依赖未验证 |
| 19 | `NppGist` 1.5.1.35 | 有公开仓库，未取得对应源码；GitHub Gist 创建、编辑、删除、重命名 | 未核验 | 清单明确网络/GitHub 服务；认证、线程和 UI 未核验 | 文本接口易代理，OAuth/token、异步网络和错误 UI 不属于 SCI；**U** | x86 JSON、`KvanTTT/NppGist` 1.5.1 发布链接；源码版本与包四段版本关系、API 未验证 |
| 20 | `NppGTags` 5.0.0 | 有公开仓库，未取得对应版本源码；GNU Global 前端、索引与导航 | 未核验 | 清单明确外部 GNU Global/索引工具语义；进程、Dock 未核验 | 文件/符号导航可代理；索引进程、结果 UI、同步/取消需宿主服务；**U** | x86 JSON、`pnedev/nppgtags` v5.0.0 发布链接；对应源码、命令行、通知未验证 |
| 21 | `NppHasher` 1.0 | 主页指向公开仓库，未找到并核对 1.0 源码；选择文本 hash/Base64 | 未核验 | 清单明确 C#/.NET Framework；UI 未核验 | selection 读写可代理，CLR 加载与导出异常边界需验证；**U** | x86 JSON、`npp-plugins/hasher`、TuxFamily 包；对应源码、CLR 位数、API 未验证 |
| 22 | `NppJavaPlugin` 0.4.0 | 有公开仓库，未取得对应版本源码；Java 编译和运行 | 未核验 | 清单明确子进程/JDK；输出 UI、线程未核验 | 文件保存、当前路径和进程输出需公共服务；双代理只覆盖文本部分；**U** | x86 JSON、`dominikcebula/npp-java-plugin` v0.4.0 发布链接；源码 tag、JDK 探测、API 未验证 |
| 23 | `NPPJSONViewer` 1.41 | **有并已检出对应 tag `v1.41`**；格式化/压缩 JSON 和树形查看 | `NPPM_GETCURRENTSCINTILLA`、`NPPM_DMMREGASDCKDLG`、`NPPM_MODELESSDIALOG`、`NPPM_SETCURRENTLANGTYPE`；`NPPN_SHUTDOWN`；`SCI_GETLENGTH`、`SCI_GET/SETSELECTIONSTART/END`、`SCI_GETSELTEXT`、`SCI_REPLACESEL`、`SCI_SETSEL`、`SCI_GETEOLMODE`、`SCI_GETTABWIDTH`、`SCI_GETUSETABS` | Win32 TreeView/Dock/资源对话框；部分控件 subclass；无后台线程/子进程；内嵌 RapidJSON | 同步 selection/缓冲区可代理；需有限 Dock 注册/生命周期支持；无 direct function/pointer；**B** | `/tmp/plugin-survey-JSON-Viewer` tag v1.41 commit `f857389`，及 `batch-priority-complex.md` §7；Dock 销毁、大树性能、真实 x86 加载未验证 |
| 24 | `NppJumpList` 1.2.2 | 有公开仓库页面，未取得对应源码；Windows 7 Jump List | 未核验 | 清单明确 Windows shell Jump List，可能 COM；细节未核验 | 双代理无助于 Shell 集成；跨平台需平台特性或不支持；**U** | x86 JSON、`chcg/JumpList` 1.2.2.10 包、SourceForge 主页；源码版本、COM、线程未验证 |
| 25 | `NppMarkdownPanel` 0.6.2 | 有公开仓库，未取得对应版本源码；Markdown 实时预览 | 未核验 | 清单确认 preview panel；浏览器控件、Dock、渲染运行时与线程未核验 | 文档通知可代理；Web/HTML 预览视图需 Dock/自定义视图，更新节流需实测；**U** | x86 JSON、`mohzy83/NppMarkdownPanel` 0.6.2 发布链接；对应源码、浏览器内核、API 未验证 |
| 26 | `NppMenuSearch` 0.9.6 | 有公开仓库页面，未取得对应源码；在工具栏搜索菜单和 Preferences | 未核验 | 清单明确插入工具栏输入框并访问菜单/Preferences；窗口结构依赖高 | 主窗口 HWND 不等于原版工具栏/菜单树；不宜扩大宿主去模拟内部控件；**U** | x86 JSON、`peter-frentrup/NppMenuSearch`、SF 0.9.6 包；源码版本、窗口遍历/subclass 未验证 |
| 27 | `NppPluginDemo` 4.2 | tag v4.2，commit `39d2758`；Hello、路径、会话示例和 Go To Line Dock | 基础路径/新建、Dock；宽文件枚举/Session API；`SCI_SETTEXT`、`ENSUREVISIBLE`、`GOTOLINE` 等 | 原生资源 Dock、工具栏通知和可选字符通知 | 官方 x64 DLL 已验证 Hello、Dock、行跳转；宽示例不为演示插件扩大宿主，**B** | v4.2 源码、官方包 SHA-256 与真实 DLL 自动回归 |
| 28 | `NppPluginOpenHost` 1.1.0.0 | 有公开仓库，清单包直接指向 `main/bin`，没有固定源码 tag 证据；打开 Windows hosts | 未核验 | 清单明确 Windows hosts 文件；权限/UAC 未核验 | `DOOPEN` 类能力可代理，提权和系统路径属平台服务；**U** | x86 JSON、`jejemorg/NppPluginOpenHost` main 包；1.1.0.0 源码对应关系、权限流程未验证 |
| 29 | `NppPluginTemplate` 4.2 | 公开模板仓库/手册，未取得 v4.2 源码快照 | 未核验 | 模板本身不是实际业务插件；六导出应有，但本轮不凭名称判定 | 适合作为最小 ABI 样本；在对应源码/包核验前 **U** | x86 JSON、`npp-plugins/plugintemplate` v4.2 包、N++ 插件手册；导出、命令和消息未验证 |
| 30 | `nppplugin_ofis2` 3.0.1 | 有公开聚合仓库，未取得对应源码；索引目录并快速打开文件 | 未核验 | 可能依赖 SolutionHub；索引线程/文件系统、UI 未核验 | 文档打开可代理；插件间 ABI、索引、结果 UI 是难点；**U** | x86 JSON、`incrediblejr/nppplugins` v3.0.1 包；对应子项目源码、依赖协议未验证 |
| 31 | `nppplugin_solutionhub` 3.0.1 | 有公开聚合仓库，未取得对应源码；其他 Solution 插件的基础服务 | 未核验 | 清单明确插件间基础依赖；配置/文件系统未核验 | 关键是插件间消息/共享 ABI，两个 SCI 代理并不能覆盖；**U** | x86 JSON、同仓库 v3.0.1 包；消息协议、加载顺序、崩溃隔离未验证 |
| 32 | `nppplugin_solutionhub_ui` 3.0.1 | 有公开聚合仓库，未取得对应源码；创建/配置 Solution 的 UI | 未核验 | 清单明确 UI 且依赖 SolutionHub；Dock/原生控件未核验 | 除插件间 ABI 外还需受控 UI/Dock；**U** | x86 JSON、同仓库 v3.0.1 包；源码、窗口层级、生命周期未验证 |
| 33 | `nppplugin_solutiontools` 3.0.1 | 有公开聚合仓库，未取得对应源码；头/源切换和行上文件跳转 | 未核验 | 清单明确依赖 Solution 概念；文件解析/UI 未核验 | 当前词/行、路径解析、打开文件可有限代理；共享 Hub ABI 是额外难点；**U** | x86 JSON、同仓库 v3.0.1 包；源码、N/S、插件消息未验证 |
| 34 | `nppplugin_svn` 3.0.1 | 有公开聚合仓库，未取得对应源码；调用 TortoiseSVN | 未核验 | 清单明确外部 TortoiseSVN 子进程和 Windows 软件依赖 | 双代理仅能提供当前路径；跨平台不能沿用 TortoiseSVN 集成；**U** | x86 JSON、同仓库 v3.0.1 包；命令行、进程等待、SolutionHub 依赖未验证 |
| 35 | `NppQCP` 2.0 | 有公开仓库页面，未取得并确认 2.0 源码；颜色代码高亮/调色/屏幕取色 | 未核验 | 清单明确 Windows Color Chooser、屏幕取色；通知、绘制、hook 未核验 | 文本颜色标记可能需 indicators；屏幕拾色与原生 dialog 属平台服务；**U** | x86 JSON、`nulled666/nppqcp`、S3 包；对应源码、SCI indicators、输入 hook 未验证 |
| 36 | `NppQrCode32` 0.0.0.1 | 有公开仓库，未取得对应源码；选区生成 QR Code | 未核验 | 图像窗口/编码库未核验 | 选区读取易代理；展示、剪贴板/保存图像需 UI 服务；**U** | x86 JSON、`vladk1973/NppQrCode` 0.0.0.1 发布链接；源码 tag、依赖、API 未验证 |
| 37 | `nppRandomStringGenerator` 1.4.0 | 有公开仓库，未取得对应源码；按配置生成随机字符串 | 未核验 | 配置对话框、随机源未核验 | selection/insert 可代理；安全随机性与 UI 不应由 SCI 代理承担；**U** | x86 JSON、`cmbsolutions/nppRandomStringGenerator` v1.4.0 包；源码 tag、精确消息未验证 |
| 38 | `NppRegExTractorPlugin` 2.1.0 | 有公开仓库页面，未取得对应源码；跨一个或多个文件运行正则并输出 XML | 未核验 | 可能有文件遍历/后台任务；UI/引擎未核验 | 当前 SCI 代理不能代替多文件扫描和结果模型；可能需要进度/取消服务；**U** | x86 JSON、`viper3400/NppRegExTractor` 2.1.0 包及 wiki；对应源码、regex 引擎、线程未验证 |
| 39 | `NppSaveAsAdmin` 1.0.211 | 有公开仓库，未取得对应源码；UAC 下保存文件 | 未核验 | 清单明确 Windows UAC/管理员权限；辅助进程/IPC 未核验 | 双代理只能提供文档内容/路径；提权保存必须平台隔离并防止权限扩大；**U** | x86 JSON、`Hsilgos/nppsaveasadmin` 1.0.211 包；源码 tag、UAC/临时文件/失败恢复未验证 |
| 40 | `NppScripts` 2.0.0.0 | 有公开仓库，未取得对应源码；基于 CS-Script 的 C# 自动化 | 未核验 | 清单明确 C#/CS-Script 运行时；脚本可形成插件级宿主调用，线程模型未核验 | API 面可能被脚本无限放大，不能只以已知插件消息白名单判断；建议取得源码后重点复评；**U** | x86 JSON、`oleg-shilo/scripts.npp` v2.0.0.0 包；源码 tag、CLR、脚本 API/隔离未验证 |
| 41 | `NppSnippets` 1.7.0 | 有公开项目/发布仓库线索，未取得对应源码；Dock 列表插入代码片段 | 未核验 | 清单明确“simple list”，是否 Dock 未由源码确认；配置持久化未核验 | 插入文本可代理，列表/Dock 与片段数据库需公共 UI/设置服务；**U** | x86 JSON、`ffes/nppsnippets` 1.7.0 包、项目站；对应源码、N/S、Dock 未验证 |
| 42 | `NppTags` 0.9.1 | 有公开项目，未取得对应源码；Universal Ctags 导航 | 未核验 | 清单明确外部 Ctags/索引；进程、结果 UI 未核验 | 当前词/路径/跳转可代理，索引进程与解析需服务；**U** | x86 JSON、`ffes/npptags` 0.9.1 包、项目站；源码 tag、ctags 版本、API 未验证 |
| 43 | `NppTaskList` 2.4 | 有公开仓库，未取得与清单 2.4 精确对应源码；扫描 TODO 并在右侧 Dock 展示 | 未核验 | 清单明确 Dock task list；扫描线程/通知未核验 | 行读取与跳转可代理；需 Dock、文档修改通知和更新节流；**U** | x86 JSON、`Megabyteceer/npp-task-list` 2.4.0 包、旧项目主页；对应源码、消息和大文档性能未验证 |
| 44 | `NppTextFX` 0.2.6 | 未取得对应版本源码；大量文本变换、排序、brace/quote、autoclose | 未核验 | 清单声明含提交 W3C（潜在网络）、Ascii Chart/UI；线程未核验 | API 面广，可能涉及高频通知、编辑器状态和网络；不能按简单文本插件处理；**U** | x86 JSON、SourceForge TextFX 0.26 包；对应源码、全部命令、N/S、网络未验证 |
| 45 | `NppTextViz` 0.4.2 | 有公开仓库，未取得对应源码；按模式隐藏/显示行 | 未核验 | UI 未核验；核心应涉及行可见性，但未由源码确认 | `SCI_HIDE/SHOWLINES` 类同步消息可代理；高频扫描/折叠状态交互待查；**U** | x86 JSON、`KubaDee/NppTextViz` v0.4.2 包；源码 tag、精确 SCI、撤销语义未验证 |
| 46 | `NppTFS` 1.0 | 未找到对应版本源码；把当前文件附加到 TFS 2010 work item | 未核验 | 清单明确 .NET 4、TFS 2010、网络/服务依赖；UI 未核验 | 当前文件路径可代理，陈旧专有运行时和 TFS 集成不能由代理解决；**U** | x86 JSON、SourceForge 包；源码、TFS API、认证、线程未验证 |
| 47 | `NppToolBucket` 1.10.6622.41336 | 未找到并核验此精确 x86 版本源码；多行替换、缩进、GUID、hash、Base64 等 | 未核验 | 清单明确 .NET 3.5；多种 WinForms 对话框可能性仅为待查 | 文本操作可代理，CLR/UI/全量替换与编码需实测；**U** | x86 JSON、项目站及 1.10 包；精确源码、CLR 导出桥、N/S 未验证 |
| 48 | `NppUISpy` 1.0.4 | 有公开仓库，未取得对应源码；识别 N++ 菜单和工具栏 command ID | 未核验 | 功能本身明确依赖原版菜单/工具栏内部结构，可能窗口命中测试/hook，细节未核验 | Qt 主窗口 HWND 不提供原版 HMENU/toolbar 层级；不应为诊断插件完整模拟；**U** | x86 JSON、`dinkumoil/NppUISpy` v1.0.4 包；源码 tag、具体 Win32 API/hook 未验证 |
| 49 | `NppXmlTreeviewPlugin` 2.0.0 | 有公开仓库，未取得对应版本源码；XML TreeView | 未核验 | 清单明确 TreeView；Dock/解析库/原生控件未核验 | 文本读取可代理，树形 UI 需 Dock/自定义视图；与 JSON Viewer 可共用有限 Dock 能力的假设待证；**U** | x86 JSON、`joaoasrosa/nppxmltreeview` v2.0.0 包；对应源码、N/S、TreeView 生命周期未验证 |
| 50 | `NWScript-Npp` 1.0.3.1950 | 有公开仓库，未取得与包精确对应源码；NWScript 编辑、编译、反汇编、依赖构建和批处理 | 未核验 | 清单明确编译器/反汇编器/批处理；子进程或内嵌工具、线程/UI 未核验 | 文档/语言设置可代理，编译工具链、批任务和结果 UI 需进程服务；**U** | x86 JSON、`Leonard-The-Wise/NWScript-Npp` v1.0.3 包；四段版本对应、源码、工具链/API 未验证 |

## 3. 已确认 API 汇总

首轮唯一达到“对应版本源码已经实际取得并逐文件搜索”条件的是
`NPPJSONViewer` 1.41。它提供以下对兼容层有直接价值的覆盖：

- 主窗口：当前 Scintilla 查询、语言设置、Dock 注册、modeless dialog；
- 编辑器：长度、选区边界、选区缓冲区读写、EOL/tab 设置查询；
- 通知：关闭；
- UI：插件自有的 Win32 TreeView/Dock 和局部 subclass；
- 高风险能力：未发现 `SCI_GETDIRECTFUNCTION`、`SCI_GETDIRECTPOINTER`、
  后台线程或跨线程 SCI 调用。

不能把插件仓库内随附的 `Notepad_plus_msgs.h` 中宏定义集合当作“实际使用 API”；
本文件只记录业务源码实际调用点。其余 49 项均没有在缺少对应源码时从功能名称
反推具体 `NPPM_*`、`NPPN_*` 或 `SCI_*`。

## 4. 双代理模型的首轮结论

从清单声明可以先形成待验证分组，但除 JSON Viewer 外不据此定级：

1. 可能以同步文本消息为主：Auto Detect Indention、NppAutoIndent、Converter、
   Random String Generator、TextViz。重点核验通知时序、输入/输出缓冲区和
   大文档扫描。
2. 需要有限 Dock/自定义 UI：NavigateTo、Favorites、FTP、GTags、Markdown
   Panel、Snippets、Task List、XML Treeview。优先验证能否复用 JSON Viewer
   所需的受控原生 Dock 容器，而不模拟 Notepad++ 内部窗口树。
3. 依赖进程、网络或运行时：DocShare、EventExec、NppExec、FSI、FTP、Gist、
   GTags、Java Plugin、TFS、Scripts、NWScript。两个 SCI HWND 只能覆盖编辑器
   端，必须另行定义进程、网络、设置和取消/关闭边界。
4. 强 Windows 平台集成：NotepadStarter、JumpList、MenuSearch、SaveAsAdmin、
   UISpy、QCP、SVN。不得为了这些插件扩大成完整 Win32 shell、菜单、工具栏或
   窗口层级模拟。
5. 插件间 ABI：EventExec↔NppExec，以及 SolutionHub 系列。即使单个 DLL 能
   加载，加载顺序、`NPPM_MSGTOPLUGIN`/自定义消息、共享结构体生命周期和异常
   隔离仍需成组测试。

## 5. 下一轮取证顺序

广度调查后建议按以下顺序补齐对应版本源码；每取得一个精确 tag/commit，再填写
六导出、完整 N/S 集合并定级：

1. `NppExec`、`NppFTP`、`NppMarkdownPanel`、`NppGTags`、`NppSnippets`：
   常用且覆盖 Dock、线程、网络、进程和文档导航。
2. `NppEditorConfig`、`nppAutoDetectIndent`、`nppConverter`、`NppTextViz`：
   验证同步 SCI 和通知兼容层的低成本覆盖面。
3. `NppScripts`、`NppEventExec`、SolutionHub 四件套：验证脚本 API 和插件间
   消息是否会不受控地扩大兼容范围。
4. `NppMenuSearch`、`NppUISpy`、`NotepadStarterPlugin`、`NppJumpList`：
   用源码确认其原版内部窗口/Shell 绑定，并据价值和成本决定 C/D。

## 6. 通用未验证项

- 50 个 x86 包均未在 Windows v8.4.6 上执行下载、SHA-256、解包和真实加载；
- 除 JSON Viewer 外，六导出、`FuncItem` 数量、命令名、快捷键和 Unicode
  返回值均未核验；
- 除 JSON Viewer 外，direct function/pointer、`Sci_TextRange`、保存 SCI
  HWND 后跨线程调用均未核验；
- DLL 附带运行时、延迟加载 DLL、COM apartment、CLR 位数和异常越过导出边界
  均未核验；
- 不兼容插件的进程级隔离、SEH/崩溃边界以及关闭阶段调用顺序尚未实测。
