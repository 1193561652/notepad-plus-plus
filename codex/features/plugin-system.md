# 插件系统功能索引

## 2026-08-09 高、中优先级真实插件批次

- 白名单新增 GotoLineCol 2.4.2.0、RandomValuesNppPlugin 0.2.1、Merge files in one 1.2.0.0、SelectToClipboard 1.0.3、urlPlugin 1.2.0.0；插件 DLL 未修改。
- `Win32PluginManager` 现在把两个永久编辑器的 `SCN_*` 逐字段转换为插件 ABI，并保留主/副代理 HWND 来源。
- 本批次新增的 `NPPM_*` / `SCI_*` 仅来自上述插件的对应版本源码调用证据。
- XMLTools、DoxyIt、SurroundSelection、ElasticTabstops、SessionMgr、Linter、WakaTime 的跳过边界见 `codex/changes/2026-08-09-priority-plugin-compatibility.md`。

## 范围

插件系统分为两个独立范围：

- 插件包管理：清单、已安装状态、安装、更新、卸载和退出后更新器，当前已实现。
- 插件加载运行：规划和 v8.4.6 x86 静态调查已完成；原版 Windows ABI 与未来
  跨平台 ABI 当前仍是隔离的预留接口。

完整 ABI 决策和插件分级见 `codex/guides/plugin-system-porting-guide.md`。

## 管理入口

- `src/MISC/PluginsManager/PluginCatalog.*`：解析 v8.4.6 插件清单、版本区间和旧版兼容映射。
- `src/MISC/PluginsManager/PluginArtifactResolver.*`：统一管理器与加载器的平台动态库路径。
- `src/WinControls/PluginsAdmin/PluginAdminModel.*`：Available、Updates、Installed、Incompatible 分类。
- `src/WinControls/PluginsAdmin/PluginAdminDialog.*`：四页、搜索、描述、批量选择和退出确认。
- `src/MISC/PluginsManager/PluginUpdatePlan.*`：持久化并验证退出后操作计划。
- `src/MISC/PluginsManager/updater/PluginArchiveExtractor.*`：ZIP 列表、目录穿越和符号链接防护。
- `src/MISC/PluginsManager/updater/PluginUpdateExecutor.*`：下载、SHA-256、平台二进制预检和事务提交/回滚。
- `src/MISC/PluginsManager/updater/PluginUpdaterMain.cpp`：等待主进程退出、执行计划并重启。
- `MainWindow::showPluginAdmin()` / `schedulePluginOperations()` /
  `launchPendingPluginUpdater()`：主程序协调入口。

## Windows 原版插件兼容入口

- `src/Win32PluginSystem/Win32PluginManager.*` 只在 Windows 构建。
- `Win32MainWindowAdapter`、`Win32MainEditorAdapter` 和
  `Win32SubEditorAdapter` 分别固化主窗口、主编辑器和副编辑器的 Qt 对象及
  稳定 HWND，作为后续 `NPPM_*` / `SCI_*` 消息适配边界。
- 主窗口适配器使用 Qt 主窗口真实 HWND；主/副编辑器适配器使用管理器在主窗口
  `(0,0)` 创建的两个隐藏 `1×1` Win32 子窗口，并分别映射永久主/副编辑器对象。
- 两个代理窗口通过 `GWLP_USERDATA` 分发到主/副 editor adapter；支持的 `SCI_*`
  由 adapter 原样调用 Scintilla 5 消息入口，未知消息回落到 `DefWindowProcW`。
- 主窗口 adapter 通过 Win32 subclass 处理 `NPPM_GETCURRENTSCINTILLA`，根据 Qt
  主窗口当前活动的永久编辑器写回 `MAIN_VIEW/SUB_VIEW`。
- `Win32PluginManager::loadPlugin()` 已按 v8.4.6 六导出 ABI 加载 DLL，并将三个
  适配 HWND 组成 `NppData` 后调用插件 `setInfo()`。
- `Win32PluginManager::loadPlugins()` 扫描 Plugins Admin/updater 共用的
  `NppPath/plugins`；当前测试调用传入 `mimeTools` 白名单，只加载该文件夹。
- Windows x64 CTest 先用 `npp-plugin-updater` 按标准计划安装已校验的官方
  mimeTools ZIP，再验证安装收据、插件名 `MIME Tools` 和非空命令表。人工集成测试
  已通过同一 updater 从官方 Release 实际下载并安装。
- mimeTools v2.8 使用的九个 `SCI_*` 已全部加入白名单。`FuncItem` 的 18 个表项
  （4 个分隔符、13 个文本转换和 About）已进入 Plugins 菜单；13 个转换均有真实
  安装 DLL 的菜单触发回归。Base64、quoted-printable 和 URL 直接执行 DLL；SAML 与
  About 因官方 DLL 在 Qt 宿主中的可复现崩溃，由 Win32 兼容层提供等价、安全的实现。
- URL 命令保留 DLL 执行，但只在其 `SCI_GETSELTEXT` 长度查询中适配原 DLL 假定的 NUL
  长度，以修复其自身临时输出缓冲区少分配问题。
- `MainWindow` 完成主/副 `DocTabView` 创建后立即构造管理类，并传入主窗口与
  两个永久 `ScintillaEditView`。
- 管理类保存 Qt 窗口引用并提供相应 `HWND`，供后续原版六导出、消息路由和 Dock
  兼容层使用。
- 当前 Qt 版与原版一样拥有永久的主/副编辑器，插件可长期保存这两个稳定 HWND；
  标签切换只更换其 Scintilla document pointer。
- 常用 v8.4.6 通知已由 `Win32PluginManager` 统一构造：启动/退出协商、Buffer 激活、
  打开/保存/关闭、加载失败、语言、样式、只读状态和暗色模式。主窗口只在拥有相应
  生命周期的代码路径触发通知，适配层保持原版 `hwndFrom`、`idFrom` 和状态位语义。
- `NPPN_READONLYCHANGED` 按原版把 Buffer ID 放在 `hwndFrom`，把
  `DOCSTATUS_READONLY` / `DOCSTATUS_BUFFERDIRTY` 放在 `idFrom`；双视图只读状态通过
  `MainWindow::setBufferReadOnly()` 同步。

## 当前行为

- Windows 内置 `nppPluginList/v1.5.4` 的 x86、x64、ARM64 JSON，并按进程架构选择。
- Linux/macOS 在没有本平台原生包清单前使用有效空清单，避免误装 Windows DLL。
- 路径保持既定兼容规则：Windows `plugins/<name>/<name>.dll`，Linux
  `plugins/<name>/linux/<name>.so`，macOS `plugins/<name>/macos/<name>.dylib`。
- 安装、更新和卸载都在主程序正常退出后执行，完成后重启，与 v8.4.6 工作流一致。
- 所有非卸载包先完成下载、SHA-256、ZIP 路径、符号链接和目标平台二进制检查；
  全部预检成功后才修改插件目录。
- 提交在插件根目录内使用 staging/backup 原子换位；批处理中途失败会逆序恢复旧目录。
- 成功安装写入 `.npp-package.json`；Windows 手工安装 DLL 可回退读取文件版本。
- Windows 程序目录不可写时使用 UAC 启动更新器，完成后使用桌面 shell 的普通用户
  token 重启主程序，避免主程序继承管理员权限。
- Linux/macOS 不自动调用系统提权工具；不可写目录会在退出前明确提示。

## 安全边界

- 更新计划拒绝空计划、重复/越界目录、`Config` 目录、非法 URL 和非法 SHA-256。
- 主程序把计划内容 SHA-256 随更新器命令行传递；更新器一次读取并在解析前校验，
  防止退出/UAC 交接期间替换计划文件。
- 远程下载遵循 Qt 代理/TLS，限制为 HTTP(S)；自动化测试使用 `file:` 本地包。
- ZIP 在解压前检查绝对路径与 `..`，解压后拒绝符号链接。
- 包必须包含当前平台约定位置和名称的动态库，否则整个批次不提交。
- 当前按已确认方案只校验包 SHA-256，不实现插件列表、更新器或动态库签名校验。

## 构建边界

- Plugin Admin 和 `npp-plugin-updater` 始终构建，不依赖 `ENABLE_PLUGIN_SYSTEM`。
- `ENABLE_PLUGIN_SYSTEM` 默认关闭，只控制实验性 Qt/C++ `IPlugin` 运行接口。
- 当前 `IPlugin` 不是冻结 ABI，也不等同于 Notepad++ 原版插件兼容。

## 验证

- `plugin-admin-tests`：版本、清单条目、兼容区间、平台路径、发现、收据和计划验证。
- `plugin-updater-tests`：真实 ZIP 安装/更新/卸载、错误哈希、缺失平台二进制、
  批次预检不改旧插件和目录穿越拒绝。
- 更新器子进程测试覆盖计划哈希不匹配时拒绝执行并保留诊断文件。
- Windows 构建已验证 PowerShell ZIP 实际执行路径。
- `plugin-investigation-tests`：核对 v8.4.6 x86 清单与 169 项 inventory、importance、
  六个字母批次、兼容等级和 API/依赖统一矩阵。
- `ui-plugin-registration-rollback` 使用真实测试 DLL 记录通知，验证打开、保存、关闭的
  顺序和 Buffer ID，以及激活、语言、只读、样式、主题和退出协商通知的 ABI 参数。
- 真实公网下载、UAC 交互确认和非 Windows 原生插件包仍属于发布环境/人工矩阵，
  不是代码逻辑缺口。

## 仍延期的插件工作

- 安全加载剩余项：PE 架构和依赖预检；本轮按决策暂不实现。
- Windows v8.4.6 原版插件 ABI 已形成有限兼容层；常用 NPPN 和 `SCN_*` 转换基线已完成，
  后续继续补齐真实语料需要的 Host Services 和插件专属通知语义，不承诺任意旧插件通用兼容。
- 跨平台稳定插件 ABI 及 SDK。
- Linux/macOS 原生插件生态清单、签名、公证和发布策略。

## 2026-08-11 可配置插件启用清单

- 启用状态独立保存在当前应用配置目录的 `pluginsEnabled.xml`，不修改
  `config.xml`、插件目录或原版配置文件。
- XML 使用插件目录名作为稳定键：

```xml
<?xml version="1.0"?>
<NotepadPlus>
    <Plugins>
        <Plugin folderName="mimeTools" enabled="yes"/>
    </Plugins>
</NotepadPlus>
```

- 仅 `enabled="yes"` 的插件会传入 Windows 原版 ABI 管理器和跨平台插件管理器；
  文件缺失、插件条目缺失、显式禁用、非法属性或 XML 解析失败都按禁用处理。
- Plugins Admin 的“已安装”页使用 `插件 | 已启用 | 版本` 三列；启用复选框即时原子
  保存，下一次启动生效。卸载操作改为作用于当前选中行，不再复用启用复选框。
- 清单由用户配置，不代表插件已经通过兼容审核；不兼容的同进程 DLL 仍可能影响宿主。

## 2026-08-08 简单插件兼容语料

- 当前同步消息白名单已从 mimeTools 扩展到 Reverse Lines、Remove Duplicate Lines、SelectQuotedText、BracketsCheck、SecurePad 和 Code Alignment。
- 这些插件继续使用原版 `NppData`、`FuncItem`、`NPPM_*` 和 `SCI_*`；编辑行为由 Scintilla 5 原样执行，Qt adapter 只负责 HWND 到现有对象的映射。
- CTest 使用项目 updater 从 `third_party/win32-plugins` 的固定官方包安装 9 个语料（mimeTools 加本轮 8 个），验证 SHA-256、安装回执、加载诊断、菜单和真实命令。
- Poor Man's T-SQL Formatter 需要 CLR 2.0/4.0 激活策略，仍未进入当前加载白名单。
- BetterMultiSelection 1.5 已在 2026-08-11 补齐
  `SCI_GETDIRECTFUNCTION/SCI_GETDIRECTPOINTER` 后进入审核加载集合；READY、Hook
  开关、direct 调用、卸载及与 XMLTools 共存均已自动验证，真实物理键盘下的多光标
  移动手感仍需人工确认。
- BracketsCheck 的核心检查已兼容；它直接调用 `GetMenu/CheckMenuItem` 的旧式菜单勾选回写尚未映射到 Qt 菜单。

## 2026-08-08 插件加载恢复基线

- `PluginLoadJournal` 在用户配置目录的 `plugin-load/` 下维护
  `plugin-load.jsonl` 和原子写入的 `plugin-load-in-progress.json`。
- 每个会话及插件加载均记录开始、成功、失败、恢复跳过和会话完成事件；日志超过
  1 MiB 时轮转为 `.1`。
- 上次进程若在某个插件加载期间退出，下次启动只跳过该插件一次并显示非模态恢复提示；
  其余兼容白名单插件继续加载。
- 注册失败会卸载 DLL、恢复命令 ID 检查点且不写入已加载集合。真实测试 DLL 验证
  随后的有效插件取得连续命令 ID。
- `-noPlugin` 经实际 `CommandLineParser` 解析后，不创建 `Win32PluginManager`、代理窗口
  或加载日志，Plugins 菜单保持禁用占位状态。

## 2026-08-09 JsonTools 3.2.0 适配

- 官方 x64 CLR4/WinForms DLL 已加入固定语料、插件管理安装计划和 Windows 兼容白名单，
  未重写插件算法或 UI。
- 主窗口适配器新增当前完整路径、文件名、新建和打开消息；编辑器适配器新增
  `SCI_APPENDTEXT`、`SCI_GOTOLINE`、`SCI_GOTOPOS`。
- 插件批量加载后发送 `NPPN_TBMODIFICATION`，关闭每个 Buffer 前发送
  `NPPN_FILEBEFORECLOSE`。前者刷新托管菜单命令 ID；JsonTools 自身仍按上游代码用
  功能索引 `4` 作为树 Dock ID。
- 真实 DLL 回归已验证 10 个 FuncItem、3 个快捷键、pretty/compress、JSON lexer、
  路径/新建/打开消息、直接 SCI 消息、WinForms JSON Tree Dock 的可见性与 HWND 父子关系。
- Settings、RemesPath、JSON Lines、YAML、树节点跳转和 4 MB 大树边界已纳入真实 DLL
  自动矩阵；内置 `Run tests` 仅剩下方记录的上游绝对路径缺陷。

## 2026-08-09 深层 JsonTools 与简单插件语料

- JsonTools 真实 DLL 矩阵现覆盖 Settings、RemesPath 查询/赋值、JSON Lines、YAML、树节点
  源码跳转，以及超过 4 MB 时先建直接子树、确认后建完整树的上游行为。
- JsonTools v3.2.0 `Run tests` 的 JsonGrepper 套件硬编码作者绝对目录并在 `GetFiles()` 外
  没有异常保护。宿主不伪造路径、不截断插件命令；修复菜单自身需要另行决定维护补丁 DLL
  或升级插件版本。
- Windows 审核语料新增官方 nppConverter 4.4.0 和 NppPluginDemo 4.2。两者沿用插件管理
  安装、白名单加载、`FuncItem` 命令和 DockingManager，不建立插件专属架构。
- 编辑器适配层新增源码证实需要的 `SCI_ADDTEXT` 与 `SCI_ENSUREVISIBLE`。Converter 已验证
  双向转换、配置和面板插入；Demo 已验证 Hello、Dock 和行跳转。

## 2026-08-11 P0 原版插件兼容

- 官方未修改的 XMLTools 3.1.1.13、DoxyIt 0.4.4、SurroundSelection 1.4.1 和
  ElasticTabstops 1.3.1 x64 已进入固定包语料与插件管理安装计划。
- 编辑器适配器必须区分 Scintilla 调用通道：窗口 `SendMessage` 使用插件自身的当前
  Win32 结构；为旧插件提供的 direct thunk 才转换 `Sci_PositionCR=long` 结构。
  不能把旧 ABI 转换无条件应用到全部 `SCI_*`，否则会截断 XMLTools 的 64 位结构。
- 自动回归覆盖四个插件的核心命令、配置/选项窗口、菜单勾选、XML 注解和输入通知；
  四插件组合加载并正常退出。详细记录见
  `codex/changes/2026-08-11-p0-win32-plugin-compatibility.md`。

## 2026-08-13 联合启动、关闭与重启

- 新增 `win32-plugin-coexistence-first-start` 和
  `win32-plugin-coexistence-restart`，共享隔离配置目录并按顺序运行。
- 测试集合包含当前 27 个已确认兼容的官方未修改 Windows x64 DLL；覆盖完整主窗口、
  READY、文档通知、编码重解释、配置策略、Session、关闭协商、SHUTDOWN 和 DLL 卸载。
- `Win32PluginManager::notifyScintilla()` 保证 insert/delete 类型的
  `SCN_MODIFIED::text` 在插件回调期间始终可读，避免 ElasticTabstops 按原版契约读取
  通知文本时解引用空指针。
- 第二次启动允许并核验第一次正常关闭保存的会话恢复，而不是错误要求空白 `new 1`
  状态；加载日志必须包含两个完整 session 且没有加载中标记。
