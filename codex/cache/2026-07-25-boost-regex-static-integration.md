# Boost.Regex 集成本地缓存

- 状态：已实施并通过测试。
- 架构：自建静态 QScintilla + `SCI_OWNREGEX` + 原版 Boost 适配器。
- 用户搜索路径：当前文档、选区、打开文档、文件范围均使用 Boost.Regex。
- 构建目标：`npp-qscintilla-build`。
- 核心服务：`ScintillaTextSearch`。
- 测试：`boost-regex-tests`。
- 不再成立的旧结论：跨文件搜索依赖 `QRegularExpression`、Boost.Regex
  仍整体延期。
- 仍有效的延期项：大文件模式。
