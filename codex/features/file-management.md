# 文件与 Buffer 管理功能分析

## 功能定位

文件与 Buffer 管理是当前编辑器的基础功能，负责新建、打开、保存、另存为、关闭、脏状态和标签标题同步。

## 当前实现入口

- `./src/ScintillaComponent/Buffer.h`
- `./src/ScintillaComponent/Buffer.cpp`
- `./src/MISC/FileManager.h`
- `./src/MISC/FileManager.cpp`
- `./src/MainWindow.h`
- `./src/MainWindow.cpp`
- `./src/WinControls/TabBar/DocTabView.h`
- `./src/WinControls/TabBar/DocTabView.cpp`

## 当前职责划分

- `Buffer`：保存文件路径、文件名、未命名状态、脏状态、编码、BOM、备份路径，以及关联的 `ScintillaEditView`。
- `FileManager`：单例，负责创建 Buffer、加载文件 Buffer、保存 Buffer、关闭 Buffer、按路径查找已打开 Buffer。
- `DocTabView`：显示 Buffer 对应标签页，支持克隆视图标签，负责标签标题更新和关闭请求信号。
- `MainWindow`：协调菜单动作、打开/保存流程、关闭询问、视图激活、状态栏和窗口标题。

## 已确认行为

- `FileManager::findBufferByPath()` 使用 `QFileInfo::canonicalFilePath()` 避免同一路径重复打开。
- `FileManager::saveBuffer()` 根据 Buffer 编码写出文本，处理 UTF-8 BOM、UTF-16LE BOM、UTF-16BE BOM。
- `Buffer::getTabLabel()` 用于标签显示，脏状态时应添加 `*`。
- `MainWindow` 拥有 `doNewBuffer()`、`doOpenFile()`、`doSave()`、`doSaveAs()`、`checkBufferSave()` 等流程入口。
- 阶段五后 `Buffer` 记录最后已知磁盘修改时间，打开、保存、reload 后同步更新。
- 阶段五后 `MainWindow` 使用 `QFileSystemWatcher` 监听已打开文件的外部变更，并提示是否从磁盘重新加载。
- 阶段五后新建文档会应用 `config.xml` 中 `NewDocDefaultSettings` 的默认 EOL 和编码。

## 风险点

- `FileManager::loadBuffer()` 当前只创建 Buffer 并验证文件可读，真实文本加载可能在 `MainWindow::doOpenFile()` 中完成，后续需索引完整链路。
- 编码检测和 EOL 保持需要进一步核验，尤其是保存后是否保持原始换行格式。
- 克隆视图与 Buffer 的所有权关系需要谨慎处理，避免关闭标签时误删共享 Buffer。
- 关闭询问、保存失败、另存为取消等边界行为需要对齐原版。

## 与原版差异

- 原版 `FileManager::loadFile()` 会创建或复用 Scintilla `Document`，并执行编码、BOM、EOL、语言和大文件相关处理。
- 原版 `Buffer` 内部持有 `Document`，并通过引用计数支持主/副视图、克隆和释放。
- 原版支持文件系统变更检测、延迟 reload、备份文件恢复、删除/移动文件等操作。
- Qt 版 `Buffer` 当前更偏状态对象，未承载完整 Scintilla Document 生命周期。
- Qt 版保存逻辑已有 UTF-8/UTF-16 BOM 写出，但读取检测和 EOL 保持还需要专项核验。

## 后续索引任务

- 补充 `MainWindow::doOpenFile()`、`doSave()`、`checkBufferSave()` 的完整行为链。
- 对比原版 Buffer/FileManager 的生命周期模型。
- 建立文件编码、EOL、只读状态、备份路径的专项索引。

## 2026-07-22 阶段二完成项

- 文件打开链路已补强：读取原始字节后检测 BOM、UTF-8 有效性、locale fallback 和 EOL。
- 文件保存链路已补强：`FileManager::saveBuffer()` 校验实际写入长度，保存成功后更新最近文件。
- Reload from Disk 已补强：重新检测 BOM、编码和 EOL。
- 关闭链路已补强：Close All 覆盖主/副视图，关闭来源不再依赖 Qt signal sender。

## 阶段二后仍保留的差距

- 原版更完整的编码检测、文件变更检测、只读状态处理、保存失败细分提示仍未完成。
- 大文件、权限错误、符号链接/网络路径等边界行为还未专项验证。

## 2026-07-22 阶段五更新

- `MainWindow::watchBufferFile()` / `unwatchBufferFile()` 建立 Buffer 与磁盘路径监听关系。
- `MainWindow::onWatchedFileChanged()` 处理外部修改提示；由当前进程保存触发的 watcher 事件会通过 `_savingPaths` 忽略。
- `MainWindow::doSave()` 成功后重新登记 watcher，并更新 Buffer 的最后已知修改时间。
- `MainWindow::reloadFromDisk()` 成功后更新最后已知修改时间并重新监听文件。
- 快照备份定时器现在尊重 `NppGUI::_isSnapshotMode`，并使用 `NppGUI::_snapshotBackupTiming` 作为间隔。
- 周期备份按 Buffer 去重，避免主/副视图共享同一 Buffer 时重复写备份。

## 阶段五后仍保留的差距

- 外部删除、重命名、权限变化、网络盘和符号链接路径的处理仍是基础提示级别。
- 编码检测仍是 BOM、UTF-8 验证和 locale fallback，未达到原版完整识别能力。
- “Encode in” 与 “Convert to” 的完整差异化语义仍需在后续阶段补齐。

## 2026-07-28 待办收口

- 已实现 `Restore Recent Closed File`，使用原版 command ID 41021 和
  `Ctrl+Shift+T`。
- 关闭历史按实际最终关闭顺序保存，主副视图仅关闭一个 clone 时不会误记录。
- 历史去重、限制为 20 项，恢复时自动跳过已经不存在的文件。
- 上述编码检测和 Encode/Convert 旧差异已由 2026-07-25 专项实现取代。
