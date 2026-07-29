# P0 编辑辅助、快捷键与工作区

更新时间：2026-07-26

## 实现范围

本轮 P0 覆盖以下原版行为：

- URL Hotspot：识别默认 URI scheme 和 `uriCustomizedSchemes`，按配置显示下划线或悬停样式，点击后交给系统打开。
- XML/HTML 标签匹配：匹配起止标签并标记对应范围，支持 HTML 大小写不敏感规则。
- 字符自动插入：支持圆括号、方括号、花括号、单双引号，以及 HTML/XML 闭合标签。
- 保存前备份：支持 simple 和 verbose 模式、默认目录和自定义目录、时间戳文件名。
- `shortcuts.xml`：读取 InternalCommands、ScintillaKeys、NextKey、Macros 和 UserDefinedCommands。
- 快捷键映射：已实现的主窗口命令使用原版 Notepad++ command ID；ScintillaKeys 应用到每个编辑视图。
- 宏回放：按原版记录顺序执行 Scintilla 数值命令、字符串命令和已映射的菜单命令。
- workspace XML：读取和写入 `NotepadPlus/Project/Folder/File`，相对路径以 workspace 文件目录为基准。
- Project Panels：三个独立项目面板，可加载、保存和编辑各自 workspace。
- Find/Replace in Projects：范围严格限定为所选项目面板的 workspace 文件成员。

## 关键代码入口

- `src/Parameters.h|cpp`
  - 配置、项目面板路径和 `shortcuts.xml` 数据模型。
- `src/Preferences/PreferenceDlg.h|cpp`
  - 备份、自定义 URI、标签匹配和自动插入偏好设置。
- `src/ScintillaComponent/ScintillaEditView.h|cpp`
  - URL、标签匹配和自动插入行为。
- `src/MISC/FileManager.h|cpp`
  - 保存前备份路径和复制策略。
- `src/MainWindow.h|cpp`
  - command ID 注册、快捷键应用、宏回放、项目面板和项目范围搜索替换。
- `src/WinControls/ProjectPanel/WorkspaceDocument.h|cpp`
  - workspace XML 数据模型和相对路径处理。
- `src/WinControls/ProjectPanel/ProjectPanel.h|cpp`
  - Project Panel UI 与 workspace 编辑。
- `src/ScintillaComponent/FindReplaceDlg.h|cpp`
  - Find/Replace in Projects UI、面板选择和历史配置。

## 配置兼容约束

- `config.xml` 和 `shortcuts.xml` 使用保守写回；没有出现过的可选节点不会因为默认值被主动补入。
- 未知 XML 节点和属性应继续保留。
- Project Panel 的 `workSpaceFile` 路径沿用原版 `ProjectPanels/ProjectPanel` 结构。
- 备份自定义目录沿用原版拼写 `useCustumDir`，不能擅自纠正 XML 属性名。
- workspace 写回保持原版根结构，不进行格式升级。

## 行为边界

- command ID 只映射 Qt 主线中已经实现的动作；尚未移植的原版命令仍保留在 XML 中，但不会产生可执行动作。
- 宏已覆盖当前配置语料和原版常用的 type 0、1、2 命令序列。保存查找状态一类的专用宏状态仍应在后续专项中核验。
- URL 和标签辅助在大文件模式中停用，遵循 v8.4.6 的功能降级方向。
- Replace in Projects 跳过二进制、只读和已打开但未保存的脏文件，并使用 `QSaveFile` 原子写回和原编码/BOM。

## 自动化验证

- `large-file-mode-tests`
  - simple/verbose 备份路径和内容。
  - 圆括号自动插入。
  - URL indicator。
  - XML 标签匹配 indicator。
- `workspace-document-tests`
  - workspace 读取、相对路径解析、成员枚举和写回。
- 配置兼容语料继续验证未知节点/属性保留。
- 2026-07-26 Debug 构建成功，CTest `18/18` 通过。

## 待授权运行时验证

以下项目需要启动 GUI 或人工交互，按用户要求尚未执行：

- URL 点击是否由系统默认应用正确打开。
- 标签匹配、自动插入在真实输入法和撤销栈中的视觉与交互表现。
- 备份失败确认框、自定义目录选择器和生成文件的实际流程。
- Shortcut Mapper 保存后重启恢复、冲突显示和多段 Scintilla 快捷键。
- 配置宏菜单的显示、快捷键和混合命令序列回放。
- 三个 Project Panel 的加载、编辑、保存和会话恢复。
- Find/Replace in Projects 的面板选择、结果跳转和确认对话框。
- 本地化文本、布局、DPI 和原版 UI 一致性截图。
