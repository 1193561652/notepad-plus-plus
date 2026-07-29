# 本地缓存：编码检测和 Session UI 状态

主线：`./`

## 当前结论

- uchardet 已直接进入 CMake 目标，不依赖外部安装。
- XML/HTML 声明、UTF-8 cookie、配置开关和 locale fallback 已集中在
  `TextFileCodec`，文件打开、reload、文件搜索和文件替换共用这条链路。
- Session `encoding` 是外部代码页；UTF-8/UTF-16 保存为 `-1`。
- Session 的标签颜色、Document Map 和文件浏览器选中项已应用到 UI。
- 快照不再固定写 UTF-8；无法按原编码表示时回退到带 BOM UTF-8。

## 修改前索引入口

- `src/MISC/TextFileCodec.*`
- `src/MISC/EncodingMapper.*`
- `src/uchardet/`
- `src/MainWindow.cpp` 中的 `decodingOptionsForPath()`、
  `sessionEncodingForBuffer()`、`saveSession()`、`restoreSession()`
- `src/ScintillaComponent/Buffer.h`
- `src/WinControls/TabBar/DocTabView.*`
- `src/WinControls/DockingWnd/DocumentMapPanel.*`
- `src/WinControls/DockingWnd/FileBrowserPanel.*`

## 待决边界

- 大文件模式和 Boost.Regex 全语义均为明确延期项，见
  `codex/decisions/2026-07-25-deferred-large-file-and-boost-regex.md`。
