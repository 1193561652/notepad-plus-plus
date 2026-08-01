# 2026-08-01 Scintilla 5 Qt 迁移

## 修改范围

- 导入 v8.4.6 对齐的 Scintilla 5.3.0、Lexilla 5.1.9 和 Boost.Regex 适配层。
- 新建静态目标 `npp-scintilla-qt` 与 `npp-lexilla`，构建标准提升到 C++17。
- `ScintillaEditView` 改为继承官方 `ScintillaEditBase`。
- lexer 改为 `CreateLexer -> ILexer5 -> SCI_SETILEXER`。
- UDL、宏、打印、纯文本搜索和便利 API 改为直接 Scintilla 消息实现。
- 删除 QScintilla 源树、qmake 子构建和所有 `Qsci*` 代码引用。

## 行为与架构

- 业务层继续使用 `execute/SendScintilla`，但入口直接调用官方 `send()`。
- 双视图共享官方 Scintilla Document 指针及撤销历史。
- Boost.Regex 继续由 `SCI_OWNREGEX` 提供原版语义。
- Lexilla 负责全部内置 lexer、LexUser 和 LexSearchResult。
- 官方 Qt 通知映射到 dirty、状态、智能高亮、书签、查找结果和宏行为。
- `SCN_INDICATORRELEASE` 直接驱动 URL 指示器点击，避免依赖 QScintilla 专属信号。
- 自动完成按配置阈值触发，候选项使用换行分隔以保留带空格的函数签名；参数提示使用 `SCI_CALLTIPSHOW`。
- 编辑器宏保留 QScintilla 历史转义序列化格式的完全读写兼容。

## 验证

- `cmake --build build -j 6`：通过。
- `ctest --test-dir build --output-on-failure -j 4`：`29/29` 通过。
- 导入树二进制扫描：`0` 个 `.obj/.lib/.dll/.exe/.pdb/.a/.so/.dylib`，上游历史构建产物已清理。
- 活跃源码、测试及 CMake 中无 `Qsci`/`QScintilla`/`qscintilla` 引用。
- 专项覆盖真实 `ILexer5`、XML/UDL styling、Boost lookbehind/`\R`/捕获替换、
  大文件 Document options 和共享文档。

## 后续注意

- Linux/macOS 必须在对应平台重新构建验证。
- 打印、自动完成、宏和完整 UI 交互仍建议进行一次人工运行回归。
- 升级 Scintilla 或 Lexilla 时必须成组评估，不能脱离 Notepad++ 基线单独升级。
