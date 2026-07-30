# 本地缓存：最新分析

缓存日期：2026-07-30

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
- QScintilla、Scintilla 和 LexUser 已纳入 `third_party/`，构建不再依赖旧工作区。
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
- `src/PluginSystem/IPlugin.h`、`PluginManager.h`。
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
- 原版 LexUser 已编入自有 QScintilla 静态库。
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
