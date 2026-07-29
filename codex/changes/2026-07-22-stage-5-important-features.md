# 2026-07-22 阶段五 重要功能补齐

## 修改范围

- `./src/Parameters.h`
- `./src/Parameters.cpp`
- `./src/ScintillaComponent/FindReplaceDlg.h`
- `./src/ScintillaComponent/FindReplaceDlg.cpp`
- `./src/ScintillaComponent/Buffer.h`
- `./src/MISC/FileManager.cpp`
- `./src/MainWindow.h`
- `./src/MainWindow.cpp`

## 行为变化

- 新增 Qt 版 `FindHistoryState`，读取/写回原版兼容的根节点 `FindHistory`。
- 查找/替换对话框启动时加载 Find/Replace/Filter/Path 历史。
- Find/Replace/Find All/Mark 操作后更新历史，并写回 `config.xml`。
- 查找对话框关闭时保存查找选项和历史。
- Search 菜单中的 Find Next、Find Previous、Mark... 从 disabled 占位改为可用入口。
- 新建文档现在应用 `config.xml` 中 `NewDocDefaultSettings` 的默认 EOL 和编码。
- Session snapshot 定时器现在尊重 `Backup@isSnapshotMode` 和 `Backup@snapshotBackupTiming`。
- 周期备份对同一个 Buffer 去重，避免主/副视图重复写同一备份。
- `Buffer` 记录最后已知磁盘修改时间。
- `FileManager` 打开/保存文件后同步最后已知修改时间。
- `MainWindow` 使用 `QFileSystemWatcher` 监听已打开文件的外部修改。
- 外部修改触发时提示是否 Reload from Disk；保存自身引发的 watcher 事件会被忽略。

## 验证方式

已在 `./build` 执行：

```bash
cmake --build .
```

结果：构建成功。

## 后续注意

- Regex 仍使用 QScintilla/Qt 行为，尚未声明为原版 Boost.Regex 完全兼容。
- Find in Files 和所有打开文件查找仍属于阶段 6 范围。
- 外部文件变更检测已具备基础提示，但删除/重命名/权限变化等边界还需增强。
- 编码检测仍是 BOM + UTF-8 验证 + locale fallback，并非原版完整编码识别。
