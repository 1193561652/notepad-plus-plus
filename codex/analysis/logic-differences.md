# 原版与 Qt 版逻辑差异索引

分析日期：2026-07-22

## 入口与生命周期

原版：

- `winmain.cpp` 建立 Win32 应用入口。
- `Notepad_plus` 初始化窗口、Scintilla、菜单、插件、配置和 dock 面板。
- 大量行为通过 Win32 消息、菜单命令和 `NppBigSwitch.cpp` 分发。

Qt 版：

- `main.cpp` 建立 `QApplication`。
- `MainWindow` 构造时初始化 UI、配置、标签、面板和插件入口。
- 行为通过 `QAction`、signal/slot 和 Qt 对象生命周期管理。

差异：

- 迁移时不能照搬消息分发，需要建立“原版命令 ID -> Qt 行为入口”的映射。

## 文档模型

原版：

- `Buffer` 持有 Scintilla `Document`。
- `FileManager` 管理 Buffer 列表、Document 引用计数、加载、保存、reload、backup、filesystem check。
- 主/副视图和克隆依赖同一个 Document 的引用。

Qt 版：

- `Buffer` 保存路径、脏状态、编码、BOM、备份路径和一个视图指针。
- `FileManager` 负责轻量 Buffer 生命周期。
- 克隆视图存在入口，但共享和释放语义需核验。

差异：

- 原版把“文档内容”和“视图”分离得更彻底；Qt 版目前 Buffer 与 view 耦合更强。

## 文件 I/O

原版：

- 加载时读取原始字节。
- 通过 BOM、cookie、uchardet 和配置检测编码。
- 检测 EOL。
- 设置 Buffer 的 Unicode mode、codepage、language、timestamp。
- 保存时使用 `Utf8_16_Write` 保持编码模式。

Qt 版：

- 保存时通过 `QTextCodec` 和 Buffer 编码名写出。
- UTF-16 BOM 路径手工生成。
- 读取路径需要继续核验。

差异：

- Qt 版需要补齐“读取检测”和“原样保持”能力，尤其是完全兼容时。

## 配置写回

原版：

- 加载和写回覆盖多个 XML。
- 维护历史文件、查找历史、项目面板、文件浏览器、Scintilla 视图、dock 数据、插件命令等。
- 初始化时会复制缺失的默认 XML 模板。

Qt 版：

- `NppParameters` 读写主要 XML。
- `writeConfigXml()` 生成支持字段。
- 存在 Qt 专用字段。

差异：

- 对完全读写兼容而言，Qt 版最大的逻辑风险是“重写 XML 时丢失原版未知字段”。

## UI 状态同步

原版：

- 菜单 checked/enabled 状态由命令、配置、当前 Buffer、Scintilla 状态和插件通知共同驱动。
- Toolbar、tabbar、statusbar、dock 状态都可配置。

Qt 版：

- `MainWindow::initActionStates()`、`updateActionStates()` 和若干 slot 维护状态。
- Preference 通过 `settingsChanged` 触发应用。

差异：

- Qt 版应建立可追踪的状态同步表，避免 UI 状态只在局部 slot 中变化。

## 搜索结果

原版：

- 搜索结果有 `Finder`、`FoundInfo`、`SearchResultMarkingLine` 等结构。
- Search result lexer 通过 Scintilla property 获得标记结构。
- 支持结果面板导航和多范围搜索。

Qt 版：

- `FindAllResult` 是简单结构。
- `FindReplaceDlg` 通过 signal 把结果交给主窗口。

差异：

- Qt 版结果模型更简单，后续若要对齐原版，需要扩展结果树、文件分组、行内 segment 和导航行为。

## 插件

原版：

- 插件通过 DLL ABI 导出函数。
- 通过 `NppData` 获取 Notepad++ 和 Scintilla 句柄。
- 使用 `NPPM_*` 消息操作宿主。
- 插件菜单、快捷键、通知、dock 面板、Plugin Admin 都在核心流程中。

Qt 版：

- 插件通过 C 函数创建 `IPlugin`。
- 通过 `IPluginHost` 访问 Qt 对象级能力。

差异：

- Qt 插件接口与原版插件 ABI 不兼容。近期只能作为预留接口，不能宣称兼容原版插件。

