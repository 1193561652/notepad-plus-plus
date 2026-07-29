# 原版与 Qt 移植版功能对照分析

分析日期：2026-07-22

## 范围

本分析对照：

- 原版：`v8.4.6:PowerEditor/src/`
- Qt 移植版：`./src/`

目标是建立功能点、功能差异和逻辑差异索引，为后续指定有限开发范围做准备。

## 总体结论

Qt 移植版已经具备 Notepad++ 核心概念的初步框架，包括主窗口、菜单、工具栏、状态栏、编辑器包装、Buffer、标签页、配置、查找替换、停靠面板、偏好设置和插件接口预留。

与原版相比，Qt 版当前仍属于“核心功能子集 + 框架占位”阶段。最大差异集中在：

- 原版大量功能由 Win32 消息、命令 ID、Scintilla Document、插件消息和 XML 配置共同驱动。
- Qt 版更多使用 Qt signal/slot、QObject、QAction、QDockWidget 和 QScintilla 包装。
- 原版 Buffer/FileManager 是完整文档生命周期模型；Qt 版目前是轻量模型。
- 原版配置系统覆盖面很广；Qt 版只覆盖主配置子集。
- 原版插件接口是 ABI 和消息级兼容；Qt 版是新的 Qt 化插件接口。

## 功能矩阵

| 功能 | 原版实现 | Qt 版实现 | 状态 | 主要差异 |
| --- | --- | --- | --- | --- |
| 程序入口 | `winmain.cpp`、`Notepad_plus` | `main.cpp`、`MainWindow` | 部分实现 | Win32 消息循环改为 Qt 应用和主窗口 |
| 主窗口 | `Notepad_plus.*`、`NppBigSwitch.cpp` | `MainWindow.*` | 部分实现 | 原版命令分发庞大，Qt 版集中在 MainWindow |
| 文件新建/打开/保存 | `NppIO.cpp`、`Buffer.*` | `MainWindow.*`、`FileManager.*` | 部分实现 | Qt 版缺少原版完整编码/EOL/文件状态逻辑 |
| Buffer 生命周期 | `Buffer` + Scintilla `Document` 引用计数 | `Buffer` + `ScintillaEditView*` | 部分实现 | 原版支持共享 Document、引用计数、文件监控、备份 |
| 多标签 | `DocTabView` Win32 控件 | `DocTabView` Qt `QTabWidget` | 部分实现 | Qt 版有基础标签和克隆入口，完整上下文菜单/排序待核验 |
| 双视图/克隆 | 原版主/副 Scintilla 视图共享 Document | Qt 主/副 `DocTabView` + cloneView | 部分实现 | Qt 版需重点核验 Buffer 所有权和 QsciDocument 共享 |
| 编码/BOM | `Utf8_16`、`EncodingMapper`、uchardet | `QString` 编码名 + `QTextCodec` 写出 | 部分实现 | 原版检测和保持能力更完整 |
| EOL | Buffer 保存 EolType，加载时检测 | 有 EOL action 和状态结构 | 部分实现 | Qt 版读取/保存是否保持原 EOL 待核验 |
| 配置 XML | `Parameters.*` 完整管理多 XML | `NppParameters` + TinyXml | 部分实现，高风险 | Qt 版写回范围较窄，未知节点保留待核验 |
| 会话 | 原版 `Session` 支持主/副视图、位置、折叠、备份等 | Qt `Session` 结构已含很多字段 | 部分实现 | Qt 恢复/保存链路需和原版逐节点对照 |
| 最近文件 | `lastRecentFileList` + config History | `NppGUI::_recentFileList` | 部分实现 | Qt 版有菜单更新，限制和格式需核验 |
| 菜单/命令 | `menuCmdID.h` + `NppBigSwitch` | `QAction` + slots | 部分实现 | Qt 版实现核心子集，缺大量命令 |
| 工具栏 | Win32 ToolBar + `toolbarIcons.xml` | Qt `QToolBar` + qrc 图标 | 部分实现 | Qt 版没有完整 toolbarIcons 配置驱动 |
| 状态栏 | 原版多个固定栏位 | Qt 多个 `QLabel` | 部分实现 | 栏位概念类似，精确内容/刷新时机待核验 |
| 语法高亮 | Scintilla/Lexilla + langs/stylers/UDL | QScintilla lexer + langs/stylers + 轻量 UDL lexer | 部分实现 | UDL 已支持 XML 加载、扩展名识别、关键字/注释/数字/操作符基础样式；复杂嵌套和完整设计器待补 |
| 自动完成 | `AutoCompletion.*` + XML | `ScintillaEditView::setupAutoComplete` | 部分实现 | 原版功能更完整，Qt 版需对照 API |
| 查找替换 | 大型 Win32 对话框 + Finder | Qt `FindReplaceDlg` | 部分实现 | Qt 版有 UI 和基本操作，缺全量搜索体系 |
| Find in Files | 原版支持目录/项目/过滤器/结果面板 | UI 存在，部分按钮禁用 | 缺失/占位 | 需要后续实现 |
| Mark/书签 | 原版多 marker/style 和结果联动 | Qt 使用 indicator/marker | 部分实现 | 多样式标记、复制/删除标记行等缺失 |
| 偏好设置 | 原版大型 PreferenceDlg 多页面 | Qt `PreferenceDlg` 多页面，部分 stub | 部分实现 | 多个页面是占位 |
| 本地化 | `localization.*` + nativeLang XML | `NativeLangSpeaker` + Qt 翻译资源 | 部分实现 | Qt 版路径和应用方式不同 |
| 深色模式 | 原版 `DarkMode/`、`NppDarkMode.*` | Preference 中 stub | 缺失/占位 | Qt 版尚无完整深色模式 |
| 停靠窗口 | 原版 DockingManager | Qt `QDockWidget` 面板 | 部分实现 | 插件 docking、布局兼容差异大 |
| 文件浏览器 | 原版 `WinControls/FileBrowser` | Qt `FileBrowserPanel` | 部分实现 | 功能范围待核验 |
| 文档地图 | 原版 DocumentMap/Snapshot | Qt `DocumentMapPanel` | 部分实现 | 渲染与同步机制不同 |
| 函数列表 | 原版 FunctionList 体系 | Qt `FunctionListPanel` | 部分实现 | parser 完整性待核验 |
| 项目面板 | 原版 3 个 ProjectPanel | 无对应完整实现 | 缺失 | Qt 版未建立项目面板体系 |
| 宏 | 原版 Scintilla/Notepad++ 命令录制回放 | `QsciMacro` 录制、回放、导入导出 | 基础实现 | 可录制编辑器命令；尚未完整覆盖 Notepad++ 菜单命令和原版宏 UI/快捷键映射 |
| 打印 | 原版 `Printer.*` | `QsciPrinter` + `QPrintDialog` | 基础实现 | 已支持打印对话框和直接打印；页眉页脚及原版打印偏好待补 |
| 插件 | 原版 ABI + `NPPM_*` 消息 + PluginAdmin | Qt `IPlugin` 接口 | 接口预留 | Qt 版不是原版插件兼容 ABI |
| 上下文菜单 | `contextMenu.xml` | 未见完整对应 | 缺失/待核验 | 需要配置兼容分析 |
| UDL | `UserDefineDialog`、`userDefineLang.xml` | `UserLangDesc` + `UserDefinedLexer` + Language 菜单 | 基础实现 | 完全读取兼容、不改写 XML；完整 UDL 设计器、嵌套状态和所有原版样式语义待补 |

## 关键逻辑差异

### 主循环与命令分发

原版以 Win32 窗口、消息、菜单命令 ID 和 `NppBigSwitch.cpp` 为核心。Qt 版以 `QApplication`、`QMainWindow`、`QAction` 和 signal/slot 为核心。

移植时应按“用户行为”映射命令，不应机械复刻 Win32 消息。

### Buffer 模型

原版 `Buffer` 持有 Scintilla `Document` 指针，并通过引用计数支持多视图共享、克隆、关闭释放和 scratch document。Qt 版 `Buffer` 当前只持有路径、状态、编码和一个 `ScintillaEditView*`。

这意味着双视图、克隆视图、关闭标签、保存点、脏状态、撤销栈和 Document 生命周期都需要专项核验。

### 编码与文件 I/O

原版使用 `Utf8_16_Read` / `Utf8_16_Write`、`EncodingMapper` 和 uchardet，加载时检测 BOM、编码、EOL 和语言提示，保存时尽量保持模式。

Qt 版当前保存使用 `QTextCodec` 和简单字符串编码名，UTF-16 写出是手工 BOM + ushort。读取链路需要进一步分析，是否完整检测编码/EOL 仍待确认。

### 配置读写

原版配置系统覆盖 config、session、langs、stylers、shortcuts、context menu、toolbar icons、UDL、find history、project panels、file browser、cloud/local config 和插件配置路径。

Qt 版 `NppParameters` 已覆盖主要 XML 文件，但写回更像“生成当前支持子集”。在完全读写兼容约束下，最大风险是写回时丢失未知节点、改变节点顺序或丢掉原版字段。

### 搜索体系

原版 `FindReplaceDlg` 包含 `Finder`、Find All、Replace All、Find in Files、Find in Projects、Mark、结果面板、搜索历史、宏命令集成和复杂状态保存。

Qt 版已有 Find/Replace/Find in Files/Mark 的 UI 结构和当前文档基础操作，但“所有打开文件”“目录搜索”“项目搜索”“复制/删除标记行”“完整结果面板语义”仍是缺失或占位。

### 插件体系

原版插件不是普通动态库调用，而是固定 ABI、`NppData`、`FuncItem`、`ShortcutKey`、`NPPM_*` 消息、Scintilla 窗口句柄、菜单命令和 docking 通知体系。

Qt 版 `IPlugin` / `IPluginHost` 是新的 Qt 化接口，只能视为未来平台层边界。若要求原版插件兼容，需要单独设计 ABI 适配层。

## 推荐优先级

1. 配置 XML 逐节点兼容性。
2. 文件打开/保存/编码/EOL/备份完整链路。
3. Buffer + DocTabView + 双视图生命周期。
4. 菜单/快捷键/命令 ID 行为映射。
5. 查找替换当前文档行为对齐。
6. 停靠窗口和偏好设置页面补齐。
7. 插件接口边界设计，暂不实现完整插件。
