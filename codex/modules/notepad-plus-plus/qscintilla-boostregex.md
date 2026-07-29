# QScintilla 与 Boost.Regex 模块索引

## 构建入口

- `third_party/qscintilla/src/npp-qscintilla-static.pro`
- `cmake/BuildNppQScintilla.cmake`
- `CMakeLists.txt`

CMake 目标 `npp-qscintilla-build` 运行 qmake，并将生成物规范化为
`<build>/qscintilla-npp-<config>/stage/` 下的静态库。
`npp-qscintilla` 是主程序和正则测试共同链接的 imported target。

## 源码边界

- `third_party/boostregex/`：原版适配器与精简 Boost。
- `third_party/qscintilla/`：只读 QScintilla/Scintilla 源码来源。
- `src/ScintillaComponent/ScintillaTextSearch.*`：对 QString 文本提供
  Scintilla target-search 包装，用于打开文档和文件范围。

## 调用路径

1. 当前编辑器查找由 `FindReplaceDlg` 调用 `ScintillaEditView`。
2. QScintilla 将 `SCI_SEARCHINTARGET` 交给 Scintilla `Document`。
3. `SCI_OWNREGEX` 工厂创建 `BoostRegexSearch`。
4. 打开文档和文件范围先把文本放入临时 QScintilla 文档，再调用同一组
   target-search 和 replacement 消息。

## 验证

`tests/BoostRegexTests.cpp` 覆盖：

- Boost 后行断言。
- `\R` 与 CRLF。
- 捕获组替换。
- 纯文本批量路径和编辑器路径使用相同语义。

二进制依赖检查应确认最终程序不依赖 `qscintilla2_qt5*.dll`。
