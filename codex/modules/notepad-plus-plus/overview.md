# Notepad++ Qt 模块概览

## 定位

仓库根目录是唯一主线，负责用 Qt Widgets、C++14、CMake 和 QScintilla
实现 Notepad++ v8.4.6 的跨平台移植版本。

## 当前状态

当前项目已完成核心编辑、配置、双视图、会话、查找替换、主要停靠窗口、
偏好设置、UDL、命令行和大文件方案 A，并具备完整自动测试入口。

- 基础 CMake 构建系统。
- 主窗口框架。
- ScintillaEditView 编辑器包装。
- 基础菜单和工具栏。
- 文档管理系统，包括 Buffer、FileManager、DocTabView。
- 文件保存、另存为、脏标记、关闭询问等基础文档行为。

插件系统和大文件方案 B 按当前决策延期。

## 关键入口

- `src/main.cpp`：应用程序入口。
- `src/MainWindow.h|cpp`：主窗口和多数用户可见行为的协调入口。
- `src/ScintillaComponent/ScintillaEditView.h|cpp`：QScintilla 适配层。
- `src/ScintillaComponent/Buffer.h|cpp`：文档缓冲区。
- `src/MISC/FileManager.h|cpp`：文件和 Buffer 生命周期管理。
- `src/WinControls/TabBar/DocTabView.h|cpp`：标签页管理。
- `src/Parameters.h|cpp`：配置管理。
- `src/PluginSystem/IPlugin.h`、`src/PluginSystem/PluginManager.h|cpp`：插件接口预留。

## 开发约束

- 修改前必须核验原版 Notepad++ 的目标行为。
- 优先保持用户行为一致，而不是保留 Win32 调用形态。
- 配置文件必须完全读写兼容，不修改 XML 结构，不自动升级配置。
- 插件不作为近期功能目标，但接口和平台隔离边界需要保留。
- 每次只处理有限且独立的功能范围。
- 功能完成后至少进行一次验证。

## 风险点

- `MainWindow` 当前承载大量职责，修改菜单、视图、状态栏、文件和插件入口时要关注联动。
- `Buffer` 与 `ScintillaEditView` 的关系会影响文件保存、标签显示、脏状态和双视图。
- 配置读写必须避免改变原版 XML 结构。
- 插件相关代码应避免过早绑定某个平台的动态库细节。
- `ENABLE_PLUGIN_SYSTEM` 当前默认开启，和“近期不实现插件功能”之间需要后续决策。
