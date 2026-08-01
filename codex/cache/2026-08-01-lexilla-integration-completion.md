# Lexilla 接入完成缓存

日期：2026-08-01

- 版本：Lexilla 5.1.9，与 Notepad++ v8.4.6 一致。
- 构建：项目内 `npp-lexilla` 静态库。
- ABI：`CreateLexer -> ILexer5 -> SCI_SETILEXER`。
- 配置：完整读取 `langs.xml` 的 9 组关键词，并合并 `stylers.xml` 用户关键词。
- 分派：简单语言使用原版 LIST 掩码；复杂语言使用原版专属 Lexilla 槽位。
- 属性：语言专属 `SCI_SETPROPERTY`、word chars 和 EOL-filled style 已接入。
- 扩展：`installExternalLexer(name, factory)` 是后续插件层提供外部 lexer 的唯一编辑器边界。
- 验证：全量构建通过，CTest `29/29` 通过。
- 运行时：`p1-runtime-capture` 退出码 `0`，并修复了 `SCI_GETSELTEXT` 选区末字节截断回归。
