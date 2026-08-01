# Scintilla 5 Qt 迁移本地缓存

日期：2026-08-01

- 当前编辑器基线：Scintilla 5.3.0 + Lexilla 5.1.9。
- CMake 静态目标：`npp-scintilla-qt`、`npp-lexilla`。
- 应用 widget：`ScintillaEditView : ScintillaEditBase`。
- lexer ABI：真实 `ILexer5`，调用方向为 `CreateLexer -> SCI_SETILEXER`。
- 搜索：Scintilla `SCI_OWNREGEX` + v8.4.6 Boost.Regex adapter。
- 文档共享：`SCI_GETDOCPOINTER/SCI_SETDOCPOINTER`；创建者引用设置后释放。
- QScintilla 已完全移除，不再存在 qmake 子构建或 `Qsci*` 类型。
- 自动完成、API call tip、URL indicator release 和宏序列化均已切换到原生 Scintilla 行为；宏格式兼容旧 QScintilla 缓存。
- 第三方源码树不包含预编译产物，所有平台都从项目内源码构建。
- Windows/MinGW 全量构建通过，CTest `29/29` 通过。
- 下次修改入口：先读 `codex/index/build.md` 和
  `codex/modules/notepad-plus-plus/qscintilla-boostregex.md`。
