# 查找替换功能分析

## 功能定位

查找替换功能由 `FindReplaceDlg` 提供 UI 和搜索操作，并通过 `MainWindow` 接入当前 `ScintillaEditView`。

## 当前实现入口

- `./src/ScintillaComponent/FindReplaceDlg.h`
- `./src/ScintillaComponent/FindReplaceDlg.cpp`
- `./src/MainWindow.cpp`
- `./src/ScintillaComponent/ScintillaEditView.h`

## 当前能力

- Find、Replace、Find in Files、Mark 等 Tab 结构已存在。
- 支持 Normal、Extended、Regex 搜索模式枚举。
- `FindOption` 包含大小写、整词、循环、方向、正则等选项。
- 支持 Find All 结果信号：`findAllResultsReady()`。
- 支持 Mark All / Clear All Marks，使用 `ScintillaEditView::FIND_MARK_INDICATOR`。
- 透明度控件已在 UI 结构中预留。
- 阶段五已接入 `config.xml` 根节点 `FindHistory` 的读取和写回。
- 阶段五已让 Find/Replace/Find All/Mark 操作更新查找、替换、过滤器和目录历史。
- 阶段五已让 Search 菜单中的 Find Next、Find Previous、Mark... 从占位入口变为可用入口。
- 阶段六已接入所有打开文档查找和 Find in Files 的基础实现。
- 阶段六后 Find Result 面板可以显示来源文件名，并在激活结果时切换/打开来源文件后定位。

## 风险点

- 用户正则统一由 `SCI_OWNREGEX` 和 v8.4.6 `BoostRegExSearch.cxx` 执行；新增路径不得回退到 `QRegularExpression`。
- 文档字节偏移和 QString 字符偏移可能不一致，影响查找结果跳转。
- Find in Files 当前是否完整实现待核验。
- 查找历史、选区查找、扩展模式转义规则需要对照原版。

## 与原版差异

- 当前文档、所有打开文档、目录和三个 Project Panel 范围均已接通查找替换。
- Finder 已有来源分组、高亮、字节位置导航和进度取消；结果 lexer 的少量绘制细节仍可能与原版不同。
- 正则执行后端与原版一致，Qt 版主要风险在边界 UI、超大目录性能和结果呈现，而非正则引擎替代。

## 2026-07-22 阶段五完成项

- `Parameters` 新增 `FindHistoryState`，字段映射原版 `FindHistory` 常用属性，包括最大历史数量、大小写、整词、循环、方向、Find in Files 选项、搜索模式和透明度。
- `FindReplaceDlg::loadFindHistory()` 在对话框创建时回填 Find、Replace、Filter、Path 下拉历史和主要搜索选项。
- `FindReplaceDlg::saveFindHistory()` 在搜索、替换、标记和关闭对话框时保守写回历史。
- `FindReplaceDlg::findNext(bool forward)` 对外暴露当前文档向前/向后查找能力，供菜单 `Find Next` / `Find Previous` 使用。
- `Mark...` 菜单入口现在会打开查找替换对话框的 Mark 页。

## 阶段五后仍保留的差距

- Find in Files、所有打开文档查找/替换仍属于阶段六范围。
- Regex 仍未承诺与原版 Boost.Regex 行为完全一致。
- 查找结果面板已能承接当前文档 Find All 结果，但原版 search result lexer、分组和跳转细节仍需继续补齐。
- 历史写回现在以功能可靠为先，后续可减少频繁写盘。

## 2026-07-22 阶段六完成项

- `FindReplaceDlg` 通过 `findAllOpenedDocsRequested()` 和 `findInFilesRequested()` 将跨范围搜索请求交给主窗口。
- `MainWindow::onFindAllOpenedDocsRequested()` 遍历主/副视图 Buffer，并对同一 Buffer 去重。
- `MainWindow::onFindInFilesRequested()` 支持目录、过滤器、递归和隐藏文件选项。
- `FindAllResult` 扩展了 `filePath` 和 `sourceName`，用于跨文件结果显示与跳转。
- Search 菜单中 `Find in Files...` 与 `Find All in All Opened Documents` 已从禁用占位变成可用入口。

## 阶段六后仍保留的差距

- Replace in Files 和 Replace All in Opened Docs 仍未实现。
- Find in Projects 仍为占位。
- Find in Files 尚未支持取消搜索、二进制跳过、超大文件分批搜索和完整编码自动识别。
- 此段是阶段六历史状态；当前正则后端已在 2026-07-25 后切换为原版 Boost.Regex 适配层。

## 后续索引任务

- 建立查找选项与原版 UI 控件/配置字段映射。
- 对比原版 FindReplaceDlg 的搜索语义。
- 明确 Regex 暂缓范围与替代策略。
- 建立 Find in Files 与原版 `Finder` 结果格式、过滤器语义和取消流程的差异索引。
# 2026-07-23 更新

- Replace All in All Opened Documents 已实现，主副视图按 Buffer 去重并保留单文档撤销组。
- Replace in Files 已实现：
  - 支持目录、过滤器、递归和隐藏文件选项。
  - 跳过二进制文件和已打开的脏文档。
  - UTF-8 BOM、UTF-16 LE/BE BOM、无 BOM UTF-8 和 locale fallback 按原形式写回。
  - 未打开文件使用 QSaveFile 原子提交。
- Find/Replace in Projects 当前复用目录和过滤器搜索路径；尚未限定到原版 workspace XML 项目成员。

## 2026-07-26 P0 更新

- 上述“复用目录”描述已过期。
- 已新增原版 `NotepadPlus/Project/Folder/File` workspace XML 数据模型和三个独立 Project Panel。
- Find/Replace in Projects 现在根据 Panel 1/2/3 选择，仅处理对应 workspace 的文件成员。
- Replace in Projects 保留原编码/BOM，使用 `QSaveFile`，并跳过二进制、只读和已打开的脏文件。
- 用户正则搜索统一使用 `npp-scintilla-qt` 中的原版 Boost.Regex 适配层。

## 2026-07-25 索引更新

- 当前文档 Replace/Replace All 已统一处理 Extended 查找与替换转义。
- Regex dot-matches-newline 选项已贯穿当前文档、所有打开文档和文件范围。
- Replace All 从文档开头开始，选区边界会按替换后的长度变化调整。
- 零长度 Regex 替换使用原始字符边界前进规则，避免插入内容导致无限匹配。
- 所有打开文档按 Buffer 去重，并跳过只读和二进制文档。
- Find/Replace in Files 使用 `TextFileCodec` 保持编码和 BOM，写回使用 `QSaveFile`。
- 文件搜索结果由 `ScintillaTextSearch` 直接返回 UTF-8 字节偏移。
- 剩余主要差异是 Finder 结果 lexer、取消/进度和大目录性能。

## 2026-07-26 P1 更新

Finder 已增加来源分组、匹配片段高亮、UTF-8 字节位置导航以及文件/项目搜索
进度取消；Mark 页已实现 Copy Marked Text；保存搜索宏 type 3 已接入。
运行验证等待用户授权，详见
`codex/validation/2026-07-26-p1-pending/README.md`。

## 2026-07-25 Boost.Regex 更新

- 当前文档、选区、所有打开文档和文件范围均走 Scintilla target-search。
- `SCI_OWNREGEX` 工厂使用 Notepad++ v8.4.6 的 `BoostRegExSearch.cxx`。
- `QRegularExpression` 不再作为用户输入正则的执行引擎。
- 自动测试覆盖后行断言、`\R`、捕获组替换和跨范围统一语义。

## 2026-07-28 Find 对话框本地化

- `MainWindow::ensureFindReplaceDialog()` 是 Find 对话框唯一的延迟创建和本地化
  入口；Find、Replace、Find Next/Previous、Find in Files、Find All、Mark 和
  Project Panel 查找入口都必须调用该函数。
- `applyNativeLang()` 会销毁未显示的旧对话框，因此任何重新创建
  `FindReplaceDlg` 的代码都不能绕过上述入口，否则控件会回退为构造时英文。
- Find in Projects 页的项目面板复选框和按钮已补齐稳定 `objectName`，并在
  `resources/nativeLang/chineseSimplified.xml` 中补齐对应翻译。
- `tests/FindDialogLocalizationTests.cpp` 检查统一入口、控件标识和语言资源覆盖。
- 2026-07-29 起，`ui-parity-capture` 还会从 `markDialogAction` 真实打开对话框，
  并在浅色/深色、100%/150% 缩放下校验中文标题、5 个标签和 Mark 按钮。
