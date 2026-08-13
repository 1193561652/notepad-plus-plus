# 仓库索引

## 目标

仓库根目录是 Notepad++ v8.4.6 Qt 跨平台移植的唯一开发主线。行为一致
优先于代码逐行一致，配置文件保持完全读写兼容。

## 主要目录

- `src/`：Qt 主线应用代码。
- `resources/`：默认配置、语言、主题、图标和解析规则。
- `tests/`：自动化测试与 v8.4.6 配置语料。
- `third_party/scintilla/`：原版基线 Scintilla 5.3.0 和官方 Qt 平台。
- `third_party/lexilla/`：原版基线 Lexilla 5.1.9。
- `third_party/boostregex/`：原版 Boost.Regex 适配源码及 Boost headers。
- `codex/`：代码知识库、索引、决策、变更和本地缓存。

## 工作区插件源码仓库

- `../comparePlus/`：用户 fork 的 ComparePlus 独立仓库，官方仓库配置为 `upstream`。
- `../NPP_HexEditor/`：用户 fork 的 HEX-Editor 独立仓库，官方仓库配置为 `upstream`。
- 路径、远程仓库、v8.4.6 tag 基线和同步方法见
  `codex/index/plugin-source-repositories.md`。

这两个兄弟目录用于插件源码分析与移植，不属于 Qt 主线仓库的构建目录。

旧 `third_party/qscintilla/`、qmake 静态子构建和
`cmake/BuildNppQScintilla.cmake` 已在 Scintilla 5 迁移中删除。

## 编辑器入口顺序

1. `CMakeLists.txt`：确认 Scintilla/Lexilla 实际编译清单和定义。
2. `src/ScintillaComponent/ScintillaEditView.*`：Qt 事件与 `SCI_*` 边界。
3. `src/ScintillaComponent/UserDefinedLexer.*`：UDL 属性、关键字和样式。
4. `src/ScintillaComponent/ScintillaTextSearch.*`：隐藏 Scintilla 文档搜索。
5. `src/ScintillaComponent/EditorMacro.*`：官方 `macroRecord` 通知录制回放。
6. `third_party/scintilla/ORIGIN.md`、`third_party/lexilla/ORIGIN.md`：来源约束。

修改编辑器代码后至少运行 Lexilla、Boost.Regex 和大文件三个专项测试；
跨模块变更运行完整 CTest。
