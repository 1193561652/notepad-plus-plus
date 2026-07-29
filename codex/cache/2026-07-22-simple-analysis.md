# 2026-07-22 简单项目分析缓存

## 项目状态

`./` 已有基础 Qt Widgets 应用结构，并包含多个 Notepad++ 核心概念的 Qt 移植版本。当前代码更接近“已有基础框架和部分功能”的状态，而不是单纯模板工程。

## 模块快照

- 主窗口：`MainWindow`。
- 编辑器：`ScintillaEditView`。
- 文档状态：`Buffer`。
- 文件生命周期：`FileManager`。
- 标签页：`DocTabView`。
- 配置：`NppParameters` + TinyXml。
- 本地化：`NativeLangSpeaker`。
- 查找替换：`FindReplaceDlg`。
- 停靠面板：`FileBrowserPanel`、`DocumentMapPanel`、`FunctionListPanel`。
- 偏好设置：`PreferenceDlg`。
- 插件预留：`IPlugin`、`IPluginHost`、`PluginManager`。

## 功能快照

- 文件管理：已有新建、打开、保存、另存为、关闭询问等入口。
- 多标签：已有 Buffer 到 Tab 的绑定、关闭请求和克隆视图入口。
- 编辑器：已有 QScintilla 包装、lexer、样式、书签、注释、偏好应用。
- 配置：已有 config/session/langs/stylers/shortcuts 的读写入口。
- UI：已有菜单、工具栏、状态栏、停靠面板、双视图相关入口。
- 查找替换：已有对话框 UI 和搜索选项结构。
- 插件：已有 Qt 化插件接口，近期不应扩展成完整功能。

## 优先风险排序

1. 配置完全读写兼容。
2. 文件编码、BOM、EOL 保持。
3. Buffer 生命周期和克隆视图所有权。
4. Scintilla/QScintilla 位置和消息语义差异。
5. 菜单、快捷键、配置驱动 UI 状态一致性。
6. 插件接口边界与近期目标控制。

## 推荐下一步

先深入做两个索引：

1. 配置系统节点级索引。
2. 文件打开/保存/Buffer 生命周期调用链索引。

这两块会影响最多后续功能，且最容易产生兼容性回归。
