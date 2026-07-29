# 2026-07-22 阶段二 文件、Buffer、Tab 与会话基础变更

## 修改范围

- `./src/MainWindow.cpp`
- `./src/MISC/FileManager.cpp`

## 行为变化

- 文件打开改为先读取原始字节，再检测 BOM、编码和 EOL。
- 打开文件时会把检测到的 EOL 模式应用到 QScintilla，后续新增行和保存更接近原文件。
- Reload from Disk 会重新检测编码、BOM 和 EOL，而不是盲目沿用旧 Buffer 状态。
- 保存文件时检查实际写入字节数，写入不完整时返回失败。
- 保存成功后将文件加入最近文件列表。
- Close All 现在同时收集主视图和副视图中的 Buffer，并去重后关闭。
- 关闭 Buffer 时不再依赖 `sender()` 猜测来源；菜单触发、工具栏触发和标签关闭都能定位实际所在视图。
- 会话保存时使用每个标签页实际显示的 `ScintillaEditView` 采集选择区、滚动位置、xOffset、scrollWidth 和 selection mode。
- 会话保存写入文件最后修改时间和当前 Buffer 编码枚举。
- 会话恢复时如果存在副视图文件，会先显示副视图。
- 会话恢复同一文件跨主/副视图时，会创建共享 QsciDocument 的 clone view，而不是只切换到已打开标签。
- 会话恢复会恢复 selection、firstVisibleLine、xOffset 和 scrollWidth。

## 验证方式

已在 `./build` 执行：

```bash
cmake --build .
```

结果：构建成功，`notepadpp-qt.exe` 链接成功。

## 后续注意

- 当前编码检测仍是 BOM + UTF-8 验证 + locale fallback，尚未达到原版完整编码检测能力。
- 会话中的 fold/bookmark、只读状态、tab color、Document Map 字段还未完整恢复。
- 副视图 clone 的长期行为需要在 UI 阶段和双视图阶段继续做人工/自动验证。
