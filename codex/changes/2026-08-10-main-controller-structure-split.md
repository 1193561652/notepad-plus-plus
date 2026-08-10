# 2026-08-10 主控制器结构拆分

## 修改范围

- 新增 `src/Notepad_plus.cpp`、`src/NppCommands.cpp`、`src/NppIO.cpp` 和
  `src/NppNotification.cpp`，按 v8.4.6 的主控制器职责组织现有成员定义。
- `src/MainWindow.cpp` 保留 Qt 窗口、视图、Dock/面板和插件宿主服务。
- 更新 CMake 源文件列表及依赖源码内容的两项静态测试。

## 行为变化

无。类声明、成员状态、信号连接、函数体和调用顺序保持不变。

## 验证方式

- 拆分前后 `MainWindow::` 成员定义名称集合一致。
- `cmake --build build --config Release --parallel 8` 通过。
- `ctest --test-dir build -C Release --output-on-failure`：38/38 通过。
- `main-controller-structure-tests` 固定关键方法的唯一性和实现单元所有权。

## 后续修改入口

- 文件、Buffer、Session、编码和磁盘监视：`NppIO.cpp`。
- 菜单、搜索替换、编辑命令和动作状态：`NppCommands.cpp`。
- 标签、编辑器、焦点和生命周期通知：`NppNotification.cpp`。
- 初始化和命令行顶层协调：`Notepad_plus.cpp`。
- Qt 窗口、Dock/面板和插件宿主：`MainWindow.cpp`。
