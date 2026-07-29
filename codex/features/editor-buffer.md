# 编辑器与 Scintilla 适配功能分析

## 功能定位

编辑器层使用 QScintilla 承载文本编辑能力，并通过 `ScintillaEditView` 保留部分 Notepad++ / Scintilla 风格接口。

## 当前实现入口

- `./src/ScintillaComponent/ScintillaEditView.h`
- `./src/ScintillaComponent/ScintillaEditView.cpp`
- `./src/ScintillaComponent/Buffer.h`
- `./src/Parameters.h`

## 当前能力

- `execute()`：保留 Scintilla 消息调用风格，内部转发到 `SendScintilla()`。
- 基础文本操作：设置、获取、追加、插入文本。
- 光标和选择：当前位置、设置位置、获取选区。
- 语法高亮：按文件路径或扩展名设置 lexer。
- 样式应用：从 `stylers.xml` 和全局样式应用编辑器样式。
- 注释切换：依赖 `langs.xml` 中语言注释信息。
- 书签：切换、下一个、上一个、清空。
- 偏好设置：字体、Tab、自动完成、换行、空白符、EOL、缩进线。
- 智能高亮和查找标记指示器：使用固定 indicator 编号。

## 关键依赖

- `NppParameters::getLangDescByExt()` / `getLangDescByName()` 用于语言识别。
- `NppParameters::getLexerStyler()` / `getGlobalStyles()` 用于样式。
- QScintilla 的 `SendScintilla()` 是兼容层核心。

## 风险点

- QScintilla API 和原版 Scintilla 消息语义不总是完全一致。
- indicator、marker、margin 编号需要避免和 QScintilla 默认行为冲突。
- 样式 ID 映射必须对齐 Lexilla/Scintilla 语言规则。
- 行/列、字节偏移和 Unicode 字符位置之间可能存在差异。

## 后续索引任务

- 建立 `SCI_*` 消息使用清单。
- 对比原版 `ScintillaEditView` 的初始化、边距、样式和指示器设置。
- 分析编码文本加载后 QScintilla 内部位置与原版字节位置的差异。

## 2026-07-22 阶段二更新

- 打开和 reload 文件时会设置 QScintilla 的 EOL mode，以保持 Windows/Unix/Mac 换行状态。
- 会话保存使用当前标签实际 `ScintillaEditView`，避免 clone/sub view 的光标和滚动位置被原始 view 覆盖。
- 会话恢复使用 `SCI_SETSEL`、`SCI_SETFIRSTVISIBLELINE`、`SCI_SETXOFFSET` 和 `SCI_SETSCROLLWIDTH` 恢复基础视图状态。

## 阶段二后仍保留的差距

- 位置恢复仍以 Scintilla position 为主，Unicode 字符位置与原版字节位置差异后续需要专项验证。
- fold、bookmark、selection mode 的完整恢复还未完成。
