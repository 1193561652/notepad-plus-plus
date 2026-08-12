# 本地缓存：最新分析

## 2026-08-12 Session 与文档策略插件

- SessionMgr 1.4.4、AutoCodepage 1.2.4、AutoEolFormat 1.0.2 和
  nppAutoDetectIndent 2.3 官方 x64 DLL 已按 v8.4.6 清单哈希固定并通过 updater 安装。
- SessionMgr 的 session 保存/加载、Buffer 位置和应用目录宿主接口已补齐。
- 文件打开通知顺序修正为原版语义；编码菜单恢复原版 command ID；扩展名查询不含点号。
- Scintilla direct adapter 可在旧 TextRange ABI 与 Scintilla 5 ABI 间按插件回调切换。
- AutoSave 真实失焦覆盖保存和 1 分钟计时覆盖保存均已通过；此前的
  `cannot save file` 是测试进程未关闭只读文件句柄造成，不是产品保存失败。
- Debug 全构建成功，完整 CTest `44/44` 通过。
- 详情：`codex/changes/2026-08-12-session-document-policy-plugins.md`。

## 2026-08-11 EditorConfig 与 AutoSave 原版 DLL 兼容

- 官方未修改的 NppEditorConfig 0.4.0 与 AutoSave 1.6.1.0 x64 包已加入固定语料，
  SHA-256 与 v8.4.6 插件清单一致，并通过 updater 事务安装。
- EditorConfig 的缩进、Tab、EOL、保存前尾空白清理、最终换行和 reload 已由真实 DLL
  自动验证。
- AutoSave 的配置、Options、主 HWND WndProc 挂接和时间戳副本已由真实 DLL自动验证；
  新增的关键宿主接口是 `NPPM_SAVECURRENTFILEAS`，副本保存不改变当前 Buffer。
- 全量主窗口消息广播在 Qt 销毁阶段会使既有插件组合阻塞，因此未采用；AutoSave 使用
  自装 WndProc，EditorConfig 使用 NPPN/SCI，两者均不依赖全广播。
- Release 全量构建及 CTest `42/42` 通过。真实失焦覆盖保存与分钟定时触发保留为
  有交互 Windows 桌面下的人工验证项。
- 详细记录：`codex/changes/2026-08-11-editorconfig-autosave-win32-compatibility.md`。

## 2026-08-11 原版 DLL 优先决策

- NppEditorConfig 0.4.0 与 AutoSave 1.6.1.0 下一步先尝试兼容官方未修改的原版
  Windows DLL。
- NppTextFX 0.2.6 暂不决定实现路线，后续取得功能和源码证据后再单独决策。
- 后续重要插件难以兼容原 DLL 时，优先保持插件边界并重构为 Qt 版插件；默认不再
  建议直接并入 Qt 主程序原生实现。
- 详细决策见 `codex/decisions/2026-08-11-original-dll-first-plugin-porting.md`。

## 2026-08-11 后续重要插件建议

- 近期优先重新评估原版 DLL：XMLTools、DoxyIt、SurroundSelection、
  ElasticTabstops。Scintilla direct ABI 已补齐，剩余重点是命令覆盖、线程键盘 Hook、
  MSXML/MFC UI 和物理输入验证。
- SessionMgr 可继续原版 DLL 路线，但应先补完整 Session Host Services、文档生命周期
  通知和插件间消息。
- ComparePlus/Compare、DSpellCheck、Explorer、Markdown 预览、NppExec、PythonScript、
  NppFTP 和 HexEditor 应以源码移植或新 Host Services 为主，不为它们扩大旧 Win32
  窗口树和内部指针模拟。
- 本节此前关于 EditorConfig、AutoSave 优先并入 Qt 主线的建议已被上述新决策取代。

## 2026-08-11 插件启用清单配置化

- 新增配置目录独立文件 `pluginsEnabled.xml`，以插件目录名为键，仅显式
  `enabled="yes"` 时加载；缺失、无条目、禁用、非法属性或损坏 XML 均默认不加载。
- Plugins Admin 已安装页增加独立 Enabled 列，修改后立即原子保存，重启生效；卸载
  改为当前行操作。
- Windows 原版 DLL 与跨平台插件管理器共用同一启用列表，主窗口不再硬编码插件目录。
- 单元测试覆盖 XML 往返、损坏配置和默认禁用；`win32-plugin-enablement` 覆盖无配置
  启动不加载、三列表格、勾选持久化和本次运行不热加载。

## 2026-08-11 BetterMultiSelection 1.5 兼容

- Win32 编辑器接收 HWND 已透传 Scintilla Qt 的
  `SCI_GETDIRECTFUNCTION/SCI_GETDIRECTPOINTER`，修复插件 READY 阶段空 direct
  function 调用。
- 插件已加入审核加载集合；单独 READY、Enable Hook 开关、direct 状态和卸载通过，
  与 XMLTools 组合也不再出现 `0xC0000005`。
- 新增 `win32-better-multi-selection` 专项测试，完整 CTest `39/39` 通过。
- 物理键盘的多选移动、删除、换行和剪贴板操作需要人工界面验证。

## 2026-08-11 无白名单 Win32 插件加载验证

- 临时移除审核白名单并无过滤加载固定真实插件语料；18 个 DLL 注册成功，
  `PoorMansTSqlFormatterNppPlugin` 以 `LoadLibraryW` 错误 1114 正常降级。
- 后续独立进程矩阵确认：`BetterMultiSelection` 单独在 READY 阶段卡死；
  `BetterMultiSelection + XMLTools` 组合产生 `0xC0000005` 访问冲突；`XMLTools` 单独
  完成加载、READY、编辑通知和卸载，不崩溃。排除 Better 后其余全量组合不崩溃。
- 当前回调期故障没有加载中标记，不能靠下一次启动恢复机制自动归因。
- 已恢复审核白名单。当前同进程原版 ABI 不能保证任意不兼容 DLL 仅失去功能而不影响
  主程序；详见 `codex/validation/2026-08-11-unfiltered-win32-plugin-loading/README.md`。
- BetterMultiSelection v1.5 的直接根因已由源码确认：插件在 `setInfo()` 获取
  `SCI_GETDIRECTFUNCTION/SCI_GETDIRECTPOINTER`，当前隐藏编辑器 HWND 返回空值，随后
  READY 阶段的 `SCI_AUTOCSETMULTI` direct 调用解引用空函数指针。Scintilla 5 Qt 本体
  已提供对应 ABI，可由 Win32 编辑器适配层透传；之后仍需验证线程键盘 Hook、双视图
  及与 XMLTools 的组合故障。

## 2026-08-09 高、中优先级插件兼容

- 新增 5 个未经修改的官方插件 DLL：GotoLineCol、Random Values、Merge files in one、SelectToClipboard、urlPlugin。
- 主/副永久编辑器现在把原始 `SCN_*` 通知转换并同步发送给插件；测试探针验证了通知码与来源代理 HWND。
- XMLTools 因线程键盘 Hook 导致真实矩阵卡死而跳过；direct-call/Hook、Session Host Services、外部进程/网络类候选均按约束记录后暂缓。
- 记录：`codex/changes/2026-08-09-priority-plugin-compatibility.md`；缓存：`codex/cache/2026-08-09-priority-plugin-compatibility.md`。

## 当前插件通知基线

- Windows v8.4.6 常用 `NPPN_*` 已统一进入 `Win32PluginManager`：`READY`、
  `TBMODIFICATION`、文件加载/打开/保存/关闭、`BUFFERACTIVATED`、`LANGCHANGED`、
  `WORDSTYLESUPDATED`、`READONLYCHANGED`、`DARKMODECHANGED`、退出协商和 `SHUTDOWN`。
- 文件通知由实际文件生命周期代码触发并保持原版顺序；关闭 clone 时发送
  `FILEBEFORECLOSE`，仅在 Buffer 真正销毁后发送 `FILECLOSED`。
- 语言切换和只读状态由主窗口统一助手同步主/副永久编辑器；只读通知保持原版特殊
  ABI：Buffer ID 位于 `hwndFrom`，只读/dirty 状态位位于 `idFrom`。
- 真实测试 DLL 已验证通知码、Buffer ID、状态位及打开、保存、关闭顺序；Release
  全量 CTest `37/37` 通过。
- 高频 `SCN_*`、`NPPN_SHORTCUTREMAPPED` 与 `NPPN_CMDLINEPLUGINMSG` 继续按真实插件
  语料增量实现，不扩大为任意插件兼容承诺。

## 2026-08-09 JsonTools 深层矩阵与简单插件

- JsonTools Settings、RemesPath 查询/赋值、JSON Lines、YAML、树节点跳转和 4 MB
  partial/full tree 阈值已由真实 DLL 自动验证。
- `Run tests` 菜单存在，但 v3.2.0 上游测试代码无保护地依赖作者绝对目录；不在宿主中伪造
  路径或改写官方 DLL，详见本轮变更记录。
- 官方 nppConverter 4.4.0 与 NppPluginDemo 4.2 已通过现有插件管理流程安装、白名单加载和
  真实命令/Dock 验证；适配层只新增 `SCI_ADDTEXT` 与 `SCI_ENSUREVISIBLE`。
- 全量 CTest `37/37` 通过。
- 记录：`codex/changes/2026-08-09-jsontools-deep-and-simple-plugins.md`。
- 缓存：`codex/cache/2026-08-09-jsontools-deep-and-simple-plugins.md`。

## 2026-08-09 JsonTools 3.2.0 适配

- 官方 x64 CLR4/WinForms DLL 已通过插件管理语料安装，并进入 Windows 审核白名单。
- 已补齐当前路径/文件名、新建/打开、3 个 SCI 消息、`NPPN_TBMODIFICATION` 和
  `NPPN_FILEBEFORECLOSE`；关闭通知另由测试 DLL 验证 Buffer ID。
- 真实 DLL 已验证加载、10 项命令表、快捷键、pretty/compress、JSON lexer 和 WinForms
  JSON Tree Dock；注册后的 Dock 会显示并切到前台。
- JsonTools 上游使用功能索引 `4` 作为 Dock ID，菜单勾选则使用宿主分配命令 ID；适配器
  必须保留这两个标识的区别。
- 实施记录：`codex/changes/2026-08-09-jsontools-v320-compatibility.md`。
- 原评估：`codex/analysis/plugins-v846/jsontools-v320-compatibility-evaluation.md`。
- 本地缓存：`codex/cache/2026-08-09-jsontools-v320-evaluation.md`。

## 2026-08-08 Windows 初始窗口外框位置

- `AppPosition` 与原版一致，表示完整窗口外框，不是 Qt 客户区。
- 恢复时使用 frame margins 换算客户区尺寸并用 `move()` 放置外框；关闭时保存 `frameGeometry()`。
- 修复默认 `x=0, y=0` 导致标题栏位于屏幕之外、调整大小后才出现的问题。
- 详细记录：`codex/changes/2026-08-08-window-frame-geometry.md`。

## 2026-08-08 前 8 个简单 Win32 插件语料

- 8 个官方 x64 包均已通过插件管理流程安装；官方 ZIP 和 SHA-256 已进入本地测试缓存。
- Reverse Lines、Remove Duplicate Lines、SelectQuotedText、BracketsCheck、SecurePad、Code Alignment 共 6 个插件已通过真实 DLL 功能测试。
- Poor Man's T-SQL Formatter 因 CLR 2.0 激活错误 1114 跳过；BetterMultiSelection 因全局 Hook、通知链和输入状态依赖跳过。
- 同步消息适配新增配置目录、语言类型、扩展名、command ID、快捷键、空函数分隔符和插件实际使用的 Scintilla 消息。
- BracketsCheck 的直接 `HMENU` 勾选回写仍是已知 UI 差异，不应通过给 Qt 主窗口强挂原生菜单临时解决。
- 详细记录：`codex/changes/2026-08-08-simple-win32-plugin-corpus.md`。

## 2026-08-08 mimeTools 2.8 完整兼容

- 官方 `mimeTools 2.8 x64` 的 18 个 `FuncItem` 表项已接入 Plugins 菜单：4 个分隔符、
  13 个文本转换和 About；命令不再只可由测试接口调用。
- 真实 DLL 直接完成 Base64 7 项、quoted-printable 2 项和 URL 3 项；回归覆盖每个
  转换的有效输入、空选区、URL target/selection、主/副永久视图和插件字节缓冲区。
- DLL URL 实现把 `SCI_GETSELTEXT` 长度当作含 NUL 值并在清零时多写 2 字节；Qt
  adapter 仅对其 11–13 号 URL 命令的长度查询补回 NUL，文本复制语义不变。
- DLL 的 SAML Decode `tinf_uncompress()` 缺少输入边界，About 的 Win32 资源 modeless
  dialog 在 Qt 宿主中均可复现崩溃。`Win32PluginManager` 对这两个命令提供等价的安全
  替代：raw-DEFLATE zlib 解码（上限 200000 字节、原错误文案和选区替换）与非模态
  About 对话框。其他命令仍直接调用 DLL。
- `mimetools-managed-install` 与 `ui-localization-runtime-100` 通过；后续全 UI
  DPI/主题矩阵和全量 CTest 必须继续覆盖本语料。

缓存日期：2026-08-02

## 2026-08-02 主/副永久 Scintilla 视图

- 文档 UI 已从“每个标签创建一个编辑器”改为与原版一致的两个永久
  `ScintillaEditView`：`MainDocTab` 与 `SubDocTab` 各持有一个。
- `DocTabView` 由 `QTabBar + ScintillaEditView` 组成；标签只保存 `Buffer*`，切换
  标签通过 `SCI_SETDOCPOINTER` 把永久编辑器附着到 Buffer 文档。
- `Buffer` 持有经 `SCI_ADDREFDOCUMENT` 保留的 document pointer，并在关闭或主窗口
  析构时通过 `SCI_RELEASEDOCUMENT` 释放；主/副 clone 共享同一文档对象。
- 每个视图的选择区、首个可见行和水平偏移按 Buffer 独立缓存；标签切换前保存，
  切换后恢复。
- 保存、reload、定时备份、打开文档批量搜索替换、编码重解释和 Session 保存/恢复
  均先激活目标 Buffer，禁止把 `Buffer::getView()` 当作“当前正显示该文档”的保证。
- Windows 插件管理器现在直接持有两个永久编辑器及其稳定 HWND。
- `Win32PluginSystem` 新增主窗口、主编辑器和副编辑器三个独立适配层；管理器保留
  原有 handle 接口并委托给相应适配器。
- 主窗口适配器映射真实主窗口 HWND；主/副编辑器适配器分别映射主窗口左上角两个
  隐藏的 `1×1` 原生子窗口。这两个窗口当前仅作为稳定消息接收器存在。
- Win32 管理器已实现原版六导出 DLL 加载顺序，并通过 `setInfo(NppData)` 注入三个
  HWND；最初的内置 smoke DLL 已由后文记录的 Plugins Admin/updater 安装链路取代。
- 全量构建成功，CTest `31/31` 通过；UI 运行测试额外断言两个永久编辑器的唯一性，
  以及同一编辑器对象切换两个 Buffer 后文本独立保留。
- 记录：`codex/changes/2026-08-02-permanent-scintilla-views.md`。

## 2026-08-02 原版模块布局与边栏光标

- Preferences 已归位到 `src/WinControls/Preference`。
- 插件运行、清单、计划和更新器已归位到 `src/MISC/PluginsManager`；
  Plugin Admin UI 已归位到 `src/WinControls/PluginsAdmin`。
- 顶层 `src/PluginSystem` 与 `src/Preferences` 不再作为当前代码结构使用。
- 打印功能保留，并按原版归位为 `src/ScintillaComponent/Printer.*`；顶层
  `src/Printing` 已删除。
- 空的迁移/占位目录 `src/PluginSystem`、`src/Preferences`、`include` 和 `cmake`
  已删除。
- 新建 `src/Win32PluginSystem`，只用于原版 Windows DLL 插件 ABI、消息和窗口
  兼容；包管理和跨平台公共模型不迁入该目录。
- 该目录允许直接使用 Windows API；顶层 CMake 仅在 `WIN32` 时进入其独立
  `CMakeLists.txt`，非 Windows 平台完全跳过。
- Windows 启动时，`MainWindow` 在创建主/副永久编辑器后构造
  `Win32PluginManager`，传入主窗口与两个 `ScintillaEditView`，并保留对应 HWND。
- Scintilla Qt 平台层已实现 `Cursor::reverseArrow`，行号和书签边栏显示热点在
  右上角的右向指针，文本区仍使用 I-beam。
- 全目标 Debug 构建成功，CTest `31/31` 与单独 UI 光标捕获均通过。
- 记录：`codex/changes/2026-08-02-original-module-layout-and-margin-cursor.md`。

## 2026-08-01 插件管理事务闭环与索引清理

- 当前编辑器基线统一为 Scintilla 5.3.0 Qt 平台层；活跃构建文档、功能索引和
  UI 状态已清除迁移前组件与旧差异描述。
- Plugin Admin 保持 v8.4.6 四页和退出后更新流程；更新器新增全批次预检、
  平台二进制检查、同卷事务替换和失败回滚。
- Windows PowerShell ZIP 参数传递已修复；程序目录不可写时支持 UAC 更新器和
  普通桌面 token 重启。Linux/macOS 不可写目录会在退出前明确提示。
- 新增 `plugin-updater-tests`，覆盖真实 ZIP 安装、更新、卸载、错误哈希、
  缺失二进制、批次不部分提交和目录穿越。
- 当前 UI 状态以 Preferences/Shortcut Mapper/Project Panel 最终收口记录为准。
- `ENABLE_PLUGIN_SYSTEM=ON` 全量构建成功，CTest `30/30` 和隔离配置启动冒烟通过。

## 2026-08-01 Preferences 与编辑器边栏最终收口

- Preferences 19 个页面按 v8.4.6 资源结构完成固定 DLU 布局、简体中文补齐和浅色/深色截图验证。
- `Searching` 与 `FinderConfig` 只更新原配置中已有节点，不为旧配置自动增加 XML 结构。
- Scintilla 行号、书签、折叠边栏分别使用 Number、Colour、Symbol margin；书签点击使用通知中的实际行号。
- 纯文本 `clearLexer()` 和字体变更后重新应用全局样式，防止 `SCI_STYLECLEARALL` 重置行号颜色。
- 边栏回归覆盖颜色、鼠标指针、点击行书签，以及 v8.4.6 配置语料往返。
- 记录：`codex/changes/2026-08-01-preferences-margin-ui-finalization.md`。

## 2026-08-01 原版/Qt 最终 UI 对照

- 完成原版 v8.4.6 浅色/深色与 Qt 浅色/深色 100%/150% UI 矩阵。
- 修复新文档窗口标题，恢复 `new 1 - Notepad++`。
- Preferences 改为原版单一 Close/关闭命令并补充中文自动断言。
- 此处原始结论已被后续 UI 收口替代；Preferences、Shortcut Mapper 和
  Project Panel 当前状态见本文件顶部和最终变更记录。
- 构建成功，CTest `29/29`，四组 Qt UI 捕获均退出 `0`。
- 报告：`codex/analysis/2026-08-01-final-ui-parity-audit.md`。

## 2026-08-01 Lexilla 上层接入完成

- 完整支持 v8.4.6 `langs.xml` 的 9 组关键词及 `stylers.xml` 用户关键词合并。
- 简单语言 LIST 掩码、复杂语言专属槽位、嵌入 HTML 语言和语言 properties 已对齐。
- 外部 `ILexer5` 工厂边界已实现，实际插件加载仍按既定计划延期。
- Windows/MinGW 全量构建成功，CTest `29/29` 通过。
- P1 隔离运行时捕获退出码 `0`；已修复 `SCI_GETSELTEXT` 选区末字节截断回归。
- 记录：`codex/changes/2026-08-01-lexilla-integration-completion.md`。

## Scintilla 5 Qt 迁移完成

- 当前主线使用 Scintilla 5.3.0、Lexilla 5.1.9 和官方
  `ScintillaEditBase`，版本与 Notepad++ v8.4.6 对齐。
- `ScintillaEditView::execute/SendScintilla` 直接调用 `send()`。
- lexer 使用真实 `ILexer5` 和 `CreateLexer -> SCI_SETILEXER`。
- `npp-scintilla-qt` 含 Scintilla core、Qt 平台和 `SCI_OWNREGEX`；
  `npp-lexilla` 含 catalogue、lexlib、内置 lexer 和 LexUser。
- QScintilla 源码、类型、include 和 qmake 构建已经删除。
- Windows/MinGW 全量构建成功，CTest `29/29` 通过。
- 详细缓存：`codex/cache/2026-08-01-scintilla5-qt-migration.md`。

---

以下内容是按日期保留的历史缓存，组件名称和“尚未完成”描述只代表当时状态，
不得用作当前实现判断。

缓存日期：2026-07-30

## 2026-07-30 Scintilla 5 Qt 迁移决策

- 原版 v8.4.6 携带 Scintilla 5.3.0（`530`）和 Lexilla 5.1.9（`519`）。
- 后续编辑器核心固定使用这两个精确版本，不在迁移期间改用最新上游版本。
- 目标采用官方 Qt `ScintillaEditBase`、独立静态 Scintilla/Lexilla、
  真正的 `ILexer5` 和原版 `CreateLexer -> SCI_SETILEXER` 调用链。
- `ScintillaEditView::execute(SCI_*, ...)` 保留为业务边界；QScintilla
  高层类型分阶段替换。
- 迁移需要 C++17；旧、新 Scintilla 不得链接进同一可执行文件。
- 当前仅完成方案和索引记录，尚未切换实现。
- 方案：`codex/analysis/2026-07-30-scintilla5-qt-migration-plan.md`。

## 2026-07-30 Lexilla 独立静态库

- Lexilla lexer、lexlib、Catalogue 和 LexUser 已从 QScintilla 静态库中分离。
- CMake 独立构建 `npp-lexilla`，应用通过 `CreateLexer` 和
  `SCI_SETILEXER` 安装 lexer，Scintilla 接管实例生命周期。
- 当前实现复刻原版库边界与调用方向，同时保留 QScintilla 2.13.3 的旧
  `ILexer` ABI；外部 `ILexer5` 二进制兼容仍属于插件后续范围。
- 空 QScintilla 子构建重建成功，完整 CTest `29/29` 通过。
- 详细分析：`codex/analysis/2026-07-30-lexilla-architecture.md`。

## 2026-07-30 插件系统移植指导

- 已核验 v8.4.6 的插件导出、加载顺序、`NPPM_*`、`NPPN_*` 和 Dock 边界。
- 后续采用“跨平台版本化 C ABI + Windows 原版 ABI 适配器”双路线。
- 当前 Qt/C++ `IPlugin` 仅是原型，不冻结为长期二进制接口。
- 项目策略进一步明确为三类分流：有限兼容简单常用旧插件、通过新 API 移植常用
  复杂插件、低价值长尾插件不提供官方移植。
- Windows 兼容范围采用目标插件驱动的接口白名单；HexEditor 走自定义文档/视图。
- 指导文档明确 Host Services、生命周期、命令/通知/Dock、可靠性、阶段计划和
  Windows/Linux/macOS 测试矩阵。
- 文档：`codex/guides/plugin-system-porting-guide.md`。

## 2026-07-30 非插件差异 1–6 收敛

- 内置语言菜单全部接入 v8.4.6 对应的 Lexilla lexer。
- 补充高级编辑、搜索样式标记、Finder 导航、Tab/View、同步滚动、按层折叠等
  命令入口及原版命令 ID。
- Style Configurator 支持全局/lexer 颜色、字体、字形和用户自定义关键字，
  并保守写回 `stylers.xml`。
- 自动完成读取 `autoCompletion/*.xml` 的关键字、重载和参数签名。
- UDL 设计器支持新建、重命名、删除、导入和单语言导出。
- Finder 已改用只读 Scintilla 结果文档，支持结果 lexer、折叠、指示器和跳转。
- Release 全量构建成功，CTest `24/24` 与 P1 功能捕获均通过。
- 记录：`codex/cache/2026-07-30-non-plugin-gap-closure.md`。

## 2026-07-30 Ubuntu 编译适配

- Ubuntu 22.04.5、GCC 11.4、Qt 5.15.3 的 Debug 全量构建成功。
- TinyXml 在所有平台统一使用 `wchar_t`、标准宽字符函数和 Qt Unicode 文件 API，
  不再依赖 `tchar.h`、`_wtoi`、`_wtof`、`_wfopen`、`wcscpy_s` 或平台格式化分支。
- 统一使用编码往返校验，避免 QTextCodec 后端静默替换不可表示字符。
- 配置语料测试改用跨平台设置目录覆盖，并在 Linux CTest 中使用 offscreen 平台。
- CTest 22/22 通过；无头主程序启动 3 秒保持运行并创建完整隔离配置。
- 记录：`codex/changes/2026-07-30-ubuntu-build-adaptation.md`。

## 2026-07-29 Git 仓库迁移

- 当前 Git 仓库根目录是 Qt 移植唯一主线。
- 原版 v8.4.6 通过 `git show v8.4.6:<path>` 查阅。
- QScintilla、Scintilla、独立 Lexilla 和 LexUser 已纳入 `third_party/`，
  构建不再依赖旧工作区。
- 配置兼容测试语料已固定在 `tests/corpus/v8.4.6/`。
- `AGENTS.md`、`codex/`、源码、资源和测试均由当前仓库管理。
- 新仓库空构建成功，CTest `25/25` 通过。
- 决策：`codex/decisions/2026-07-29-repository-root-migration.md`。

## 2026-07-29 非人工工作最终收口

- 按用户要求排除插件系统和大文件方案 B。
- 非插件菜单没有未实现的 disabled 占位。
- UI 捕获改走真实 Mark/Preferences 入口，并使用隔离简体中文配置。
- 补齐自动插入、备份目录、文件关联、标签匹配等 Preferences 中文资源。
- 简体中文浅色/深色、100%/150% 四组 UI CTest 均通过。
- 全量构建成功，CTest `30/30` 通过。
- 当前没有可在本机自动实施的边界明确代码待办。
- 权威结论：`codex/analysis/2026-07-29-automatic-work-closure.md`。

## 2026-07-28 无人工介入待办收口

- 恢复最近关闭文件已接入原版 command ID 41021、`Ctrl+Shift+T` 和
  可测试的 LIFO 历史模型。
- Function List 已部署并加载 v8.4.6 的 34 份 XML 规则，支持用户目录覆盖、
  注释过滤、名称提取链和类范围。
- config、Qt state、FindHistory、shortcuts、session 写入会传播目录或保存失败。
- 全量构建成功，CTest 25/25 通过。
- 只剩明确需要既有决策变更、人工观察、外部系统、网络或设备的项目。
- 权威审计：`codex/analysis/2026-07-28-automatic-backlog-audit.md`。

## 2026-07-28 跨平台配置路径

- 新增 `ConfigPathResolver`，配置路径优先级统一为命令行、便携模式、平台默认。
- Windows 保持 `%APPDATA%\Notepad++`；Linux/macOS 使用标准配置目录。
- 支持原版 `-settingsDir=` 及 `--settings-dir` 跨平台别名和 `~` 展开。
- 显式目录会验证存在性和写权限，默认目录及兼容子目录创建失败会阻止加载。
- 真实进程验证确认所有 XML、`qtState.ini` 和兼容目录写入指定路径。
- 完整构建成功，CTest 23/23 通过。

## 2026-07-28 P1 收尾

- 已将已实现主菜单动作的 v8.4.6 command ID 集中到
  `src/NppCommandRegistry.*`，修复 Word Wrap、Restore Zoom 和反向排序映射。
- 已补齐 Windows 文件关联管理：原版扩展名分类、管理员限制、ProgID 写入、
  原关联备份和移除恢复；非 Windows 保留系统设置入口。
- CMake Debug 构建成功，CTest 21/21 通过。
- 本轮没有启动主程序，没有执行截图、真实网络共享、物理打印机或注册表写入验证。
- 待授权运行项见 `codex/validation/2026-07-28-p1-pending/README.md`。

## 2026-07-28 P1 运行验证

- UI 基线和 P1 功能捕获程序均退出码 0。
- Windows 管理员文件关联测试完成注册、枚举、打开命令、恢复和清理，扫描无
  临时扩展名残留。
- CTest 21/21 通过。
- 主程序已启动供用户检查。
- 真实 UNC/SMB、物理打印机和用户主观 UI 对照仍待对应环境或反馈。
- 报告：`codex/validation/2026-07-28-p1-runtime/README.md`。

## 2026-07-28 UI 完善

- `toolbarIcons.xml` 已按 v8.4.6 目录和固定文件名语义接入，支持部分图标集、
  `_disabled.ico` 和内置回退。
- 工具栏使用 16 px 图标，补齐已实现的 Print、UDL 和 Document List 按钮。
- 深色模式扩展到菜单、工具栏、状态栏、Dock、Tab、对话框控件和滚动条；
  编辑区继续尊重 `stylers.xml`。
- 浅色/深色截图矩阵退出码 0，最终深色截图无已知漏白、不可读标题或文字按钮。
- 完整构建成功，CTest 22/22。
- 报告：`codex/validation/2026-07-28-ui-polish/README.md`。

## 范围

本缓存覆盖 `./` 主线项目的项目分析、代码索引、知识库建立、功能分析，以及原版 Notepad++ v8.4.6 与 Qt 移植版的功能/逻辑差异对照。迁移计划阶段 0 到阶段 7 的当前基础范围均已完成，并已通过 CMake 构建验证。

## 已扫描

- 主线目录结构。
- `CMakeLists.txt`。
- `README.md` 和 `BUILD_AND_TEST.md`。
- `src/MainWindow.h|cpp` 的主要入口。
- `src/Parameters.h|cpp` 的配置入口。
- `src/ScintillaComponent/ScintillaEditView.h|cpp` 的编辑器适配入口。
- `src/ScintillaComponent/Buffer.*`。
- `src/MISC/FileManager.*`。
- `src/WinControls/TabBar/DocTabView.*`。
- `src/ScintillaComponent/FindReplaceDlg.h`。
- `src/MISC/PluginsManager/IPlugin.h`、`PluginManager.h`。
- `resources/resources.qrc`。
- 原版 `PowerEditor/src/` 下的关键模块，包括 `Notepad_plus.cpp`、`NppIO.cpp`、`Parameters.*`、`Buffer.*`、`FindReplaceDlg.*`、`PluginsManager/*`、`menuCmdID.h` 等。
- 阶段一 XML 资源基础：默认 XML 模型、qrc 回退、配置目录骨架。

## 主要结论

- `./` 是唯一主线，已经具备基础编辑器架构，不是空项目。
- 当前核心协调类是 `MainWindow`，职责覆盖文件、视图、菜单、状态栏、会话、偏好设置和插件入口。
- 配置系统集中在 `NppParameters`，使用 TinyXml 管理 Notepad++ 风格 XML。
- 文件管理由 `Buffer`、`FileManager`、`DocTabView` 和 `MainWindow` 共同完成。
- 编辑器层由 `ScintillaEditView` 包装 QScintilla，并保留 `execute()` / `SendScintilla()` 兼容入口。
- 查找替换已有完整 UI 骨架和部分行为入口，但 Regex、Find in Files、偏移语义需要进一步核验。
- 插件系统已有 Qt 化接口和 `PluginManager`，但近期只应作为预留接口维护。
- 原版功能覆盖面远大于 Qt 版，Qt 版当前是核心功能子集。
- 最大差异集中在配置完整读写兼容、Buffer/Document 生命周期、编码/EOL、搜索体系和插件 ABI。
- 阶段一已补齐默认 XML 资源入口：`config.model.xml`、`langs.model.xml`、`shortcuts.xml`、`contextMenu.xml`、`toolbarIcons.xml`、`userDefineLang.xml`。
- 阶段一已让 `langs` 和 `shortcuts` 加载支持 Qt 资源回退。
- 阶段一已实现默认 XML 缺失补齐，以及 `config.xml` / `shortcuts.xml` 的保守写回策略。
- 阶段二已补强文件打开/reload 的 BOM、编码和 EOL 检测。
- 阶段二已补强保存写入校验、最近文件更新、Close All 主/副视图去重关闭。
- 阶段二已补强会话保存/恢复的副视图、clone view、selection、滚动位置和编码字段。
- 阶段三已补齐原版主要菜单 UI 骨架，未实现业务入口以 disabled 占位。
- 阶段三已让工具栏和状态栏从 `config.xml` 初始化显示/隐藏，并在 View 菜单提供切换入口。
- 阶段三已为后续阶段 4 UI 一致性检验建立本地分析入口。
- 阶段四已完成代码级 UI 一致性审计，并保存 `stage-4-ui-consistency-audit.md`。
- 阶段四已将 Qt 版顶层菜单顺序调整为原版顺序，移除非原版顶层 `Format`，新增 `Tools` 骨架，将 Help 标题调整为 `?`。
- 阶段五已补齐当前文档查找历史、Find Next/Previous、Mark 入口、新建文档默认 EOL/编码、快照备份配置生效和外部文件变更基础检测。
- 阶段六已补齐 Find in Files 基础搜索、所有打开文档查找、跨文件 Find Result 跳转和 Dark Mode 基础偏好设置。
- 阶段七已完成后置功能基础闭环：Qt 打印、宏生命周期修正、列编辑、高频排序/文本处理、UDL 只读加载与基础高亮、插件加载边界收口。

## 本地知识库入口

- `codex/index/repository.md`
- `codex/index/modules.md`
- `codex/index/build.md`
- `codex/analysis/migration-plan.md`
- `codex/analysis/stage-1-xml-configuration.md`
- `codex/changes/2026-07-22-stage-1-xml-resources.md`
- `codex/changes/2026-07-22-stage-2-file-buffer-session.md`
- `codex/changes/2026-07-22-stage-3-ui-skeleton.md`
- `codex/analysis/stage-3-ui-skeleton.md`
- `codex/changes/2026-07-22-stage-4-ui-consistency.md`
- `codex/changes/2026-07-22-stage-5-important-features.md`
- `codex/changes/2026-07-22-stage-6-complex-compat.md`
- `codex/changes/2026-07-22-stage-7-final-features.md`
- `codex/analysis/stage-4-ui-consistency-audit.md`
- `codex/modules/notepad-plus-plus/overview.md`
- `codex/modules/notepad-plus-plus/configuration.md`
- `codex/modules/notepad-plus-plus/file-buffer.md`
- `codex/features/configuration.md`
- `codex/features/file-management.md`
- `codex/features/editor-buffer.md`
- `codex/features/ui-menus-toolbars.md`
- `codex/features/search-replace.md`
- `codex/features/plugin-system.md`
- `codex/features/advanced-editing.md`
- `codex/analysis/original-vs-qt-feature-comparison.md`
- `codex/analysis/2026-07-22-implementation-feature-gap-report.md`
- `codex/cache/2026-07-22-original-vs-qt-diff.md`

## 待深入分析

- 配置 XML 的逐文件、逐节点兼容性应作为最优先分析与实现范围。
- 原版配置 XML 的逐节点兼容性。
- 文件打开的完整编码检测，尤其是非 UTF-8、无 BOM、多语言 locale 和大文件场景。
- `MainWindow` 菜单/动作/快捷键映射。
- `DocTabView` 克隆视图和 Buffer 生命周期。
- `ScintillaEditView` 的 Scintilla 消息兼容范围。
- Replace in Files、Replace All in Opened Docs、Find in Projects 和原版 Regex 行为兼容。
- Dark Mode 原版色彩系统、图标资源和视觉细节对齐。
- 外部文件删除、重命名、权限变化等边界处理。
- 插件接口与原版 Notepad++ 插件 ABI 的差异。
# 2026-07-23 最新状态

主线 `./` 已完成一轮非插件功能补全。主菜单仅保留插件导入为禁用项；9 个偏好设置 stub 页已替换为实际页面。

关键新增：

- 文件平台服务、批量关闭、重命名、回收站、手工会话。
- 打开文档批量替换和文件批量替换，保留编码/BOM，跳过脏打开文档。
- Document List、3 Project Panels、Clipboard History、Character Panel。
- MD5/SHA-256、Run、UserDefinedCommands、Window、Help。
- `contextMenu.xml` 驱动编辑器右键菜单。
- config.xml 原版节点扩展读写、未知属性/节点保留、默认 XML 写权限修复。
- 会话书签与折叠状态。

验证：

- CMake Debug 构建通过。

- Windows GUI 自动关闭烟雾测试退出码 0。
- 便携配置未知属性和未知节点往返保留。

详细记录：

- `codex/changes/2026-07-23-non-plugin-port-completion.md`
- `codex/analysis/2026-07-23-non-plugin-port-status.md`

该段为 2026-07-23 时点状态；其中 Shortcut、打印、workspace、UDL、Boost.Regex、
命令行全集和大文件方案 A 已在后续专项完成。当前仍需跨平台、真实环境和像素级 UI
一致性验证。

## 2026-07-23 本地化增量

- 已修复多个 `Entries` 分组只解析首组造成的子菜单漏译。
- 本地化首次应用已延后到完整 UI 树创建完成。
- 当前主窗口静态菜单、动作、停靠面板和工具栏的简体中文资源覆盖率检查无已知缺项。
- 偏好设置 19 个页面的静态控件、下拉选项、单位和标准按钮已纳入简体中文资源，覆盖率反查无已知缺项。
- CMake Debug 构建通过。

## 2026-07-25 最新缓存

编码/EOL、Buffer/双视图、会话和搜索替换已完成一轮基础差异修复，并通过应用构建和
`core-behavior-tests`。最新详细缓存：

- `codex/cache/2026-07-25-encoding-buffer-session-search.md`
- `codex/changes/2026-07-25-core-compatibility-fixes.md`
- `codex/analysis/2026-07-25-core-compatibility-status.md`

## 2026-07-25 编码检测与 Session UI 增量

- uchardet、XML/HTML 编码声明、UTF-8 cookie 和代码页映射已接入主线。
- Session 标签颜色、Document Map 状态和文件浏览器选中项已应用到 UI。
- 大文件模式与 Boost.Regex 全语义明确延期，等待后续决策。
- 最新缓存：`codex/cache/2026-07-25-encoding-session-ui-completion.md`
- 变更记录：`codex/changes/2026-07-25-encoding-detection-session-ui.md`
- 决策记录：`codex/decisions/2026-07-25-deferred-large-file-and-boost-regex.md`

## 2026-07-26 大文件模式决策

- 大文件模式延期已解除，第一版采用完全复刻 Notepad++ v8.4.6 的方案 A。
- 固定 200 MiB 阈值并静默降级；方案 A 缺陷作为已接受风险。
- 方案 B 的非模态提示、状态标记、按 Buffer Wrap 和进度/取消仅作为后续可选优化。
- 该历史状态已由下方“大文件模式方案 A 完成”记录取代。
- 最新缓存：`codex/cache/2026-07-26-large-file-mode-decision.md`
- 功能索引：`codex/features/large-file-mode.md`
- 决策记录：`codex/decisions/2026-07-26-large-file-mode-v846-parity.md`

## 2026-07-26 P0 配置与 UI 验证

- 配置真实语料测试已覆盖 v8.4.6、v7.8.1 和当前隔离用户配置，共 14 项。
- config、FindHistory、最近文件和 Session 已改为保守原位写回，保留未知结构。
- v8.4.6 原版成功读取 Qt 写回后的隔离配置。
- Qt 100% / 150% UI 矩阵均覆盖主窗口、5 个查找页和 19 个首选项页。
- Windows UI 字体已对齐为 Segoe UI，主窗口和 Find 标题已去除 Qt 差异。
- 详细报告：`codex/analysis/2026-07-26-p0-config-ui-validation.md`
- 本地缓存：`codex/cache/2026-07-26-p0-config-ui-validation.md`

## 2026-07-26 大文件模式方案 A 完成

- v8.4.6 固定 200 MiB 大文件模式已实现。
- 已接入带原版选项的 Scintilla 文档、128 KiB 流式加载/保存、功能降级、
  reload、Session、双视图和指针宽度搜索替换。
- 主程序构建成功，完整 CTest `17/17` 通过。
- 方案 B 仍是后续可选优化。
- 最新缓存：`codex/cache/2026-07-26-large-file-mode-implementation.md`
- 变更记录：`codex/changes/2026-07-26-large-file-mode-v846.md`

## 2026-07-26 P0 功能兼容完成

- 已完成 URL Hotspot、XML/HTML 标签匹配、字符/标签自动插入和保存前备份。
- 已完成 InternalCommands、ScintillaKeys、NextKey、原版 command ID 映射及宏 type 0/1/2 序列。
- 已完成 workspace XML、三个 Project Panel，以及严格限定到 workspace 成员的 Find/Replace in Projects。
- Debug 构建成功，完整 CTest `18/18` 通过。
- 按用户要求未启动 GUI、未截图、未执行人工交互确认；等待明确授权。
- 功能索引：`codex/features/p0-editor-shortcuts-workspace.md`
- 分析报告：`codex/analysis/2026-07-26-p0-functional-compatibility-report.md`
- 本地缓存：`codex/cache/2026-07-26-p0-functional-compatibility.md`
- 变更记录：`codex/changes/2026-07-26-p0-functional-compatibility.md`

## 2026-07-26 P0 运行时验证

- UI 捕获已覆盖主窗口、Project Panels、5 个查找页、19 个首选项页和
  Shortcut Mapper 四个分类页。
- 运行时发现并修复 Shortcut Mapper 平铺全部 QAction 的差异。
- Main menu、Macros、Run commands、Scintilla commands 现已分类显示和写回。
- 完整 Debug 构建成功，CTest `18/18` 通过。
- 截图和验证记录：`codex/validation/2026-07-26-p0-runtime/README.md`。

## 2026-07-28 命令行兼容

- 完成 Notepad++ v8.4.6 命令行参数全集建模和启动行为接入。
- 完成 `QLocalServer` 单实例参数转发，并按配置目录隔离实例组。
- 完成 settingsDir、本地化、会话、语言/UDL、行列/位置、只读、监视、
  工作区、托盘、打印和 Function List 导出参数。
- Debug 全量构建通过，CTest `19/19`。
- 真实进程 Function List 导出和双实例 IPC 验证通过。
- 说明：`codex/features/command-line-compatibility.md`。
- 验证：`codex/validation/2026-07-28-command-line/README.md`。

## 2026-07-26 P1 兼容开发

- Finder、Copy Marked Text、外部文件变化、打印、高级排序、Column Editor、
  UDL 2.1 和保存搜索宏 type 3 的代码开发已完成。
- 原版 LexUser 已编入独立 `npp-lexilla` 静态库。
- Debug 构建成功，CTest `18/18` 通过。
- 按用户要求未启动程序、未截图、未执行人工交互验证。
- 最新缓存：`codex/cache/2026-07-26-p1-compatibility.md`。
- 待授权验证：`codex/validation/2026-07-26-p1-pending/README.md`。

## 2026-07-26 P1 运行验证完成

- P1 专用运行验证退出码 0。
- Finder、剪贴板、保存搜索宏、排序、Column Editor、UDL、PDF 打印和外部文件
  变化均已验证。
- 修复全文排序额外空行和 Finder 结果叠绘。
- 全量 Debug 构建成功，CTest `18/18` 通过。
- 验证报告：`codex/validation/2026-07-26-p1-runtime/README.md`。

## 2026-07-28 Find 对话框本地化

- 修复通过 Mark、Find Next/Previous、Find in Files、Find All 和 Project Panel
  打开或使用 Find 对话框时，重建后的控件保持英文的问题。
- 根因是语言切换会销毁隐藏的 Find 对话框，而部分入口重建后没有调用
  `NativeLangSpeaker::changeDlgLang()`。
- 所有入口现统一经过 `MainWindow::ensureFindReplaceDialog()`。
- Find in Projects 页补齐 3 个项目面板选项和 3 个按钮的控件标识及中文资源。
- Debug 主程序构建成功；CTest `26/26` 通过。
- 变更记录：`codex/changes/2026-07-28-find-dialog-localization.md`。

## 2026-07-30 插件管理首轮移植

- 完成跨平台插件 artifact 定位、v8.4.6 形状的清单解析和四类 Plugin Admin
  数据模型。
- 完成 Qt Plugin Admin 对话框，以及退出后执行下载、SHA-256、ZIP 校验、
  安装/更新/卸载和重启的独立更新器。
- Windows 保持原版插件路径；Linux/macOS 使用插件目录内的平台子目录，不增加
  架构目录。管理器与现有加载器共用定位实现。
- 已从 GitHub `nppPluginList/v1.5.4` tag 原样归档 x86 169 项、x64 134 项、
  ARM64 20 项清单；合并后共 178 个插件。Windows 按进程架构启用相应清单，
  Linux/macOS 保持空清单，避免安装 Windows DLL。
- Ubuntu 下主程序、更新器和 `plugin-admin-tests` 构建通过，针对性测试通过；
  Windows 与真实插件包验证项已单独记录。
- 插件清单采集方法和逐插件兼容调查模板已写入 `codex/analysis/`。

## 2026-08-01 插件移植阶段 0/1

- 阶段 0 已按实际依赖重排插件路线；插件包管理确认为已完成基线。
- 阶段 1 已覆盖 v8.4.6 x86 清单 169/169 项：62 项有对应版本源码证据，107 项
  因证据不足保持 `U`，未反汇编。
- 兼容初评为 A 13、B 23、C 25、D 1、U 107；重要度为高 20、中 94、低 55。
- 统一结论：`codex/analysis/plugins-v846/api-dependency-matrix.md`。
- `plugin-investigation-tests` 自动校验 JSON、索引、重要度、六批次和统一矩阵。
- Debug 全目标构建成功，CTest `31/31` 通过。
- 阶段 1 不代表真实 DLL 已兼容；下一阶段为安全加载基础。

## 2026-08-01 编辑器边框一致性

- 原版 v8.4.6 的 `ScintillaEditView::setBorderEdge()` 在浅色模式使用 3D client edge，
  深色模式使用单线 border；Qt 官方基类默认强制 `QFrame::NoFrame`。
- Qt 适配层已恢复对应 frame 语义，并接通 `ScintillaPrimaryView borderEdge`、
  `borderWidth` 与 Preferences 的“无边缘”和宽度滑块。
- 浅色使用 `WinPanel/Sunken` 双线边缘，深色使用 `Box/Plain`；DocTabView 按原版
  默认保留 2px 外部间距，宽度配置范围为 0-30px。
- UI 捕获自动断言边框类型和默认外部间距，覆盖 100%/150% DPI。
- Debug 全目标构建成功，CTest `31/31` 通过。
# 2026-08-02 Win32 real plugin fixture

- Win32 ABI 运行语料由自制 `NppWin32SmokePlugin.dll` 改为官方插件列表中的真实
  `mimeTools 2.8 x64`。
- 固定包来自官方 GitHub Release；ZIP SHA-256 为
  `ed5133f8a0552e974135ada78a0260581be979f916ddbcd45697d2a1a1b8f280`，与
  `pl.x64.json` 一致；DLL SHA-256 为
  `b9a8ca258aa3edca1aa1b3ea4e264d3b0cda7c82a30b7464586d8be95701ea61`。
- DLL 为 PE x86-64，只导入 `KERNEL32.dll`、`USER32.dll`，并导出
  `beNotified/getFuncsArray/getName/isUnicode/messageProc/setInfo`。
- 插件管理 updater 已从官方 Release 真实下载、校验并事务安装到
  `build/plugins/mimeTools`，并生成 `.npp-package.json` 收据。
- Win32 加载器改为扫描 `NppPath/plugins`。当前测试传入 `mimeTools` 白名单；加入
  另一个结构合法的 DLL 目录后仍只加载 `MIME Tools`，证明过滤生效。
- CTest 使用相同 updater 和标准计划从固定 ZIP 缓存重复安装，随后验证收据、插件名
  和非空命令表；应用构建不再直接复制测试 DLL。
- Windows 全目标构建成功，CTest `32/32` 通过，其中
  `mimetools-managed-install` 是新增的插件管理安装夹具。
- `Win32MainWindowAdapter` 已通过 Win32 subclass 处理
  `NPPM_GETCURRENTSCINTILLA`；主/副 editor adapter 通过代理 HWND 白名单转发
  mimeTools v2.8 使用的九个标准 `SCI_*`。
- 官方 mimeTools DLL 的 Base64 Encode 已分别修改主/副永久视图，URL Encode 已
  覆盖显式 target range、selection 和插件分配输出缓冲。
- 永久视图迁移将标签容器改为独立 `QTabBar` 后，Qt 默认的 expanding 行为曾把少量
  标签拉伸到整行；`NppTabBar` 现显式 `setExpanding(false)`，恢复原版按内容宽度布局。

## 2026-08-08 插件加载恢复基线

- 完成 JSON Lines 结构化加载日志和原子加载中标记，状态位于用户配置目录
  `plugin-load/`。
- 异常退出后，下次启动跳过最后正在加载的插件一次，记录恢复事件、清除标记并显示提示。
- 注册失败会释放 DLL、恢复命令 ID 检查点，不污染已加载插件集合。
- `-noPlugin` 经命令行解析回归验证，不创建插件管理器、代理 HWND 或加载日志。
- PE 架构和依赖预检按用户决策延期。
- 详细记录：`codex/changes/2026-08-08-plugin-load-recovery.md`。

## 2026-08-08 Qt DockingManager 结构恢复

- 新增与原版职责对应的跨平台 `DockingManager`、`DockingData` 和四侧
  `DockingCont`；底层继续使用 Qt Dock 布局引擎。
- `MainWindow` 不再直接创建、添加、显示隐藏或持久化 `QDockWidget`，全部 10 个
  内置面板经管理器注册。
- 同侧 Dock 标签化，四侧尺寸重新读写原版 `config.xml/DockingManager` 属性，
  完整 Qt 布局继续保存在 `qtState.ini`。
- `AGENTS.md` 已将原版逻辑、类职责、生命周期和调用关系提升为强制移植基线。
- 详细记录：`codex/changes/2026-08-08-qt-docking-manager-structure.md`。

## 2026-08-09 全量回归与 DarkMode 兼容

- Release 全量构建通过，CTest `37/37` 通过。
- 重新采集原版 v8.4.6 亮/暗 100% 和 Qt 版亮/暗 100%/150% UI 矩阵，
  覆盖 Find 与 Preferences 全部页面。
- Qt 版改为读写原版 `DarkMode@enable`；旧的
  `qtState.ini/Editor/darkMode` 不再覆盖公共配置，并会在写入时删除。
- 原版 v8.4.6 成功回读 Qt 写出的亮色和暗色配置。
- 验证报告：`codex/validation/2026-08-09-full-regression/README.md`。
- 修改记录：`codex/changes/2026-08-09-dark-mode-config-compatibility.md`。

## 2026-08-09 原版结构与接口审计

- 以 `v8.4.6:PowerEditor/src` 为基线完成目录、所有者、核心接口和调用逻辑审计。
- EncodingMapper、localization、FileManager、DocTabView、自动完成、文件关联、
  ToolBar、DocumentMap、FileBrowser、FunctionList 和 preferenceDlg 已归位到原版职责目录。
- `DockingWnd` 只保留 DockingManager，业务面板不再混入布局管理目录。
- MainWindow 的插件宿主方法改为平台中性命名；`NPPM_*` 解释继续只存在于
  `Win32PluginSystem`。
- 主控制器已按原版职责拆为 `Notepad_plus.cpp`、`NppCommands.cpp`、`NppIO.cpp`、
  `NppNotification.cpp` 和 Qt 窗口宿主 `MainWindow.cpp`；类状态和调用关系保持不变。
- 审计：`codex/analysis/2026-08-09-original-structure-interface-audit.md`。
- 修改：`codex/changes/2026-08-09-original-structure-alignment.md`。
- Release 全目标构建和 CTest `37/37` 通过；验证见
  `codex/validation/2026-08-09-original-structure-alignment/README.md`。

## 2026-08-10 主控制器结构拆分

- 文件/Buffer/Session/编码流程：`src/NppIO.cpp`。
- 菜单、搜索替换、编辑命令和 QAction 状态：`src/NppCommands.cpp`。
- 编辑器、标签、焦点及生命周期通知：`src/NppNotification.cpp`。
- 构造、析构和命令行顶层协调：`src/Notepad_plus.cpp`。
- Qt 窗口、Dock/面板和插件宿主服务：`src/MainWindow.cpp`。
- Release 全目标构建和 CTest `38/38` 通过；详见
  `codex/validation/2026-08-10-main-controller-structure-split/README.md`。

## 2026-08-11 P0 Win32 插件兼容

- 完成官方未修改的 XMLTools、DoxyIt、SurroundSelection 和 ElasticTabstops x64
  安装、加载与核心功能兼容。
- 修正旧 direct Scintilla ABI 与当前 HWND 消息 ABI 混用导致的访问冲突风险。
- Release 全目标构建成功，CTest `41/41` 通过；详见
  `codex/changes/2026-08-11-p0-win32-plugin-compatibility.md`。
