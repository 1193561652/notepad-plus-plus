# 自动换行配置持久化

## 原因与原版职责

菜单和工具栏共用 `wordWrapAction`，均进入 `MainWindow::toggleWordWrap`。
Qt 原实现只设置当前视图和动作勾选，没有更新 `ScintillaViewParams::_doWrap`。
退出时 `MainWindow::closeEvent` 调用 `NppParameters::writeNppGUI`，而
`writeConfigXml` 写入的 `GUIConfig/ScintillaPrimaryView@Wrap` 来自 `_svp._doWrap`，
因此保存的是旧值。启动时 `applyPreferencesToView` 又应用这个旧值。

已核验 v8.4.6 `PowerEditor/src/NppCommands.cpp` 的 `IDM_VIEW_WRAP`：命令处理
同步主、副视图，更新菜单／工具栏，并更新 `_svp._doWrap`。原版 Parameters.cpp
同样通过既有 `Wrap=yes/no` 属性读写。继续保留命令处理、参数管理和视图应用的
职责边界，不增加独立配置或更改 XML 结构。

## 修改

- `toggleWordWrap` 将新状态写入 `_svp._doWrap`。
- 主、副编辑器均通过 `applyWordWrap` 应用，保留各自的大文件禁止换行策略。
- 保留活动文档为大文件时的原有拒绝逻辑，不用受限视图的状态覆盖全局偏好。
- 正常退出仍通过已有配置写入路径保存，不新增即时磁盘写入。

## 验证

主程序及 UI 测试程序构建成功。新增 `word-wrap-restart` 回归通过：使用临时
配置目录启动三个独立进程，依次执行工具栏开启并正常退出、重启检查主副视图及
勾选恢复并通过菜单共用动作关闭、再次启动检查关闭状态恢复。
每次退出使用真实 `MainWindow::closeEvent` 保存，避免同进程参数缓存掩盖问题。

## 安装包

Ubuntu 修订号升为 `8.4.6-qt4`。完整 Release 构建及 39/39 CTest 通过。
生成 `build-ubuntu-package/notepad-plus-plus-qt_8.4.6-qt4_amd64.deb`，已检查
元数据、文件清单，解包后确认主程序与本次构建逐字节一致。
