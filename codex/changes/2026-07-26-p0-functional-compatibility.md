# 2026-07-26 P0 功能兼容变更

## 变更摘要

- 补齐 URL Hotspot、XML/HTML 标签匹配、字符自动插入。
- 补齐保存前 simple/verbose 备份及自定义目录配置。
- 扩展 `shortcuts.xml` 数据模型，接入 InternalCommands、ScintillaKeys、NextKey 和宏命令序列。
- 使用原版 command ID 绑定当前已实现的 Qt 动作，并将配置快捷键应用到 QAction 和 Scintilla。
- 新增原版结构的 workspace XML 模型和三个 Project Panel。
- 将 Find/Replace in Projects 从目录替代实现改为严格的 workspace 成员范围。
- 扩展偏好设置和 `config.xml` 保守读写，保留未知结构。

## 新增文件

- `src/WinControls/ProjectPanel/WorkspaceDocument.h`
- `src/WinControls/ProjectPanel/WorkspaceDocument.cpp`
- `src/WinControls/ProjectPanel/ProjectPanel.h`
- `src/WinControls/ProjectPanel/ProjectPanel.cpp`
- `tests/WorkspaceDocumentTests.cpp`

## 验证

- CMake Debug 构建成功。
- CTest `18/18` 通过。
- 未启动 GUI；所有需要截图或人工确认的工作留待用户授权。

## 剩余边界

- 插件不在本轮范围内。
- 未移植的原版 command ID 不会获得可执行 QAction，但 XML 内容保持可往返。
- 宏中的专用保存查找状态需要后续按真实配置语料专项核验。
- 像素级 UI、DPI、本地化和系统 URL 打开行为需要运行时验证。

## 运行时验证增量

- UI 捕获覆盖主窗口、5 个查找页、19 个首选项页、Project Panels 和
  Shortcut Mapper。
- 首次截图发现 Shortcut Mapper 平铺所有 QAction，并混入编码和语言动作。
- 已修复为 Main menu、Macros、Run commands、Scintilla commands 四页。
- Main menu 仅显示拥有原版 command ID 的动作。
- 宏、Run commands 和 ScintillaKeys 的快捷键可分别编辑并保守写回。
- 宏与 UserDefinedCommands 写回改为原位更新，减少未知 XML 数据丢失风险。
- 验证截图：`codex/validation/2026-07-26-p0-runtime/`。
