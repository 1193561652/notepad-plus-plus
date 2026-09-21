# 分块加载期间保持编辑器视口稳定

## 问题

Qt 版 `FileManager::loadBufferContent()` 直接向当前 `ScintillaEditView`
逐块追加文本。每个 128 KiB 块都会更新 Scintilla 的滚动范围并重绘可见控件，
因此加载较大文件时，用户会看到文档界面随每批内容轻微滚动。

Notepad++ v8.4.6 的 `FileManager::loadFileData()` 使用不可见的 scratch
Scintilla 完成分块加载，可见编辑器不会呈现中间状态。Qt 主线现在采用相同的
职责和发布顺序：`FileManager` 在加载调用内创建不可见 scratch Scintilla，完整加载
成功后才把其文档交给目标视图。

## 修改

- `FileManager::loadBufferContent()` 的清空、预分配、解码和分块追加全部作用于
  scratch 视图。
- 只有成功结束批量加载后才调用 `setDocument()`，可见视图只发生一次文档切换。
- 初次打开不再预先清空当前可见视图；reload 也不再提前创建标准文档。
- 加载错误时目标视图仍持有原文档。
- 会话或初始缓冲区创建完成后统一应用标签栏可见性，偏好设置中的
  `TabBar hide` 也会即时应用。
- 对应原版 `DocTabView::addBuffer()` 在插入标签后发送 `WM_SIZE` 的流程，Qt
  版在标签增删后同步由当前主题和 DPI 计算的动态最小高度，并重新激活
  `DocTabView` 布局。这避免 Qt 5 在顶层窗口显示前创建首个标签时，仍使用
  空标签阶段的零高度。
- UI 回归验证同时检查标签数量、可见状态和实际高度，防止“逻辑可见但高度
  为 0”的回归。
