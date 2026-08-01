# 2026-08-01 Lexilla 接入完善

## 范围

- `langs.xml` 的内置语言关键词模型由 4 组扩展为 v8.4.6 的完整 9 组：`instre1`、`instre2`、`type1` 至 `type7`。
- 文件语言识别优先使用配置中的扩展名映射，特殊文件名继续使用现有回退规则。
- 简单语言按原版 `LIST_*` 掩码向 Lexilla 发送关键词。
- C/C++ 家族、JavaScript、TypeScript、Objective-C、Tcl、JSON、XML、HTML/PHP/ASP/JSP 使用原版专属槽位映射。
- HTML lexer 同时安装 HTML、嵌入 JavaScript、VB/ASP 和 PHP 的关键词、样式及 folding properties。
- 迁移 SQL、C/C++、XML/HTML、Python、Pascal、AutoIt、Verilog、BaanC 等语言专属 properties、word chars 和 EOL-filled styles。
- Style Configurator 的用户关键词先按原版 keyword class 合并，再进入对应 Lexilla 槽位。
- 增加 `ExternalLexerFactory`/`installExternalLexer()` 边界，插件层以后可直接提供 `ILexer5*` 工厂，无需修改编辑器核心。

## 验证

- 验证 `type7` 可从 v8.4.6 `langs.xml` 读入。
- 验证 C++ 指令、类型、Doxygen 槽位和 preprocessor property。
- 验证 HTML 的 0/1/2/4 嵌入语言关键词槽。
- 验证 BaanC 第 9 关键词槽及专属 folding property。
- 使用真实 Lexilla `CreateLexer` 验证外部 `ILexer5` 工厂接口。
- Windows/MinGW 全量构建通过；CTest `29/29` 通过。
- 独立 `p1-runtime-capture` 使用隔离配置完整运行，退出码 `0`；Finder、列编辑器、UDL、外部文件状态和打印 PDF 产物均生成。
- 运行时回归发现 `SCI_GETSELTEXT` 返回长度不含结尾 NUL，旧包装错误截断选区最后一个字节；已修正并加入直接断言。

## 边界

外部 lexer 的编辑器接入口已完成；动态库发现、平台加载和插件生命周期仍属于后续插件系统，不在本次范围。
