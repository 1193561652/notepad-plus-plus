# Scintilla 5、Lexilla 与 Boost.Regex 模块索引

文件名保留用于兼容旧索引链接；当前实现不再使用 QScintilla。

## 调用链

```text
ScintillaEditView::execute / SendScintilla
  -> ScintillaEditBase::send
  -> ScintillaQt / Scintilla core
  -> Document
  -> BoostRegexSearch (SCI_OWNREGEX，仅正则搜索)
```

词法器调用链：

```text
Notepad++ language/config data
  -> configured extension/name lookup
  -> v8.4.6 keyword mask or complex-language slot mapping
  -> stylers.xml user-keyword merge and lexer properties
  -> builtinLexerName
  -> Lexilla::CreateLexer(name) -> ILexer5*
  -> SCI_SETILEXER
  -> SCI_SETPROPERTY / SCI_SETKEYWORDS / SCI_STYLE*
```

Lexilla 接入约束：

- `LangDesc` 必须保留 `instre1`、`instre2`、`type1` 至 `type7` 共 9 组关键词。
- 简单 lexer 使用 v8.4.6 的 `LIST_*` 掩码；不能把所有语言统一映射为连续槽位。
- HTML、C/C++、JavaScript、TypeScript、Objective-C、Tcl 和 JSON 必须走专属槽位映射。
- Style Configurator 用户关键词按 keyword class 合并后再发送给 Lexilla。
- 外部 lexer 通过 `installExternalLexer(name, factory)` 提供 `ILexer5*`；动态库加载属于插件层。

## 应用边界

- `ScintillaEditView` 直接继承官方 `ScintillaEditBase`。
- `execute()`、`SendScintilla()` 和 `SendScintillaNpp()` 是兼容现有业务代码的
  薄入口，全部直接调用 `send()`。
- Qt 字符串只在边界转 UTF-8；Scintilla position 和指针参数使用
  `sptr_t/uptr_t`。
- 双视图通过 `SCI_GETDOCPOINTER/SCI_SETDOCPOINTER` 共享 Document。
- 官方 `modified`、`updateUi`、`charAdded`、`marginClicked`、`doubleClick`
  和 `macroRecord` 信号映射到现有业务行为。

## 迁移后的便利层

原 QScintilla 高层 API 已用局部消息映射替代：文本、选择、行列换算、EOL、
wrap、margin、marker、fold、undo/redo、自动完成、打印和宏。不要重新引入
Qt lexer 对象层；新增行为优先使用原版已有的 `SCI_*` 消息。

## 验证

- 最终程序导入表中不应出现 `qscintilla2_qt5*.dll`。
- 源码与 CMake 中不应存在 `Qsci*` 类型或 QScintilla include。
- 关键测试：`lexilla-integration-tests`、`boost-regex-tests`、
  `large-file-mode-tests`。
