# 2026-08-02 主/副永久 Scintilla 视图

## 修改范围

- `Buffer` 增加 Scintilla document pointer 的引用持有和释放。
- `DocTabView` 从每标签 editor widget 改为 `QTabBar +` 单个永久编辑器。
- `MainWindow` 的文件、双视图、备份、搜索替换、Session 和编码流程改为先激活
  目标 Buffer，再访问永久编辑器。
- Windows 插件管理器接收永久主/副编辑器。
- Windows 插件系统增加 `Win32MainWindowAdapter`、`Win32MainEditorAdapter` 和
  `Win32SubEditorAdapter`，分别封装三个稳定 Qt 窗口与 HWND。
- 编辑器适配器不直接暴露 Qt Scintilla HWND；管理器创建两个隐藏的 `1×1` Win32
  子窗口作为主/副编辑器代理句柄，并与相应永久编辑器建立映射。
- 新增 v8.4.6 Win32 插件 ABI 头和最小 smoke DLL；管理器加载时调用
  `setInfo(NppData)` 注入三个适配 HWND，并校验其余五个标准导出。

## 行为变化

- 进程生命周期内主/副文档编辑器各创建一次，与原版 v8.4.6 一致。
- 每个 Buffer 拥有独立 Scintilla document；标签切换只执行 document pointer 切换。
- clone 在两个永久视图中共享文档内容，但光标和滚动状态按视图、按 Buffer 保存。
- 关闭标签不销毁编辑器；最后一个 Buffer 关闭后，永久编辑器切换到空标准文档。

## 关键约束

- `Buffer::views()` 表示该 Buffer 出现在哪些主/副视图，不表示这些编辑器当前显示它。
- 操作非当前 Buffer 前必须调用 `MainWindow::activateBufferView()`，或在指定
  `DocTabView` 中执行 `activateBuffer()` 后核对 document pointer。
- 文档重建后必须先释放 Buffer 的旧引用，再 addref 新 document，并同步另一视图中
  当前显示的 clone。

## 验证

- `cmake --build build --parallel 4`：通过。
- `ctest --test-dir build --output-on-failure`：31/31 通过。
- `UiParityCapture`：验证主/副各一个永久编辑器、Windows 插件句柄指向这两个对象，
  以及同一编辑器在两个 Buffer 间切换时对象不变、文本分别保留。
