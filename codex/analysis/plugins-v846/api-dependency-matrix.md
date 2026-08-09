# Notepad++ v8.4.6 x86 插件 API、依赖与代理矩阵

日期：2026-08-01

## 1. 状态与边界

本文件是插件移植阶段 1 的统一结论入口。身份和版本唯一来源为
`resources/pluginList/windows/pl.x86.json` v1.5.4，不混入 x64、ARM64 或较新
插件版本。

| 项目 | 结果 |
| --- | ---: |
| x86 插件 | 169 |
| 对应版本源码可证 | 64 |
| 对应版本源码不足、保持未知 | 105 |
| 代理初评 A / B / C / D / U | 13 / 25 / 25 / 1 / 105 |
| 重要度 高 / 中 / 低 | 20 / 94 / 55 |

169 项身份、版本、源码状态、重要度和等级见 `inventory.md`；逐插件 API、依赖、
Win32/UI、代理风险和未验证项见六个 `batch-*.md`。本文件归并宿主能力，不复制
169 份长记录。

## 2. 证据规则

| 证据级别 | 数量 | 可作结论 | 禁止推断 |
| --- | ---: | --- | --- |
| 对应版本源码 | 64 | 对应 revision 实际业务调用、许可证、运行时和代理风险 | 未扫描路径、发布包运行行为、x86 实机稳定性 |
| 对应版本证据不足 | 105 | JSON 身份、版本、包哈希、主页和用户可见描述 | 具体 `NPPM_*`、`NPPN_*`、`SCI_*`、导出、依赖或兼容性 |

无源码项不反汇编。它们保持 `U`，后续只通过公开源码/文档、作者提供资料、包内容
清单和 Windows 行为观察增加证据。

## 3. 原版二进制入口

Windows 原版 ABI 的所有候选 DLL 均按以下六导出进行加载前验证：

- `isUnicode`
- `getName`
- `setInfo`
- `getFuncsArray`
- `beNotified`
- `messageProc`

阶段 1 没有运行真实 DLL，因此“源码存在六导出实现”不能替代 PE 架构、导出名称、
调用约定和发布包依赖检查；这些属于阶段 2 安全加载基线。

## 4. NPPM 能力矩阵

下表归并当前 64 项源码记录中确认的调用。单个插件的精确集合仍以其批次记录为准。

| 能力组 | 已确认代表消息 | 代理结论 |
| --- | --- | --- |
| 版本与目录 | `NPPM_GETNPPVERSION`、`NPPM_GETNPPDIRECTORY`、`NPPM_GETPLUGINSCONFIGDIR`、`NPPM_GETPLUGINHOMEPATH` | 首批基础白名单候选 |
| 当前视图与 Buffer | `NPPM_GETCURRENTSCINTILLA`、`NPPM_GETCURRENTBUFFERID`、`NPPM_GETCURRENTDOCINDEX`、`NPPM_GETNBOPENFILES` | 必须与双视图和 clone 生命周期统一 |
| 文件与路径 | `NPPM_GETFULLCURRENTPATH`、`NPPM_GETFULLPATHFROMBUFFERID`、`NPPM_GETOPENFILENAMES`、`NPPM_GETCURRENTDIRECTORY`、`NPPM_GETFILENAME`、`NPPM_GETNAMEPART`、`NPPM_GETEXTPART` | UTF-16 输出缓冲区和长度语义必须复刻 |
| 文档操作 | `NPPM_DOOPEN`、`NPPM_SWITCHTOFILE`、`NPPM_ACTIVATEDOC`、`NPPM_MAKECURRENTBUFFERDIRTY` | 通过现有 Buffer/MainWindow 服务实现 |
| 语言与编码 | `NPPM_GETBUFFERLANGTYPE`、`NPPM_SETBUFFERLANGTYPE`、`NPPM_GETBUFFERENCODING`、`NPPM_SETBUFFERENCODING`、`NPPM_GETLANGUAGENAME` | 需要原版枚举和失败返回值映射 |
| 命令与状态 | `NPPM_MENUCOMMAND`、`NPPM_SETMENUITEMCHECK`、`NPPM_SETSTATUSBAR`、`NPPM_GETSHORTCUTBYCMDID`、`NPPM_ALLOCATECMDID` | 使用宿主命令注册表，不暴露 QAction |
| 工具栏与主题 | `NPPM_ADDTOOLBARICON`、`NPPM_ADDTOOLBARICON_FORDARKMODE`、`NPPM_GETENABLETHEMETEXTUREFUNC` | Windows 句柄只在 legacy bridge 内转换 |
| Dock | `NPPM_DMMREGASDCKDLG`、`NPPM_DMMHIDE`、`NPPM_DMMSHOW`、`NPPM_MODELESSDIALOG` | 需要专属 Win32 子窗口托管原型，不能直接等同 QWidget |
| 插件间通信 | `NPPM_MSGTOPLUGIN` | 结构体布局、同步返回和目标模块生命周期高风险 |
| 原版菜单/窗口结构 | `NPPM_GETMENUHANDLE`、`NPPM_HIDETABBAR`、`NPPM_ISTABBARHIDDEN` | 默认不进入有限白名单；目标插件需单独批准 |
| 动态编辑器/lexer | `NPPM_CREATESCINTILLAHANDLE`、`NPPM_DESTROYSCINTILLAHANDLE`、`NPPM_ENCODESCI`、`NPPM_DECODESCI` | 自定义视图或 lexer 专项，不与普通命令插件混写 |
| 内部消息 | `NPPM_INTERNAL_*` | 不公开兼容；源码移植或专用适配 |

## 5. 通知矩阵

已确认并已实现的常用通知基线包括：

- 生命周期：`NPPN_READY`、`NPPN_BEFORESHUTDOWN`、`NPPN_CANCELSHUTDOWN`、
  `NPPN_SHUTDOWN`。
- 文档：`NPPN_BUFFERACTIVATED`、`NPPN_FILEBEFORELOAD`、`NPPN_FILEBEFOREOPEN`、
  `NPPN_FILEOPENED`、`NPPN_FILELOADFAILED`、`NPPN_FILEBEFORECLOSE`、
  `NPPN_FILECLOSED`、`NPPN_FILEBEFORESAVE`、`NPPN_FILESAVED`、
  `NPPN_READONLYCHANGED`。
- UI 与配置：`NPPN_LANGCHANGED`、`NPPN_WORDSTYLESUPDATED`、
  `NPPN_DARKMODECHANGED`、`NPPN_TBMODIFICATION`。

仍按真实插件需求推进：`NPPN_SHORTCUTREMAPPED`、`NPPN_CMDLINEPLUGINMSG`。
Scintilla 产生的 `SCN_*` 已逐字段转换并同步转发，使用对应主/副代理 HWND；当前真实
探针明确覆盖 `SCN_MODIFIED` 和 `SCN_UPDATEUI`，其余通知的插件专属语义继续随语料验证。

通知必须在 UI 线程发送，并保持主/副视图来源 HWND、Buffer ID、关闭前后顺序和
回调重入边界。`NPPN_READONLYCHANGED` 按原版把 Buffer ID 放在 `hwndFrom`、文档状态
位放在 `idFrom`。高频 `SCN_MODIFIED/UPDATEUI` 不能每次构造全文副本。

## 6. Scintilla 消息矩阵

| 消息族 | 代表调用 | 代理评价 |
| --- | --- | --- |
| 文本与长度 | `SCI_GETLENGTH`、`SCI_GETTEXT`、`SCI_SETTEXT`、`SCI_GETTEXTRANGE`、`SCI_INSERTTEXT`、`SCI_APPENDTEXT`、`SCI_DELETERANGE`、`SCI_REPLACESEL` | 首批支持；严格使用 UTF-8 字节长度 |
| 光标与选择 | `SCI_GETCURRENTPOS`、`SCI_GETSELECTION*`、`SCI_SETSEL`、`SCI_GETSELTEXT`、word/line/position 消息 | 首批支持；结构体和输出缓冲区专项测试 |
| 可见区与双视图 | `SCI_GET/SETFIRSTVISIBLELINE`、`SCI_LINESONSCREEN`、`SCI_VISIBLEFROMDOCLINE`、`SCI_DOCLINEFROMVISIBLE`、`SCI_SETXOFFSET` | 第二批；要求 view/document 映射稳定 |
| 样式与语言 | `SCI_COLOURISE`、`SCI_SETKEYWORDS`、`SCI_SETIDENTIFIERS` | 可转发，但 lexer 生命周期单独验证 |
| Marker/Indicator/Annotation/Fold | 各批次记录中的对应消息族 | Dock/编辑器型插件需要，不能只实现标量子集 |
| 指针缓冲区 | `SCI_GETRANGEPOINTER`、`SCI_GETCHARACTERPOINTER`、`Sci_TextRange` | 高风险；指针有效期与文档修改规则必须明确 |
| Direct call | `SCI_GETDIRECTFUNCTION`、`SCI_GETDIRECTPOINTER` | 有限旧兼容层明确不模拟；命中即转源码移植评估 |
| 私有 lexer | `SCI_PRIVATELEXERCALL`、`SCI_LOADLEXERLIBRARY` | lexer 专项；默认不进入普通插件白名单 |
| 文档指针 | `SCI_GETDOCPOINTER` | 只允许经过批准的同进程 Windows 兼容场景，不暴露给新 ABI |

## 7. 平台与运行时依赖

| 类型 | 已发现实例 | 处理路线 |
| --- | --- | --- |
| Win32/MFC/COM | 原生资源对话框、MFC、MSXML 6、WinInet、窗口枚举 | Windows legacy bridge 或源码替换，不进入跨平台 Host Services |
| 窗口 subclass/hook | 主窗口、Tab、状态栏 WndProc，`SetWindowsHookEx` | C/D；默认拒绝直接兼容 |
| Dock/UI | WinForms、原生 TreeView、Yamui、MFC 属性页 | Windows 托管原型；跨平台改 Qt UI |
| 托管运行时 | .NET Framework、CLR 导出桥、WinForms | Windows 发布依赖检查；跨平台重写宿主层 |
| 脚本运行时 | PythonScript、Python、C# 脚本、Lua | 插件间 ABI/脚本服务专项，不扩大基础白名单 |
| 外部进程/网络 | Git、OpenEdge、WakaTime CLI、下载器、REST 服务 | 关闭取消、代理、超时和隐私单独测试 |
| 本机库 | libgit2、sqlite、XML/JSON 引擎、拼写词典 | 包依赖清单和架构检查；可移植核心优先复用源码 |
| 后台线程 | 比较、拼写、网络、Timer/Task | 回调门禁、退出等待和卸载后无回调是阶段 2/5 门槛 |

## 8. 双代理结论

| 等级 | 数量 | 含义 |
| --- | ---: | --- |
| A | 13 | 有限同步消息和基础通知即可进入原 DLL 试验 |
| B | 25 | 需要有限新增消息、Dock、运行时或线程保护后试验 |
| C | 25 | direct call、复杂窗口结构、脚本宿主或特殊视图，优先源码移植/新 API |
| D | 1 | 价值不足以承担兼容风险，不支持 |
| U | 105 | 证据不足，不加载即不声称兼容 |

代理 HWND 可以保持同进程 `SendMessage`、指针参数和返回值，但不能伪装成原版可见
Scintilla 窗口层级，也不能安全提供 Qt 编辑器内部 direct function/pointer。主窗口
HWND 可用于 owner 和受控 `NPPM_*` 路由，不承诺原版 HMENU、Tab、状态栏或子窗口树。

## 9. 后续实施输入

- 最小 A 类语料：`mimeTools`，用于六导出、命令、文本和关闭流程。
- B 类语料：`JsonTools`、`NPPJSONViewer`、`nppConverter`、`NppPluginDemo`、`XMLTools`，
  用于缓冲区、Dock、原生面板和运行时。
- C 类需求样本：`ComparePlus`、`DSpellCheck`、`Explorer`、`HexEditor`，只提炼新
  Host Services，不反向扩大旧兼容层。
- `BigFiles`、`HugeFiles` 与当前内置大文件模式重叠，不作为首个兼容样本。

阶段 1 只冻结调查事实和风险分流，不宣称任何真实 DLL 已兼容。下一阶段必须先完成
架构/导出/依赖检查、加载日志、异常恢复和注册回滚，再允许加载首个语料。
