# 大文件模式功能索引

## 状态

- 目标：复刻 Notepad++ v8.4.6 方案 A。
- 实现状态：已完成。
- 固定阈值：`200 * 1024 * 1024` 字节，达到阈值后静默启用。
- 后续可选优化：方案 B，见
  `codex/decisions/2026-07-26-large-file-mode-v846-parity.md`。

## 原版依据

| 行为 | v8.4.6 入口 |
|---|---|
| 阈值、Buffer 状态、文档创建、分块加载 | `PowerEditor/src/ScintillaComponent/Buffer.*` |
| 自动完成、配对和 Smart Highlight 降级 | `PowerEditor/src/NppNotification.cpp` |
| 括号匹配和保存流程 | `PowerEditor/src/Notepad_plus.cpp` |
| 文件打开与约 2 GiB 分支 | `PowerEditor/src/NppIO.cpp`、`Buffer.cpp` |

## Qt 实现索引

| 行为 | Qt 主线入口 |
|---|---|
| 200 MiB 判定及约 2 GiB 公式 | `Buffer::isLargeFileSize()`、`requiresHugeFileConfirmation()` |
| 32 位拒绝、64 位确认 | `MainWindow::confirmHugeFileOpen()` |
| 大文本 Scintilla 文档 | `ScintillaEditView::createLargeDocument()` |
| 128 KiB 流式加载 | `FileManager::loadBufferContent()` |
| 流式保存和 gap 分段 | `FileManager::saveBufferCopy()` |
| UTF-16 跨块代理项保护 | `takeIncompleteUtf16Tail()` |
| 指针宽度消息接口 | `QsciScintillaBase::SendScintillaNpp()` |
| 初次打开、reload、强制编码重解释 | `MainWindow::doOpenFile()`、`reloadFromDisk()`、`reinterpretAs()` |
| 双视图和 Document Map | `cloneToOtherView()`、`DocumentMapPanel::syncWith()` |
| Session 保存/恢复 | `MainWindow::saveSession()`、`restoreSession()` |
| 降级策略 | `ScintillaEditView::setLargeFileMode()`、`applyPreferencesToView()` |
| 周期备份禁用 | `MainWindow::onBackupTimer()` |

## 行为

- 使用 `SC_DOCUMENTOPTION_STYLES_NONE | SC_DOCUMENTOPTION_TEXT_LARGE`。
- 关闭 lexer、Word Wrap、自动完成、括号匹配和 Smart Highlight。
- Function List 不对大文件生成全文副本。
- 周期 Session 快照不备份大文件，且不保留其备份路径。
- 查找、替换、编辑、撤销、手动保存和 Save As 保持可用。
- 不增加大文件提示、状态栏标记或可配置阈值。
- reload 可在普通文档和大文档之间切换；clone 继续共享同一文档。
- 打开和保存不经过 `readAll -> QString -> setText` 或 `view->text()` 全文副本。

当前 Qt 版尚未全局实现 URL Hotspot、XML 标签匹配、字符自动配对和保存时备份；
因此这些路径没有额外的大文件运行时代码，现有大文件结果等同于原版的禁用状态。
未来实现这些功能时必须先检查 `Buffer::isLargeFile()`。

## 构建边界

- QScintilla 的 Notepad++ 静态构建清单位于
  `third_party/qscintilla/src/npp-qscintilla-static.pro`。
- 该静态库启用 `SCI_OWNREGEX`，同时编入原版 Boost.Regex 适配层。
- `QsciDocument(int options)` 保持 QsciDocument 引用计数和显示生命周期，
  不直接从业务层操作 `SCI_SETDOCPOINTER`。
- Windows 64 位位置、长度、搜索、替换、Session 选择区和结果导航使用
  `qintptr`/`quintptr`，不经过 32 位 `long`。

## 验证

`tests/LargeFileModeTests.cpp` 覆盖：

- `200 MiB - 1`、`200 MiB`、`200 MiB + 1`。
- `SCI_GETDOCUMENTOPTIONS`。
- lexer、Wrap、自动完成降级和 clone 共享。
- UTF-8、UTF-16LE、UTF-16BE、Windows-1252 的跨 128 KiB 分块加载/保存。
- UTF-16 高代理项恰好落在块尾。
- 编辑造成 Scintilla gap 后的流式保存。
- 大文件文档内查找和替换。

2026-07-26 验证结果：主程序构建成功，CTest `17/17` 通过。
32 位拒绝分支由条件编译和公式测试覆盖，尚未在 32 位构建上实机执行；真实
2 GiB 以上文件未纳入常规自动测试，以避免测试环境产生多 GiB 内存占用。
