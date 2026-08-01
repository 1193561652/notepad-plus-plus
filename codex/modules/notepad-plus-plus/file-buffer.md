# Notepad++ Qt 文件与 Buffer 模块索引

## 文件

- `src/ScintillaComponent/Buffer.h`
- `src/ScintillaComponent/Buffer.cpp`
- `src/MISC/FileManager.h`
- `src/MISC/FileManager.cpp`
- `src/WinControls/TabBar/DocTabView.h`
- `src/WinControls/TabBar/DocTabView.cpp`
- `src/MainWindow.h`
- `src/MainWindow.cpp`

## 主要类

- `Buffer`
- `FileManager`
- `DocTabView`
- `MainWindow`

## 主要方法

- `FileManager::newBuffer()`
- `FileManager::loadBuffer()`
- `FileManager::saveBuffer()`
- `FileManager::closeBuffer()`
- `FileManager::findBufferByPath()`
- `MainWindow::doNewBuffer()`
- `MainWindow::doOpenFile()`
- `MainWindow::doSave()`
- `MainWindow::doSaveAs()`
- `MainWindow::checkBufferSave()`
- `MainWindow::watchBufferFile()`
- `MainWindow::unwatchBufferFile()`
- `MainWindow::onWatchedFileChanged()`
- `DocTabView::addBuffer()`
- `DocTabView::addClone()`
- `DocTabView::removeBuffer()`

## 缓存结论

文件/Buffer 模块已经形成基础三层：Buffer 持状态，FileManager 管生命周期，MainWindow 协调 UI 流程。后续需要继续核验文件读取、编码检测、EOL 保持和克隆视图所有权。

## 2026-07-22 阶段二更新

- `MainWindow::doOpenFile()` 现在基于原始字节检测 BOM、编码和 EOL，再写入 `ScintillaEditView`。
- `MainWindow::reloadFromDisk()` 同步使用相同检测策略，避免 reload 后编码/EOL 状态漂移。
- `FileManager::saveBuffer()` 现在检查实际写入字节数，写入不完整时返回失败。
- `MainWindow::closeAllFiles()` 同时覆盖主/副视图并对 Buffer 去重。
- `MainWindow::onBufferCloseRequested()` 不再依赖 `sender()` 作为唯一来源，菜单/工具栏关闭当前文件时能定位正确视图。
- `MainWindow::saveSession()` 使用标签页实际 view 保存 selection、滚动位置和编码。
- `MainWindow::restoreSession()` 支持恢复副视图，并对同一文件跨视图创建共享文档 clone。

## 2026-07-22 阶段五更新

- `Buffer` 新增最后已知磁盘修改时间。
- 打开、保存和 reload 后都会刷新最后已知修改时间。
- 主窗口通过 `QFileSystemWatcher` 监听已打开文件，外部修改时提示 reload。
- 保存自身触发的 watcher 事件通过 `_savingPaths` 忽略。
- 新建 Buffer 应用 `NewDocDefaultSettings` 的默认 EOL 和编码。

## 2026-07-25 索引更新

- 新增 `src/MISC/TextFileCodec.*`，作为打开、reload、保存、文件搜索替换的统一编码入口。
- `Buffer` 现在维护 `QList<ScintillaEditView*>`，`getView()`只返回当前首选 view；
  涉及同步时必须遍历 `views()`。
- `DocTabView::addBufferView()`用于移动现有 editor widget；`addClone()`使用
  Scintilla document pointer 注册共享文档的新 view。
- 关闭单个 clone 只调用 `Buffer::removeView()`；仅最后一个 view 关闭时才调用
  `FileManager::closeBuffer()`。
- 文本脏状态与编码/BOM/EOL 元数据脏状态分开记录，`Buffer::isDirty()`合并两者。
- Session 保存和恢复位于 `MainWindow::saveSession()` / `restoreSession()`；
  磁盘时间戳按原版 FILETIME 低/高 32 位保存。

## 2026-07-26 大文件模式更新

- `Buffer` 保存固定 200 MiB 判定结果及源文件字节数。
- `FileManager::loadBufferContent()` 统一承担首次打开、reload、Session 备份恢复
  和强制编码重解释的 128 KiB 流式加载。
- `FileManager::saveBufferCopy()` 从 Scintilla 连续范围流式转码并原子写出，
  跨 gap 时拆分范围。
- 大文件文档使用无样式、大文本 Scintilla 选项；clone 和 Document Map
  继续通过 `SCI_GETDOCPOINTER` / `SCI_SETDOCPOINTER` 共享文档。
- lexer、Wrap、自动完成、匹配、Smart Highlight、Function List 和周期备份
  通过 Buffer 大文件状态统一降级。
- 位置、长度、搜索替换和 Session 选择区使用指针宽度消息接口。
