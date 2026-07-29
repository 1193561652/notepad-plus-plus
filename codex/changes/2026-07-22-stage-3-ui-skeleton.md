# 2026-07-22 阶段三 UI 与视图骨架变更

## 修改范围

- `./src/MainWindow.cpp`

## 行为变化

- File 菜单补齐原版主要可见入口：
  - Open Containing Folder
  - Open in Default Viewer
  - Open Folder as Workspace
  - Save a Copy As
  - Rename
  - Close Multiple Documents
  - Move to Recycle Bin
  - Load/Save Session
  - Print / Print Now
- Edit 菜单补齐原版主要可见分组：
  - Insert
  - Line Operations 扩展项
  - Comment/Uncomment 扩展项
  - Convert Case 扩展项
  - Blank Operations
  - Auto-Completion
- Search 菜单补齐原版主要可见入口：
  - Find Next / Previous
  - Find in Files
  - Find All in All Opened Documents
  - Go to Matching Brace
  - Mark 子菜单
- View 菜单补齐原版主要可见入口：
  - Always on Top
  - Full Screen
  - Post-It
  - Toolbar / Status Bar
  - Show Symbol 扩展项
  - Document List
  - Project Panels
  - Clipboard History
  - Character Panel
  - Fold All 子菜单
- Settings 菜单补齐 Style Configurator、Shortcut Mapper、Edit Popup ContextMenu 和 Import 入口。
- 新增 Run 菜单骨架。
- 新增 Window 菜单骨架。
- Help 菜单补齐 Home、Online Documentation、Command Line Arguments、Debug Info 入口。
- `Toolbar` 和 `Status Bar` 可见状态现在从 `config.xml` 初始化，并可在 View 菜单切换。
- `_closeAllAction` 启用状态现在同时考虑主视图和副视图。
- EOL 菜单项会根据当前编辑器 EOL mode 同步 checked 状态。

## 占位策略

本阶段以 UI 骨架为目标。尚未实现实际行为的原版入口统一设置为 disabled，占位但不误导用户。

## 验证方式

已在 `./build` 执行：

```bash
cmake --build .
```

结果：构建成功。

## 后续衔接

- 阶段 4 应基于本阶段形成的菜单/工具栏/状态栏/停靠面板骨架，对照原版 UI 做一致性检验。
- 阶段 5 以后再逐步启用 disabled 占位入口背后的业务逻辑。
