# 模块索引

- `codex/changes/2026-08-09-priority-plugin-compatibility.md`：高、中优先级插件真实 DLL 接入结果、SCN 桥接和跳过边界。
- `codex/cache/2026-08-09-priority-plugin-compatibility.md`：上述批次的快速本地缓存入口。

## Notepad++ Qt 主线

当前主线实现已包含以下模块：

- `src/main.cpp`：程序入口。
- `src/MainWindow.h`：Qt 主控制器的共享声明和状态所有者。
- `src/Notepad_plus.cpp`：构造、析构、命令行调用和顶层协调，对应原版 `Notepad_plus.cpp`。
- `src/NppCommands.cpp`：菜单、工具栏、命令状态、搜索替换和编辑命令，对应原版 `NppCommands.cpp`。
- `src/NppIO.cpp`：文件、Buffer、编码、会话、监视和最近文件流程，对应原版 `NppIO.cpp`。
- `src/NppNotification.cpp`：编辑器、标签、焦点和生命周期通知，对应原版 `NppNotification.cpp`。
- `src/MainWindow.cpp`：Qt 窗口、永久双视图、Dock/面板和插件宿主服务。
- `src/ScintillaComponent/`：编辑器组件、Buffer/FileManager、DocTabView、自动完成、查找替换、打印和 UDL 自定义词法器。
- `src/WinControls/DockingWnd/DockingManager.*`：对应原版同名管理类，统一管理四侧
  Dock 容器、面板注册、标签化、显示/隐藏、尺寸和状态。
- `src/WinControls/DocumentMap/`、`FileBrowser/`、`FunctionList/`：各业务面板；
  `MainWindow` 创建后交给 `DockingManager` 注册。
- `src/WinControls/Preference/preferenceDlg.*`：偏好设置对话框，与原版路径和大小写对齐。
- `src/Parameters.*`：配置管理。
- `src/localization.*`：本地化和 `NativeLangSpeaker` 语言切换。
- `src/TinyXml/`：XML 配置解析。
- `src/MISC/PluginsManager/`：插件运行边界、清单、更新计划和独立更新器。
- `src/WinControls/PluginsAdmin/`：插件管理 UI 与展示模型。
- `src/Win32PluginSystem/`：原版 Windows DLL 插件 ABI、消息和窗口语义兼容层。

## 关键关系

- `MainWindow` 继承 `QMainWindow`，同时实现 `IPluginHost`。
- `MainWindow` 通过两个 `DocTabView` 管理主/副视图，每个视图永久持有一个
  `ScintillaEditView`。
- `Buffer` 保存文件状态并拥有一个 addref 后的 Scintilla document pointer；标签只映射
  Buffer，永久编辑器在标签切换时通过 `SCI_SETDOCPOINTER` 切换文档。
- `FileManager` 是单例，负责创建、加载、保存和关闭 `Buffer`。
- `PluginAdminModel` 与独立更新器负责包管理；`PluginManager` 的 Qt 动态库接口仍是未冻结的运行 ABI 预留。

## 待补充索引

- `Parameters` 配置读写格式和原版 XML 对应关系。
- `ScintillaEditView` 与官方 Scintilla Qt 平台层的消息边界。
- `DocTabView` 标签生命周期和 Buffer 映射。
- 文件保存、另存为、关闭询问、备份恢复的完整调用链。
- 插件接口是否应默认参与构建，以及 `ENABLE_PLUGIN_SYSTEM` 默认值是否符合近期目标。

## 已建立模块索引

- `codex/modules/notepad-plus-plus/overview.md`
- `codex/modules/notepad-plus-plus/configuration.md`
- `codex/modules/notepad-plus-plus/file-buffer.md`

## 已建立功能索引

- `codex/features/configuration.md`
- `codex/features/file-management.md`
- `codex/features/editor-buffer.md`
- `codex/features/ui-menus-toolbars.md`
- `codex/features/search-replace.md`
- `codex/features/plugin-system.md`
- `codex/features/advanced-editing.md`
- `codex/features/p1-compatibility.md`
- `codex/features/p0-editor-shortcuts-workspace.md`

## 已建立对照分析

- `codex/analysis/migration-plan.md`
- `codex/analysis/stage-1-xml-configuration.md`
- `codex/analysis/original-vs-qt-feature-comparison.md`
- `codex/analysis/feature-points-and-gaps.md`
- `codex/analysis/logic-differences.md`
- `codex/analysis/2026-07-22-implementation-feature-gap-report.md`

## 已建立变更摘要

- `codex/changes/2026-07-22-stage-1-xml-resources.md`
- `codex/changes/2026-07-22-stage-7-final-features.md`
- `codex/changes/2026-07-23-menu-localization-coverage.md`
- `codex/changes/2026-07-26-p0-functional-compatibility.md`
- `codex/changes/2026-08-02-original-module-layout-and-margin-cursor.md`

## 2026-07-26 P0 索引增量

- `src/WinControls/ProjectPanel/WorkspaceDocument.*`：原版 workspace XML 数据模型、相对路径和成员枚举。
- `src/WinControls/ProjectPanel/ProjectPanel.*`：三个独立 Project Panel 的加载、保存和成员编辑 UI。
- `NppParameters::loadShortcuts()` / `writeShortcuts()`：InternalCommands、ScintillaKeys、NextKey、Macros 和 UserDefinedCommands。
- `MainWindow::registerNppCommandIds()` / `applyConfiguredShortcuts()`：原版 command ID 与当前 QAction 的映射。
- `MainWindow::playConfiguredMacro()`：type 0、1、2 宏命令序列。
- `FileManager::backupFileBeforeSave()`：simple/verbose 保存前备份。
- `ScintillaEditView`：URL Hotspot、XML/HTML 标签匹配和字符/标签自动插入。
- `MainWindow::onFindInProjectsRequested()` / `onReplaceInProjectsRequested()`：workspace 成员限定的项目搜索替换。
- 分析报告：`codex/analysis/2026-07-26-p0-functional-compatibility-report.md`。
- 本地缓存：`codex/cache/2026-07-26-p0-functional-compatibility.md`。
# 2026-07-23 索引增量

- `src/MISC/PlatformServices.*`：跨平台文件管理器、终端、默认应用和回收站边界。
- `src/MISC/RegExt/FileAssociationModel.*`：v8.4.6 文件关联分类和扩展名规范化。
- `src/WinControls/ToolBar/ToolbarIconTheme.*`：解析 `toolbarIcons.xml` 并按原版固定名称加载
- `src/MISC/ConfigPathResolver.*`：跨平台配置根目录决策、规范化与可写性验证
- `src/MISC/ClosedFileHistory.*`：最近关闭文件 LIFO、去重和失效路径过滤
- `src/WinControls/FunctionList/functionParser.*`：v8.4.6 Function List XML 加载和规则执行
- `codex/changes/2026-07-28-cross-platform-config-path.md`
- `codex/analysis/2026-07-28-automatic-backlog-audit.md`
- `codex/changes/2026-07-28-automatic-backlog-completion.md`
  用户图标和禁用状态图标。
- `src/NppCommandRegistry.*`：已实现 QAction 与原版 command ID 的集中映射，
  供 shortcuts.xml、Shortcut Mapper 和宏 type 2 共用。
- `MainWindow::showEditorContextMenu()`：运行时读取用户目录 `contextMenu.xml`，映射到现有 QAction。
- `MainWindow::onReplaceAllOpenedDocsRequested()`：按 Buffer 去重替换打开文档。
- `MainWindow::onReplaceInFilesRequested()`：目录批量替换、二进制跳过、脏文档保护、编码/BOM 保持和 QSaveFile 原子写回。
- `MainWindow::setupAuxiliaryPanels()`：Document List、Project Panels、Clipboard History、Character Panel。
- `NppParameters::feedGUIConfig()` / `writeConfigXml()`：管理 openSaveDir、Print、SmartHighLight、DateTime、delimiterSelection、multiInst、URL、searchEngine，并通过原版 `DarkMode@enable` 共享暗色模式；Qt 私有状态不再覆盖该值。

## 命令行与实例边界

- `./src/CommandLineOptions.*`
  - v8.4.6 参数全集解析、路径规范化、递归通配符、本地化代码映射和 JSON IPC 数据模型。
- `./src/main.cpp`
  - `settingsDir`/`-L` 启动前覆盖、multiInst 决策、`QLocalServer` 单实例转发、托盘和加载耗时。
- `MainWindow::applyCommandLineInvocation()`
  - 统一首次启动与后续实例的文件、会话、工作区、语言、定位、只读、监视、打印和导出行为。
- `NppParameters::setUserPathOverride()` / `setStartupLocalizationFile()`
  - 必须在 `NppParameters::load()` 前调用。
- `ScintillaEditView::setLexerByName()`
  - `-lLanguage` 的内建 lexer 强制入口。
- `NppParameters::loadShortcuts()`：同时读取 Macros 和 UserDefinedCommands。
- `PreferenceDlg`：19 页均为实际页面，不再使用 stub page。

修改配置代码前必须保留 `makeCfgEl()` 的“复用元素、保留未知属性”语义；默认 XML 文件落盘后必须保持用户可写。

## 2026-07-28 P1 索引增量

- `PlatformServices::registeredFileAssociations()`：枚举 Windows HKCR 中默认值为
  `Notepad++_file` 的扩展名。
- `PlatformServices::registerFileAssociation()`：写入 ProgID、图标、打开命令，
  并用 `Notepad++_backup` 保存原默认关联。
- `PlatformServices::unregisterFileAssociation()`：恢复备份关联或清除默认值。
- `PreferenceDlg::makePage_FileAssociation()`：原版三列文件关联管理交互；
  非管理员只读，非 Windows 转到系统设置。
- 命令 ID 与文件关联纯逻辑分别由
  `tests/NppCommandRegistryTests.cpp` 和
  `tests/FileAssociationModelTests.cpp` 覆盖。

## 2026-07-28 UI 索引增量

- `MainWindow::applyToolbarIcons()`：QAction 到原版工具栏 icon ID 的映射和
  qrc 回退。
- `MainWindow::applyDarkMode()`：全应用深色 palette 与组件 QSS，运行时可通过
  Preferences Apply 刷新。
- `tests/ToolbarIconThemeTests.cpp`：自定义目录、默认目录、部分图标集、
  图标实际加载和路径越界保护。
- UI 验证：`codex/validation/2026-07-28-ui-polish/README.md`。

## 2026-08-01 最终 UI 对照索引

- `tests/UiParityCapture.cpp`：Qt 主窗口、Project Panel、Shortcut Mapper、
  Find 和 19 个 Preferences 页面自动捕获及标题/本地化断言。
- `tests/ui/CaptureUiMatrix.ps1`：原版 v8.4.6 Win32 UI 捕获，使用原版命令 ID、
  控件消息和 UI Automation 建立可重复基线。
- `codex/analysis/2026-08-01-final-ui-parity-audit.md`：最终逐区域对照结论和
  后续 UI 开发范围。
- `codex/validation/2026-08-01-final-ui-parity/README.md`：验证矩阵与本地产物索引。

## 2026-07-25 索引增量

- `src/MISC/TextFileCodec.*`：统一文本文件检测、显式代码页解码和可逆编码校验。
- `Buffer::views()` / `FileManager::findBufferByView()`：共享文档的 view 注册和反向查找。
- `DocTabView::addBufferView()`：向主/副视图注册 Buffer 标签；不移动或创建 editor
  widget，跨视图移动和 clone 都复用对应视图的永久编辑器。
- `MainWindow::saveSession()` / `restoreSession()`：会话 UI 状态与原版 FILETIME 字段。
- `FindReplaceDlg::onReplaceAll()`：选区、Extended 和零长度正则前进。

## 2026-07-28 Find 本地化索引增量

- `MainWindow::ensureFindReplaceDialog()`：统一负责 Find 对话框延迟创建、信号连接
  和当前 `NativeLangSpeaker` 翻译应用。
- 禁止菜单或面板入口直接 `new FindReplaceDlg`；语言切换后隐藏对话框会被销毁，
  直接重建会产生英文控件。
- `FindReplaceDlg::makeFipOptions()` / `makeFipButtons()`：Find in Projects 页
  可本地化控件的稳定对象名入口。
- `tests/FindDialogLocalizationTests.cpp`：Find 对话框入口及简体中文资源静态
  完整性测试。

## 2026-07-25 编码与会话索引增量

- `src/EncodingMapper.*`：Notepad++ 代码页与 Qt codec 名称映射。
- `src/uchardet/`：无 BOM 传统编码探测，作为主程序和核心测试的内部源码。
- `TextFileCodec::decode(data, TextDecodingOptions)`：BOM、声明、UTF-8、
  uchardet、fallback 的统一检测入口。
- `BufferMapState`：Document Map 的 session 状态载体。
- `MainWindow::syncDocumentMap()`：活动文档切换时采集并恢复地图状态。
- `DocTabView::setIndividualTabColour()`：恢复 `tabColourId` 并触发重绘。
- `FileBrowserPanel::selectedPath()` / `setSelectedPath()`：读写
  `FileBrowser latestSelectedItem`。

## 2026-08-01 Scintilla 5 Qt 当前架构

- 目标版本固定为原版 v8.4.6 的 Scintilla 5.3.0 和 Lexilla 5.1.9。
- `ScintillaEditView` 已切换到官方 `ScintillaEditBase`，继续以
  `execute(SCI_*, ...)` 为主要业务边界。
- 模块索引：`codex/modules/notepad-plus-plus/scintilla5-qt.md`。
- 迁移已完成；计划文件保留为历史设计依据，当前状态见
  `codex/cache/2026-08-01-scintilla5-qt-migration.md`。

## 2026-08-01 插件调查索引

- `codex/analysis/plugins-v846/inventory.md`：v8.4.6 x86 169 项身份、版本、源码、
  重要度和兼容等级总索引。
- `codex/analysis/plugins-v846/api-dependency-matrix.md`：六导出、NPPM/NPPN/SCI、
  平台依赖、线程/运行时和双代理 HWND 风险统一入口。
- `codex/analysis/plugins-v846/batch-*.md`：逐插件静态证据和未验证项。
- `tests/PluginInvestigationTests.cpp`：调查数据与 x86 JSON 基线的一致性校验。
- `src/Win32PluginSystem/Win32PluginManager.*`：Windows 原版插件六导出加载、三个稳定
  HWND、常用 `NPPN_*` 构造与派发入口。
- `MainWindow::notifyCurrentLanguageChanged()` / `setBufferReadOnly()`：语言和 Buffer
  只读状态的双视图同步及插件通知所有者。
- `tests/plugins/Win32ValidRegistrationPlugin.cpp` 与 `ui-plugin-registration-rollback`：
  文件生命周期顺序、Buffer ID、只读状态位及常用通知 ABI 回归。
- `codex/analysis/plugins-v846/jsontools-v320-compatibility-evaluation.md`：
  JsonTools 3.2.0 官方包、CLR/WinForms 边界、精确消息缺口和实施顺序。
- `codex/changes/2026-08-09-jsontools-deep-and-simple-plugins.md`：
  JsonTools 深层功能矩阵、上游内置测试路径边界，以及 nppConverter/NppPluginDemo
  真实 DLL 的有限兼容结果。
