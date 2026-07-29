# 2026-07-25 Boost.Regex 静态集成

## 已完成

- 从 Notepad++ v8.4.6 主线复制 Boost.Regex 适配层和精简 Boost 1.78。
- 为 QScintilla 2.13.3 增加不修改上游源码的薄兼容适配。
- 新增可由 CMake 自动触发的跨平台 qmake 静态构建。
- 主程序从预编译 QScintilla DLL 切换到自建静态库。
- 当前文档、选区、打开文档和文件范围统一为 Scintilla Boost.Regex。
- 新增 Boost 特有语法和替换行为测试。

## 验证结果

- Debug 全量构建通过。
- `core-behavior-tests` 通过。
- `boost-regex-tests` 通过。
- 最终 `notepadpp-qt.exe` 的导入表不含 QScintilla DLL。
- 静态归档包含 `BoostRegExSearch.o` 和 `UTF8DocumentIterator.o`。

## 保留差异

- 大文件模式仍按原决定延期。
- Finder 结果 lexer、搜索取消/进度和超大目录性能仍需后续专项完善。
