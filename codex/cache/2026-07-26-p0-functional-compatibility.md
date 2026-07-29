# 本地缓存：P0 功能兼容

缓存日期：2026-07-26

## 当前结论

P0 的非插件代码范围已经完成：

- 编辑辅助：URL、XML/HTML 标签匹配、字符和标签自动插入。
- 保存保护：simple/verbose 保存前备份与自定义目录。
- 输入映射：InternalCommands、ScintillaKeys、NextKey 和宏 type 0/1/2 序列。
- 项目功能：workspace XML、三个 Project Panel、项目成员范围搜索替换。
- 配置：相关 `config.xml` 和 `shortcuts.xml` 字段的保守读写。

## 主要调用关系

1. `NppParameters` 加载配置、快捷键和 Project Panel workspace 路径。
2. `MainWindow` 创建动作和编辑视图后注册原版 command ID，并应用动作与 Scintilla 快捷键。
3. `ScintillaEditView` 根据偏好执行 URL、标签匹配和自动插入。
4. `MainWindow::doSave()` 在正常覆盖保存前调用 `FileManager::backupFileBeforeSave()`。
5. `ProjectPanel` 通过 `WorkspaceDocument` 管理原版 workspace XML。
6. `FindReplaceDlg` 产生项目面板掩码，`MainWindow` 仅遍历对应 workspace 成员。

## 验证状态

- Debug 构建：通过。
- CTest：`18/18` 通过。
- GUI 启动、截图和人工交互：未执行，等待用户授权。

## 运行时验证更新

- 用户已授权启动和截图验证。
- `ui-parity-capture` 成功生成主窗口、Project Panels、5 个查找页、
  19 个首选项页和 Shortcut Mapper 四个分类页截图。
- 验证中发现并修复 Shortcut Mapper 动作平铺问题。
- 修复后完整 Debug 构建成功，CTest `18/18` 通过。
- 仍需用户直接操作的系统集成和真实文件流程见
  `codex/validation/2026-07-26-p0-runtime/README.md`。

## 下一次修改前

- 先查阅 `codex/features/p0-editor-shortcuts-workspace.md`。
- 修改配置字段时核验原版 XML 拼写和缺省节点行为。
- 修改项目搜索时保持“仅 workspace 成员”边界。
- 修改宏或快捷键时核对原版 command ID 和 Scintilla message 参数类型。
