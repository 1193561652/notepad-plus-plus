# UI、菜单与工具栏功能分析

## 功能定位

UI 层应尽量保持原版 Notepad++ 用户体验，包括菜单、工具栏、状态栏、标签页、双视图和停靠面板。

## 当前实现入口

- `./src/MainWindow.h`
- `./src/MainWindow.cpp`
- `./src/WinControls/TabBar/DocTabView.*`
- `./src/WinControls/DockingWnd/*.h|cpp`
- `./resources/resources.qrc`
- `./resources/icons/`

## 当前 UI 结构

- `MainWindow` 负责创建菜单、编码菜单、工具栏和状态栏。
- `DocTabView` 负责文档标签。
- `QSplitter` 承载主视图和副视图。
- `QDockWidget` 承载文件浏览、文档地图、函数列表、查找结果等面板。
- `resources.qrc` 已包含 Notepad++ 风格图标、中文语言资源、样式模型。

## 已确认入口

- `createActions()`
- `createMenus()`
- `createEncodingMenu()`
- `createToolBars()`
- `createStatusBar()`
- `setupTabViews()`
- `setupFileBrowser()`
- `setupDocumentMap()`
- `setupFunctionList()`
- `setupFindResultPanel()`

## 风险点

- `MainWindow` 职责较集中，修改 UI 动作容易影响文件、视图、配置和插件入口。
- UI 行为要求对齐原版，不应因为 Qt 控件方便而改变交互。
- 菜单和快捷键后续应与 `shortcuts.xml` 兼容。
- 动态初始化、配置驱动显示/隐藏状态需要和原版保持一致。

## 与原版差异

- 原版菜单命令由 `menuCmdID.h` 定义，并通过 Win32 菜单、快捷键 XML 和 `NppBigSwitch.cpp` 分发。
- Qt 版菜单由 `QAction` 和 slot/lambda 直接连接，当前实现核心子集。
- 原版 File/Edit/Search/View 等菜单包含大量高级命令，例如批量关闭、打印、排序、大小写转换、标记行操作、折叠层级、Post-it、全屏、同步滚动等。
- Qt 版已有文件、基础编辑、搜索、视图、格式、设置、宏占位和帮助菜单，但命令覆盖远未完整。
- 原版工具栏可受配置和 `toolbarIcons.xml` 影响；Qt 版目前使用 qrc 内固定图标。

## 后续索引任务

- 建立菜单项、动作、快捷键和槽函数映射表。
- 对照原版菜单资源和命令 ID。
- 分析工具栏图标和动作启用/禁用状态同步。

## 2026-07-22 阶段三更新

- `MainWindow::createMenus()` 已补齐原版主要菜单骨架，包括 File/Edit/Search/View/Encoding/Language/Format/Macro/Plugins/Settings/Run/Window/Help。
- 未实现业务逻辑的入口统一 disabled，占位但不误导。
- View 菜单新增 Toolbar 和 Status Bar 切换入口，并写入 `NppGUI` 状态。
- 工具栏和状态栏可见性从 `config.xml` 初始化。
- EOL 菜单 checked 状态随当前 view 同步。

## 阶段三后仍保留的差距

- 快捷键仍未完全由 `shortcuts.xml` 驱动。
- `contextMenu.xml` 尚未正式驱动编辑区右键菜单。
- `toolbarIcons.xml` 尚未驱动工具栏图标映射。
- Language 菜单仍是静态常用语言分组，未完全由 `langs.xml` 动态生成。
- 原版 UI 一致性需在阶段 4 单独检验。

## 2026-07-22 阶段四更新

- 已完成代码级 UI 一致性审计，报告见 `codex/analysis/stage-4-ui-consistency-audit.md`。
- 顶层菜单运行时顺序已对齐原版：File/Edit/Search/View/Encoding/Language/Settings/Tools/Macro/Run/Plugins/Window/?。
- 移除了非原版顶层 `Format` 菜单。
- Settings 的 Import 改为子菜单。
- 新增 Tools 顶层菜单骨架，包含 MD5 和 SHA-256 占位分组。

## 阶段四后仍保留的差距

- 本阶段未启动 GUI 做截图级视觉检验；完成的是源码结构审计与构建验证。
- 右键菜单、Tab 右键菜单、动态快捷键、工具栏图标配置、Language 动态生成仍待后续。

## 2026-07-22 阶段六更新

- Search 菜单中的 `Find in Files...` 和 `Find All in All Opened Documents` 已从 disabled 占位改为可用入口。
- Preferences 的 `Dark Mode` 页面已从 stub 改为基础配置页。
- 主窗口新增基础深色 palette/style 应用逻辑，可随偏好设置 Apply 切换。

## 阶段六后仍保留的差距

- Dark Mode 视觉仍是基础 Qt 样式，未完成原版深色模式的图标、边框、dock、菜单细节对齐。
- `Project Panels` 仍未实现，当前仍依赖 Folder as Workspace 文件浏览器作为近期可用替代。
# 2026-07-23 更新

- 主菜单中除 `Import plugin(s)` 外不再存在 `addPlaceholder()` 项。
- 新增可用停靠窗口：Document List、Project Panels（3 页）、Clipboard History、Character Panel。
- Settings：Style Configurator、Shortcut Mapper、主题导入和 ContextMenu 编辑入口可用。
- Tools：MD5、SHA-256 文本/文件/剪贴板路径可用。
- Run：自由命令和 shortcuts.xml UserDefinedCommands 可用。
- Window：窗口列表、标签排序、名称/路径复制可用。
- Help：主页、文档、命令行文档和 Debug Info 可用。
- 仍需阶段性 UI 截图比对；当前只证明结构、构建和启动，不证明像素级一致。

## 2026-07-23 本地化修复

- 修复语言解析器只读取第一个 `Entries` 分组导致全部子菜单漏译的问题。
- 首次应用语言的时机移动到所有停靠面板完成创建之后。
- 为排序、普通文本、清理最近文件和“未加载插件”等静态动作补充稳定 `objectName`。
- `chineseSimplified.xml` 已覆盖当前主窗口全部具有稳定标识的菜单、动作、面板和工具栏。
- 动态用户内容保持原文；新增静态菜单项时必须同时更新目标语言 XML。

## 2026-07-29 本地化自动收口

- `UiParityCapture` 通过真实 Mark 和 Preferences 动作打开对话框，覆盖语言切换
  后的销毁与重建时序。
- Preferences 补齐自动插入、备份目录、文件关联、标签匹配等控件标识和简体
  中文资源。
- 文件关联的固定分类使用 `fileAssociationCategories_N` 列表资源翻译，扩展名
  等用户/系统数据保持原文。
- CTest 固定运行简体中文浅色/深色、100%/150% 四组 UI 矩阵。

## 2026-07-28 UI 完善

- `toolbarIcons.xml` 已正式驱动工具栏图标：
  - 读取 `NotepadPlus/ToolBarIcons/@icoFolderName`。
  - 空名称对应 `toolbarIcons/default/`。
  - 非空名称对应 `toolbarIcons/<icoFolderName>/`。
  - 使用原版固定 `.ico` 文件名和可选 `_disabled.ico`。
  - 允许不完整图标集，缺失项回退到 qrc 内置图标。

## 2026-07-28 Function List 规则

- Function List 不再仅依赖少量硬编码语言正则。
- 构建和部署包含 Notepad++ v8.4.6 的 34 份 `functionList/*.xml`。
- 优先读取用户配置目录 `functionList/`，再读取程序目录，支持兼容覆盖。
- XML 解析支持 commentExpr、mainExpr、名称提取链、classRange 和
  openSymbole/closeSymbole 范围；无规则或无结果时保留原 Qt 回退解析器。
- 工具栏图标尺寸调整为原版小工具栏的 16 px，并补入 Print、UDL、
  Document List；“显示全部字符”替代原先不等价的“仅显示空白字符”按钮。
- UDL 默认工具栏图标直接使用 v8.4.6 原始资源。
- 深色模式现为全应用 palette/QSS，而非仅 MainWindow 局部样式。
- 深色样式覆盖菜单、工具栏、状态栏、Dock、Tab、列表/树/表格、输入控件、
  按钮、GroupBox、滚动条、分隔条和 ToolTip。
- 保持编辑区颜色继续由 `stylers.xml` 控制；启用深色 UI 不擅自改写用户主题。
- 自动化截图覆盖浅色与深色主窗口、5 个查找页和 19 个偏好设置页。
