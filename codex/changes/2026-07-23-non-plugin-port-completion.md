# 2026-07-23 非插件功能补全

## 修改范围

- 文件与平台服务：补齐另存副本、重命名、回收站、批量关闭、手工会话、资源管理器、终端、默认查看器和工作区目录入口。
- 编辑与搜索：补齐日期时间、块注释、大小写变换、空白转换、括号跳转、标记跳转、打开文档批量替换和文件批量替换。
- UI 与工具：补齐 Document List、三 Project Panel、Clipboard History、Character Panel、折叠、全屏、置顶、Post-It、MD5/SHA-256、Run、Window 和 Help 命令。
- 偏好设置：移除 9 个空白页，并接入原版 `config.xml` 节点。
- 配置兼容：复用已有 XML 元素更新已知字段，保留未知属性和未知节点；修复 Qt 资源复制后目标 XML 只读的问题。
- 会话：保存并恢复书签和折叠状态。
- `contextMenu.xml`：编辑器右键菜单按配置动态构建，插件命令在本轮范围内跳过。
- `shortcuts.xml`：加载 `UserDefinedCommands`，在 Run 菜单中展开常用 Notepad++ 变量并执行。

## 关键实现位置

- `./src/MainWindow.cpp`
  - 文件、编辑、搜索、视图、工具、Run、Window、Help 命令。
  - `showEditorContextMenu()`。
  - `onReplaceAllOpenedDocsRequested()` / `onReplaceInFilesRequested()`。
  - 会话书签与折叠状态采集和恢复。
- `./src/Parameters.cpp`
  - 原版 GUIConfig 节点读写。
  - 保守 XML 更新和默认文件权限修复。
  - `UserDefinedCommands` 读取。
- `./src/Preferences/PreferenceDlg.cpp`
  - 19 个偏好页均有实际 UI；不再存在 stub 页。
- `./src/MISC/PlatformServices.*`
  - 文件管理器、终端、默认应用和回收站的平台隔离。

## 验证

- `cmake --build build -j 4`：通过，`[100%] Built target notepadpp-qt`。
- Windows 正常平台启动并由 `NPPQT_SMOKE_TEST` 自动正常关闭：退出码 `0`。
- 便携模式 XML 往返：
  - `ToolBar` 手工加入的未知属性 `futureAttr="keep"` 被保留。
  - 未知 `FutureFeature` GUIConfig 节点被保留。
  - `Print`、`openSaveDir`、`searchEngine`、`SmartHighLight`、`DateTime` 等新增受管节点存在。
- 测试生成的便携配置、目录和 `doLocalConf.xml` 已清理。

## 仍需专门验证或深化

- Boost.Regex 与 `QRegularExpression`/QScintilla 正则语义并不完全等价。
- 打印偏好已兼容读写，但 QsciPrinter 尚未完整渲染原版页眉、页脚和全部颜色模式。
- File Association 页当前打开系统默认应用设置，不直接修改各平台关联数据库。
- Project Panels 提供三个可用文件树，但尚未读写原版 workspace 文件。
- Shortcut Mapper 可修改运行时 QAction；原版 InternalCommands/ScintillaKeys 的完整 ID 映射和持久化仍需专项实现。
- 完整 UDL 设计器、大文件策略、命令行参数全集和像素级 UI 一致性仍是非插件差异。

> 后续状态：UDL、大文件方案 A 和命令行参数全集已于后续专项完成；本条保留为
> 2026-07-23 时点记录。
