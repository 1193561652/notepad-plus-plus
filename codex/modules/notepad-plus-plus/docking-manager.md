# DockingManager

## 原版基线

- v8.4.6：`PowerEditor/src/WinControls/DockingWnd/DockingManager.*`、
  `DockingCont.*` 和 `Docking.h`。
- `Notepad_plus` 在主/副文档视图创建后初始化 `_dockingManager`。
- 管理器拥有 left/right/top/bottom 四个基础容器；同侧 client 以标签组织，浮动
  面板使用额外容器。
- `NPPM_DMMREGASDCKDLG` 由主窗口接收，再调用 `createDockableDlg(tTbData, ...)`。

## Qt 实现

- `MainWindow` 同样持有一个 `_dockingManager`，在永久主/副视图创建后初始化。
- `DockingManager` 保留原版职责和主要入口，内部使用 `QMainWindow/QDockWidget`
  实现布局，不复制 Win32 splitter、hook 或自绘 caption。
- 所有内置 Dock 都经 `createDockableDlg(DockingData, container, visible)` 注册；
  `MainWindow` 不直接创建 `QDockWidget` 或调用 `addDockWidget/saveState/restoreState`。
- 四侧尺寸继续使用原版 `config.xml` 的 `DockingManager` 属性；完整 Qt 布局保存在
  `qtState.ini`。

## 调用链

```text
MainWindow::setup*Panel
  -> DockingManager::createDockableDlg
     -> QMainWindow::addDockWidget/tabifyDockWidget

View QAction
  -> DockingManager::toggleDockableDlg/showDockableDlg
     -> QDockWidget visibility/raise
```

## 后续边界

- Win32 插件 Dock 必须把 `tTbData` 转换为 `DockingData`，并由本管理器创建原生
  client host；不得在 `Win32PluginManager` 或 `MainWindow` 自建另一套 Dock 布局。
- 接入真实插件时补齐动态浮动容器、previous-container、Dock 通知和 `ActiveTabs`
  XML 语义。
