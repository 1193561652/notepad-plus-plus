# Preferences 与编辑器边栏最终收口

日期：2026-08-01

## 范围

- 完善 v8.4.6 Preferences 其余页面及浅色、深色布局。
- 对齐 Find 对话框、文档标签和状态栏的剩余视觉细节。
- 修复编辑器左侧边栏颜色、鼠标指针和点击行书签行为。

## 编辑器边栏

- margin 0：`SC_MARGIN_NUMBER`，颜色来自 `Line number margin`。
- margin 1：`SC_MARGIN_COLOUR`，颜色来自 `Bookmark margin`，仅接收书签 marker。
- margin 2：`SC_MARGIN_SYMBOL`，仅接收折叠 marker。
- margin 1 使用 `SC_CURSORREVERSEARROW`；正文保持 IBeam。
- `SCN_MARGINCLICK` 的行号直接传给 `toggleBookmark(line)`，不得读取当前 caret 行。
- `setFont()` 会执行 `SCI_STYLECLEARALL`；调用方随后必须执行 `applyGlobalStyles()`。

## Preferences 与配置

- 页面结构和控件语义以 v8.4.6 `preference.rc` 为准。
- 简体中文静态文本优先按稳定 `objectName` 定位；Qt 新增静态控件允许按英文源文本回退。
- `Searching` 和 `FinderConfig` 字段按 v8.4.6 名称读取和写回。
- 写回不得创建原文件不存在的上述节点，未知节点与属性继续保留。
- 搜索引擎枚举 3 是旧 Bing 值，加载到 Preferences 时按原版迁移为 DuckDuckGo。

## 验证

- `tests/LargeFileModeTests.cpp`：margin 类型、颜色、mask、hover 指针和点击行。
- `tests/FindDialogLocalizationTests.cpp`：Preferences 新静态控件必须可本地化。
- `tests/ConfigCorpusTests.cpp`：v8.4.6 XML 逐结构往返。
- `tests/UiParityCapture.cpp`：19 页 Preferences、5 页 Find、Shortcut Mapper 和主窗口截图。
