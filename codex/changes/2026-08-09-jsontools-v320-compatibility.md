# JsonTools 3.2.0 Windows 兼容接入

## 范围

- 使用官方 `Release_x64.zip`，SHA-256
  `884704a9dc046616831a8df6013114100472e4a7b7a4bb62e40309b286b33a0d`。
- 通过现有插件管理安装计划部署 `JsonTools.dll`，Windows 仅加载审核白名单中的
  `JsonTools`，不把 CLR 或 WinForms 引入通用插件层。
- 保留原 DLL 的解析、格式化、RemesPath 和 UI 实现，只扩展 Win32 宿主适配器。

## 实现

- `Win32MainWindowAdapter` 支持 `NPPM_GETFULLCURRENTPATH`、`NPPM_GETFILENAME`、
  `NPPM_MENUCOMMAND(IDM_FILE_NEW)` 和 `NPPM_DOOPEN`。路径消息兼容插件使用
  `wParam=0`、`MAX_PATH` 预分配缓冲区的调用方式。
- `Win32EditorMessageAdapter` 原样转发 `SCI_APPENDTEXT`、`SCI_GOTOLINE` 和
  `SCI_GOTOPOS` 到永久主/副 Scintilla 视图。
- 批量插件加载并分配命令 ID 后发送 `NPPN_TBMODIFICATION`；Buffer 真正关闭前发送
  `NPPN_FILEBEFORECLOSE`，退出仍按原版逆序发送 `NPPN_SHUTDOWN`。
- Dock 注册后由 `DockingManager` 显示并提升当前标签，避免 WinForms 树窗口被已有
  JSON Viewer Dock 遮住。只有 Dock host 及其必要 Qt 控件成为原生窗口。
- JsonTools 上游把功能索引 `4` 写入 `tTbData::dlgID`，菜单勾选使用宿主分配的命令 ID。
  适配层和测试分别保留这两个标识。

## 自动验证

- 官方语料安装、收据、发现和 CLR4 DLL 加载成功，无加载错误。
- 10 个 `FuncItem` 名称和 Ctrl+Alt+Shift+P/C/J 快捷键一致。
- 真实 DLL 完成 pretty-print、compress，并把当前文档设为 JSON lexer。
- 当前路径、文件名、新建、打开及 3 个新增 SCI 消息通过消息级断言。
- JSON Tree WinForms HWND 已嵌入 Qt Dock，Dock 可见、菜单勾选同步、父窗口关系正确，
  并生成自动截图和诊断文件。
- 测试插件确认关闭文档时收到一次 `NPPN_FILEBEFORECLOSE`，且 Buffer ID 非零。
- `ui-localization-runtime-100` 与 `ui-plugin-registration-rollback` 通过。

## 深层验证更新

Settings、RemesPath 查询/赋值、JSON Lines、JSON-to-YAML、树节点跳转、多 Dock 交互和
4 MB partial/full tree 边界已在 2026-08-09 纳入真实 DLL 自动矩阵。`Run tests` 菜单仍受
上游 JsonGrepper 测试硬编码作者绝对目录影响，详细边界见
`codex/changes/2026-08-09-jsontools-deep-and-simple-plugins.md`。
