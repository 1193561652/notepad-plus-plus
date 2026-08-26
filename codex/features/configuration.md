# 配置系统功能分析

## 功能定位

配置系统负责保持 Notepad++ 原有配置文件和目录结构兼容。当前 Qt 主线由 `NppParameters` 统一管理配置、会话、语言、样式和快捷键数据。

## 当前实现入口

- `./src/Parameters.h`
- `./src/Parameters.cpp`
- `./src/TinyXml/`
- `./resources/config.model.xml`
- `./resources/langs.model.xml`
- `./resources/stylers.model.xml`
- `./resources/shortcuts.xml`
- `./resources/contextMenu.xml`
- `./resources/toolbarIcons.xml`
- `./resources/userDefineLang.xml`
- `./installer_common/nativeLang/chineseSimplified.xml`

## 当前涉及文件

`NppParameters` 已声明并实现以下路径：

- `config.xml`
- `session.xml`
- `nativeLang.xml`
- `langs.xml`
- `stylers.xml`
- `shortcuts.xml`
- `backup/`

## 2026-07-28 跨平台配置路径

- 路径决策由 `src/MISC/ConfigPathResolver.*` 统一负责。
- 优先级为：命令行显式目录、`doLocalConf.xml` 便携目录、平台默认目录。
- Windows 默认目录固定为 `%APPDATA%\Notepad++`，保持与原版完全兼容。
- Linux/macOS 使用 `QStandardPaths::AppConfigLocation`，并提供平台回退。
- `-settingsDir=` 与 `--settings-dir` 必须指向已存在且可写的目录；
  无效覆盖会提示并回退，默认目录则可以自动创建。
- 配置根目录会在读取任何 XML 前确定；配置、会话、语言、主题、快捷键、
  Qt 私有状态和兼容子目录不会各自重新选择路径。

阶段一已将原版默认 `config.4zipPackage.xml`、`langs.model.xml`、`shortcuts.xml`、`contextMenu.xml`、`toolbarIcons.xml`、`userDefineLang.xml` 复制到 Qt 资源目录并加入 qrc，其中 `config.4zipPackage.xml` 在 Qt 版中命名为 `config.model.xml`。

## 数据结构

- `NppGUI`：主 GUI 配置、Tab 行为、窗口位置、最近文件、新建文档默认值、备份、自动完成、分割视图等。
- `ScintillaViewParams`：行号、书签边距、折叠边距、缩进线、换行、空白符、EOL、缩放等编辑器视图参数。
- `Session` / `sessionFileInfo`：主/副视图文件列表、活动索引、文件浏览器状态、光标/滚动/折叠/标记等会话信息。
- `LangDesc`：`langs.xml` 语言和扩展名映射。
- `LexerStyler` / `WordsStyle`：`stylers.xml` 词法样式和全局样式。
- `MacroDef` / `MacroAction`：`shortcuts.xml` 宏定义。

## 已确认行为

- 配置读写使用 TinyXml，而不是 Qt 原生 `QSettings` 作为长期格式。
- 存在 `migrateFromQSettings()`，说明历史上可能有 Qt 版旧配置迁移逻辑。
- `writeNppGUI()`、`writeSession()`、`writeShortcuts()` 是主要写入口。
- `writeFindHistory()` 是阶段五新增的查找历史写入口。
- `feedGUIConfig()` 是 `config.xml` 的 GUI 配置解析入口。
- `loadLangs()` 已支持用户目录、exe 目录、Qt 资源三级回退。
- `loadShortcuts()` 已支持用户目录、exe 目录、Qt 资源三级回退。
- 默认 XML 缺失时会被复制到用户配置目录，不覆盖已有用户配置。
- `config.xml` 写回会基于现有 XML 更新已支持节点，降低丢失原版未知节点的风险。
- `shortcuts.xml` 写回只替换 `Macros` 节点，保留其它原版命令节点。
- `FindHistory` 写回只替换根节点下同名节点，保留其它配置节点。
- 阶段六后 Dark Mode 基础开关通过 Qt 扩展配置 `EditorSettings@darkMode` 读写。

## 兼容要求

- 必须完全读写兼容 Notepad++ 原始 XML 格式。
- 不应修改 XML 结构。
- 不应自动升级用户配置。
- 不应把 Qt 扩展字段混入会破坏原版读取的结构中。

## 风险点

- `NppGUI` 中的 Qt 移植扩展字段不得写入原版 `config.xml`。
- `QMainWindow::saveState()` 和 Qt 编辑器私有设置存入独立 `qtState.ini`。
- 配置写回时要避免丢失原版未知节点和属性；当前是否保留未知字段待进一步核验。
- 完全读写兼容要求较高，后续修改配置系统前应对照原版 `Parameters` 实现。

## 与原版差异

- 原版 `Parameters` 覆盖 config、session、langs、stylers、shortcuts、contextMenu、toolbarIcons、UDL、find history、project panels、file browser、cloud/local config 和 plugin config 路径。
- Qt 版当前覆盖主要配置文件入口，但未确认完整保留未知节点、未知属性和原版节点顺序。
- 原版在初始化时会复制默认模型文件并处理本地/云端配置路径；Qt 版路径策略更简单。
- 原版 DockingManager 的 left/right/top/bottom 尺寸现已由 Qt `DockingManager`
  读取和写回；`ActiveTabs` 仍未形成 XML 级等价实现。
- 原版 FindHistory、ProjectPanels、Plugin 命令等配置尚未在 Qt 版形成完整等价实现。

## 后续索引任务

- 对比原版 `v8.4.6:PowerEditor/src/Parameters.*`。
- 建立 `config.xml`、`session.xml`、`shortcuts.xml` 的节点级索引。
- 检查当前写回逻辑是否会重排、丢字段或改变默认值。
- 为 `contextMenu.xml`、`toolbarIcons.xml`、`userDefineLang.xml` 建立正式解析入口。
- 建立 XML 往返测试，验证未知节点和属性保留。

## 2026-07-22 阶段五更新

- `FindHistoryState` 覆盖原版 `FindHistory` 常用属性和四类历史列表：`Path`、`Filter`、`Find`、`Replace`。
- `loadFindHistory()` 从 `config.xml` 根节点读取 `FindHistory`，缺失时保留默认值。
- `writeFindHistory()` 基于现有 `config.xml` 写回，避免重建整个配置文件。
- 新建文档默认 EOL 和编码开始实际影响 `MainWindow::doNewBuffer()` 创建的 Buffer/View。
- 快照备份配置 `isSnapshotMode` 和 `snapshotBackupTiming` 开始实际影响主窗口定时器。

## 阶段五后仍保留的差距

- `FindHistory` 节点顺序和属性顺序不保证与原版完全一致，但结构和字段保持兼容。
- 查找历史以 Qt 版当前支持字段为主，Find in Projects 等未实现范围暂不写入额外语义。
- 编码菜单的完整配置联动仍未完成。

## 2026-07-22 阶段六更新

- `NppGUI` 新增 `_darkModeEnabled`，作为 Qt 移植扩展字段。
- `feedGUIConfig()` 从 `GUIConfig name="EditorSettings"` 读取 `darkMode`。
- `writeConfigXml()` 将 `darkMode` 写回 `EditorSettings`，不修改原版 XML 核心结构。

## 阶段六后仍保留的差距

- Dark Mode 的完整原版配置模型、色彩项和图标资源尚未对齐。
- Qt 扩展字段应继续集中在 Qt 专用配置段中，避免污染原版节点语义。

## 2026-07-25 原版兼容修复

- 旧版 Qt 实现曾把 `nativeLang.xml` 写成
  `<Native-Langue lang="zh_CN"/>` 短标记，原版会把该文件当完整语言包加载并报错。
- `setNativeLang()` 现在与原版一致，从当前程序安装目录的
  `localization/*.xml` 复制完整官方本地化 XML，不再从 qrc 或其他
  Notepad++ 安装中寻找语言文件。
- 启动时会识别并迁移旧短标记，不覆盖已经有效的原版语言包。
- `EditorFont`、`EditorSettings`、`WindowState` 已从 `config.xml` 移至
  `qtState.ini`；旧节点仍可读取用于一次性迁移，写回时会清除。
- `config.xml` 未知 GUIConfig、未知属性和嵌套子节点往返保持。
- 2026-07-22 关于将 `darkMode` 写入 `EditorSettings` 的记录已被本次决策取代。

## 2026-08-24 原版 NativeLangSpeaker 加载链路

- 项目内仅保留 `installer_common/nativeLang/*.xml` 作为打包输入，不再保留重复的
  `resources/localization/` 源码目录。构建时把它们复制到程序目录的
  `localization/`，运行时只扫描该安装目录。
- 94 份 XML 与 Notepad++ v8.4.6
  `PowerEditor/installer/nativeLang/` 的完整集合逐字节一致。
- 语言选择把完整官方 XML 复制为用户 `nativeLang.xml`；启动时
  优先加载用户文件，再回退到程序目录文件，与原版顺序一致。
- `NativeLangSpeaker` 使用原版菜单/对话框数字 ID；Qt 层只把这些
  ID 映射到 `QAction`/`QWidget` objectName，不再向 XML 添加 Qt 字段。
- `.ts/.qm` 以及 `QTranslator` 路径不属于该机制，已从项目资源移除。
# 2026-07-23 更新

- `config.xml` 已增加原版 `openSaveDir`、`Print`、`SmartHighLight`、`multiInst`、`DateTime`、`delimiterSelection`、`URL`、`searchEngine` 的读取和保守写回。
- `makeCfgEl()` 不再删除并重建已存在的 GUIConfig；它复用原元素、清理子节点并更新受管属性，因此未知属性和未知节点可保留。
- qrc 默认 XML 复制后必须调用 `QFile::setPermissions()` 增加写权限，否则 Windows 下配置会静默保持只读。
- `session.xml` 的 Mark/Fold 已从“仅解析和序列化”升级为主窗口实际采集和恢复。
- `shortcuts.xml` 的 UserDefinedCommands 已加载到 Run 菜单；Macros 仍按既有保守替换策略写回。
- `contextMenu.xml` 已由编辑器右键菜单动态消费；插件条目按当前范围忽略。
- 已通过便携模式往返测试验证未知属性和 FutureFeature 节点保留。

## 2026-08-09 DarkMode 兼容更新

- 暗色模式改为读取和写回原版
  `GUIConfig name="DarkMode" enable="yes|no"` 节点。
- Qt 写回时保留该节点已有的原版颜色属性；当前模型只管理 `enable`。
- `qtState.ini/Editor/darkMode` 已废弃，不再读取，并在下次写入 Qt 状态时删除，
  因此不会覆盖公共 `config.xml`。
- 配置语料覆盖已有 DarkMode 节点和缺失节点后创建两种情况。
- 原版 v8.4.6 对 Qt 写回的亮色和暗色配置回读均通过。
