# 编辑器与 Scintilla 适配功能分析

## 功能定位

编辑器层直接使用官方 Scintilla 5.3.0 Qt 平台实现，并通过 `ScintillaEditView`
保持 Notepad++ 的 `execute(SCI_*, ...)` 消息调用边界。

## 当前实现入口

- `./src/ScintillaComponent/ScintillaEditView.h`
- `./src/ScintillaComponent/ScintillaEditView.cpp`
- `./src/ScintillaComponent/Buffer.h`
- `./src/Parameters.h`

## 当前能力

- `execute()`：保留 Scintilla 消息调用风格，内部转发到 Scintilla direct-call 接口。
- 基础文本操作：设置、获取、追加、插入文本。
- 光标和选择：当前位置、设置位置、获取选区。
- 语法高亮：按文件路径或扩展名设置 lexer。
- 样式应用：从 `stylers.xml` 和全局样式应用编辑器样式。
- 注释切换：依赖 `langs.xml` 中语言注释信息。
- 书签：切换、下一个、上一个、清空。
- 偏好设置：字体、Tab、自动完成、换行、空白符、EOL、缩进线。
- 智能高亮和查找标记指示器：使用固定 indicator 编号。
- 主/副视图各有且仅有一个永久 `ScintillaEditView`；文档标签不拥有编辑器。
- 标签切换调用 `SCI_SETDOCPOINTER`，Buffer 通过 `SCI_ADDREFDOCUMENT` /
  `SCI_RELEASEDOCUMENT` 管理文档引用。
- 主/副视图显示同一 Buffer 时共享 document pointer，但分别保存选择区和滚动状态。

## 关键依赖

- `NppParameters::getLangDescByExt()` / `getLangDescByName()` 用于语言识别。
- `NppParameters::getLexerStyler()` / `getGlobalStyles()` 用于样式。
- `ScintillaEditBase::WndProc()` 与 Scintilla direct function 是兼容层核心。

## 风险点

- Qt 平台事件与原版 Win32 平台事件不完全一致，鼠标、IME、DPI 和焦点行为需要验证。
- indicator、marker、margin 编号必须与原版 Scintilla 配置保持一致。
- 样式 ID 映射必须对齐 Lexilla/Scintilla 语言规则。
- 行/列、字节偏移和 Unicode 字符位置之间可能存在差异。
- `Buffer::getView()` 只表示视图成员关系，不保证该永久编辑器当前显示此 Buffer；
  后台或批量操作必须先经 `MainWindow::activateBufferView()` 激活目标文档。

## 后续索引任务

- 建立 `SCI_*` 消息使用清单。
- 对比原版 `ScintillaEditView` 的初始化、边距、样式和指示器设置。
- 分析编码文本加载后 UTF-8 字节位置与原版消息调用的差异。

## 2026-07-22 阶段二更新

- 打开和 reload 文件时会设置 Scintilla EOL mode，以保持 Windows/Unix/Mac 换行状态。
- 会话保存使用当前标签实际 `ScintillaEditView`，避免 clone/sub view 的光标和滚动位置被原始 view 覆盖。
- 会话恢复使用 `SCI_SETSEL`、`SCI_SETFIRSTVISIBLELINE`、`SCI_SETXOFFSET` 和 `SCI_SETSCROLLWIDTH` 恢复基础视图状态。

## 阶段二后仍保留的差距

- 位置恢复仍以 Scintilla position 为主，Unicode 字符位置与原版字节位置差异后续需要专项验证。
- fold、bookmark、selection mode 的完整恢复还未完成。
